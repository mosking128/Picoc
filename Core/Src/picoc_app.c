#include "picoc_app.h"

#include <string.h>

#include "serial_app.h"
#include "picoc.h"
#include "interpreter.h"

#define PICOC_APP_STACK_SIZE             (64 * 1024)
#define PICOC_APP_SOURCE_BUFFER_SIZE     2048U
#define PICOC_APP_EXEC_BUFFER_SIZE       2304U
#define PICOC_APP_LOAD_BUFFER_SIZE       8192U
#define PICOC_APP_RX_CHUNK_SIZE          64U
#define PICOC_APP_LOAD_PROMPT            "load> "
#define PICOC_APP_LOAD_ENTER_MESSAGE     "\r\nload mode: send C source, ':end' to run, ':abort' to cancel\r\n"
#define PICOC_APP_LOAD_ABORT_MESSAGE     "\r\nload cancelled\r\n"
#define PICOC_APP_LOAD_EMPTY_MESSAGE     "\r\nno source loaded\r\n"
#define PICOC_APP_LOAD_READY_MESSAGE     "\r\nready for next file\r\n"
#define PICOC_APP_LOAD_OVERFLOW_MESSAGE  "\r\nload buffer full, upload cancelled\r\n"
#define PICOC_APP_LINE_OVERFLOW_MESSAGE  "\r\ninput line too long\r\n"

typedef struct
{
    int paren_depth;
    int bracket_depth;
    int brace_depth;
    int in_string;
    int in_char;
    int in_block_comment;
    int ends_with_do_block;
    char last_non_space;
} PicocApp_SourceState;

typedef enum
{
    PICOC_APP_MODE_REPL = 0,
    PICOC_APP_MODE_LOAD
} PicocApp_Mode;

typedef enum
{
    PICOC_APP_COMMAND_NONE = 0,
    PICOC_APP_COMMAND_LOAD,
    PICOC_APP_COMMAND_END,
    PICOC_APP_COMMAND_ABORT
} PicocApp_Command;

static Picoc g_picoc;
static uint8_t g_source_buffer[PICOC_APP_SOURCE_BUFFER_SIZE];
static uint8_t g_load_buffer[PICOC_APP_LOAD_BUFFER_SIZE];
static uint32_t g_source_length = 0U;
static uint32_t g_load_length = 0U;
static uint8_t g_prompt_pending = 0U;
static uint8_t g_last_char_was_cr = 0U;
static PicocApp_Mode g_mode = PICOC_APP_MODE_REPL;
static const char *g_prompt_text = INTERACTIVE_PROMPT_STATEMENT;

static void PicocApp_WriteString(const char *text);
static void PicocApp_WriteByte(uint8_t ch);
static void PicocApp_ShowPrompt(void);
static void PicocApp_HandleChar(uint8_t ch);
static void PicocApp_HandleReplChar(uint8_t ch);
static void PicocApp_HandleLoadChar(uint8_t ch);
static int PicocApp_AppendByte(uint8_t *buffer, uint32_t *length, uint32_t capacity, uint8_t ch);
static int PicocApp_AppendBlock(uint8_t *buffer, uint32_t *length, uint32_t capacity, const uint8_t *data, uint32_t size);
static void PicocApp_ResetSource(void);
static void PicocApp_ResetLoadBuffer(void);
static void PicocApp_ResetLineBuffer(void);
static void PicocApp_EnterLoadMode(void);
static void PicocApp_LeaveLoadMode(void);
static void PicocApp_ExecuteReplSource(void);
static void PicocApp_ExecuteLoadSource(void);
static void PicocApp_RunSource(Picoc *pc, const char *file_name, const char *source, int auto_print_expression, int auto_call_main);
static int PicocApp_HasMainFunction(Picoc *pc);
static PicocApp_Command PicocApp_ParseCommand(const uint8_t *buffer, uint32_t length);
static void PicocApp_AnalyseSource(const char *source, PicocApp_SourceState *state);
static int PicocApp_IsSourceComplete(const char *source, const PicocApp_SourceState *state);
static int PicocApp_ShouldAutoPrintExpression(const char *source, const PicocApp_SourceState *state);
static const char *PicocApp_SkipLeadingSpace(const char *text);
static const char *PicocApp_FindTrimmedEnd(const char *text);
static int PicocApp_StartsWithKeyword(const char *text, const char *keyword);

void PicocApp_Init(void)
{
    PicocInitialise(&g_picoc, PICOC_APP_STACK_SIZE);
    PicocApp_ResetSource();
    PicocApp_ResetLoadBuffer();
    PicocApp_WriteString(INTERACTIVE_PROMPT_START);
    g_prompt_text = INTERACTIVE_PROMPT_STATEMENT;
    PicocApp_ShowPrompt();
}

void PicocApp_Task(void)
{
    uint8_t rx_buffer[PICOC_APP_RX_CHUNK_SIZE];
    uint32_t rx_len;
    uint32_t index;

    rx_len = SerialApp_Read(rx_buffer, sizeof(rx_buffer));
    for (index = 0U; index < rx_len; index++)
    {
        PicocApp_HandleChar(rx_buffer[index]);
    }

    if (g_prompt_pending != 0U && rx_len == 0U)
    {
        PicocApp_ShowPrompt();
    }
}

int PicocApp_ConsoleGetCharBlocking(void)
{
    uint8_t ch;

    while (SerialApp_Read(&ch, 1U) == 0U)
    {
    }

    return (int)ch;
}

static void PicocApp_WriteString(const char *text)
{
    if (text != NULL)
    {
        const uint8_t *buffer = (const uint8_t *)text;
        uint32_t remaining = (uint32_t)strlen(text);

        while (remaining > 0U)
        {
            uint32_t written = SerialApp_Write(buffer, remaining);
            buffer += written;
            remaining -= written;
        }
    }
}

static void PicocApp_WriteByte(uint8_t ch)
{
    while (SerialApp_Write(&ch, 1U) == 0U)
    {
    }
}

static void PicocApp_ShowPrompt(void)
{
    PicocApp_WriteString(g_prompt_text);
    g_prompt_pending = 0U;
}

static void PicocApp_HandleChar(uint8_t ch)
{
    if (ch == 0U)
    {
        return;
    }

    if (ch == '\n' && g_last_char_was_cr != 0U)
    {
        g_last_char_was_cr = 0U;
        return;
    }

    if (g_mode == PICOC_APP_MODE_LOAD)
    {
        PicocApp_HandleLoadChar(ch);
    }
    else
    {
        PicocApp_HandleReplChar(ch);
    }
}

static void PicocApp_HandleReplChar(uint8_t ch)
{
    if (ch == '\r' || ch == '\n')
    {
        PicocApp_SourceState state;
        PicocApp_Command command;

        g_last_char_was_cr = (ch == '\r') ? 1U : 0U;
        g_source_buffer[g_source_length] = '\0';
        command = PicocApp_ParseCommand(g_source_buffer, g_source_length);
        if (command == PICOC_APP_COMMAND_LOAD)
        {
            PicocApp_WriteString("\r\n");
            PicocApp_ResetSource();
            PicocApp_EnterLoadMode();
            g_prompt_pending = 1U;
            return;
        }

        PicocApp_WriteString("\r\n");
        (void)PicocApp_AppendByte(g_source_buffer, &g_source_length, sizeof(g_source_buffer), '\n');
        g_source_buffer[g_source_length] = '\0';

        PicocApp_AnalyseSource((const char *)g_source_buffer, &state);

        if (PicocApp_IsSourceComplete((const char *)g_source_buffer, &state) != 0)
        {
            PicocApp_ExecuteReplSource();
            PicocApp_ResetSource();
            g_prompt_text = INTERACTIVE_PROMPT_STATEMENT;
        }
        else
        {
            g_prompt_text = INTERACTIVE_PROMPT_LINE;
        }

        g_prompt_pending = 1U;
        return;
    }

    g_last_char_was_cr = 0U;

    if (ch == '\b' || ch == 0x7fU)
    {
        if (g_source_length > 0U)
        {
            g_source_length--;
            g_source_buffer[g_source_length] = '\0';
            PicocApp_WriteString("\b \b");
        }
        return;
    }

    if (PicocApp_AppendByte(g_source_buffer, &g_source_length, sizeof(g_source_buffer), ch) == 0)
    {
        PicocApp_WriteString(PICOC_APP_LINE_OVERFLOW_MESSAGE);
        PicocApp_ResetSource();
        g_prompt_text = INTERACTIVE_PROMPT_STATEMENT;
        g_prompt_pending = 1U;
        return;
    }

    PicocApp_WriteByte(ch);
}

static void PicocApp_HandleLoadChar(uint8_t ch)
{
    if (ch == '\r' || ch == '\n')
    {
        PicocApp_Command command;

        g_last_char_was_cr = (ch == '\r') ? 1U : 0U;
        g_source_buffer[g_source_length] = '\0';
        command = PicocApp_ParseCommand(g_source_buffer, g_source_length);

        if (command == PICOC_APP_COMMAND_END)
        {
            PicocApp_WriteString("\r\n");
            PicocApp_ResetLineBuffer();
            PicocApp_ExecuteLoadSource();
            PicocApp_ResetLoadBuffer();
            PicocApp_WriteString(PICOC_APP_LOAD_READY_MESSAGE);
            g_prompt_text = PICOC_APP_LOAD_PROMPT;
            g_prompt_pending = 1U;
            return;
        }

        if (command == PICOC_APP_COMMAND_ABORT)
        {
            PicocApp_WriteString("\r\n");
            PicocApp_ResetLineBuffer();
            PicocApp_ResetLoadBuffer();
            PicocApp_WriteString(PICOC_APP_LOAD_ABORT_MESSAGE);
            PicocApp_LeaveLoadMode();
            g_prompt_pending = 1U;
            return;
        }

        PicocApp_WriteString("\r\n");

        if (PicocApp_AppendBlock(g_load_buffer,
                                 &g_load_length,
                                 sizeof(g_load_buffer),
                                 g_source_buffer,
                                 g_source_length) == 0 ||
            PicocApp_AppendByte(g_load_buffer, &g_load_length, sizeof(g_load_buffer), '\n') == 0)
        {
            PicocApp_WriteString(PICOC_APP_LOAD_OVERFLOW_MESSAGE);
            PicocApp_ResetLineBuffer();
            PicocApp_ResetLoadBuffer();
            PicocApp_LeaveLoadMode();
            g_prompt_pending = 1U;
            return;
        }

        PicocApp_ResetLineBuffer();
        g_prompt_pending = 1U;
        return;
    }

    g_last_char_was_cr = 0U;

    if (ch == '\b' || ch == 0x7fU)
    {
        if (g_source_length > 0U)
        {
            g_source_length--;
            g_source_buffer[g_source_length] = '\0';
            PicocApp_WriteString("\b \b");
        }
        return;
    }

    if (PicocApp_AppendByte(g_source_buffer, &g_source_length, sizeof(g_source_buffer), ch) == 0)
    {
        PicocApp_WriteString(PICOC_APP_LINE_OVERFLOW_MESSAGE);
        PicocApp_ResetLineBuffer();
        PicocApp_ResetLoadBuffer();
        PicocApp_LeaveLoadMode();
        g_prompt_pending = 1U;
        return;
    }

    PicocApp_WriteByte(ch);
}

static int PicocApp_AppendByte(uint8_t *buffer, uint32_t *length, uint32_t capacity, uint8_t ch)
{
    if (*length >= (capacity - 1U))
    {
        return 0;
    }

    buffer[*length] = ch;
    (*length)++;
    buffer[*length] = '\0';
    return 1;
}

static int PicocApp_AppendBlock(uint8_t *buffer, uint32_t *length, uint32_t capacity, const uint8_t *data, uint32_t size)
{
    if ((*length + size) >= capacity)
    {
        return 0;
    }

    (void)memcpy(&buffer[*length], data, size);
    *length += size;
    buffer[*length] = '\0';
    return 1;
}

static void PicocApp_ResetSource(void)
{
    g_source_length = 0U;
    g_source_buffer[0] = '\0';
}

static void PicocApp_ResetLoadBuffer(void)
{
    g_load_length = 0U;
    g_load_buffer[0] = '\0';
}

static void PicocApp_ResetLineBuffer(void)
{
    PicocApp_ResetSource();
}

static void PicocApp_EnterLoadMode(void)
{
    g_mode = PICOC_APP_MODE_LOAD;
    g_prompt_text = PICOC_APP_LOAD_PROMPT;
    PicocApp_ResetLineBuffer();
    PicocApp_ResetLoadBuffer();
    PicocApp_WriteString(PICOC_APP_LOAD_ENTER_MESSAGE);
}

static void PicocApp_LeaveLoadMode(void)
{
    g_mode = PICOC_APP_MODE_REPL;
    g_prompt_text = INTERACTIVE_PROMPT_STATEMENT;
    PicocApp_ResetLineBuffer();
    PicocApp_ResetLoadBuffer();
}

static void PicocApp_ExecuteReplSource(void)
{
    PicocApp_RunSource(&g_picoc, "serial", (const char *)g_source_buffer, TRUE, FALSE);
}

static void PicocApp_ExecuteLoadSource(void)
{
    Picoc isolated_picoc;

    if (g_load_length == 0U)
    {
        PicocApp_WriteString(PICOC_APP_LOAD_EMPTY_MESSAGE);
        return;
    }

    PicocInitialise(&isolated_picoc, PICOC_APP_STACK_SIZE);
    PicocApp_RunSource(&isolated_picoc, "serial_load", (const char *)g_load_buffer, FALSE, TRUE);
    PicocCleanup(&isolated_picoc);
}

static void PicocApp_RunSource(Picoc *pc, const char *file_name, const char *source, int auto_print_expression, int auto_call_main)
{
    PicocApp_SourceState state;
    char exec_buffer[PICOC_APP_EXEC_BUFFER_SIZE];
    const char *parse_source = source;
    int had_main_before = 0;

    if (source == NULL || source[0] == '\0')
    {
        return;
    }

    PicocApp_AnalyseSource(source, &state);

    if (auto_print_expression != 0 && PicocApp_ShouldAutoPrintExpression(source, &state) != 0)
    {
        const char *start = PicocApp_SkipLeadingSpace(source);
        const char *end = PicocApp_FindTrimmedEnd(start);
        uint32_t expr_len = (uint32_t)(end - start);

        if ((sizeof("printf(\"%d\\n\",(") - 1U) + expr_len + (sizeof("));\n") - 1U) < sizeof(exec_buffer))
        {
            (void)memcpy(exec_buffer, "printf(\"%d\\n\",(", sizeof("printf(\"%d\\n\",(") - 1U);
            (void)memcpy(exec_buffer + (sizeof("printf(\"%d\\n\",(") - 1U), start, expr_len);
            (void)memcpy(exec_buffer + (sizeof("printf(\"%d\\n\",(") - 1U) + expr_len, "));\n", sizeof("));\n"));
            parse_source = exec_buffer;
        }
    }

    had_main_before = PicocApp_HasMainFunction(pc);

    if (PicocPlatformSetExitPoint(pc) == 0)
    {
        PicocParse(pc,
                   file_name,
                   parse_source,
                   (int)strlen(parse_source),
                   TRUE,
                   TRUE,
                   FALSE,
                   TRUE);

        if (auto_call_main != 0 && had_main_before == 0 && PicocApp_HasMainFunction(pc) != 0)
        {
            PicocCallMain(pc, 0, NULL);
        }
    }
}

static int PicocApp_HasMainFunction(Picoc *pc)
{
    return VariableDefined(pc, TableStrRegister(pc, "main"));
}

static PicocApp_Command PicocApp_ParseCommand(const uint8_t *buffer, uint32_t length)
{
    uint32_t start = 0U;
    uint32_t end = length;

    while (start < length &&
           (buffer[start] == ' ' || buffer[start] == '\t' || buffer[start] == '\r' || buffer[start] == '\n'))
    {
        start++;
    }

    while (end > start &&
           (buffer[end - 1U] == ' ' || buffer[end - 1U] == '\t' || buffer[end - 1U] == '\r' || buffer[end - 1U] == '\n'))
    {
        end--;
    }

    length = end - start;

    if (length == (sizeof(":load") - 1U) && memcmp(&buffer[start], ":load", sizeof(":load") - 1U) == 0)
    {
        return PICOC_APP_COMMAND_LOAD;
    }

    if (length == (sizeof(":end") - 1U) && memcmp(&buffer[start], ":end", sizeof(":end") - 1U) == 0)
    {
        return PICOC_APP_COMMAND_END;
    }

    if (length == (sizeof(":abort") - 1U) && memcmp(&buffer[start], ":abort", sizeof(":abort") - 1U) == 0)
    {
        return PICOC_APP_COMMAND_ABORT;
    }

    return PICOC_APP_COMMAND_NONE;
}

static void PicocApp_AnalyseSource(const char *source, PicocApp_SourceState *state)
{
    uint32_t index;
    int escape = 0;
    int line_comment = 0;

    (void)memset(state, 0, sizeof(*state));

    for (index = 0U; source[index] != '\0'; index++)
    {
        char ch = source[index];
        char next = source[index + 1U];

        if (line_comment != 0)
        {
            if (ch == '\n')
            {
                line_comment = 0;
            }
            continue;
        }

        if (state->in_block_comment != 0)
        {
            if (ch == '*' && next == '/')
            {
                state->in_block_comment = 0;
                index++;
            }
            continue;
        }

        if (state->in_string != 0)
        {
            if (escape != 0)
            {
                escape = 0;
            }
            else if (ch == '\\')
            {
                escape = 1;
            }
            else if (ch == '"')
            {
                state->in_string = 0;
            }
            continue;
        }

        if (state->in_char != 0)
        {
            if (escape != 0)
            {
                escape = 0;
            }
            else if (ch == '\\')
            {
                escape = 1;
            }
            else if (ch == '\'')
            {
                state->in_char = 0;
            }
            continue;
        }

        if (ch == '/' && next == '/')
        {
            line_comment = 1;
            index++;
            continue;
        }

        if (ch == '/' && next == '*')
        {
            state->in_block_comment = 1;
            index++;
            continue;
        }

        if (ch == '"')
        {
            state->in_string = 1;
            continue;
        }

        if (ch == '\'')
        {
            state->in_char = 1;
            continue;
        }

        if (ch == '(')
        {
            state->paren_depth++;
        }
        else if (ch == ')' && state->paren_depth > 0)
        {
            state->paren_depth--;
        }
        else if (ch == '[')
        {
            state->bracket_depth++;
        }
        else if (ch == ']' && state->bracket_depth > 0)
        {
            state->bracket_depth--;
        }
        else if (ch == '{')
        {
            state->brace_depth++;
        }
        else if (ch == '}' && state->brace_depth > 0)
        {
            state->brace_depth--;
        }

        if (ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n')
        {
            state->last_non_space = ch;
        }
    }

    if (state->brace_depth == 0 && state->last_non_space == '}')
    {
        const char *trimmed = PicocApp_SkipLeadingSpace(source);
        if (PicocApp_StartsWithKeyword(trimmed, "do") != 0)
        {
            const char *while_pos = strstr(trimmed, "while");
            state->ends_with_do_block = (while_pos == NULL) ? 1 : 0;
        }
    }
}

static int PicocApp_IsSourceComplete(const char *source, const PicocApp_SourceState *state)
{
    const char *trimmed = PicocApp_SkipLeadingSpace(source);

    if (*trimmed == '\0')
    {
        return 1;
    }

    if (state->in_string != 0 || state->in_char != 0 || state->in_block_comment != 0)
    {
        return 0;
    }

    if (state->paren_depth != 0 || state->bracket_depth != 0 || state->brace_depth != 0)
    {
        return 0;
    }

    if (state->last_non_space == ';')
    {
        return 1;
    }

    if (state->last_non_space == '}')
    {
        return (state->ends_with_do_block == 0) ? 1 : 0;
    }

    if (*trimmed == '#')
    {
        return 1;
    }

    return PicocApp_ShouldAutoPrintExpression(source, state);
}

static int PicocApp_ShouldAutoPrintExpression(const char *source, const PicocApp_SourceState *state)
{
    const char *trimmed = PicocApp_SkipLeadingSpace(source);
    const char *end = PicocApp_FindTrimmedEnd(trimmed);
    uint32_t index;

    if (*trimmed == '\0')
    {
        return 0;
    }

    if (state->brace_depth != 0 || state->paren_depth != 0 || state->bracket_depth != 0)
    {
        return 0;
    }

    if (state->last_non_space == ';' || state->last_non_space == '}' || state->last_non_space == ':' || state->last_non_space == '{')
    {
        return 0;
    }

    if (*trimmed == '#')
    {
        return 0;
    }

    if (PicocApp_StartsWithKeyword(trimmed, "if") != 0 ||
        PicocApp_StartsWithKeyword(trimmed, "for") != 0 ||
        PicocApp_StartsWithKeyword(trimmed, "while") != 0 ||
        PicocApp_StartsWithKeyword(trimmed, "switch") != 0 ||
        PicocApp_StartsWithKeyword(trimmed, "do") != 0 ||
        PicocApp_StartsWithKeyword(trimmed, "else") != 0 ||
        PicocApp_StartsWithKeyword(trimmed, "return") != 0 ||
        PicocApp_StartsWithKeyword(trimmed, "typedef") != 0 ||
        PicocApp_StartsWithKeyword(trimmed, "struct") != 0 ||
        PicocApp_StartsWithKeyword(trimmed, "union") != 0 ||
        PicocApp_StartsWithKeyword(trimmed, "enum") != 0 ||
        PicocApp_StartsWithKeyword(trimmed, "int") != 0 ||
        PicocApp_StartsWithKeyword(trimmed, "char") != 0 ||
        PicocApp_StartsWithKeyword(trimmed, "short") != 0 ||
        PicocApp_StartsWithKeyword(trimmed, "long") != 0 ||
        PicocApp_StartsWithKeyword(trimmed, "void") != 0 ||
        PicocApp_StartsWithKeyword(trimmed, "unsigned") != 0 ||
        PicocApp_StartsWithKeyword(trimmed, "signed") != 0 ||
        PicocApp_StartsWithKeyword(trimmed, "static") != 0 ||
        PicocApp_StartsWithKeyword(trimmed, "extern") != 0 ||
        PicocApp_StartsWithKeyword(trimmed, "auto") != 0 ||
        PicocApp_StartsWithKeyword(trimmed, "register") != 0 ||
        PicocApp_StartsWithKeyword(trimmed, "break") != 0 ||
        PicocApp_StartsWithKeyword(trimmed, "continue") != 0 ||
        PicocApp_StartsWithKeyword(trimmed, "goto") != 0)
    {
        return 0;
    }

    for (index = 0U; trimmed[index] != '\0' && &trimmed[index] < end; index++)
    {
        if (trimmed[index] == '{')
        {
            return 0;
        }
    }

    return 1;
}

static const char *PicocApp_SkipLeadingSpace(const char *text)
{
    while (*text == ' ' || *text == '\t' || *text == '\r' || *text == '\n')
    {
        text++;
    }

    return text;
}

static const char *PicocApp_FindTrimmedEnd(const char *text)
{
    const char *end = text + strlen(text);

    while (end > text)
    {
        char ch = end[-1];
        if (ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n')
        {
            break;
        }
        end--;
    }

    return end;
}

static int PicocApp_StartsWithKeyword(const char *text, const char *keyword)
{
    uint32_t len = (uint32_t)strlen(keyword);
    char next = text[len];

    if (strncmp(text, keyword, len) != 0)
    {
        return 0;
    }

    if ((next >= 'a' && next <= 'z') ||
        (next >= 'A' && next <= 'Z') ||
        (next >= '0' && next <= '9') ||
        next == '_')
    {
        return 0;
    }

    return 1;
}

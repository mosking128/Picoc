
/*
 * grep - modified as self-contained test program
 * Original: simplified implementation of UNIX grep command
 * Modified: added built-in test function, no command line arguments needed
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define LMAX    512
#define PMAX    256

#define CHAR    1
#define BOL     2
#define EOL     3
#define ANY     4
#define CLASS   5
#define NCLASS  6
#define STAR    7
#define PLUS    8
#define MINUS   9
#define ALPHA   10
#define DIGIT   11
#define NALPHA  12
#define PUNCT   13
#define RANGE   14
#define ENDPAT  15

int cflag=0, fflag=0, nflag=0, vflag=0, nfile=0, debug=0;

char *pp, lbuf[LMAX], pbuf[PMAX];

char *cclass();
int pmatch();


/*** Display a file name *******************************/
void file(char *s)
{
   printf("File %s:\n", s);
}

/*** Report unopenable file ****************************/
void cant(char *s)
{
   fprintf(stderr, "%s: cannot open\n", s);
}

/*** Give good help ************************************/
void help(char **hp)
{
   char   **dp;

   for (dp = hp; *dp; ++dp)
      printf("%s\n", *dp);
}

/*** Display usage summary *****************************/
void usage(char *s)
{
   fprintf(stderr, "?GREP-E-%s\n", s);
   fprintf(stderr,
      "Usage: grep [-cfnv] pattern [file ...].  grep ? for help\n");
   exit(1);
}

/*** Compile the pattern into global pbuf[] ************/
void compile(char *source)
{
   char  *s;         /* Source string pointer     */
   char  *lp;        /* Last pattern pointer      */
   int   c;          /* Current character         */
   int            o;          /* Temp                      */
   char           *spp;       /* Save beginning of pattern */

   s = source;
   if (debug)
      printf("Pattern = \"%s\"\n", s);
   pp = pbuf;
   while (c = *s++) {
      /*
       * STAR, PLUS and MINUS are special.
       */
      if (c == '*' || c == '+' || c == '-') {
         if (pp == pbuf ||
              (o=pp[-1]) == BOL ||
              o == EOL ||
              o == STAR ||
              o == PLUS ||
              o == MINUS)
            badpat("Illegal occurrance op.", source, s);
         store(ENDPAT);
         store(ENDPAT);
         spp = pp;               /* Save pattern end     */
         while (--pp > lp)       /* Move pattern down    */
            *pp = pp[-1];        /* one byte             */
         *pp =   (c == '*') ? STAR :
            (c == '-') ? MINUS : PLUS;
         pp = spp;               /* Restore pattern end  */
         continue;
      }
      /*
       * All the rest.
       */
      lp = pp;         /* Remember start       */
      switch(c) {

      case '^':
         store(BOL);
         break;

      case '$':
         store(EOL);
         break;

      case '.':
         store(ANY);
         break;

      case '[':
         s = cclass(source, s);
         break;

      case ':':
         if (*s) {
            switch(tolower(c = *s++)) {

            case 'a':
            case 'A':
               store(ALPHA);
               break;

            case 'd':
            case 'D':
               store(DIGIT);
               break;

            case 'n':
            case 'N':
               store(NALPHA);
               break;

            case ' ':
               store(PUNCT);
               break;

            default:
               badpat("Unknown : type", source, s);

            }
            break;
         }
         else    badpat("No : type", source, s);

      case '\\':
         if (*s)
            c = *s++;

      default:
         store(CHAR);
         store(tolower(c));
      }
   }
   store(ENDPAT);
   store(0);                /* Terminate string     */
   if (debug) {
      for (lp = pbuf; lp < pp;) {
         if ((c = (*lp++ & 0377)) < ' ')
            printf("\\%o ", c);
         else    printf("%c ", c);
        }
        printf("\n");
   }
}

/*** Compile a class (within []) ***********************/
char *cclass(char *source, char *src)
/* char       *source;   // Pattern start -- for error msg. */
/* char       *src;      // Class start */
{
   char   *s;        /* Source pointer    */
   char   *cp;       /* Pattern start     */
   int    c;         /* Current character */
   int             o;         /* Temp              */

   s = src;
   o = CLASS;
   if (*s == '^') {
      ++s;
      o = NCLASS;
   }
   store(o);
   cp = pp;
   store(0);                          /* Byte count      */
   while ((c = *s++) && c!=']') {
      if (c == '\\') {                /* Store quoted char    */
         if ((c = *s++) == '\0')      /* Gotta get something  */
            badpat("Class terminates badly", source, s);
         else    store(tolower(c));
      }
      else if (c == '-' &&
            (pp - cp) > 1 && *s != ']' && *s != '\0') {
         c = pp[-1];             /* Range start     */
         pp[-1] = RANGE;         /* Range signal    */
         store(c);               /* Re-store start  */
         c = *s++;               /* Get end char and*/
         store(tolower(c));      /* Store it        */
      }
      else {
         store(tolower(c));      /* Store normal char */
      }
   }
   if (c != ']')
      badpat("Unterminated class", source, s);
   if ((c = (pp - cp)) >= 256)
      badpat("Class too large", source, s);
   if (c == 0)
      badpat("Empty class", source, s);
   *cp = c;
   return(s);
}

/*** Store an entry in the pattern buffer **************/
void store(int op)
{
   /* Simplified for testing - assume pattern fits in buffer */
   *pp++ = op;
}

/*** Report a bad pattern specification ****************/
void badpat(char *message, char *source, char *stop)
/* char  *message;       // Error message */
/* char  *source;        // Pattern start */
/* char  *stop;          // Pattern end   */
{
   fprintf(stderr, "-GREP-E-%s, pattern is\"%s\"\n", message, source);
   fprintf(stderr, "-GREP-E-Stopped at byte %d, '%c'\n",
         stop-source, stop[-1]);
   error("?GREP-E-Bad pattern\n");
}

/*** Simple line reading function for PicoC compatibility ***/
char *read_line(FILE *fp, char *buf, int maxlen)
{
    int i = 0;
    int c;
    
    while (i < maxlen - 1) {
        c = fgetc(fp);
        if (c == EOF) {
            if (i == 0) return NULL;
            break;
        }
        if (c == '\n') {
            buf[i++] = c;
            break;
        }
        buf[i++] = c;
    }
    buf[i] = '\0';
    return buf;
}

/*** Scan the file for the pattern in pbuf[] ***********/
void grep(FILE *fp, char *fn)
/* FILE       *fp;       // File to process            */
/* char       *fn;       // File name (for -f option)  */
{
   int lno, count, m;

   lno = 0;
   count = 0;
   while (read_line(fp, lbuf, LMAX) != NULL) {
      ++lno;
      m = match();
      if ((m && !vflag) || (!m && vflag)) {
         ++count;
         if (!cflag) {
            if (fflag && fn) {
               file(fn);
               fn = 0;
            }
            if (nflag)
               printf("%d\t", lno);
            printf("%s", lbuf);
            if (lbuf[strlen(lbuf)-1] != '\n')
                printf("\n");
         }
      }
   }
   if (cflag) {
      if (fflag && fn)
         file(fn);
      printf("%d\n", count);
   }
}

/*** Match line (lbuf) with pattern (pbuf) return 1 if match ***/
int match()
{
   char   *l;        /* Line pointer       */

   for (l = lbuf; *l; ++l) {
      if (pmatch(l, pbuf) != 0)
         return(1);
   }
   return(0);
}

/*** Match partial line with pattern *******************/
int pmatch(char *line, char *pattern)
{
   char   *l = line;        /* Current line pointer         */
   char   *p = pattern;     /* Current pattern pointer      */
   char   c;                /* Current character            */
   char   *e;               /* End for STAR and PLUS match  */
   int    op;               /* Pattern operation            */
   int    n;                /* Class counter                */
   char   *are;             /* Start of STAR match          */

   if (debug > 1)
      printf("pmatch(\"%s\")\n", line);
   
   while ((op = *p++) != ENDPAT) {
      if (debug > 1)
         printf("byte = '%c', op = %d\n", *l, op);
      
      switch(op) {

      case CHAR:
         if (tolower(*l++) != *p++)
            return(0);
         break;

      case BOL:
         /* Check if we're at the beginning of the line */
         /* Use string comparison instead of pointer comparison */
         if (strcmp(line, lbuf) != 0)  /* If not at the very beginning */
            return(0);
         break;

      case EOL:
         if (*l != '\0')
            return(0);
         break;

      case ANY:
         if (*l++ == '\0')
            return(0);
         break;

      case DIGIT:
         if ((c = *l++) < '0' || (c > '9'))
            return(0);
         break;

      case ALPHA:
         c = tolower(*l++);
         if (c < 'a' || c > 'z')
            return(0);
         break;

      case NALPHA:
         c = tolower(*l++);
         if (c >= 'a' && c <= 'z')
            break;
         else if (c < '0' || c > '9')
            return(0);
         break;

      case PUNCT:
         c = *l++;
         if (c == 0 || c > ' ')
            return(0);
         break;

      case CLASS:
      case NCLASS:
         c = tolower(*l++);
         n = *p++ & 0377;
         do {
            if (*p == RANGE) {
               p += 3;
               n -= 2;
               if (c >= p[-2] && c <= p[-1])
                  break;
            }
            else if (c == *p++)
               break;
         } while (--n > 1);
         if ((op == CLASS) == (n <= 1))
            return(0);
         if (op == CLASS)
            p += n - 2;
         break;

      case MINUS:
         e = (char *)pmatch(l, p);       /* Look for a match    */
         while (*p++ != ENDPAT); /* Skip over pattern   */
         if (e)                  /* Got a match?        */
            l = e;               /* Yes, update string  */
         break;                  /* Always succeeds     */

      case PLUS:                 /* One or more ...     */
         e = (char *)pmatch(l, p);
         if (e == 0)
            return(0);           /* Gotta have a match  */
         l = e;
      case STAR:                 /* Zero or more ...    */
         are = l;                /* Remember line start */
         while (*l && (e = (char *)pmatch(l, p)))
            l = e;               /* Get longest match   */
         while (*p++ != ENDPAT); /* Skip over pattern   */
         while (l >= are) {      /* Try to match rest   */
            e = (char *)pmatch(l, p);
            if (e)
               return((int)e);
            --l;                 /* Nope, try earlier   */
         }
         return(0);              /* Nothing else worked */

      default:
         printf("Bad op code %d\n", op);
         error("Cannot happen -- match\n");
      }
   }
   return((int)l);
}

/*** Report an error ***********************************/
void error(char *s)
{
   fprintf(stderr, "%s", s);
   exit(1);
}

/*** Test function: test grep basic function *********************/
void test_grep_basic() {
    printf("=== Testing grep basic function ===\n");
    
    // Test 1: Create test file
    FILE *test_file = fopen("grep_test.txt", "w");
    if (test_file == NULL) {
        printf("Error: Cannot create test file\n");
        return;
    }
    
    fprintf(test_file, "Hello world\n");
    fprintf(test_file, "This is a test line\n");
    fprintf(test_file, "Another test here\n");
    fprintf(test_file, "No match in this line\n");
    fprintf(test_file, "final test\n");
    fclose(test_file);
    
    // Test 2: Search "test"
    printf("Test 1: Search 'test' pattern\n");
    compile("test");
    FILE *fp = fopen("grep_test.txt", "r");
    if (fp != NULL) {
        int count = 0;
        while (read_line(fp, lbuf, LMAX) != NULL) {
            if (match()) {
                count++;
                printf("  Matched line: %s", lbuf);
            }
        }
        fclose(fp);
        printf("  Found %d matches\n", count);
        if (count == 3) {
            printf("  Test PASSED\n");
        } else {
            printf("  Test FAILED\n");
        }
    }
    
    // Test 3: Search lines starting with "A"
    printf("\nTest 2: Search lines starting with 'A'\n");
    compile("^A");
    fp = fopen("grep_test.txt", "r");
    if (fp != NULL) {
        int count = 0;
        while (read_line(fp, lbuf, LMAX) != NULL) {
            if (match()) {
                count++;
                printf("  Matched line: %s", lbuf);
            }
        }
        fclose(fp);
        printf("  Found %d matches\n", count);
        if (count == 1) {
            printf("  Test PASSED\n");
        } else {
            printf("  Test FAILED\n");
        }
    }
    
    // Test 4: Search lines ending with "line"
    printf("\nTest 3: Search lines ending with 'line'\n");
    compile("line$");
    fp = fopen("grep_test.txt", "r");
    if (fp != NULL) {
        int count = 0;
        while (read_line(fp, lbuf, LMAX) != NULL) {
            if (match()) {
                count++;
                printf("  Matched line: %s", lbuf);
            }
        }
        fclose(fp);
        printf("  Found %d matches\n", count);
        if (count == 2) {
            printf("  Test PASSED\n");
        } else {
            printf("  Test FAILED\n");
        }
    }
    
    // Clean up test file
    remove("grep_test.txt");
    
    printf("\n=== grep test completed ===\n\n");
}

/*** Main program - modified as self-contained test program *******************/
int main(int argc, char **argv)
{
   printf("========================================\n");
   printf("grep Test Program (PicoC Compatible Version)\n");
   printf("========================================\n");
   
   // If command line arguments are provided, execute original function
   if (argc > 1 && strcmp(argv[1], "--test") != 0) {
       // Original command line logic (keep compatibility)
       char   *p;
       int    c, i;
       int             gotpattern;

       FILE            *f;

       if (argc <= 1)
          usage("No arguments");
       if (argc == 2 && argv[1][0] == '?' && argv[1][1] == 0) {
          // help(documentation);
          // help(patdoc);
          return 0;
          }
       nfile = argc-1;
       gotpattern = 0;
       for (i=1; i < argc; ++i) {
          p = argv[i];
          if (*p == '-') {
             ++p;
             while (c = *p++) {
                switch(tolower(c)) {

                case '?':
                   // help(documentation);
                   break;

                case 'C':
                case 'c':
                   ++cflag;
                   break;

                case 'D':
                case 'd':
                   ++debug;
                   break;

                case 'F':
                case 'f':
                   ++fflag;
                   break;

                case 'n':
                case 'N':
                   ++nflag;
                   break;

                case 'v':
                case 'V':
                   ++vflag;
                   break;

                default:
                   usage("Unknown flag");
                }
             }
             argv[i] = 0;
             --nfile;
          } else if (!gotpattern) {
             compile(p);
             argv[i] = 0;
             ++gotpattern;
             --nfile;
          }
       }
       if (!gotpattern)
          usage("No pattern");
       if (nfile == 0)
          grep(stdin, 0);
       else {
          fflag = fflag ^ (nfile > 0);
          for (i=1; i < argc; ++i) {
             if (p = argv[i]) {
                if ((f=fopen(p, "r")) == NULL)
                   cant(p);
                else {
                   grep(f, p);
                   fclose(f);
                }
             }
          }
       }
   } else {
       // No arguments or --test argument: execute built-in test
       test_grep_basic();
   }
   
   return 0;
}

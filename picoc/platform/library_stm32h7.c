#include "../interpreter.h"

static const char StdboolDefs[] =
    "typedef int bool;\n"
    "#define true 1\n"
    "#define false 0\n";

#ifndef NO_FP
static double M_EValue =        2.7182818284590452354;
static double M_LOG2EValue =    1.4426950408889634074;
static double M_LOG10EValue =   0.43429448190325182765;
static double M_LN2Value =      0.69314718055994530942;
static double M_LN10Value =     2.30258509299404568402;
static double M_PIValue =       3.14159265358979323846;
static double M_PI_2Value =     1.57079632679489661923;
static double M_PI_4Value =     0.78539816339744830962;
static double M_1_PIValue =     0.31830988618379067154;
static double M_2_PIValue =     0.63661977236758134308;
static double M_2_SQRTPIValue = 1.12837916709551257390;
static double M_SQRT2Value =    1.41421356237309504880;
static double M_SQRT1_2Value =  0.70710678118654752440;

static void MathSin(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)NumArgs;
    ReturnValue->Val->FP = sin(Param[0]->Val->FP);
}

static void MathCos(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)NumArgs;
    ReturnValue->Val->FP = cos(Param[0]->Val->FP);
}

static void MathTan(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)NumArgs;
    ReturnValue->Val->FP = tan(Param[0]->Val->FP);
}

static void MathAsin(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)NumArgs;
    ReturnValue->Val->FP = asin(Param[0]->Val->FP);
}

static void MathAcos(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)NumArgs;
    ReturnValue->Val->FP = acos(Param[0]->Val->FP);
}

static void MathAtan(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)NumArgs;
    ReturnValue->Val->FP = atan(Param[0]->Val->FP);
}

static void MathAtan2(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)NumArgs;
    ReturnValue->Val->FP = atan2(Param[0]->Val->FP, Param[1]->Val->FP);
}

static void MathSinh(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)NumArgs;
    ReturnValue->Val->FP = sinh(Param[0]->Val->FP);
}

static void MathCosh(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)NumArgs;
    ReturnValue->Val->FP = cosh(Param[0]->Val->FP);
}

static void MathTanh(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)NumArgs;
    ReturnValue->Val->FP = tanh(Param[0]->Val->FP);
}

static void MathExp(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)NumArgs;
    ReturnValue->Val->FP = exp(Param[0]->Val->FP);
}

static void MathFabs(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)NumArgs;
    ReturnValue->Val->FP = fabs(Param[0]->Val->FP);
}

static void MathFmod(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)NumArgs;
    ReturnValue->Val->FP = fmod(Param[0]->Val->FP, Param[1]->Val->FP);
}

static void MathFrexp(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)NumArgs;
    ReturnValue->Val->FP = frexp(Param[0]->Val->FP, (int *)Param[1]->Val->Pointer);
}

static void MathLdexp(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)NumArgs;
    ReturnValue->Val->FP = ldexp(Param[0]->Val->FP, Param[1]->Val->Integer);
}

static void MathLog(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)NumArgs;
    ReturnValue->Val->FP = log(Param[0]->Val->FP);
}

static void MathLog10(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)NumArgs;
    ReturnValue->Val->FP = log10(Param[0]->Val->FP);
}

static void MathModf(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)NumArgs;
    ReturnValue->Val->FP = modf(Param[0]->Val->FP, (double *)Param[1]->Val->Pointer);
}

static void MathPow(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)NumArgs;
    ReturnValue->Val->FP = pow(Param[0]->Val->FP, Param[1]->Val->FP);
}

static void MathSqrt(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)NumArgs;
    ReturnValue->Val->FP = sqrt(Param[0]->Val->FP);
}

static void MathRound(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)NumArgs;
    ReturnValue->Val->FP = floor(Param[0]->Val->FP + 0.5);
}

static void MathCeil(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)NumArgs;
    ReturnValue->Val->FP = ceil(Param[0]->Val->FP);
}

static void MathFloor(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)NumArgs;
    ReturnValue->Val->FP = floor(Param[0]->Val->FP);
}

static struct LibraryFunction MathFunctions[] =
{
    { MathAcos,   "float acos(float);" },
    { MathAsin,   "float asin(float);" },
    { MathAtan,   "float atan(float);" },
    { MathAtan2,  "float atan2(float, float);" },
    { MathCeil,   "float ceil(float);" },
    { MathCos,    "float cos(float);" },
    { MathCosh,   "float cosh(float);" },
    { MathExp,    "float exp(float);" },
    { MathFabs,   "float fabs(float);" },
    { MathFloor,  "float floor(float);" },
    { MathFmod,   "float fmod(float, float);" },
    { MathFrexp,  "float frexp(float, int *);" },
    { MathLdexp,  "float ldexp(float, int);" },
    { MathLog,    "float log(float);" },
    { MathLog10,  "float log10(float);" },
    { MathModf,   "float modf(float, float *);" },
    { MathPow,    "float pow(float, float);" },
    { MathRound,  "float round(float);" },
    { MathSin,    "float sin(float);" },
    { MathSinh,   "float sinh(float);" },
    { MathSqrt,   "float sqrt(float);" },
    { MathTan,    "float tan(float);" },
    { MathTanh,   "float tanh(float);" },
    { NULL,       NULL }
};

static void MathSetupFunc(Picoc *pc)
{
    VariableDefinePlatformVar(pc, NULL, "M_E", &pc->FPType, (union AnyValue *)&M_EValue, FALSE);
    VariableDefinePlatformVar(pc, NULL, "M_LOG2E", &pc->FPType, (union AnyValue *)&M_LOG2EValue, FALSE);
    VariableDefinePlatformVar(pc, NULL, "M_LOG10E", &pc->FPType, (union AnyValue *)&M_LOG10EValue, FALSE);
    VariableDefinePlatformVar(pc, NULL, "M_LN2", &pc->FPType, (union AnyValue *)&M_LN2Value, FALSE);
    VariableDefinePlatformVar(pc, NULL, "M_LN10", &pc->FPType, (union AnyValue *)&M_LN10Value, FALSE);
    VariableDefinePlatformVar(pc, NULL, "M_PI", &pc->FPType, (union AnyValue *)&M_PIValue, FALSE);
    VariableDefinePlatformVar(pc, NULL, "M_PI_2", &pc->FPType, (union AnyValue *)&M_PI_2Value, FALSE);
    VariableDefinePlatformVar(pc, NULL, "M_PI_4", &pc->FPType, (union AnyValue *)&M_PI_4Value, FALSE);
    VariableDefinePlatformVar(pc, NULL, "M_1_PI", &pc->FPType, (union AnyValue *)&M_1_PIValue, FALSE);
    VariableDefinePlatformVar(pc, NULL, "M_2_PI", &pc->FPType, (union AnyValue *)&M_2_PIValue, FALSE);
    VariableDefinePlatformVar(pc, NULL, "M_2_SQRTPI", &pc->FPType, (union AnyValue *)&M_2_SQRTPIValue, FALSE);
    VariableDefinePlatformVar(pc, NULL, "M_SQRT2", &pc->FPType, (union AnyValue *)&M_SQRT2Value, FALSE);
    VariableDefinePlatformVar(pc, NULL, "M_SQRT1_2", &pc->FPType, (union AnyValue *)&M_SQRT1_2Value, FALSE);
}
#endif

static void Stm32SetupFunc(Picoc *pc)
{
    (void)pc;
}

static struct LibraryFunction Stm32Functions[] =
{
    { NULL, NULL }
};

void PlatformLibraryInit(Picoc *pc)
{
    IncludeRegister(pc, "stdio.h", NULL, NULL, NULL);
    IncludeRegister(pc, "stdlib.h", NULL, NULL, NULL);
    IncludeRegister(pc, "string.h", NULL, NULL, NULL);
    IncludeRegister(pc, "stdbool.h", NULL, NULL, StdboolDefs);
#ifndef NO_FP
    IncludeRegister(pc, "math.h", &MathSetupFunc, &MathFunctions[0], NULL);
#endif
    IncludeRegister(pc, "picoc_stm32.h", &Stm32SetupFunc, &Stm32Functions[0], NULL);
}

#ifndef DEF
#define DEF(token, value)
#endif // DEF

#define KEYWORDS                                                               \
    DEF(FUNCTION_START, "fn");                                                 \
    DEF(FUNCTION_RET_TYPE, "->");                                              \
    DEF(IF, "if");                                                             \
    DEF(ELSE, "else");                                                         \
    DEF(RETURN, "return");                                                     \
    DEF(DEF, "def");
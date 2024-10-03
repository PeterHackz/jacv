#ifndef DEF
#define DEF(token, value)
#endif // DEF

#define _TYPED(type)                                                           \
    DEF(Primitve_##type##8, #type "8");                                        \
    DEF(Primitve_##type##16, #type "16");                                      \
    DEF(Primitve_##type##32, #type "32");                                      \
    DEF(Primitve_##type##64, #type "64");

#define TYPED_INTS()                                                           \
    _TYPED(I)                                                                  \
    _TYPED(U)

#define PRIMITIVES TYPED_INTS()
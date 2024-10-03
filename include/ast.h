#ifndef AST_H
#define AST_H

#include "defines.h"
#include "jcontext.h"
#include "jutils/jutils.h"
#include "lexer.h"
#include "list.h"
#include <stdint.h>

enum
{
    TYPE_NONE,
    TYPE_BODY,
    TYPE_DECLARATION,
    TYPE_EXPRESSION,
    TYPE_SCOPE,
    TYPE_FUNCTION,
    TYPE_FUNCTION_CALL,
    TYPE_FUNCTION_DECLARATION,
    TYPE_FUNCTION_PARAMETER,
    TYPE_IF,
    TYPE_LITERAL,
    TYPE_NODE,
    TYPE_RETURN,
    TYPE_VARIABLE,
    TYPE_WHILE,
    TYPE_IDENTIFIER,
    TYPE_STRING,
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_OPERATION,
};

typedef struct ASTExpression ASTExpression;
typedef struct ASTNode ASTNode;

LIST_INIT_TYPE(ASTExpression);
LIST_INIT_TYPE(ASTNode);

#ifndef DUMP_EXPR
#define DUMP(type)
#else
#define DUMP(type) void (*dump)(struct type *, JContext * jctx)
#endif

#define DESTROY(type) void (*destroy)(struct type *, JContext * jctx)

#define AST_EXPRESSION(_type)                                                  \
    int type;                                                                  \
    DESTROY(_type);                                                            \
    DUMP(_type)

typedef struct ASTExpression
{
    AST_EXPRESSION(ASTExpression);
} ASTExpression;

typedef struct ASTNode
{
    int type;
    LIST_TYPEOF(ASTExpression) * body;
    DESTROY(ASTNode);
    JContext *jctx;
} ASTNode;

typedef struct DeclarationExpression
{
    AST_EXPRESSION(DeclarationExpression);

    struct StringExpression *var;
    struct ASTExpression *value;

    const char *varType;
} DeclarationExpression;

typedef struct StringExpression
{
    AST_EXPRESSION(StringExpression);

    char *value;
    int size;
} StringExpression;

typedef struct NumericalValue
{
    AST_EXPRESSION(NumericalValue);

    union
    {
        uint64_t uint64;
        double float64;
    } value;
} NumericalValue;

typedef struct BinaryOperation
{
    AST_EXPRESSION(BinaryOperation);

    struct ASTExpression *left;
    struct ASTExpression *right;

    bool wrapped; // (a <op> b)

    char op;
} BinaryOperation;

typedef struct VariableExpression
{
    AST_EXPRESSION(VariableExpression);

    char *value;
    int size;
} VariableExpression;

typedef struct ScopeExpression
{
    AST_EXPRESSION(ScopeExpression);
    ASTNode *body;
} ScopeExpression;

typedef struct FunctionArg
{
    char *name;
    char *type;
} FunctionArg;

LIST_INIT_TYPE(FunctionArg);

typedef struct FunctionExpression
{
    AST_EXPRESSION(FunctionExpression);
    ScopeExpression *scope;
    char *name;
    char *retType;
    LIST_TYPEOF(FunctionArg) * args;
} FunctionExpression;

MAKE_CTOR_DEC(ASTNode, ast_node);
MAKE_CTOR_DEC(ASTExpression, ast_expr);
MAKE_CTOR_DEC(DeclarationExpression, declaration_expr);
MAKE_CTOR_DEC(StringExpression, string_expr);
MAKE_CTOR_DEC(NumericalValue, numerical_value);
MAKE_CTOR_DEC(BinaryOperation, bin_op);
MAKE_CTOR_DEC(VariableExpression, variable);
MAKE_CTOR_DEC(FunctionExpression, func_expr);
MAKE_CTOR_DEC(ScopeExpression, scope_expr);

MAKE_DTOR_DEC(ASTNode, ast_node);
MAKE_DTOR_DEC(DeclarationExpression, declaration_expr);
MAKE_DTOR_DEC(StringExpression, string_expr);
MAKE_DTOR_DEC(VariableExpression, variable);
MAKE_DTOR_DEC(BinaryOperation, bin_op);
MAKE_DTOR_DEC(FunctionExpression, func_expr);
MAKE_DTOR_DEC(ScopeExpression, scope_expr);

ASTNode *ast_from_lexer(Lexer *lexer);

void expr_print(ASTExpression *expr);
void expr__print(ASTExpression *expr, JContext *jctx);

void ast_print(ASTNode *node);
void ast_print_node(ASTNode *node, int indent);

#endif // AST_H
#ifndef AST_H
#define AST_H

#include "jutils.h"
#include "lexer.h"
#include "list.h"

enum
{
    TYPE_BODY,
    TYPE_DECLARATION,
    TYPE_EXPRESSION,
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
    TYPE_STRING
};

typedef struct ASTExpression ASTExpression;
typedef struct ASTNode ASTNode;

LIST_INIT_TYPE(ASTExpression);
LIST_INIT_TYPE(ASTNode);

typedef struct ASTExpression
{
    int type;
    void (*destructor)(struct ASTExpression *);
} ASTExpression;

typedef struct ASTNode
{
    int type;
    LIST_TYPEOF(ASTExpression) * body;
    void (*destructor)(struct ASTNode *);
} ASTNode;

typedef struct DeclarationExpression
{
    int type;
    void (*destructor)(struct DeclarationExpression *);
    struct StringExpression *var;
    struct ASTExpression *value;
} DeclarationExpression;

typedef struct StringExpression
{
    int type;
    void (*destructor)(struct StringExpression *);
    char *value;
    int size;
} StringExpression;

MAKE_CTOR_DEC(ASTNode, ast_node);
MAKE_CTOR_DEC(ASTExpression, ast_expr);
MAKE_CTOR_DEC(DeclarationExpression, declaration_expr);
MAKE_CTOR_DEC(StringExpression, string_expr);

MAKE_DTOR_DEC(ASTNode, ast_node);
MAKE_DTOR_DEC(DeclarationExpression, declaration_expr);
MAKE_DTOR_DEC(StringExpression, string_expr);

ASTNode *ast_from_lexer(Lexer *lexer);

void expr_print(ASTExpression *expr);
void expr__print(ASTExpression *expr, int indent);

void ast_print(ASTNode *node);
void ast_print_node(ASTNode *node, int indent);

#endif // AST_H
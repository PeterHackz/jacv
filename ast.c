#include "include/ast.h"
#include "include/jmemory.h"
#include "include/jutils.h"
#include "stdio.h"

#ifdef DEF
#undef DEF
#endif

#define DEF(token, value) const char *TOK_##token = (char *)value
#include "include/keywords.h"
KEYWORDS
#undef DEF

char *expr2str(int expr)
{
#define FIELD(name)                                                            \
    if (expr == TOKEN_TYPE_##name)                                             \
        return #name;
#include "include/tokens.h"
    TOKENS
#undef FIELD
    return NULL;
}

MAKE_CTOR(ASTNode, ast_node, {
    ast_node->type = TYPE_BODY;
    ast_node->body = LIST_CREATE_EMPTY(ASTExpression);
    ast_node->destructor = NULL;
});

ASTNode *ast_from_lexer(Lexer *lexer)
{
    char error[50];
    error[0] = 0;
#define SET_ERROR(str)                                                         \
    memcpy(error, str, sizeof(str));                                           \
    error[sizeof(str)] = 0;

#define CHECK_BOUNDS(i)                                                        \
    if (!(i < tokens->size))                                                   \
        goto bounds_error;
    ASTNode *node = ast_node_new();
    LIST_TYPEOF(Token) *tokens = lexer->tokens;
    int i = 0;
    while (i < tokens->size)
    {
        Token *token = tokens->items[i];
        if (token->type == TOKEN_TYPE_IDENTIFIER)
        {
            if (STREQ(token->value, TOK_DEF))
            {
                CHECK_BOUNDS(++i);
                DeclarationExpression *expr = declaration_expr_new();
                Token *varToken = tokens->items[i];
                if (varToken->type != TOKEN_TYPE_IDENTIFIER)
                {
                    snprintf(error, 50,
                             "expected identifier after 'def', got %s",
                             varToken->value);
                    goto err;
                }
                expr->var = string_expr_new();
                expr->var->value = varToken->value;
                varToken->value =
                    NULL; // prevent free if lexer is destroyed b4 ast is done
                LIST_PUSH(node->body, expr);
            }
        }
        i++;
    }
    // ASTExpression *expr = ast_expression_new();
    return node;
bounds_error:
    SET_ERROR("bounds error");
err:
    printf("%s\n", error[0] == 0 ? "unknown error" : error);
    ast_node_destruct(node);
    return NULL;
}

void expr__print(ASTExpression *expr, int indent)
{
    switch (expr->type)
    {
    case TYPE_DECLARATION:
    {
        DeclarationExpression *declExpr = (DeclarationExpression *)expr;
        printf("DeclarationExpression: %s\n", declExpr->var->value);
    }
    break;
    case TYPE_STRING:
    {
        StringExpression *strExpr = (StringExpression *)expr;
        printf("StringExpression: %s\n", strExpr->value);
    }
    break;
    default:
        printf("UnknownExpression: %d\n", expr->type);
        break;
    }
}

void expr_print(ASTExpression *expr) { expr__print(expr, 0); }

void ast_print_node(ASTNode *node, int indent)
{
    for (int i = 0; i < indent; i++)
        printf(" ");
    printf("ASTNode: %d\n", node->type);
    if (node->type == TYPE_BODY)
    {
        LIST_FOREACH(node->body, ASTExpression *,
                     { expr__print(item, indent + 1); });
    }
}

void ast_print(ASTNode *node) { ast_print_node(node, 0); }

MAKE_DTOR(ASTNode, ast_node, {
    if (ast_node->type == TYPE_BODY)
    {
        LIST_FOREACH(ast_node->body, ASTExpression *, {
            if (item->destructor != NULL)
                item->destructor(item);
        });
        LIST_FREE(ast_node->body);
    }
    if (ast_node->destructor != NULL)
        ast_node->destructor(
            ast_node); /* special code destructors, ex if the node have
                  memory resources more than a body */
    FREE(ast_node);
});

MAKE_CTOR_QUICK(ASTExpression, ast_expression_new);

MAKE_CTOR(StringExpression, string_expr, {
    string_expr->type = TYPE_STRING;
    string_expr->value = NULL;
    string_expr->destructor = &string_expr_destruct;
});

MAKE_CTOR(DeclarationExpression, declaration_expr, {
    declaration_expr->type = TYPE_DECLARATION;
    declaration_expr->var = NULL;
    declaration_expr->value = NULL;
    declaration_expr->destructor = &declaration_expr_destruct;
});

MAKE_DTOR(DeclarationExpression, declaration_expr, {
    if (declaration_expr->var != NULL)
    {
        if (declaration_expr->var->destructor != NULL)
            declaration_expr->var->destructor(declaration_expr->var);
        else
            FREE(declaration_expr->var);
    }
    if (declaration_expr->value != NULL)
    {
        if (declaration_expr->value->destructor != NULL)
            declaration_expr->value->destructor(declaration_expr->value);
        else
            FREE(declaration_expr->value);
    }
    FREE(declaration_expr);
})

MAKE_DTOR(StringExpression, string_expr, {
    if (string_expr->value != NULL)
        FREE(string_expr->value);
    FREE(string_expr);
})
#include "ast.h"
#include "jcontext.h"
#include "jutils/jutils.h"
#include "lexer.h"
#include "list.h"
#include "stdio.h"
#include <ctype.h>
#include <string.h>

#include "defines.h"

#include "jutils/string.h"
#include "stdlib.h"

#define INDENT()                                                               \
    for (int __i = 0; __i < jctx->indents; __i++)                              \
    printf(" ")

#ifdef DEF
#undef DEF
#endif

#define DEF(token, value) const char *TOK_##token = (char *)value
#include "keywords.h"
KEYWORDS
#undef DEF

bool isKeyword(const char *str)
{
#define DEF(_, value)                                                          \
    if (STREQ(value, str))                                                     \
    return true
    KEYWORDS
#undef DEF
    return false;
}

bool validateName(const char *str)
{
    if (isKeyword(str))
        return false;
    for (int i = 0; str[i]; i++)
    {
        char c = str[i];
        if (i == 0)
            if (!isalpha(c) && c != '_' && c != '$')
                return false;
        if (!isalpha(c) && !isdigit(c) && c != '_' && c != '$')
            return false;
    }
    return true;
}

char *expr2str(int expr)
{
#define FIELD(name)                                                            \
    if (expr == TOKEN_TYPE_##name)                                             \
        return #name;
#include "tokens.h"
    TOKENS
#undef FIELD
    return NULL;
}

#ifdef BR_DEBUG
#define BR() __debugbreak();
#else
#define BR()
#endif

#define DBG_CHECK_BOUNDS_WERR(i, err)                                          \
    if (!(i < tokens->size))                                                   \
    {                                                                          \
        BR();                                                                  \
        ERROR(err);                                                            \
    }

#define CHECK_BOUNDS_WERR(i, err)                                              \
    if (!(i < tokens->size))                                                   \
    {                                                                          \
        ERROR(err);                                                            \
    }

#define CHECK_BOUNDS(i) DBG_CHECK_BOUNDS_WERR(i, "bounds error")

#define CHECK_BOUNDS_CAN_EOF_WERR(i, err)                                      \
    ({                                                                         \
        int _i = i;                                                            \
        if (!(_i <= tokens->size))                                             \
            ERROR(err);                                                        \
        _i == tokens->size;                                                    \
    })

#define CHECK_BOUNDS_CAN_EOF(i) CHECK_BOUNDS_CAN_EOF_WERR(i, "bounds error")

#define ERROR(str)                                                             \
    {                                                                          \
        String_init(&error, str);                                              \
        goto err;                                                              \
    }

#define ERRORF(FMT, ...)                                                       \
    {                                                                          \
        String_init(&error, FMT);                                              \
        String_format(&error, __VA_ARGS__);                                    \
        goto err;                                                              \
    }

#define ERROR_STR(string)                                                      \
    {                                                                          \
        error = string;                                                        \
        goto err;                                                              \
    }

typedef struct ASTParseResult
{
    ASTExpression *expr;
    String error;
    bool eof;
} ASTParseResult;

typedef struct ASTParseOptions
{
    ASTExpression *current;
    bool allowKeywords;
    bool allowOps;
    int stopTokenType;
} ASTParseOptions;

ASTParseOptions EmptyOptions = {0};

const char OPS_ORDERED_LIST[] = {'!', '~', '*', '/', '%',
                                 '+', '-', '&', '^', '|'};

const char MDM[] = {'*', '/', '%'};

const char PM[] = {'+', '-'};

#define EQRL(List, op1, op2)                                                   \
    ({                                                                         \
        bool a = false, b = false;                                             \
        for (int i = 0; i < sizeof(List); i++)                                 \
        {                                                                      \
            if (List[i] == op1)                                                \
                a = true;                                                      \
            if (List[i] == op2)                                                \
                b = true;                                                      \
            if (a && b)                                                        \
                break;                                                         \
        }                                                                      \
        bool r = a && b;                                                       \
        r;                                                                     \
    })

bool isLeftToRightPr(char op1, char op2)
{
    if (EQRL(MDM, op1, op2) || EQRL(PM, op1, op2))
        return true;

    return false;
}

#define OP_PRIORITY(op)                                                        \
    ({                                                                         \
        int pr = -1;                                                           \
        for (int i = 0; i < sizeof(OPS_ORDERED_LIST); i++)                     \
            if (OPS_ORDERED_LIST[i] == op)                                     \
            {                                                                  \
                pr = i;                                                        \
                break;                                                         \
            }                                                                  \
        pr;                                                                    \
    })

void process_operation(ASTExpression *expr)
{
    if (expr->type != TYPE_OPERATION)
        return;

    BinaryOperation *left = (BinaryOperation *)expr;

    if (left->wrapped)
        return;

    ASTExpression *rightExpr = left->right;
    if (rightExpr->type != TYPE_OPERATION)
        return;

    BinaryOperation *right = (BinaryOperation *)rightExpr;

    if (right->wrapped)
        return;

    int opLPr = OP_PRIORITY(left->op);
    int opRPr = OP_PRIORITY(right->op);

    if (opLPr < opRPr || isLeftToRightPr(left->op, right->op))
    {
        // a * b + c would be a * (b + c)
        // turn it into (a * b) + c
        ASTExpression *l = left->left;
        ASTExpression *r = left->right;
        ASTExpression *rr = right->right;
        char opr = right->op;
        right->right = right->left;
        right->left = left->left;
        left->right = rr;
        left->left = (ASTExpression *)right;
        right->op = left->op;
        left->op = opr;
    }

    process_operation(left->left);
    process_operation(left->right);
}

ASTParseResult ast_parse_expr(Lexer *lexer, int *i, ASTParseOptions *options)
{
    String error = {
        .allocator = &lexer->jctx->jr->allocator,
    };

    ASTParseResult result = {
        .error =
            {
                .allocator = &lexer->jctx->jr->allocator,
            },
        .expr = NULL,
        .eof = false,
    };

#define NEXT_TOKEN(err, canEof)                                                \
    ({                                                                         \
        Token *tok;                                                            \
        bool b = false;                                                        \
        if (canEof)                                                            \
            b = CHECK_BOUNDS_CAN_EOF_WERR(++(*i), err);                        \
        else                                                                   \
            CHECK_BOUNDS_WERR(++(*i), err);                                    \
        if (!b)                                                                \
        {                                                                      \
            tok = tokens->items[*i];                                           \
            while (tok->type == TOKEN_TYPE_NEWLINE ||                          \
                   tok->type == TOKEN_TYPE_WHITESPACE)                         \
            {                                                                  \
                if (canEof)                                                    \
                {                                                              \
                    b = CHECK_BOUNDS_CAN_EOF_WERR(++(*i), err);                \
                    if (b)                                                     \
                    {                                                          \
                        tok = NULL;                                            \
                        break;                                                 \
                    }                                                          \
                }                                                              \
                else                                                           \
                    CHECK_BOUNDS_WERR(++(*i), err);                            \
                tok = tokens->items[*i];                                       \
            }                                                                  \
        }                                                                      \
        else                                                                   \
            tok = NULL;                                                        \
        tok;                                                                   \
    })

    JContext *jctx = lexer->jctx;

    LIST_TYPEOF(Token) *tokens = lexer->tokens;

    if (!(*i < tokens->size))
    {
        result.eof = true;
        return result;
    }

    Token *token = tokens->items[*i];
    int type = token->type;

    if (type == TOKEN_TYPE_NEWLINE || type == TOKEN_TYPE_WHITESPACE ||
        type == TOKEN_TYPE_SEMICOLON)
    {
        CHECK_BOUNDS_CAN_EOF(++(*i));
        return result;
    }

#define CHECK_KW()                                                             \
    if (!options->allowKeywords)                                               \
    ERROR("expected expression or value, found keyword")

#define CHECK_OPS()                                                            \
    if (!options->allowOps)                                                    \
    ERROR("expected expression or value, found operator")

    if (type == TOKEN_TYPE_IDENTIFIER)
    {
#define __EQ(tok) STREQ(token->value.ptr, tok)
#define IF(tok) if (__EQ(tok))
#define ELIF(tok) else IF(tok)

        IF(TOK_DEF)
        {
            CHECK_KW()

            if (CHECK_BOUNDS_CAN_EOF(++(*i)))
                ERROR("def expression was used without a variable name");

            DeclarationExpression *expr = declaration_expr_new(jctx);
            result.expr = (ASTExpression *)expr;

            Token *varToken = tokens->items[*i];

            if (varToken->type != TOKEN_TYPE_IDENTIFIER)
            {
                ERRORF("expected identifier after 'def', got %s",
                       varToken->value.ptr);
            }

            expr->var = string_expr_new(jctx);
            expr->var->value = varToken->value.ptr;
            expr->var->type = TYPE_IDENTIFIER;
            varToken->value.ptr =
                NULL; // prevent free if lexer is destroyed b4 ast is done

            if (!validateName(expr->var->value))
            {
                if (isKeyword(expr->var->value))
                {
                    ERROR("cannot use a keyword as a variable name");
                }
                ERROR("variable name contains invalid characters");
            }

            if (CHECK_BOUNDS_CAN_EOF(++(*i)))
            declr_fail_novt:
                ERROR(
                    "def expression was used without a variable value or type");

            Token *nextToken = tokens->items[*i];

            if (nextToken->type != TOKEN_TYPE_OPERATOR)
                ERROR("expected : or = after def");

            bool expectType = nextToken->value.c == ':';

            if (expectType)
            {

                if (CHECK_BOUNDS_CAN_EOF(++(*i)))
                {
                    ERROR("expected a type after variable declaration")
                }

                Token *typeToken = tokens->items[*i];
                const char *typ = typeToken->value.ptr;
                expr->varType = typ;
                typeToken->value.ptr = NULL;

                if (!validateName(expr->varType))
                {
                    if (isKeyword(expr->varType))
                    {
                        ERROR("cannot use a keyword as a variable type");
                    }
                    ERROR("variable type contains invalid characters");
                }

                if (CHECK_BOUNDS_CAN_EOF(++(*i)))
                {
                    return result;
                }

                nextToken = tokens->items[*i];
            }

            if (nextToken->type == TOKEN_TYPE_SEMICOLON ||
                nextToken->type == TOKEN_TYPE_NEWLINE)
            {
                if (!expectType)
                {
                    goto declr_fail_novt;
                }
                else
                {
                    return result;
                }
            }

            if (nextToken->type != TOKEN_TYPE_OPERATOR ||
                nextToken->value.c != '=')
            {
                ERRORF("expected '=' operator after def, got %c",
                       *nextToken->value.ptr);
            }

            if (CHECK_BOUNDS_CAN_EOF(++(*i)))
            {
                ERROR("missing value in variable declaration after =");
            }

            ASTParseResult res = ast_parse_expr(lexer, i, &EmptyOptions);
            expr->value = res.expr;

            if (res.error.length != 0)
                ERROR_STR(res.error)

            if (*i < tokens->size)
            {
                int nextToken = tokens->items[*i]->type;
                if (nextToken != TOKEN_TYPE_SEMICOLON &&
                    nextToken != TOKEN_TYPE_NEWLINE)
                {
                    ERROR("invalid declaration expression")
                }
            }
            return result;
        }
        ELIF(TOK_FUNCTION_START) { goto func_def; }
        else
        {
            if (!isKeyword(token->value.ptr))
            {
                bool b = CHECK_BOUNDS_CAN_EOF(++(*i));
                VariableExpression *ve = variable_new(jctx);
                result.expr = (ASTExpression *)ve;
                ve->value = token->value.ptr;
                token->value.ptr = NULL;

                if (!b)
                {
                    Token *nextToken = tokens->items[*i];
                    if (nextToken->type == TOKEN_TYPE_NEWLINE ||
                        nextToken->type == TOKEN_TYPE_SEMICOLON)
                        goto end;

                    if (nextToken->type !=
                        TOKEN_TYPE_OPERATOR) // TODO: support '.' for function
                                             // calls
                    {
                        ERRORF("%s is not a keyword", ve->value);
                    }
                    goto next;
                }

                goto end;
            }
            else
            {
                ERRORF("illegal use of a reserved keyword: %s",
                       token->value.ptr);
            }
        }
    }
    else if (token->type == TOKEN_TYPE_LEFT_BRACE)
    {
        goto scope_expr;
    }
    else if (token->type == TOKEN_TYPE_STRING)
    {
        StringExpression *strExpr = string_expr_new(jctx);
        strExpr->value = token->value.ptr;
        token->value.ptr = NULL;
        result.expr = (ASTExpression *)strExpr;
        if (!CHECK_BOUNDS_CAN_EOF(++(*i)))
            goto next;
        goto end;
    }
    else if (type == TOKEN_TYPE_NUMBER_INT || type == TOKEN_TYPE_NUMBER_FLOAT)
    {
        NumericalValue *nv = numerical_value_new(jctx);
        if (type == TOKEN_TYPE_NUMBER_INT)
            nv->value.uint64 = token->value.uint64;
        else
            nv->value.float64 = token->value.float64;

        result.expr = (ASTExpression *)nv;

        if (!CHECK_BOUNDS_CAN_EOF(++(*i)))
            goto next;
        goto end;
    }
    else if (type == TOKEN_TYPE_OPERATOR)
    {
        CHECK_OPS()
        CHECK_BOUNDS(++(*i));
    }
    else if (type == TOKEN_TYPE_RIGHT_PAREN)
    {
        ERROR("missing expression opening '(");
    }
    else if (type == TOKEN_TYPE_LEFT_PAREN)
    {
        ASTParseOptions opts = {
            .stopTokenType = TOKEN_TYPE_RIGHT_PAREN,
        };

        NEXT_TOKEN("missing expression after (", false);

        Token *nextToken = tokens->items[*i];
        if (nextToken->type != TOKEN_TYPE_RIGHT_PAREN)
        {

            ASTParseResult res = ast_parse_expr(lexer, i, &opts);
            if (res.expr != NULL)
            {
                result.expr = res.expr;
                res.expr = NULL;
            }
            if (res.error.length != 0)
                ERROR_STR(res.error);

            if (result.expr->type == TYPE_OPERATION)
            {
                BinaryOperation *binOp = (BinaryOperation *)result.expr;
                binOp->wrapped = true;
            }

            --(*i);
            Token *closeTok =
                NEXT_TOKEN("missing expression terminator ')'", true);
            if (closeTok->type != TOKEN_TYPE_RIGHT_PAREN)
                ERROR("missing expression terminator ')'");
        }

        if (!CHECK_BOUNDS_CAN_EOF(++(*i)))
        {
            goto next;
        }

        goto end;
    }
    else
    {
        printf("unknown token type: %d\n", token->type);
        CHECK_BOUNDS_CAN_EOF(++(*i));
        return result;
    }

    CHECK_BOUNDS(++(*i));
end:
    return result;
err:
    if (result.expr != NULL)
    {
        result.expr->destroy(result.expr, jctx);
        result.expr = NULL;
    }
    result.error = error;
    return result;

// next would look up for more expressions till a ; or new line are found
// it would continue cases like ) or numerical values, ex: 4 + 3, would find 4
// then goto next, find + and look for right operation
next:;
    ASTExpression *expr = result.expr;
    token = tokens->items[*i];
    if (token->type == TOKEN_TYPE_SEMICOLON ||
        token->type == TOKEN_TYPE_NEWLINE)
        goto end;

    if (token->type == TOKEN_TYPE_OPERATOR)
    {
        BinaryOperation *op = bin_op_new(jctx);
        op->left = result.expr;
        result.expr = (ASTExpression *)op;
        op->op = token->value.c;

        NEXT_TOKEN("missing operation right value", false);

        ASTParseOptions opts = {
            .stopTokenType = options->stopTokenType,
        };

        opts.current = (ASTExpression *)op;

        ASTParseResult res = ast_parse_expr(lexer, i, &opts);
        if (res.expr != NULL)
        {
            op->right = res.expr;
            res.expr = NULL;
        }
        if (res.error.length != 0)
            ERROR_STR(res.error);

        process_operation((ASTExpression *)op);
    }

    goto end;

func_def:;
    FunctionExpression *func = NULL;

    IF(TOK_FUNCTION_START)
    {
        CHECK_BOUNDS_WERR(++(*i),
                          "missing function name in function definition");

        FunctionExpression *expr = func_expr_new(jctx);
        result.expr = (ASTExpression *)expr;

        func = expr;

        token = tokens->items[*i];

        if (token->type != TOKEN_TYPE_IDENTIFIER)
        {
            ERROR("expected function name");
        }

        expr->name = token->value.ptr;
        token->value.ptr = NULL;

        if (isKeyword(expr->name))
            ERRORF("expected a function name, got keyword '%s'", expr->name);

        char *expectedArgsList = "expected args list after function name";
        CHECK_BOUNDS_WERR(++(*i), expectedArgsList);

        Token *argsStartParen = tokens->items[*i];
        if (argsStartParen->type != TOKEN_TYPE_LEFT_PAREN)
            ERROR(expectedArgsList);

        Token *nextToken =
            NEXT_TOKEN("function args list was not closed (add ')'", true);

        while (nextToken->type != TOKEN_TYPE_RIGHT_PAREN)
        {
            if (nextToken->type != TOKEN_TYPE_IDENTIFIER)
            {
                printf("%d\n", nextToken->type);
                ERROR("expected an identifier as argument name");
            }

            if (isKeyword(nextToken->value.ptr))
            {
                ERROR("expected identifier as argument name, got "
                      "keyword");
            }

            FunctionArg *arg = JContext_alloc(jctx, sizeof(FunctionArg));
            arg->name = nextToken->value.ptr;
            nextToken->value.ptr = NULL;

            LIST_PUSH(expr->args, arg, &jctx->jr->allocator);

            CHECK_BOUNDS_WERR(++(*i), "expected type for function arg");

            Token *type = tokens->items[*i];
            arg->type = type->value.ptr;
            type->value.ptr = NULL;

            nextToken = NEXT_TOKEN("", true);
            if (nextToken == NULL)
            {
                ERROR("function args list was not closed (add ')' 2");
            }

            if (nextToken->type == TOKEN_TYPE_COMMA)
            {
                printf("comma\n");
                CHECK_BOUNDS_WERR(++(*i), "missing argument name after comma");
                nextToken = tokens->items[*i];
                if (nextToken->type != TOKEN_TYPE_IDENTIFIER)
                {
                    ERROR("missing argument name after ','");
                }
            }
        }

        // printf("tokens iter done");

        // ERROR("not done yet");
        if (nextToken->type == TOKEN_TYPE_RIGHT_PAREN)
        {
            CHECK_BOUNDS_WERR(++(*i),
                              "expected ; or scope after function definition");
            nextToken = tokens->items[*i];
        }

        if (nextToken->type == TOKEN_TYPE_IDENTIFIER)
        {
            expr->retType = nextToken->value.ptr;
            nextToken->value.ptr = NULL;
            if (isKeyword(expr->retType))
            {
                ERROR("expected function return type, got keyword");
            }

            CHECK_BOUNDS_WERR(
                ++(*i), "expected ; or scope start after function definition");

            nextToken = tokens->items[*i];
            if (nextToken->type == TOKEN_TYPE_SEMICOLON)
                goto end;
            if (nextToken->type != TOKEN_TYPE_LEFT_BRACE)
            {
                ERROR("expected ; or scope start after function "
                      "definition");
            }
            if (nextToken->type == TOKEN_TYPE_LEFT_BRACE)
            {
                token = nextToken;
                goto scope_expr;
            func_scope:
                func->scope = (ScopeExpression *)result.expr;
                result.expr = (ASTExpression *)func;
            }
        }
        else
        {
            ERROR("expected ; or scope after function definition");
        }

        goto end;
    }
    goto end;

scope_expr:;

    CHECK_BOUNDS_WERR(++(*i), "unclosed left brace in expression");

    //    typedef struct
    // {
    //     uint8_t *source;
    //     int size;
    //     int index;
    //     LIST_TYPEOF(Token) * tokens;
    //     JContext *jctx;
    // } Lexer;

    int size = 0;
    int start = *i;
    int openedBraces = 1;

    while (openedBraces != 0)
    {
        Token *tok = tokens->items[start];
        if (tok->type == TOKEN_TYPE_LEFT_BRACE)
            openedBraces++;
        else if (tok->type == TOKEN_TYPE_RIGHT_BRACE)
            openedBraces--;
        if (openedBraces != 0)
        {
            CHECK_BOUNDS_WERR(++start,
                              "missing close curly-brace in expression");
        }
    }

    Lexer *scopeLexer = JContext_alloc(jctx, sizeof(Lexer));
    scopeLexer->tokens = JContext_alloc(jctx, sizeof(LIST_TYPEOF(Token)));
    scopeLexer->tokens->items = lexer->tokens->items + *i;
    scopeLexer->tokens->size = start - *i;
    scopeLexer->jctx = jctx;
    ASTNode *node = ast_from_lexer(scopeLexer);

    JContext_free(jctx, scopeLexer->tokens);
    JContext_free(jctx, scopeLexer);

    if (node == NULL)
    {
        ERROR("parsing scope failed");
    }

    printf("parsed lexer!!!");

    ScopeExpression *se = scope_expr_new(jctx);

    se->body = node;

    *i = start + 1;

    result.expr = (ASTExpression *)se;

    if (func != NULL)
        goto func_scope;

    goto end;

scope_parse_end:;
    // if (isFuncDef)
    // goto

    goto end;

#undef NEXT_TOKEN
}

ASTNode *ast_from_lexer(Lexer *lexer)
{
    String error = {
        .allocator = &lexer->jctx->jr->allocator,
    };

    ASTNode *node = ast_node_new(lexer->jctx);
    LIST_TYPEOF(Token) *tokens = lexer->tokens;

    ASTParseOptions options = {0};
    options.allowKeywords = true;

    int i = 0;
    while (i < tokens->size)
    {
        ASTParseResult result = ast_parse_expr(lexer, &i, &options);
        if (result.error.length != 0)
            ERROR_STR(result.error);

        if (result.eof)
            return node;
        if (result.expr != NULL)
        {
            LIST_PUSH(node->body, result.expr, &node->jctx->jr->allocator);
        }
    }
    // ASTExpression *expr = ast_expression_new();
    return node;
err:
    printf("ERROR: %s\n",
           error.length == 0 ? "unknown error" : String_getContents(&error));
    String_clear(&error);
    ast_node_destroy(node, node->jctx);
    return NULL;
}

void expr__print(ASTExpression *expr, JContext *jctx)
{
    if (expr->dump != NULL)
    {
        expr->dump(expr, jctx);
    }
    else
    {
        printf("UnknownExpression: %d\n", expr->type);
    }
    // switch (expr->type)
    // {
    // case TYPE_DECLARATION:
    // {
    //     DeclarationExpression *declExpr = (DeclarationExpression *)expr;
    //     ASTExpression *val = declExpr->value;
    //     printf("DeclarationExpression: %s = ", declExpr->var->value);
    //     if (val->type == TYPE_STRING)
    //     {
    //         StringExpression *strExpr = (StringExpression *)declExpr->value;
    //         printf("%s\n", strExpr->value);
    //     }
    //     else if (val->type == TYPE_INT || val->type == TYPE_FLOAT)
    //     {
    //         NumericalValue *nv = (NumericalValue *)val;
    //         if (val->type == TYPE_INT)
    //         {
    //             printf("%llu\n", nv->value.uint64);
    //         }
    //         else
    //         {
    //             printf("%f\n", nv->value.float64);
    //         }
    //     }
    // }
    // break;
    // case TYPE_STRING:
    // {
    //     StringExpression *strExpr = (StringExpression *)expr;
    //     printf("StringExpression: %s\n", strExpr->value);
    // }
    // break;
    // case TYPE_INT:
    // {
    //     NumericalValue *nv = (NumericalValue *)expr;
    //     printf("NumericalValue: %llu\n", nv->value.uint64);
    // }
    // break;
    // case TYPE_FLOAT:
    // {
    //     NumericalValue *nv = (NumericalValue *)expr;
    //     printf("NumericalValue: %f\n", nv->value.float64);
    // }
    // break;
    // default:
    // printf("UnknownExpression: %d\n", expr->type);
    //     break;
    // }
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
                     { expr__print(item, node->jctx); });
    }
}

void ast_print(ASTNode *node) { ast_print_node(node, 0); }

void _default_destructor(void *ptr, JContext *jctx)
{
    JContext_free(jctx, ptr);
}

MAKE_DTOR(ASTNode, ast_node, {
    if (ast_node->type == TYPE_BODY)
    {
        LIST_FOREACH(ast_node->body, ASTExpression *,
                     { item->destroy(item, ast_node->jctx); });
        LIST_FREE(ast_node->body, &ast_node->jctx->jr->allocator);
    }
    ast_node->destroy(ast_node, ast_node->jctx);
});

MAKE_CTOR(ASTNode, ast_node, {
    ast_node->type = TYPE_BODY;
    ast_node->body = LIST_CREATE_EMPTY(ASTExpression, jctx);
    ast_node->jctx = jctx;
    ast_node->destroy = (void *)_default_destructor;
});

// MAKE_CTOR_QUICK(ASTExpression, ast_expression);

MAKE_EXPR_CTOR(
    StringExpression, string_expr,
    {
        string_expr->type = TYPE_STRING;
        string_expr->value = NULL;
        string_expr->destroy = &string_expr_destroy;
    },
    { printf("\"%s\"", expr->value); });

MAKE_EXPR_CTOR(
    NumericalValue, numerical_value,
    {
        numerical_value->type = TYPE_INT;
        numerical_value->value.uint64 = 0;
    },
    {
        if (expr->type == TYPE_INT)
            printf("%llu", expr->value.uint64);
        else
            printf("%f", expr->value.float64);
    })

MAKE_EXPR_CTOR(
    BinaryOperation, bin_op,
    {
        bin_op->type = TYPE_OPERATION;
        bin_op->right = NULL;
        bin_op->left = NULL;
        bin_op->op = 0;
        bin_op->wrapped = false;
        bin_op->destroy = &bin_op_destroy;
    },
    {
        printf("(");
        if (expr->left != NULL)
            expr->left->dump(expr->left, jctx);

        printf(" %c ", expr->op);

        if (expr->right != NULL)
            expr->right->dump(expr->right, jctx);
        printf(")");
    })

MAKE_EXPR_CTOR(
    VariableExpression, variable,
    {
        variable->type = TYPE_VARIABLE;
        variable->value = NULL;
        variable->destroy = &variable_destroy;
    },
    { printf("%s", expr->value == NULL ? "<null>" : expr->value); })

MAKE_EXPR_CTOR(
    DeclarationExpression, declaration_expr,
    {
        declaration_expr->type = TYPE_DECLARATION;
        declaration_expr->var = NULL;
        declaration_expr->value = NULL;
        declaration_expr->varType = NULL;
        declaration_expr->destroy = &declaration_expr_destroy;
    },
    {
        String maybeType = {.allocator = &jctx->jr->allocator};
        if (expr->varType != NULL)
        {
            const char *varType = expr->varType;
            String_init(&maybeType, ": %s");
            String_format(&maybeType, expr->varType);
        }

        printf("DeclarationExpression: %s%s = ", expr->var->value,
               String_getContents(&maybeType));

        String_clear(&maybeType);
        expr->value->dump(expr->value, jctx);

        printf("\n");
    });

MAKE_EXPR_CTOR(
    FunctionExpression, func_expr,
    {
        func_expr->type = TYPE_FUNCTION;
        func_expr->scope = NULL;
        func_expr->args = LIST_CREATE_EMPTY(FunctionArg, jctx);
        func_expr->destroy = &func_expr_destroy;
        func_expr->name = NULL;
        func_expr->retType = NULL;
    },
    {
        printf("func(");
        LIST_FOREACH(expr->args, FunctionArg *, {
            printf("%s %s%s", item->name, item->type,
                   idx == expr->args->size - 1 ? "" : ", ");
        });
        printf(") %s", expr->retType);
        if (expr->scope == NULL)
            printf(";\n");
        else
        {
            printf(" {\n");
            expr->scope->dump(expr->scope, jctx);
            printf("\n}\n");
        }
    })

MAKE_EXPR_CTOR(
    ScopeExpression, scope_expr,
    {
        scope_expr->type = TYPE_SCOPE;
        scope_expr->body = NULL;
        scope_expr->destroy = &scope_expr_destroy;
    },
    {
        printf("Scope {\n");
        ast_print_node(expr->body, 4);
        printf("}");
    })

MAKE_DTOR(FunctionExpression, func_expr, {
    if (func_expr->scope != NULL)
        func_expr->scope->destroy(func_expr->scope, jctx);

    if (func_expr->name != NULL)
        JContext_free(jctx, func_expr->name);

    if (func_expr->retType != NULL)
        JContext_free(jctx, func_expr->retType);

    LIST_FOREACH(func_expr->args, FunctionArg *, {
        if (item->name != NULL)
            JContext_free(jctx, item->name);
        if (item->type != NULL)
            JContext_free(jctx, item->type);
        JContext_free(jctx, item);
    })

    LIST_FREE(func_expr->args, &jctx->jr->allocator);

    JContext_free(jctx, func_expr);
})

MAKE_DTOR(ScopeExpression, scope_expr, {
    if (scope_expr->body != NULL)
    {
        ast_node_destroy(scope_expr->body, jctx);
    }
    JContext_free(jctx, scope_expr);
})

MAKE_DTOR(DeclarationExpression, declaration_expr, {
    if (declaration_expr->var != NULL)
    {
        declaration_expr->var->destroy(declaration_expr->var, jctx);
    }
    if (declaration_expr->value != NULL)
    {
        declaration_expr->value->destroy(declaration_expr->value, jctx);
    }

    if (declaration_expr->varType != NULL)
        JContext_free(jctx, (void *)declaration_expr->varType);

    JContext_free(jctx, declaration_expr);
})

MAKE_DTOR(BinaryOperation, bin_op, {
    if (bin_op->right != NULL)
    {
        bin_op->right->destroy(bin_op->right, jctx);
    }

    if (bin_op->left != NULL)
    {
        bin_op->left->destroy(bin_op->left, jctx);
    }

    JContext_free(jctx, bin_op);
})

MAKE_DTOR(StringExpression, string_expr, {
    if (string_expr->value != NULL)
        JContext_free(jctx, string_expr->value);
    JContext_free(jctx, string_expr);
})

MAKE_DTOR(VariableExpression, variable, {
    if (variable->value != NULL)
        JContext_free(jctx, variable->value);
    JContext_free(jctx, variable);
})
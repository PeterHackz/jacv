#include "lexer.h"

#include "jcontext.h"
#include "stdio.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "jutils/jutils.h"

#include "stdlib.h"

char *get_token_name(int token)
{
#define FIELD(name)                                                            \
    if (token == TOKEN_TYPE_##name)                                            \
        return "TOKEN_" #name;
#include "tokens.h"
    TOKENS
#undef FIELD
#undef TOKENS
    return NULL;
}

char *clone_string(char *str, int size, JContext *jctx)
{
    char *clone = JContext_alloc(jctx, size + 1);
    memcpy(clone, str, size);
    clone[size] = 0; // ensure that it is null-terminated
    return clone;
}

Token *token_new_string(char *value, int size, JContext *jctx)
{
    Token *token = JContext_alloc(jctx, sizeof(Token));
    token->type = TOKEN_TYPE_STRING;
    char *str = JContext_alloc(jctx, size);
    memcpy(str, value, size);
    str[size - 1] = 0; // ensure it is null-terminated
    token->value.ptr = str;
    return token;
}

Token *token_new_identifier(char *value, int size, JContext *jctx)
{
    Token *token = token_new_string(value, size, jctx);
    token->type = TOKEN_TYPE_IDENTIFIER;
    return token;
}

Lexer *lexer_new(uint8_t *source, int size, JContext *jctx)
{
    Lexer *lexer = JContext_alloc(jctx, sizeof(Lexer));
    lexer->source = source;
    lexer->size = size;
    lexer->tokens = LIST_CREATE_EMPTY(Token, jctx);
    lexer->jctx = jctx;
    return lexer;
}

void lexer_free(Lexer *lexer)
{
    if (lexer->tokens != NULL)
    {
        LIST_FOREACH(lexer->tokens, Token *, {
            if ((item->type == TOKEN_TYPE_STRING ||
                 item->type == TOKEN_TYPE_IDENTIFIER) &&
                item->value.ptr !=
                    NULL) // if it is null, it means AST copied it
                JContext_free(lexer->jctx, item->value.ptr);
            JContext_free(lexer->jctx, item);
        })
        LIST_FREE(lexer->tokens, &lexer->jctx->jr->allocator);
    }
    JContext_free(lexer->jctx, lexer);
}

void lexer_print_tokens(Lexer *lexer)
{
    LIST_FOREACH(lexer->tokens, Token *, {
        if (item->type == TOKEN_TYPE_STRING)
        {
            printf("Token: (str) %s\n", item->value.ptr);
        }
        else if (item->type == TOKEN_TYPE_IDENTIFIER)
        {
            printf("Token (id): %s\n", item->value.ptr);
        }
        else if (item->type == TOKEN_TYPE_OPERATOR)
        {
            printf("Token: (op): %c\n", item->value.c);
        }
        else if (item->type == TOKEN_TYPE_NEWLINE)
        {
            printf("Token newline\n");
        }
        else if (item->type == TOKEN_TYPE_NUMBER_INT)
        {
            printf("Token: (int): %llu\n", item->value.uint64);
        }
        else if (item->type == TOKEN_TYPE_NUMBER_FLOAT)
        {
            printf("Token: (float): %f\n", item->value.float64);
        }
        else
        {
            printf("Token: (type: %s), value: %s\n", get_token_name(item->type),
                   item->value.ptr == NULL ? "<null>" : item->value.ptr);
        }
    })
}

typedef enum
{
    ID_NONE = -1,
    ID_NUM,
    ID_CHAR,
    ID_SYM,
} IdentifierTypeT;

int get_line(uint8_t *source, int pos)
{
    int line = 1;
    for (int i = 0; i < pos; i++)
    {
        if (source[i] == '\n')
        {
            line++;
        }
    }
    return line;
}

int get_pos(uint8_t *source, int pos)
{
    int line = 1;
    int lastNewline = 0;
    for (int i = 0; i < pos; i++)
    {
        if (source[i] == '\n')
        {
            line++;
            lastNewline = i;
        }
    }
    return pos - lastNewline;
}

int identifierType(char c)
{
    if (c >= '0' && c <= '9')
        return ID_NUM;
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
        return ID_CHAR;
    if (c == '_' || c == '$')
        return ID_SYM;
    return ID_NONE;
}

Error lexer_parse(Lexer *lexer)
{
    JContext *jctx = lexer->jctx;

#define RET_ERROR(FMT, CB, ...)                                                \
    ({                                                                         \
        Error err = {.state = true,                                            \
                     .val = {                                                  \
                         .allocator = &jctx->jr->allocator,                    \
                     }};                                                       \
        String_init(&err.val, FMT);                                            \
        String_format(&err.val, __VA_ARGS__);                                  \
        CB;                                                                    \
        err;                                                                   \
    })

    int i = 0;
    Token *token = NULL;
next:
    while (i < lexer->size)
    {

        token = NULL;
        char c = lexer->source[i];

        switch (c)
        {
        case ' ':
        case '\t':
            i++;
            goto next;
        }

        if (c == '(')
        {
            token = JContext_alloc(jctx, sizeof(Token));
            token->type = TOKEN_TYPE_LEFT_PAREN;
            goto push_and_jump;
        }
        if (c == ')')
        {
            token = JContext_alloc(jctx, sizeof(Token));
            token->type = TOKEN_TYPE_RIGHT_PAREN;
            goto push_and_jump;
        }
        if (c == '{')
        {
            token = JContext_alloc(jctx, sizeof(Token));
            token->type = TOKEN_TYPE_LEFT_BRACE;
            goto push_and_jump;
        }
        if (c == '}')
        {
            token = JContext_alloc(jctx, sizeof(Token));
            token->type = TOKEN_TYPE_RIGHT_BRACE;
            goto push_and_jump;
        }
        if (c == '[')
        {
            token = JContext_alloc(jctx, sizeof(Token));
            token->type = TOKEN_TYPE_LEFT_BRACKET;
            goto push_and_jump;
        }
        if (c == ']')
        {
            token = JContext_alloc(jctx, sizeof(Token));
            token->type = TOKEN_TYPE_RIGHT_BRACKET;
            goto push_and_jump;
        }
        if (c == ',')
        {
            token = JContext_alloc(jctx, sizeof(Token));
            token->type = TOKEN_TYPE_COMMA;
            goto push_and_jump;
        }
        if (c == ';')
        {
            token = JContext_alloc(jctx, sizeof(Token));
            token->type = TOKEN_TYPE_SEMICOLON;
            goto push_and_jump;
        }
        if (c == '\n')
        {
            token = JContext_alloc(jctx, sizeof(Token));
            token->type = TOKEN_TYPE_NEWLINE;
            goto push_and_jump;
        }

        switch (c)
        {
        case '+':
        case '-':
        case '*':
        case '/':
        case '%':
        case '^':
        case '&':
        case '|':
        case '~':
        case '!':
        case '=':
        case '<':
        case '>':
        case '?':
        case ':':
            token = JContext_alloc(jctx, sizeof(Token));
            token->type = TOKEN_TYPE_OPERATOR;
            token->value.c = lexer->source[i];
            goto push_and_jump;
        }

        if (c == '"' || c == '`')
        {
            token = JContext_alloc(jctx, sizeof(Token));
            token->type = TOKEN_TYPE_STRING;

            int end = lexer_parse_string_at(lexer, i);

            bool allowMultiLine = c == '`';
            for (int j = i + 1; j < end; j++)
            {
                if (lexer->source[j] == '\n' && !allowMultiLine)
                {
                    return RET_ERROR(
                        "unterminated string found at line: %d, pos: %d\n", {},
                        get_line(lexer->source, i), get_pos(lexer->source, i));
                }
            }

            if (end == -1)
            {
                return RET_ERROR(
                    "unterminated string found at line: %d, pos: %d\n", {},
                    get_line(lexer->source, i), get_pos(lexer->source, i));
            }

            int size = end - i;

            token->value.ptr =
                clone_string((char *)lexer->source + i + 1, size - 1, jctx);

            i = end;
            goto push_and_jump;
        }

        int id_t = identifierType(c);

        if (id_t == ID_NONE && c == '.')
        {
            if (i != lexer->size - 1)
            {
                char next = lexer->source[i + 1];
                if (next >= '0' && next <= '9')
                {
                    id_t = ID_NUM;
                }
            }
        }

        if (id_t != ID_NONE)
        {
            token = JContext_alloc(jctx, sizeof(Token));
            token->type = TOKEN_TYPE_IDENTIFIER;

            int end = 0;
            if (id_t == ID_NUM)
            {
                char extraIds[] = {'.'};
                end = lexer_parse_identifier_with_extra_ids(lexer, i, extraIds,
                                                            sizeof(extraIds));
            }
            else
                end = lexer_parse_identifier_at(lexer, i);

            int size = end - i + 1;

            token->value.ptr =
                clone_string((char *)lexer->source + i, size - 1, jctx);

            i = end - 1;

            if (id_t == ID_NUM)
            {
                token->type = TOKEN_TYPE_NUMBER_INT;

                ParseNumberResult result = parse_number(token->value.ptr);

                if (result.state == STATE_FAIL)
                {
                    return RET_ERROR(
                        "invalid number found at pos %d: %s\n",
                        { JContext_free(jctx, token->value.ptr); }, i,
                        token->value.ptr);
                }

                JContext_free(jctx, token->value.ptr);

                if (result.state == STATE_SUCCESS_FLOAT)
                {
                    token->value.float64 = result.num.float64;
                    token->type = TOKEN_TYPE_NUMBER_FLOAT;
                }
                else
                    token->value.uint64 = result.num.uint64;
            }
            goto push_and_jump;
        }

        if (c == '.')
        {
            token = JContext_alloc(jctx, sizeof(Token));
            token->type = TOKEN_TYPE_DOT;
            goto push_and_jump;
        }

    push_and_jump:
        if (token != NULL)
            LIST_PUSH(lexer->tokens, token, &jctx->jr->allocator);
        i++;
        goto next;
    }

#undef RET_ERROR

    Error e = {0};
    return e;
}

int lexer_parse_string_at(Lexer *lexer, int startIndex)
{
    char quote = (char)lexer->source[startIndex];
    int i = startIndex + 1;
    bool unescaping = false;
    while (i < lexer->size)
    {
        if (lexer->source[i] == quote)
        {
            if (unescaping)
                unescaping = false;
            else
                return i;
        }
        if (unescaping)
            unescaping = false;
        if (lexer->source[i] == '\\')
            unescaping = true;
        i++;
    }
    return -1;
}

int lexer_parse_identifier_at(Lexer *lexer, int startIndex)
{
    return lexer_parse_identifier_with_extra_ids(lexer, startIndex, NULL, 0);
}

int lexer_parse_identifier_with_extra_ids(Lexer *lexer, int startIndex,
                                          char *ids, int idsCount)
{
    int i = startIndex;
    while (i < lexer->size)
    {
        bool isInExtraIds = false;
        for (int j = 0; j < idsCount; j++)
        {
            if (lexer->source[i] == ids[j])
            {
                isInExtraIds = true;
                break;
            }
        }
        if (isInExtraIds || identifierType(lexer->source[i]) != ID_NONE)
        {
            i++;
            continue;
        }
        return i;
    }
    if (i == lexer->size)
        return lexer->size;
    return -1;
}
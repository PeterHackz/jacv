#include "include/lexer.h"

#include "stdio.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "include/jmemory.h"

char *clone_string(char *str, int size)
{
    char *clone = (char *)MALLOC(size);
    memcpy(clone, str, size);
    clone[size - 1] = 0; // ensure that it is null-terminated
    return clone;
}

Token *token_new_string(char *value, int size)
{
    Token *token = (Token *)MALLOC(sizeof(Token));
    token->type = TOKEN_TYPE_STRING;
    char *str = (char *)MALLOC(size);
    memcpy(str, value, size);
    str[size - 1] = 0; // ensure it is null-terminated
    token->value = str;
    return token;
}

Token *token_new_identifier(char *value, int size)
{
    Token *token = token_new_string(value, size);
    token->type = TOKEN_TYPE_IDENTIFIER;
    return token;
}

Lexer *lexer_new(uint8_t *source, int size)
{
    Lexer *lexer = (Lexer *)MALLOC(sizeof(Lexer));
    lexer->source = source;
    lexer->size = size;
    lexer->tokens = LIST_CREATE_EMPTY(Token);
    return lexer;
}

void lexer_free(Lexer *lexer)
{
    if (lexer->tokens != NULL)
    {
        LIST_FOREACH(lexer->tokens, Token *, {
            if ((item->type == TOKEN_TYPE_STRING ||
                 item->type == TOKEN_TYPE_IDENTIFIER) &&
                item->value != NULL) // if it is null, it means AST copied it
                FREE(item->value);
            FREE(item);
        });
        LIST_FREE(lexer->tokens);
    }
    FREE(lexer);
}

void lexer_print_tokens(Lexer *lexer)
{
    LIST_FOREACH(lexer->tokens, Token *, {
        if (item->type == TOKEN_TYPE_STRING)
        {
            printf("Token: (str) %s\n", item->value);
        }
        else if (item->type == TOKEN_TYPE_IDENTIFIER)
        {
            printf("Token (id): %s\n", item->value);
        }
        else if (item->type == TOKEN_TYPE_OPERATOR)
        {
            printf("Token: (op): %c\n", *item->value);
        }
        else
        {
            printf("Token: (type: %d) %s\n", item->type, item->value);
        }
    });
}

void lexer_parse(Lexer *lexer)
{
    int i = 0;
    Token *token;
next:
    while (i < lexer->size)
    {

        switch (lexer->source[i])
        {
        case ' ':
        case '\n':
        case '\t':
            i++;
            goto next;
        }

        if (lexer->source[i] == '(')
        {
            Token *token = (Token *)MALLOC(sizeof(Token));
            token->type = TOKEN_TYPE_LEFT_PAREN;
            goto push_and_jump;
        }
        if (lexer->source[i] == ')')
        {
            Token *token = (Token *)MALLOC(sizeof(Token));
            token->type = TOKEN_TYPE_RIGHT_PAREN;
            goto push_and_jump;
        }
        if (lexer->source[i] == '{')
        {
            Token *token = (Token *)MALLOC(sizeof(Token));
            token->type = TOKEN_TYPE_LEFT_BRACE;
            goto push_and_jump;
        }
        if (lexer->source[i] == '}')
        {
            Token *token = (Token *)MALLOC(sizeof(Token));
            token->type = TOKEN_TYPE_RIGHT_BRACE;
            goto push_and_jump;
        }
        if (lexer->source[i] == '[')
        {
            Token *token = (Token *)MALLOC(sizeof(Token));
            token->type = TOKEN_TYPE_LEFT_BRACKET;
            goto push_and_jump;
        }
        if (lexer->source[i] == ']')
        {
            Token *token = (Token *)MALLOC(sizeof(Token));
            token->type = TOKEN_TYPE_RIGHT_BRACKET;
            goto push_and_jump;
        }
        if (lexer->source[i] == ',')
        {
            Token *token = (Token *)MALLOC(sizeof(Token));
            token->type = TOKEN_TYPE_COMMA;
            goto push_and_jump;
        }
        if (lexer->source[i] == ';')
        {
            Token *token = (Token *)MALLOC(sizeof(Token));
            token->type = TOKEN_TYPE_SEMICOLON;
            goto push_and_jump;
        }

        switch (lexer->source[i])
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
            token = (Token *)MALLOC(sizeof(Token));
            token->type = TOKEN_TYPE_OPERATOR;
            token->value = (char *)(lexer->source + i);
            goto push_and_jump;
        }

        if (lexer->source[i] == '"' || lexer->source[i] == '`')
        {
            token = (Token *)MALLOC(sizeof(Token));
            token->type = TOKEN_TYPE_STRING;

            int end = lexer_parse_string_at(lexer, i);

            int size = end - i;

            token->value = clone_string((char *)lexer->source + i + 1, size);

            i = end;
            goto push_and_jump;
        }

        if (lexer->source[i] >= 'a' && lexer->source[i] <= 'z' ||
            lexer->source[i] >= 'A' && lexer->source[i] <= 'Z' ||
            lexer->source[i] == '_' || lexer->source[i] == '$')
        {
            token = (Token *)MALLOC(sizeof(Token));
            token->type = TOKEN_TYPE_IDENTIFIER;

            int end = lexer_parse_identifier_at(lexer, i);

            int size = end - i + 1;

            token->value = clone_string((char *)lexer->source + i, size);

            i = end;
            goto push_and_jump;
        }

    push_and_jump:
        LIST_PUSH(lexer->tokens, token);
        i++;
        goto next;
    }
}

int lexer_parse_string_at(Lexer *lexer, int startIndex)
{
    char quote = lexer->source[startIndex];
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
    int i = startIndex;
    while (i < lexer->size)
    {
        if (lexer->source[i] >= 'a' && lexer->source[i] <= 'z' ||
            lexer->source[i] >= 'A' && lexer->source[i] <= 'Z' ||
            lexer->source[i] == '_' || lexer->source[i] == '$')
        {
            i++;
            continue;
        }
        return i;
    }
    return -1;
}
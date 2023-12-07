#ifndef LEXER_H
#define LEXER_H

#include "list.h"
#include <stdint.h>

/*
int x = 0;
fn y() -> int {
    return 1;
}
*/

#define FIELD(name) TOKEN_TYPE_##name,
enum TokenType
{
#include "tokens.h"
    TOKENS
};
#undef FIELD

#include "keywords.h"

typedef struct
{
    int type;
    char *value;
    void (*destructor)(void *);
} Token;

LIST_INIT_TYPE(Token);

typedef struct
{
    uint8_t *source;
    int size;
    int index;
    LIST_TYPEOF(Token) * tokens;
} Lexer;

Token *token_new_string(char *value, int size);
Token *token_new_identifier(char *value, int size);

char *clone_string(char *str, int size);

Lexer *lexer_new(uint8_t *source, int size);
void lexer_free(Lexer *lexer);

void lexer_print_tokens(Lexer *lexer);

void lexer_parse(Lexer *lexer);

int lexer_parse_string_at(Lexer *lexer, int startIndex);

int lexer_parse_identifier_at(Lexer *lexer, int startIndex);

void lexer_analyze(Lexer *lexer);

#endif // LEXER_H
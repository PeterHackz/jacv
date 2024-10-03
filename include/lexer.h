#ifndef LEXER_H
#define LEXER_H

#include "jcontext.h"
#include "list.h"
#include <stdint.h>

#include "jutils/error.h"

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
    union
    {
        char *ptr;
        char c;
        uint64_t uint64; // incase it is negative we put a NEG operator before
                         // it, and an optimization pass would write it signed
                         // directly instead of PUSH <num>; NEG
        double float64;  // same changes will be done as uint64
    } value;
    void (*destructor)(void *);
} Token;

LIST_INIT_TYPE(Token);

typedef struct
{
    uint8_t *source;
    int size;
    int index;
    LIST_TYPEOF(Token) * tokens;
    JContext *jctx;
} Lexer;

Token *token_new_string(char *value, int size, JContext *);
Token *token_new_identifier(char *value, int size, JContext *);

char *clone_string(char *str, int size, JContext *);

Lexer *lexer_new(uint8_t *source, int size, JContext *);
void lexer_free(Lexer *lexer);

void lexer_print_tokens(Lexer *lexer);

Error lexer_parse(Lexer *lexer);

int lexer_parse_string_at(Lexer *lexer, int startIndex);

int lexer_parse_identifier_at(Lexer *lexer, int startIndex);
int lexer_parse_identifier_with_extra_ids(Lexer *lexer, int startIndex,
                                          char *ids, int idsCount);

void lexer_analyze(Lexer *lexer);

#endif // LEXER_H
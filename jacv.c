#include "stdio.h"

#include "include/jacv.h"

#include "include/io_utils.h"
#include "include/lexer.h"

#include "include/jmemory.h"

#include "include/ast.h"

int memory_allocated = 0;

JValue *JValue_New()
{
    JValue *jvalue = GC_NEW(JValue);
    return jvalue;
}

int main()
{
    int size = 0;
    uint8_t *source = read_file("test.jacv", &size);

    Lexer *lexer = lexer_new(source, size);
    lexer_parse(lexer);
    lexer_print_tokens(lexer);

    ASTNode *ast = ast_from_lexer(lexer);

    printf("used memory: %d\n", memory_allocated);

    ast_print(ast);

    ast_node_destruct(ast);
    lexer_free(lexer);
    FREE(source);

    printf("memory after destructs: %d\n", memory_allocated);

    return 0;
}
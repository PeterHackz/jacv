#include "jcontext.h"
#include "jruntime.h"
#include "stdio.h"

#include "jacv.h"

#include "io_utils.h"
#include "lexer.h"

#include "ast.h"

JValue *JValue_New()
{
    JValue *jvalue = GC_NEW(JValue);
    return jvalue;
}

#include "jutils/string.h"

int main()
{
    JRuntime *jr = JRuntime_new();
    JContext *jctx = JContext_new(jr);

    // String s = {.allocator = &jctx->jr->allocator};
    // String_init(&s, "test1234taest56729");
    // printf("%d\n", String_indexOf(&s, "test"));
    // bool b = String_replace(&s, "taest", "testtttttttttttttttttttttttt2");
    // printf("b: %d, s: %s, len: %d\n", b, String_getContents(&s), s.length);
    // String_clear(&s);
    // STRING_NEW(str, "my test %s", &jr->allocator);
    // String_format(&str, "string");
    // printf("str: %s\n", String_getContents(&str));
    // String_clear(&s);
    // String_clear(&str);

    int size = 0;
    uint8_t *source = read_file(jctx, "test.jacv", &size);

    Lexer *lexer = lexer_new(source, size, jctx);
    Error error = lexer_parse(lexer);
    if (error.state)
    {
        printf("lexer error: %s\n", String_getContents(&error.val));
        String_clear(&error.val);
        return 1;
    }
    lexer_print_tokens(lexer);

    ASTNode *ast = ast_from_lexer(lexer);

    printf("used memory: %d\n", jr->memory_allocated);

    if (ast != NULL)
    {
        ast_print(ast);
        ast_node_destroy(ast, jctx);
    }

    lexer_free(lexer);
    JContext_free(jctx, source);
    JContext_destroy(jctx);

    printf("memory after destructs: %d\n", jr->memory_allocated);
    JRuntime_destroy(jr);

    return 0;
}
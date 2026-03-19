/*
 * main.c — MFZ dil çalıştırıcı
 *
 * Kullanım:
 *   ./mfz program.mfz          → çalıştır
 *   ./mfz program.mfz --tokens → token listesini göster
 *   ./mfz program.mfz --ast    → AST'yi göster
 *   ./mfz program.mfz --all    → token + AST + çalıştır
 *
 * Derleme:
 *   gcc -Wall -o mfz lexer.c parser.c interpreter.c main.c -lm
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "interpreter.h"

static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "Hata: '%s' açılamadı\n", path); return NULL; }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    /* FIX #3: ftell() bir dizin veya özel dosya icin -1 doner.
     * -1 + 1 = 0 -> malloc(0), sonra fread'e size_t olarak cast edilince
     * 18 kentrilyon byte okumaya calisir -> heap overflow. */
    if (size < 0) { fclose(f); fprintf(stderr, "Hata: '%s' okunabilir bir dosya degil\n", path); return NULL; }
    char *buf = malloc((size_t)size + 1);
    if (!buf) { fclose(f); return NULL; }
    fread(buf, 1, (size_t)size, f);
    buf[size] = '\0';
    fclose(f);
    return buf;
}

static int has_ext(const char *path, const char *ext) {
    size_t pl = strlen(path), el = strlen(ext);
    return pl >= el && strcmp(path + pl - el, ext) == 0;
}

static void usage(const char *prog) {
    fprintf(stderr,
        "MFZ Interpreter\n"
        "Kullanim: %s <dosya.mfz> [secenekler]\n\n"
        "Secenekler:\n"
        "  --tokens   Token listesini goster\n"
        "  --ast      AST agacini goster\n"
        "  --all      Hepsini goster\n"
        "  --help     Bu mesaji goster\n\n"
        "Ornek:\n"
        "  %s program.mfz\n"
        "  %s program.mfz --ast\n",
        prog, prog, prog);
}

int main(int argc, char *argv[]) {
    if (argc < 2) { usage(argv[0]); return 1; }

    const char *filepath = argv[1];
    if (strcmp(filepath,"--help")==0 || strcmp(filepath,"-h")==0) {
        usage(argv[0]); return 0;
    }

    if (!has_ext(filepath, ".mfz"))
        fprintf(stderr, "Uyari: '%s' .mfz uzantili degil, yine de calistiriliyor...\n\n", filepath);

    int show_tokens = 0, show_ast = 0;
    for (int i = 2; i < argc; i++) {
        if      (strcmp(argv[i],"--tokens")==0) show_tokens = 1;
        else if (strcmp(argv[i],"--ast"   )==0) show_ast    = 1;
        else if (strcmp(argv[i],"--all"   )==0) { show_tokens=1; show_ast=1; }
        else { fprintf(stderr,"Bilinmeyen secenek: %s\n",argv[i]); usage(argv[0]); return 1; }
    }

    char *src = read_file(filepath);
    if (!src) return 1;

    int tok_count;
    Token *tokens = tokenize(src, &tok_count);
    if (!tokens) { fprintf(stderr,"Lexer: bellek hatasi\n"); free(src); return 1; }

    if (show_tokens) {
        printf("--- TOKENLAR (%d adet) ---\n", tok_count);
        for (int i = 0; i < tok_count; i++) print_token(&tokens[i]);
        printf("\n");
    }

    for (int i = 0; i < tok_count; i++) {
        if (tokens[i].type == TOK_ERROR) {
            fprintf(stderr,"[%d:%d] Lexer hatasi: %s\n",
                tokens[i].line, tokens[i].col, tokens[i].lexeme);
            free(tokens); free(src); return 1;
        }
    }

    Parser P;
    parser_init(&P, tokens, tok_count);
    Node *ast = parse_program(&P);

    if (show_ast) {
        printf("--- AST ---\n");
        node_print(ast, 0);
        printf("\n");
    }

    if (P.had_error) {
        fprintf(stderr,"Parse hatasi — calistirma iptal edildi.\n");
        node_free(ast); free(tokens); free(src); return 1;
    }

    Interpreter I;
    interp_init(&I);
    interp_run(&I, ast);

    node_free(ast); free(tokens); free(src);
    return I.had_error ? 1 : 0;
}

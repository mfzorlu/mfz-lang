/*
 * parser.h — AST düğüm tipleri ve Parser arayüzü
 *
 * Yeni sözdizimi eklemek:
 *   1. NodeType enum'una yeni sabit ekle
 *   2. Node union'ına gerekli alanları ekle
 *   3. parser.c'de parse_* fonksiyonu yaz
 *   4. parse_stmt() veya parse_expr()'e yönlendirme ekle
 */

#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

/* ─────────────────────────────────────────
 * 1. AST DÜĞÜM TİPLERİ
 * ───────────────────────────────────────── */
typedef enum {
    /* İfadeler (Expressions) */
    NODE_INT_LIT,       /* 42              */
    NODE_FLOAT_LIT,     /* 3.14            */
    NODE_STRING_LIT,    /* "hello"         */
    NODE_IDENT,         /* x               */
    NODE_BINOP,         /* a + b           */
    NODE_UNOP,          /* -x, !x          */
    NODE_ASSIGN,        /* x = expr        */
    NODE_CALL,          /* f(a, b)         */
    /* YENİ İFADE: NODE_INDEX, NODE_MEMBER, ... */

    /* Deyimler (Statements) */
    NODE_VAR_DECL,      /* int x = expr;   */
    NODE_EXPR_STMT,     /* expr;           */
    NODE_BLOCK,         /* { stmt* }       */
    NODE_IF,            /* if/else         */
    NODE_WHILE,         /* while           */
    NODE_FOR,           /* for             */
    NODE_RETURN,        /* return expr;    */
    NODE_FUNC_DECL,     /* int f(p){body}  */
    /* YENİ DEYİM: NODE_MATCH, NODE_BREAK, ... */

    NODE_PROGRAM        /* kök düğüm       */
} NodeType;

/* ─────────────────────────────────────────
 * 2. AST DÜĞÜM YAPISI  (özyinelemeli)
 * ───────────────────────────────────────── */
typedef struct Node Node;
struct Node {
    NodeType type;
    int      line;  /* hata raporlama için */

    union {
        /* ── Literaller ── */
        struct { long   val; }          int_lit;
        struct { double val; }          float_lit;
        struct { char   val[256]; }     string_lit;
        struct { char   name[64]; }     ident;

        /* ── İkili işlem:  left OP right ── */
        struct {
            TokenType  op;
            Node      *left;
            Node      *right;
        } binop;

        /* ── Tekli işlem:  OP operand ── */
        struct {
            TokenType  op;
            Node      *operand;
        } unop;

        /* ── Atama:  name = value ── */
        struct {
            char  name[64];
            Node *value;
        } assign;

        /* ── Fonksiyon çağrısı:  name(args[0..n]) ── */
        struct {
            char   name[64];
            Node **args;
            int    arg_count;
        } call;

        /* ── Değişken bildirimi:  type name = init ── */
        struct {
            TokenType  var_type;   /* TOK_INT / TOK_FLOAT / TOK_VOID */
            char       name[64];
            Node      *init;       /* NULL ise başlatıcı yok */
        } var_decl;

        /* ── İfade deyimi ── */
        struct { Node *expr; } expr_stmt;

        /* ── Blok:  { stmts[0..n] } ── */
        struct {
            Node **stmts;
            int    count;
        } block;

        /* ── if (cond) then [else alt] ── */
        struct {
            Node *cond;
            Node *then_branch;
            Node *else_branch;  /* NULL olabilir */
        } if_stmt;

        /* ── while (cond) body ── */
        struct {
            Node *cond;
            Node *body;
        } while_stmt;

        /* ── for (init; cond; step) body ── */
        struct {
            Node *init;   /* NULL olabilir */
            Node *cond;   /* NULL olabilir */
            Node *step;   /* NULL olabilir */
            Node *body;
        } for_stmt;

        /* ── return [expr] ── */
        struct { Node *value; } ret;

        /* ── Fonksiyon bildirimi ── */
        struct {
            TokenType  ret_type;
            char       name[64];
            char     (*params)[64];   /* parametre isimleri */
            TokenType *param_types;   /* parametre tipleri  */
            int        param_count;
            Node      *body;          /* NODE_BLOCK */
        } func_decl;

        /* ── Program kökü ── */
        struct {
            Node **decls;
            int    count;
        } program;

    } as;
};

/* ─────────────────────────────────────────
 * 3. PARSER YAPISI
 * ───────────────────────────────────────── */
typedef struct {
    Token *tokens;      /* lexer çıktısı   */
    int    count;       /* token sayısı    */
    int    pos;         /* mevcut konum    */
    int    had_error;   /* hata bayrağı    */
} Parser;

/* ─────────────────────────────────────────
 * 4. PARSER API
 * ───────────────────────────────────────── */
void  parser_init  (Parser *P, Token *tokens, int count);
Node *parse_program(Parser *P);          /* kök — buradan başla  */
void  node_free    (Node *n);            /* AST'yi serbest bırak */
void  node_print   (const Node *n, int indent); /* debug yazdır  */

#endif /* PARSER_H */
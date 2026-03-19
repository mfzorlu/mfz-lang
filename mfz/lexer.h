/*
 * lexer.h — Token tipleri, Token yapısı, Lexer arayüzü
 *
 * Bu başlık dosyası lexer.c, parser.c ve interpreter.c
 * tarafından #include edilir. Tanımlar burada, implementasyon lexer.c'de.
 */

#ifndef LEXER_H
#define LEXER_H

/* ─────────────────────────────────────────
 * 1. TOKEN TÜRLERİ
 *    Yeni söz dizimi → enum + TOKEN_NAMES + KEYWORDS tablosuna ekle
 * ───────────────────────────────────────── */
typedef enum {
    /* Literaller */
    TOK_INT_LIT,
    TOK_FLOAT_LIT,
    TOK_STRING_LIT,

    /* Tanımlayıcı */
    TOK_IDENT,

    /* Anahtar kelimeler */
    TOK_IF,
    TOK_ELSE,
    TOK_WHILE,
    TOK_FOR,
    TOK_RETURN,
    TOK_INT,
    TOK_FLOAT,
    TOK_VOID,
    /* YENİ: TOK_FN, TOK_MATCH, TOK_LET ... */

    /* Aritmetik operatörler */
    TOK_PLUS,        /* +  */
    TOK_MINUS,       /* -  */
    TOK_STAR,        /* *  */
    TOK_SLASH,       /* /  */
    TOK_PERCENT,     /* %  */

    /* Karşılaştırma */
    TOK_EQ,          /* == */
    TOK_NEQ,         /* != */
    TOK_LT,          /* <  */
    TOK_GT,          /* >  */
    TOK_LEQ,         /* <= */
    TOK_GEQ,         /* >= */

    /* Atama & mantıksal */
    TOK_ASSIGN,      /* =  */
    TOK_AND,         /* && */
    TOK_OR,          /* || */
    TOK_NOT,         /* !  */

    /* Noktalama */
    TOK_LPAREN,      /* (  */
    TOK_RPAREN,      /* )  */
    TOK_LBRACE,      /* {  */
    TOK_RBRACE,      /* }  */
    TOK_LBRACKET,    /* [  */
    TOK_RBRACKET,    /* ]  */
    TOK_SEMICOLON,   /* ;  */
    TOK_COLON,       /* :  */
    TOK_COMMA,       /* ,  */
    TOK_DOT,         /* .  */
    TOK_ARROW,       /* -> */

    /* Sistem */
    TOK_EOF,
    TOK_ERROR,

    TOK_COUNT        /* — silme */
} TokenType;

/* Token'ın yazdırılabilir ismi */
extern const char *TOKEN_NAMES[TOK_COUNT];

/* ─────────────────────────────────────────
 * 2. TOKEN YAPISI
 * ───────────────────────────────────────── */
typedef struct {
    TokenType type;
    char      lexeme[256];
    int       line;
    int       col;
    union {
        long   ival;
        double fval;
    } value;
} Token;

/* ─────────────────────────────────────────
 * 3. LEXER YAPISI
 * ───────────────────────────────────────── */
typedef struct {
    const char *src;
    int         pos;
    int         line;
    int         col;
} Lexer;

/* ─────────────────────────────────────────
 * 4. LEXER API
 * ───────────────────────────────────────── */
void   lexer_init  (Lexer *L, const char *src);
Token  next_token  (Lexer *L);
Token *tokenize    (const char *src, int *out_count);
void   print_token (const Token *t);

#endif /* LEXER_H */
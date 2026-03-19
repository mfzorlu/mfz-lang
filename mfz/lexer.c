/*
 * lexer.c — Lexer implementasyonu
 *
 * Yeni token/sözdizimi eklemek:
 *   1. lexer.h → TokenType enum + TOKEN_NAMES
 *   2. KEYWORDS[] tablosuna satır ekle  (anahtar kelimeler için)
 *   3. DUAL[] veya SINGLE[] tablosuna satır ekle (operatörler için)
 *   4. Yeni literal → match_* fonksiyonu yaz, next_token()'a dal ekle
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"

/* ─────────────────────────────────────────
 * TOKEN İSİMLERİ  (lexer.h'deki enum ile aynı sıra)
 * ───────────────────────────────────────── */
const char *TOKEN_NAMES[TOK_COUNT] = {
    "INT_LIT", "FLOAT_LIT", "STRING_LIT",
    "IDENT",
    "IF", "ELSE", "WHILE", "FOR", "RETURN", "INT", "FLOAT", "VOID",
    "PLUS", "MINUS", "STAR", "SLASH", "PERCENT",
    "EQ", "NEQ", "LT", "GT", "LEQ", "GEQ",
    "ASSIGN", "AND", "OR", "NOT",
    "LPAREN", "RPAREN", "LBRACE", "RBRACE",
    "LBRACKET", "RBRACKET",
    "SEMICOLON", "COLON", "COMMA", "DOT", "ARROW",
    "EOF", "ERROR"
};

/* ─────────────────────────────────────────
 * ANAHTAR KELİME TABLOSU
 *    Yeni kelime → tek satır ekle
 * ───────────────────────────────────────── */
typedef struct { const char *word; TokenType type; } Keyword;

static const Keyword KEYWORDS[] = {
    { "if",     TOK_IF     },
    { "else",   TOK_ELSE   },
    { "while",  TOK_WHILE  },
    { "for",    TOK_FOR    },
    { "return", TOK_RETURN },
    { "int",    TOK_INT    },
    { "float",  TOK_FLOAT  },
    { "void",   TOK_VOID   },
    /* YENİ: { "fn", TOK_FN }, */
    { NULL,     TOK_ERROR  }   /* sentinel */
};

/* ─────────────────────────────────────────
 * DÜŞÜK SEVİYE YARDIMCILAR
 * ───────────────────────────────────────── */
static char peek_c     (const Lexer *L) { return L->src[L->pos]; }
static char peek_next_c(const Lexer *L) {
    return L->src[L->pos] ? L->src[L->pos + 1] : '\0';
}
static char advance_c(Lexer *L) {
    char c = L->src[L->pos++];
    if (c == '\n') { L->line++; L->col = 1; }
    else           { L->col++; }
    return c;
}

static Token make_tok(TokenType t, const char *lex, int line, int col) {
    Token tok;
    tok.type       = t;
    tok.line       = line;
    tok.col        = col;
    tok.value.ival = 0;
    strncpy(tok.lexeme, lex, sizeof(tok.lexeme) - 1);
    tok.lexeme[sizeof(tok.lexeme) - 1] = '\0';
    return tok;
}
static Token err_tok(const char *msg, int line, int col) {
    return make_tok(TOK_ERROR, msg, line, col);
}

/* ─────────────────────────────────────────
 * BOŞLUK & YORUM ATLAMA
 * ───────────────────────────────────────── */
static void skip_ws(Lexer *L) {
    while (1) {
        while (isspace((unsigned char)peek_c(L))) advance_c(L);

        /* // tek satır */
        if (peek_c(L) == '/' && peek_next_c(L) == '/') {
            while (peek_c(L) && peek_c(L) != '\n') advance_c(L);
            continue;
        }
        /* çok satır yorum: slash-star ... star-slash */
        if (peek_c(L) == '/' && peek_next_c(L) == '*') {
            advance_c(L); advance_c(L);
            while (peek_c(L)) {
                if (peek_c(L) == '*' && peek_next_c(L) == '/') {
                    advance_c(L); advance_c(L); break;
                }
                advance_c(L);
            }
            continue;
        }
        break;
    }
}

/* ─────────────────────────────────────────
 * MATCH FONKSİYONLARI
 *   Her kategori ayrı fonksiyon — yeni literal
 *   türü için yeni match_* yaz.
 * ───────────────────────────────────────── */

static Token match_number(Lexer *L) {
    int  line = L->line, col = L->col;
    char buf[64]; int i = 0, is_float = 0;

    /* FIX #1: buf[64] -> max i=63 + null terminator=buf[63].
     * Nokta + ondalik icin yer birak: tam sayi kismi en fazla 61 karakter. */
    while (isdigit((unsigned char)peek_c(L)) && i < 61)
        buf[i++] = advance_c(L);

    if (peek_c(L) == '.' && isdigit((unsigned char)peek_next_c(L))) {
        is_float = 1;
        if (i < 62) buf[i++] = advance_c(L); /* nokta — yer garantili */
        while (isdigit((unsigned char)peek_c(L)) && i < 63)
            buf[i++] = advance_c(L);
    }
    buf[i] = '\0';  /* i <= 63, her zaman guvenli */

    Token t = make_tok(is_float ? TOK_FLOAT_LIT : TOK_INT_LIT, buf, line, col);
    if (is_float) t.value.fval = atof(buf);
    else          t.value.ival = atol(buf);
    return t;
}

static Token match_string(Lexer *L) {
    int  line = L->line, col = L->col;
    char buf[256]; int i = 0;
    advance_c(L); /* " */

    while (peek_c(L) && peek_c(L) != '"' && i < 255) {
        if (peek_c(L) == '\\') {
            advance_c(L);  /* '\\' karakterini tüket */
            /* FIX #2: EOF kontrolü — dosya '\' ile bitiyorsa
             * ikinci advance_c NULL okur ve bellek dışına çıkar. */
            if (!peek_c(L)) break;
            switch (advance_c(L)) {
                case 'n':  buf[i++] = '\n'; break;
                case 't':  buf[i++] = '\t'; break;
                case '"':  buf[i++] = '"';  break;
                case '\\': buf[i++] = '\\'; break;
                default:   buf[i++] = '?';  break;
            }
        } else { buf[i++] = advance_c(L); }
    }
    buf[i] = '\0';
    if (peek_c(L) != '"') return err_tok("Bitmemiş string", line, col);
    advance_c(L);
    return make_tok(TOK_STRING_LIT, buf, line, col);
}

static Token match_ident(Lexer *L) {
    int  line = L->line, col = L->col;
    char buf[64]; int i = 0;

    while ((isalnum((unsigned char)peek_c(L)) || peek_c(L) == '_') && i < 63)
        buf[i++] = advance_c(L);
    buf[i] = '\0';

    for (int k = 0; KEYWORDS[k].word; k++)
        if (strcmp(buf, KEYWORDS[k].word) == 0)
            return make_tok(KEYWORDS[k].type, buf, line, col);

    return make_tok(TOK_IDENT, buf, line, col);
}

static Token match_op(Lexer *L) {
    int  line = L->line, col = L->col;
    char c    = advance_c(L);
    char buf[3] = { c, '\0', '\0' };

    /* Çift karakter — yeni eklemek için satır ekle */
    typedef struct { char a, b; TokenType t; } DualOp;
    static const DualOp DUAL[] = {
        { '=','=', TOK_EQ    }, { '!','=', TOK_NEQ   },
        { '<','=', TOK_LEQ   }, { '>','=', TOK_GEQ   },
        { '&','&', TOK_AND   }, { '|','|', TOK_OR    },
        { '-','>', TOK_ARROW },
        /* YENİ: { '+','+', TOK_INC }, */
        { 0, 0, TOK_ERROR }
    };
    for (int i = 0; DUAL[i].a; i++) {
        if (c == DUAL[i].a && peek_c(L) == DUAL[i].b) {
            buf[1] = advance_c(L);
            return make_tok(DUAL[i].t, buf, line, col);
        }
    }

    /* Tek karakter — yeni eklemek için satır ekle */
    typedef struct { char c; TokenType t; } SingleOp;
    static const SingleOp SINGLE[] = {
        {'+',TOK_PLUS},    {'-',TOK_MINUS},   {'*',TOK_STAR},
        {'/',TOK_SLASH},   {'%',TOK_PERCENT},  {'=',TOK_ASSIGN},
        {'<',TOK_LT},      {'>',TOK_GT},       {'!',TOK_NOT},
        {'(',TOK_LPAREN},  {')',TOK_RPAREN},   {'{',TOK_LBRACE},
        {'}',TOK_RBRACE},  {'[',TOK_LBRACKET},{']',TOK_RBRACKET},
        {';',TOK_SEMICOLON},{':',TOK_COLON},  {',',TOK_COMMA},
        {'.',TOK_DOT},
        /* YENİ: {'@', TOK_AT}, */
        {0, TOK_ERROR}
    };
    for (int i = 0; SINGLE[i].c; i++)
        if (c == SINGLE[i].c)
            return make_tok(SINGLE[i].t, buf, line, col);

    return err_tok("Bilinmeyen karakter", line, col);
}

/* ─────────────────────────────────────────
 * PUBLIC API
 * ───────────────────────────────────────── */
void lexer_init(Lexer *L, const char *src) {
    L->src = src; L->pos = 0; L->line = 1; L->col = 1;
}

Token next_token(Lexer *L) {
    skip_ws(L);
    if (!peek_c(L)) return make_tok(TOK_EOF, "EOF", L->line, L->col);
    char c = peek_c(L);
    if (isdigit((unsigned char)c))           return match_number(L);
    if (c == '"')                             return match_string(L);
    if (isalpha((unsigned char)c) || c=='_') return match_ident(L);
    return match_op(L);
}

Token *tokenize(const char *src, int *out_count) {
    Lexer L; lexer_init(&L, src);
    int cap = 64, count = 0;
    Token *toks = malloc(cap * sizeof(Token));
    if (!toks) return NULL;
    while (1) {
        if (count >= cap) {
            cap *= 2;
            /* FIX #5: realloc NULL donerse eski pointer kaybolur (memory leak).
             * Gecici pointer kullan; basarisizsa eskiyi serbest birak. */
            Token *tmp = realloc(toks, cap * sizeof(Token));
            if (!tmp) { free(toks); return NULL; }
            toks = tmp;
        }
        toks[count] = next_token(&L);
        count++;
        if (toks[count-1].type == TOK_EOF ||
            toks[count-1].type == TOK_ERROR) break;
    }
    *out_count = count;
    return toks;
}

void print_token(const Token *t) {
    const char *name = (t->type < TOK_COUNT) ? TOKEN_NAMES[t->type] : "???";
    printf("[%2d:%-2d] %-12s '%s'", t->line, t->col, name, t->lexeme);
    if (t->type == TOK_INT_LIT)   printf("  → %ld",  t->value.ival);
    if (t->type == TOK_FLOAT_LIT) printf("  → %g",   t->value.fval);
    printf("\n");
}

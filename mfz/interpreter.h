/*
 * interpreter.h — Değer tipleri ve Interpreter arayüzü
 *
 * Yeni tip eklemek:
 *   1. ValueType enum'una sabit ekle
 *   2. Value union'ına alan ekle
 *   3. interpreter.c'de eval_* fonksiyonlarında işle
 */

#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "parser.h"

/* ─────────────────────────────────────────
 * 1. ÇALIŞMA ZAMANI DEĞERİ
 * ───────────────────────────────────────── */
typedef enum {
    VAL_INT,
    VAL_FLOAT,
    VAL_STRING,
    VAL_VOID,
    /* YENİ: VAL_BOOL, VAL_LIST, VAL_NULL ... */
} ValueType;

typedef struct {
    ValueType type;
    union {
        long   ival;
        double fval;
        char   sval[256];
    } as;
} Value;

/* Değer oluşturucular */
static inline Value val_int   (long v)         { Value r; r.type=VAL_INT;    r.as.ival=v;            return r; }
static inline Value val_float (double v)        { Value r; r.type=VAL_FLOAT;  r.as.fval=v;            return r; }
static inline Value val_void  (void)            { Value r; r.type=VAL_VOID;   r.as.ival=0;            return r; }
static inline Value val_string(const char *s)   {
    Value r; r.type=VAL_STRING;
    strncpy(r.as.sval, s, 255); r.as.sval[255]='\0';
    return r;
}

/* ─────────────────────────────────────────
 * 2. ORTAM (değişken kapsamı)
 *    Bağlantılı liste — her blok yeni kapsam
 * ───────────────────────────────────────── */
#define ENV_SIZE 64

typedef struct Env Env;
struct Env {
    char  names[ENV_SIZE][64];
    Value values[ENV_SIZE];
    int   count;
    Env  *parent;   /* dış kapsam */
};

/* ─────────────────────────────────────────
 * 3. FONKSİYON TABLOSU
 * ───────────────────────────────────────── */
#define FUNC_MAX 64

typedef struct {
    char  name[64];
    Node *decl;     /* NODE_FUNC_DECL düğümü */
} FuncEntry;

/* ─────────────────────────────────────────
 * 4. INTERPRETER YAPISI
 * ───────────────────────────────────────── */
typedef struct {
    FuncEntry funcs[FUNC_MAX];
    int       func_count;
    int       had_error;
    Env      *global_env;  /* FIX #4: eval_call icin global kapsam referansi */
} Interpreter;

/* ─────────────────────────────────────────
 * 5. INTERPRETER API
 * ───────────────────────────────────────── */
void  interp_init   (Interpreter *I);
void  interp_run    (Interpreter *I, Node *program);
Value interp_eval   (Interpreter *I, Node *expr,  Env *env);
Value interp_exec   (Interpreter *I, Node *stmt,  Env *env);
void  value_print   (Value v);

/* Ortam API */
Env  *env_new       (Env *parent);
void  env_free      (Env *env);
void  env_set       (Env *env, const char *name, Value val);
int   env_get       (Env *env, const char *name, Value *out);
void  env_set_local (Env *env, const char *name, Value val);

#endif /* INTERPRETER_H */

/*
 * interpreter.c — Tree-walk Interpreter
 *
 * AST düğümlerini doğrudan yürüterek sonuç üretir.
 * Derleme yok — her düğüm ziyaret edilip değerlendirilir.
 *
 * Yeni özellik eklemek:
 *   - Yeni operatör   → eval_binop() / eval_unop() switch'ine case ekle
 *   - Yeni deyim      → interp_exec() switch'ine case + exec_* fonksiyonu
 *   - Yeni built-in   → BUILTINS[] tablosuna satır ekle
 *   - Yeni değer tipi → interpreter.h'de ValueType + burada işle
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "interpreter.h"

/* ─────────────────────────────────────────
 * RETURN / SINYAL MEKANİZMASI
 *   return deyimi bloğu erken kesmek için
 *   longjmp yerine sinyal bayrağı kullanıyoruz
 * ───────────────────────────────────────── */
typedef struct {
    int   active;   /* 1 = return tetiklendi */
    Value value;
} ReturnSignal;

/* exec fonksiyonları ReturnSignal* alır — NULL geçmek güvenli */
static ReturnSignal *g_ret = NULL; /* aktif sinyal (çağrı zincirinde taşınır) */

/* ─────────────────────────────────────────
 * ORTAM (kapsam zinciri)
 * ───────────────────────────────────────── */
Env *env_new(Env *parent) {
    Env *e = calloc(1, sizeof(Env));
    if (!e) { fprintf(stderr, "Bellek hatası\n"); exit(1); }
    e->parent = parent;
    e->count  = 0;
    return e;
}

void env_free(Env *env) {
    free(env);
}

/* Mevcut veya üst kapsamda ara, bul → güncelle; bulamazsan yerel oluştur */
void env_set(Env *env, const char *name, Value val) {
    for (Env *e = env; e; e = e->parent) {
        for (int i = 0; i < e->count; i++) {
            if (strcmp(e->names[i], name) == 0) {
                e->values[i] = val;
                return;
            }
        }
    }
    /* Yeni değişken — mevcut kapsamda oluştur */
    env_set_local(env, name, val);
}

/* Sadece mevcut kapsamda oluştur (var_decl için) */
void env_set_local(Env *env, const char *name, Value val) {
    if (env->count >= ENV_SIZE) {
        fprintf(stderr, "Kapsam doldu: %s\n", name); return;
    }
    strncpy(env->names[env->count], name, 63);
    env->values[env->count] = val;
    env->count++;
}

/* Değişkeni kapsam zincirinde ara */
int env_get(Env *env, const char *name, Value *out) {
    for (Env *e = env; e; e = e->parent) {
        for (int i = 0; i < e->count; i++) {
            if (strcmp(e->names[i], name) == 0) {
                *out = e->values[i];
                return 1;
            }
        }
    }
    return 0;
}

/* ─────────────────────────────────────────
 * HATA YARDIMCISI
 * ───────────────────────────────────────── */
static Value runtime_error(Interpreter *I, int line, const char *msg) {
    fprintf(stderr, "[satır %d] Çalışma hatası: %s\n", line, msg);
    I->had_error = 1;
    return val_void();
}

/* ─────────────────────────────────────────
 * DEĞER YAZDIRICI
 * ───────────────────────────────────────── */
void value_print(Value v) {
    switch (v.type) {
        case VAL_INT:    printf("%ld",  v.as.ival); break;
        case VAL_FLOAT:  printf("%g",   v.as.fval); break;
        case VAL_STRING: printf("%s",   v.as.sval); break;
        case VAL_VOID:   printf("void"); break;
    }
}

/* ─────────────────────────────────────────
 * BUILT-IN FONKSİYONLAR
 *   Yeni built-in → BUILTINS tablosuna satır ekle
 *   ve aşağıya bir call_builtin_* fonksiyonu yaz
 * ───────────────────────────────────────── */
typedef Value (*BuiltinFn)(Interpreter *I, Value *args, int argc, int line);

static Value builtin_print(Interpreter *I, Value *args, int argc, int line) {
    (void)I; (void)line;
    for (int i = 0; i < argc; i++) {
        if (i) printf(" ");
        value_print(args[i]);
    }
    printf("\n");
    return val_void();
}

static Value builtin_println(Interpreter *I, Value *args, int argc, int line) {
    return builtin_print(I, args, argc, line);
}

static Value builtin_sqrt(Interpreter *I, Value *args, int argc, int line) {
    if (argc != 1) return runtime_error(I, line, "sqrt() 1 argüman ister");
    double v = (args[0].type == VAL_INT) ? (double)args[0].as.ival : args[0].as.fval;
    return val_float(sqrt(v));
}

static Value builtin_abs(Interpreter *I, Value *args, int argc, int line) {
    if (argc != 1) return runtime_error(I, line, "abs() 1 argüman ister");
    if (args[0].type == VAL_INT)   return val_int((long)labs(args[0].as.ival));
    if (args[0].type == VAL_FLOAT) return val_float(fabs(args[0].as.fval));
    return runtime_error(I, line, "abs() sayısal değer ister");
}

static Value builtin_int_cast(Interpreter *I, Value *args, int argc, int line) {
    if (argc != 1) return runtime_error(I, line, "int() 1 argüman ister");
    if (args[0].type == VAL_FLOAT) return val_int((long)args[0].as.fval);
    if (args[0].type == VAL_INT)   return args[0];
    return runtime_error(I, line, "int() dönüştürülemiyor");
}

static Value builtin_float_cast(Interpreter *I, Value *args, int argc, int line) {
    if (argc != 1) return runtime_error(I, line, "float() 1 argüman ister");
    if (args[0].type == VAL_INT)   return val_float((double)args[0].as.ival);
    if (args[0].type == VAL_FLOAT) return args[0];
    return runtime_error(I, line, "float() dönüştürülemiyor");
}

/* YENİ built-in → buraya static Value builtin_yeni(...) fonksiyonu ekle */

typedef struct { const char *name; BuiltinFn fn; } Builtin;

static const Builtin BUILTINS[] = {
    { "print",   builtin_print      },
    { "println", builtin_println    },
    { "sqrt",    builtin_sqrt       },
    { "abs",     builtin_abs        },
    { "int",     builtin_int_cast   },
    { "float",   builtin_float_cast },
    /* YENİ: { "len", builtin_len }, */
    { NULL, NULL }
};

static BuiltinFn find_builtin(const char *name) {
    for (int i = 0; BUILTINS[i].name; i++)
        if (strcmp(BUILTINS[i].name, name) == 0)
            return BUILTINS[i].fn;
    return NULL;
}

/* ─────────────────────────────────────────
 * FORWARD DECLARATIONS
 * ───────────────────────────────────────── */
static Value exec_block(Interpreter *I, Node *block, Env *parent, ReturnSignal *ret);

/* ─────────────────────────────────────────
 * HESAPLAMA: İFADELER
 * ───────────────────────────────────────── */

/* İki değeri sayısal olarak karşılaştırmak için ortak dönüşüm */
static double to_float(Value v) {
    return (v.type == VAL_INT) ? (double)v.as.ival : v.as.fval;
}
static int is_numeric(Value v) {
    return v.type == VAL_INT || v.type == VAL_FLOAT;
}
static int is_truthy(Value v) {
    if (v.type == VAL_INT)   return v.as.ival != 0;
    if (v.type == VAL_FLOAT) return v.as.fval != 0.0;
    if (v.type == VAL_STRING) return v.as.sval[0] != '\0';
    return 0;
}

/* Aritmetik / karşılaştırma sonucu tipi: iki int → int, aksi → float */
static Value numeric_result(int int_ok, long ival, double fval) {
    return int_ok ? val_int(ival) : val_float(fval);
}

static Value eval_binop(Interpreter *I, Node *n, Env *env) {
    Value L = interp_eval(I, n->as.binop.left,  env);
    Value R = interp_eval(I, n->as.binop.right, env);
    TokenType op = n->as.binop.op;

    /* String birleştirme: "a" + "b" */
    if (op == TOK_PLUS && L.type == VAL_STRING && R.type == VAL_STRING) {
        char buf[512];
        snprintf(buf, sizeof(buf), "%s%s", L.as.sval, R.as.sval);
        return val_string(buf);
    }

    /* Sayısal işlemler */
    if (is_numeric(L) && is_numeric(R)) {
        int  both_int = (L.type == VAL_INT && R.type == VAL_INT);
        long li = L.as.ival, ri = R.as.ival;
        double lf = to_float(L), rf = to_float(R);

        switch (op) {
            case TOK_PLUS:    return numeric_result(both_int, li+ri, lf+rf);
            case TOK_MINUS:   return numeric_result(both_int, li-ri, lf-rf);
            case TOK_STAR:    return numeric_result(both_int, li*ri, lf*rf);
            case TOK_SLASH:
                if (both_int) {
                    if (ri == 0) return runtime_error(I, n->line, "Sıfıra bölme");
                    return val_int(li / ri);
                }
                if (rf == 0.0) return runtime_error(I, n->line, "Sıfıra bölme");
                return val_float(lf / rf);
            case TOK_PERCENT:
                if (!both_int) return runtime_error(I, n->line, "% sadece tam sayı");
                if (ri == 0)   return runtime_error(I, n->line, "Sıfıra mod");
                return val_int(li % ri);
            case TOK_LT:  return val_int(lf <  rf);
            case TOK_GT:  return val_int(lf >  rf);
            case TOK_LEQ: return val_int(lf <= rf);
            case TOK_GEQ: return val_int(lf >= rf);
            case TOK_EQ:  return val_int(lf == rf);
            case TOK_NEQ: return val_int(lf != rf);
            /* YENİ operatör → buraya case ekle */
            default: break;
        }
    }

    /* Mantıksal — kısa devre değil (AST tabanlı olduğu için) */
    switch (op) {
        case TOK_AND: return val_int(is_truthy(L) && is_truthy(R));
        case TOK_OR:  return val_int(is_truthy(L) || is_truthy(R));
        case TOK_EQ:
            if (L.type==VAL_STRING && R.type==VAL_STRING)
                return val_int(strcmp(L.as.sval, R.as.sval)==0);
            break;
        case TOK_NEQ:
            if (L.type==VAL_STRING && R.type==VAL_STRING)
                return val_int(strcmp(L.as.sval, R.as.sval)!=0);
            break;
        default: break;
    }

    return runtime_error(I, n->line, "Geçersiz ikili işlem");
}

static Value eval_unop(Interpreter *I, Node *n, Env *env) {
    Value v = interp_eval(I, n->as.unop.operand, env);
    switch (n->as.unop.op) {
        case TOK_MINUS:
            if (v.type == VAL_INT)   return val_int(-v.as.ival);
            if (v.type == VAL_FLOAT) return val_float(-v.as.fval);
            break;
        case TOK_NOT:
            return val_int(!is_truthy(v));
        /* YENİ: case TOK_TILDE: ... */
        default: break;
    }
    return runtime_error(I, n->line, "Geçersiz tekli işlem");
}

static Value eval_call(Interpreter *I, Node *n, Env *env) {
    /* Argümanları hesapla */
    Value args[32];
    int   argc = n->as.call.arg_count;
    if (argc > 32) argc = 32;
    for (int i = 0; i < argc; i++)
        args[i] = interp_eval(I, n->as.call.args[i], env);

    /* Built-in mi? */
    BuiltinFn bf = find_builtin(n->as.call.name);
    if (bf) return bf(I, args, argc, n->line);

    /* Kullanıcı fonksiyonu */
    for (int i = 0; i < I->func_count; i++) {
        if (strcmp(I->funcs[i].name, n->as.call.name) == 0) {
            Node *decl = I->funcs[i].decl;

            /* Yeni kapsam — parametre bağlama */
            Env *fenv = env_new(NULL); /* global kapsam — NULL */
            int pc = decl->as.func_decl.param_count;
            if (argc != pc) {
                fprintf(stderr, "[satır %d] '%s': %d argüman beklendi, %d verildi\n",
                        n->line, n->as.call.name, pc, argc);
                I->had_error = 1;
                env_free(fenv);
                return val_void();
            }
            for (int p = 0; p < pc; p++)
                env_set_local(fenv, decl->as.func_decl.params[p], args[p]);

            /* g_ret'i kaydet, fonksiyon için yeni sinyal kur */
            ReturnSignal  ret      = {0, val_void()};
            ReturnSignal *prev_ret = g_ret;
            g_ret = &ret;
            exec_block(I, decl->as.func_decl.body, fenv, &ret);
            g_ret = prev_ret;   /* çağıranın sinyalini geri yükle */
            env_free(fenv);
            return ret.active ? ret.value : val_void();
        }
    }

    char msg[128];
    snprintf(msg, sizeof(msg), "Tanımsız fonksiyon: %s", n->as.call.name);
    return runtime_error(I, n->line, msg);
}

/* Ana ifade değerlendirici */
Value interp_eval(Interpreter *I, Node *expr, Env *env) {
    if (!expr || I->had_error) return val_void();

    switch (expr->type) {
        case NODE_INT_LIT:    return val_int(expr->as.int_lit.val);
        case NODE_FLOAT_LIT:  return val_float(expr->as.float_lit.val);
        case NODE_STRING_LIT: return val_string(expr->as.string_lit.val);

        case NODE_IDENT: {
            Value v;
            if (env_get(env, expr->as.ident.name, &v)) return v;
            char msg[128];
            snprintf(msg, sizeof(msg), "Tanımsız değişken: %s", expr->as.ident.name);
            return runtime_error(I, expr->line, msg);
        }

        case NODE_BINOP:  return eval_binop(I, expr, env);
        case NODE_UNOP:   return eval_unop (I, expr, env);

        case NODE_ASSIGN: {
            Value v = interp_eval(I, expr->as.assign.value, env);
            env_set(env, expr->as.assign.name, v);
            return v;
        }

        case NODE_CALL: return eval_call(I, expr, env);

        /* YENİ ifade türü → buraya case ekle */

        default:
            return runtime_error(I, expr->line, "Bilinmeyen ifade türü");
    }
}

/* ─────────────────────────────────────────
 * YÜRÜTME: DEYİMLER
 * ───────────────────────────────────────── */

static Value exec_block(Interpreter *I, Node *block, Env *parent, ReturnSignal *ret) {
    Env *env = env_new(parent);
    for (int i = 0; i < block->as.block.count; i++) {
        interp_exec(I, block->as.block.stmts[i], env);
        /* return sinyali geldiyse bloğu kes */
        if (ret && ret->active) break;
        if (g_ret && g_ret->active) break;
        if (I->had_error) break;
    }
    env_free(env);
    return val_void();
}

/* Ana deyim yürütücü */
Value interp_exec(Interpreter *I, Node *stmt, Env *env) {
    if (!stmt || I->had_error) return val_void();

    switch (stmt->type) {

        /* Değişken bildirimi: int x = expr; */
        case NODE_VAR_DECL: {
            Value v = stmt->as.var_decl.init
                      ? interp_eval(I, stmt->as.var_decl.init, env)
                      : val_int(0);
            env_set_local(env, stmt->as.var_decl.name, v);
            return v;
        }

        /* İfade deyimi: expr; */
        case NODE_EXPR_STMT:
            return interp_eval(I, stmt->as.expr_stmt.expr, env);

        /* Blok: { ... } */
        case NODE_BLOCK: {
            ReturnSignal ret = {0, val_void()};
            exec_block(I, stmt, env, &ret);
            if (ret.active && g_ret) *g_ret = ret;
            return val_void();
        }

        /* if/else */
        case NODE_IF: {
            Value cond = interp_eval(I, stmt->as.if_stmt.cond, env);
            if (is_truthy(cond))
                interp_exec(I, stmt->as.if_stmt.then_branch, env);
            else if (stmt->as.if_stmt.else_branch)
                interp_exec(I, stmt->as.if_stmt.else_branch, env);
            return val_void();
        }

        /* while */
        case NODE_WHILE: {
            while (!I->had_error) {
                Value cond = interp_eval(I, stmt->as.while_stmt.cond, env);
                if (!is_truthy(cond)) break;
                ReturnSignal ret = {0, val_void()};
                ReturnSignal *prev = g_ret;
                g_ret = &ret;
                interp_exec(I, stmt->as.while_stmt.body, env);
                g_ret = prev;
                if (ret.active) { if (g_ret) *g_ret = ret; break; }
            }
            return val_void();
        }

        /* for */
        case NODE_FOR: {
            Env *fenv = env_new(env);
            if (stmt->as.for_stmt.init)
                interp_eval(I, stmt->as.for_stmt.init, fenv);

            while (!I->had_error) {
                if (stmt->as.for_stmt.cond) {
                    Value c = interp_eval(I, stmt->as.for_stmt.cond, fenv);
                    if (!is_truthy(c)) break;
                }
                ReturnSignal ret = {0, val_void()};
                ReturnSignal *prev = g_ret;
                g_ret = &ret;
                interp_exec(I, stmt->as.for_stmt.body, fenv);
                g_ret = prev;
                if (ret.active) { if (g_ret) *g_ret = ret; break; }

                if (stmt->as.for_stmt.step)
                    interp_eval(I, stmt->as.for_stmt.step, fenv);
            }
            env_free(fenv);
            return val_void();
        }

        /* return */
        case NODE_RETURN: {
            Value v = stmt->as.ret.value
                      ? interp_eval(I, stmt->as.ret.value, env)
                      : val_void();
            if (g_ret) { g_ret->active = 1; g_ret->value = v; }
            return v;
        }

        /* Fonksiyon bildirimi — kayıt et, çalıştırma */
        case NODE_FUNC_DECL: {
            if (I->func_count >= FUNC_MAX) {
                fprintf(stderr, "Fonksiyon tablosu doldu\n");
                I->had_error = 1; break;
            }
            strncpy(I->funcs[I->func_count].name,
                    stmt->as.func_decl.name, 63);
            I->funcs[I->func_count].decl = stmt;
            I->func_count++;
            return val_void();
        }

        /* YENİ deyim türü → buraya case ekle */

        default:
            return runtime_error(I, stmt->line, "Bilinmeyen deyim türü");
    }
    return val_void();
}

/* ─────────────────────────────────────────
 * PUBLIC API
 * ───────────────────────────────────────── */
void interp_init(Interpreter *I) {
    I->func_count = 0;
    I->had_error  = 0;
}

void interp_run(Interpreter *I, Node *program) {
    if (program->type != NODE_PROGRAM) return;

    /* Global kapsam */
    Env *global = env_new(NULL);

    /* 1. geçiş: fonksiyon bildirimlerini kaydet */
    for (int i = 0; i < program->as.program.count; i++) {
        Node *n = program->as.program.decls[i];
        if (n->type == NODE_FUNC_DECL)
            interp_exec(I, n, global);
    }

    /* 2. geçiş: geri kalan deyimleri çalıştır */
    ReturnSignal ret = {0, val_void()};
    g_ret = &ret;
    for (int i = 0; i < program->as.program.count; i++) {
        Node *n = program->as.program.decls[i];
        if (n->type != NODE_FUNC_DECL)
            interp_exec(I, n, global);
        if (I->had_error || ret.active) break;
    }
    g_ret = NULL;
    env_free(global);
}
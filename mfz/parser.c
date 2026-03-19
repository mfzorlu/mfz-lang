/*
 * parser.c — Recursive Descent Parser
 *
 * Gramer (yukarıdan aşağı öncelik):
 *
 *   program    → decl*  EOF
 *   decl       → func_decl | var_decl | stmt
 *   func_decl  → type IDENT '(' params ')' block
 *   var_decl   → type IDENT ('=' expr)? ';'
 *   stmt       → if_stmt | while_stmt | for_stmt
 *              | return_stmt | block | expr_stmt
 *   expr       → assign
 *   assign     → IDENT '=' assign | or_expr
 *   or_expr    → and_expr ('||' and_expr)*
 *   and_expr   → eq_expr  ('&&' eq_expr)*
 *   eq_expr    → rel_expr (('=='|'!=') rel_expr)*
 *   rel_expr   → add_expr (('<'|'>'|'<='|'>=') add_expr)*
 *   add_expr   → mul_expr (('+'|'-') mul_expr)*
 *   mul_expr   → unary   (('*'|'/'|'%') unary)*
 *   unary      → ('!'|'-') unary | call_expr
 *   call_expr  → primary ('(' args ')')?
 *   primary    → INT_LIT | FLOAT_LIT | STRING_LIT
 *              | IDENT | '(' expr ')'
 *
 * Yeni sözdizimi eklemek:
 *   - Yeni stmt türü → parse_stmt()'e case + parse_yeni() fonksiyonu
 *   - Yeni operatör önceliği → ilgili katmana veya yeni katman araya ekle
 *   - Yeni ifade türü → parse_primary() veya yeni katman
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"

/* ─────────────────────────────────────────
 * İÇ YARDIMCILAR
 * ───────────────────────────────────────── */

static Token *cur(Parser *P) {
    return &P->tokens[P->pos];
}
static Token *peek_tok(Parser *P, int offset) {
    int i = P->pos + offset;
    if (i >= P->count) return &P->tokens[P->count - 1]; /* EOF */
    return &P->tokens[i];
}
static Token advance_tok(Parser *P) {
    Token t = *cur(P);
    if (P->pos < P->count - 1) P->pos++;
    return t;
}
static int check(Parser *P, TokenType t) {
    return cur(P)->type == t;
}
static int match_tok(Parser *P, TokenType t) {
    if (check(P, t)) { advance_tok(P); return 1; }
    return 0;
}
static Token expect(Parser *P, TokenType t, const char *msg) {
    if (check(P, t)) return advance_tok(P);
    fprintf(stderr, "[%d:%d] Hata: %s (beklenen '%s', bulunan '%s')\n",
            cur(P)->line, cur(P)->col, msg,
            TOKEN_NAMES[t], TOKEN_NAMES[cur(P)->type]);
    P->had_error = 1;
    return *cur(P);
}

/* Hata sonrası ilerle (panic mode recovery)
 * Yeni sync noktaları → SYNC dizisine ekle */
static void synchronize(Parser *P) {
    static const TokenType SYNC[] = {
        TOK_SEMICOLON, TOK_RBRACE, TOK_IF, TOK_WHILE,
        TOK_FOR, TOK_RETURN, TOK_INT, TOK_FLOAT, TOK_VOID, TOK_EOF,
        /* YENİ: TOK_FN, ... */
    };
    while (!check(P, TOK_EOF)) {
        for (size_t i = 0; i < sizeof(SYNC)/sizeof(SYNC[0]); i++)
            if (check(P, SYNC[i])) return;
        advance_tok(P);
    }
}

/* ─────────────────────────────────────────
 * DÜĞÜM OLUŞTURMA YARDIMCILARI
 * ───────────────────────────────────────── */
static Node *new_node(NodeType type, int line) {
    Node *n = calloc(1, sizeof(Node));
    if (!n) { fprintf(stderr, "Bellek hatası\n"); exit(1); }
    n->type = type;
    n->line = line;
    return n;
}

/* ─────────────────────────────────────────
 * ÖNE BILDIRGI (forward declarations)
 * ───────────────────────────────────────── */
static Node *parse_expr (Parser *P);
static Node *parse_stmt (Parser *P);
static Node *parse_block(Parser *P);

/* ─────────────────────────────────────────
 * İFADE KATMANLARI (öncelik düşükten yükseğe)
 * ───────────────────────────────────────── */

/* primary → literal | ident | '(' expr ')' */
static Node *parse_primary(Parser *P) {
    Token t = *cur(P);

    if (check(P, TOK_INT_LIT)) {
        advance_tok(P);
        Node *n = new_node(NODE_INT_LIT, t.line);
        n->as.int_lit.val = t.value.ival;
        return n;
    }
    if (check(P, TOK_FLOAT_LIT)) {
        advance_tok(P);
        Node *n = new_node(NODE_FLOAT_LIT, t.line);
        n->as.float_lit.val = t.value.fval;
        return n;
    }
    if (check(P, TOK_STRING_LIT)) {
        advance_tok(P);
        Node *n = new_node(NODE_STRING_LIT, t.line);
        strncpy(n->as.string_lit.val, t.lexeme, 255);
        return n;
    }
    if (check(P, TOK_IDENT)) {
        advance_tok(P);
        Node *n = new_node(NODE_IDENT, t.line);
        strncpy(n->as.ident.name, t.lexeme, 63);
        return n;
    }
    if (match_tok(P, TOK_LPAREN)) {
        Node *n = parse_expr(P);
        expect(P, TOK_RPAREN, "')' beklendi");
        return n;
    }

    /* YENİ literal türü → buraya if bloğu ekle */

    fprintf(stderr, "[%d:%d] Hata: Beklenmedik token '%s'\n",
            t.line, t.col, t.lexeme);
    P->had_error = 1;
    advance_tok(P);
    return new_node(NODE_INT_LIT, t.line); /* dummy */
}

/* call_expr → primary ('(' args ')')? */
static Node *parse_call(Parser *P) {
    Node *left = parse_primary(P);

    /* Fonksiyon çağrısı: ident( ... ) */
    if (left->type == NODE_IDENT && check(P, TOK_LPAREN)) {
        advance_tok(P); /* '(' */
        Node *call = new_node(NODE_CALL, left->line);
        strncpy(call->as.call.name, left->as.ident.name, 63);
        node_free(left);

        /* Argümanları topla */
        int   cap  = 8;
        Node **args = malloc(cap * sizeof(Node *));
        int   argc  = 0;

        if (!check(P, TOK_RPAREN)) {
            do {
                if (argc >= cap) {
                    cap *= 2;
                    args = realloc(args, cap * sizeof(Node *));
                }
                args[argc++] = parse_expr(P);
            } while (match_tok(P, TOK_COMMA));
        }
        expect(P, TOK_RPAREN, "')' beklendi");
        call->as.call.args      = args;
        call->as.call.arg_count = argc;
        return call;
    }
    return left;
}

/* unary → ('!' | '-') unary | call */
static Node *parse_unary(Parser *P) {
    if (check(P, TOK_NOT) || check(P, TOK_MINUS)) {
        Token op = advance_tok(P);
        Node *n  = new_node(NODE_UNOP, op.line);
        n->as.unop.op      = op.type;
        n->as.unop.operand = parse_unary(P);
        return n;
    }
    return parse_call(P);
}

/* İkili işlem katmanı oluşturucu (yardımcı makro yerine fonksiyon) */
static Node *parse_binop_layer(Parser *P,
                                Node *(*next)(Parser *),
                                const TokenType *ops, int op_count) {
    Node *left = next(P);
    while (1) {
        int matched = 0;
        for (int i = 0; i < op_count; i++) {
            if (check(P, ops[i])) {
                Token op = advance_tok(P);
                Node *n  = new_node(NODE_BINOP, op.line);
                n->as.binop.op    = op.type;
                n->as.binop.left  = left;
                n->as.binop.right = next(P);
                left = n;
                matched = 1;
                break;
            }
        }
        if (!matched) break;
    }
    return left;
}

/* mul_expr → unary (('*'|'/'|'%') unary)* */
static Node *parse_mul(Parser *P) {
    static const TokenType OPS[] = { TOK_STAR, TOK_SLASH, TOK_PERCENT };
    return parse_binop_layer(P, parse_unary, OPS, 3);
}

/* add_expr → mul (('+' | '-') mul)* */
static Node *parse_add(Parser *P) {
    static const TokenType OPS[] = { TOK_PLUS, TOK_MINUS };
    return parse_binop_layer(P, parse_mul, OPS, 2);
}

/* rel_expr → add (('<'|'>'|'<='|'>=') add)* */
static Node *parse_rel(Parser *P) {
    static const TokenType OPS[] = { TOK_LT, TOK_GT, TOK_LEQ, TOK_GEQ };
    return parse_binop_layer(P, parse_add, OPS, 4);
}

/* eq_expr → rel (('=='|'!=') rel)* */
static Node *parse_eq(Parser *P) {
    static const TokenType OPS[] = { TOK_EQ, TOK_NEQ };
    return parse_binop_layer(P, parse_rel, OPS, 2);
}

/* and_expr → eq ('&&' eq)* */
static Node *parse_and(Parser *P) {
    static const TokenType OPS[] = { TOK_AND };
    return parse_binop_layer(P, parse_eq, OPS, 1);
}

/* or_expr → and ('||' and)* */
static Node *parse_or(Parser *P) {
    static const TokenType OPS[] = { TOK_OR };
    return parse_binop_layer(P, parse_and, OPS, 1);
}

/* assign → IDENT '=' assign | or_expr */
static Node *parse_assign(Parser *P) {
    /* IDENT '=' assign  (sağ-ilişkili) */
    if (peek_tok(P, 0)->type == TOK_IDENT &&
        peek_tok(P, 1)->type == TOK_ASSIGN) {
        Token name = advance_tok(P); /* IDENT */
        advance_tok(P);              /* '='   */
        Node *n = new_node(NODE_ASSIGN, name.line);
        strncpy(n->as.assign.name, name.lexeme, 63);
        n->as.assign.value = parse_assign(P);
        return n;
    }
    return parse_or(P);
}

/* expr → assign */
static Node *parse_expr(Parser *P) {
    return parse_assign(P);
}

/* ─────────────────────────────────────────
 * DEYİM PARSE FONKSİYONLARI
 *   Yeni deyim → parse_yeni() + parse_stmt()'e case ekle
 * ───────────────────────────────────────── */

/* block → '{' stmt* '}' */
static Node *parse_block(Parser *P) {
    int line = cur(P)->line;
    expect(P, TOK_LBRACE, "'{' beklendi");

    int    cap   = 16;
    Node **stmts = malloc(cap * sizeof(Node *));
    int    count = 0;

    while (!check(P, TOK_RBRACE) && !check(P, TOK_EOF)) {
        if (count >= cap) {
            cap *= 2;
            stmts = realloc(stmts, cap * sizeof(Node *));
        }
        stmts[count++] = parse_stmt(P);
    }
    expect(P, TOK_RBRACE, "'}' beklendi");

    Node *n = new_node(NODE_BLOCK, line);
    n->as.block.stmts = stmts;
    n->as.block.count = count;
    return n;
}

/* if_stmt → 'if' '(' expr ')' stmt ('else' stmt)? */
static Node *parse_if(Parser *P) {
    int line = cur(P)->line;
    advance_tok(P); /* 'if' */
    expect(P, TOK_LPAREN, "'(' beklendi");
    Node *cond = parse_expr(P);
    expect(P, TOK_RPAREN, "')' beklendi");

    Node *n = new_node(NODE_IF, line);
    n->as.if_stmt.cond        = cond;
    n->as.if_stmt.then_branch = parse_stmt(P);
    n->as.if_stmt.else_branch = NULL;

    if (match_tok(P, TOK_ELSE))
        n->as.if_stmt.else_branch = parse_stmt(P);
    return n;
}

/* while_stmt → 'while' '(' expr ')' stmt */
static Node *parse_while(Parser *P) {
    int line = cur(P)->line;
    advance_tok(P); /* 'while' */
    expect(P, TOK_LPAREN, "'(' beklendi");
    Node *cond = parse_expr(P);
    expect(P, TOK_RPAREN, "')' beklendi");

    Node *n = new_node(NODE_WHILE, line);
    n->as.while_stmt.cond = cond;
    n->as.while_stmt.body = parse_stmt(P);
    return n;
}

/* for_stmt → 'for' '(' [expr] ';' [expr] ';' [expr] ')' stmt */
static Node *parse_for(Parser *P) {
    int line = cur(P)->line;
    advance_tok(P); /* 'for' */
    expect(P, TOK_LPAREN, "'(' beklendi");

    Node *init = check(P, TOK_SEMICOLON) ? NULL : parse_expr(P);
    expect(P, TOK_SEMICOLON, "';' beklendi (for init)");
    Node *cond = check(P, TOK_SEMICOLON) ? NULL : parse_expr(P);
    expect(P, TOK_SEMICOLON, "';' beklendi (for cond)");
    Node *step = check(P, TOK_RPAREN)    ? NULL : parse_expr(P);
    expect(P, TOK_RPAREN, "')' beklendi");

    Node *n = new_node(NODE_FOR, line);
    n->as.for_stmt.init = init;
    n->as.for_stmt.cond = cond;
    n->as.for_stmt.step = step;
    n->as.for_stmt.body = parse_stmt(P);
    return n;
}

/* return_stmt → 'return' [expr] ';' */
static Node *parse_return(Parser *P) {
    int line = cur(P)->line;
    advance_tok(P); /* 'return' */
    Node *n = new_node(NODE_RETURN, line);
    n->as.ret.value = check(P, TOK_SEMICOLON) ? NULL : parse_expr(P);
    expect(P, TOK_SEMICOLON, "';' beklendi");
    return n;
}

/* var_decl → type IDENT ('=' expr)? ';' */
static Node *parse_var_decl(Parser *P) {
    int       line = cur(P)->line;
    TokenType vt   = advance_tok(P).type; /* int/float/void */
    Token     name = expect(P, TOK_IDENT, "değişken ismi beklendi");

    Node *n = new_node(NODE_VAR_DECL, line);
    n->as.var_decl.var_type = vt;
    strncpy(n->as.var_decl.name, name.lexeme, 63);
    n->as.var_decl.init = match_tok(P, TOK_ASSIGN) ? parse_expr(P) : NULL;
    expect(P, TOK_SEMICOLON, "';' beklendi");
    return n;
}

/* Tip token'ı mı? — yeni tip eklenince buraya da ekle */
static int is_type(Parser *P) {
    TokenType t = cur(P)->type;
    return t == TOK_INT || t == TOK_FLOAT || t == TOK_VOID;
}

/* func_decl → type IDENT '(' params ')' block
 * params    → (type IDENT (',' type IDENT)*)?             */
static Node *parse_func_decl(Parser *P) {
    int       line = cur(P)->line;
    TokenType rt   = advance_tok(P).type;
    Token     name = expect(P, TOK_IDENT, "fonksiyon ismi beklendi");
    expect(P, TOK_LPAREN, "'(' beklendi");

    int        pcap   = 8;
    char     (*pnames)[64] = malloc(pcap * sizeof(*pnames));
    TokenType *ptypes = malloc(pcap * sizeof(TokenType));
    int        pcount = 0;

    if (!check(P, TOK_RPAREN)) {
        do {
            if (pcount >= pcap) {
                pcap *= 2;
                pnames = realloc(pnames, pcap * sizeof(*pnames));
                ptypes = realloc(ptypes, pcap * sizeof(TokenType));
            }
            if (!is_type(P)) {
                fprintf(stderr, "[%d:%d] Hata: Parametre tipi beklendi\n",
                        cur(P)->line, cur(P)->col);
                P->had_error = 1; break;
            }
            ptypes[pcount] = advance_tok(P).type;
            Token pn = expect(P, TOK_IDENT, "parametre ismi beklendi");
            strncpy(pnames[pcount], pn.lexeme, 63);
            pcount++;
        } while (match_tok(P, TOK_COMMA));
    }
    expect(P, TOK_RPAREN, "')' beklendi");

    Node *n = new_node(NODE_FUNC_DECL, line);
    n->as.func_decl.ret_type    = rt;
    strncpy(n->as.func_decl.name, name.lexeme, 63);
    n->as.func_decl.params      = pnames;
    n->as.func_decl.param_types = ptypes;
    n->as.func_decl.param_count = pcount;
    n->as.func_decl.body        = parse_block(P);
    return n;
}

/* stmt — giriş noktası
 * YENİ DEYİM → buraya case + parse_yeni() ekle   */
static Node *parse_stmt(Parser *P) {
    if (P->had_error) { synchronize(P); }

    switch (cur(P)->type) {
        case TOK_IF:     return parse_if(P);
        case TOK_WHILE:  return parse_while(P);
        case TOK_FOR:    return parse_for(P);
        case TOK_RETURN: return parse_return(P);
        case TOK_LBRACE: return parse_block(P);
        /* YENİ: case TOK_MATCH: return parse_match(P); */
        default: break;
    }

    /* Değişken bildirimi */
    if (is_type(P) && peek_tok(P, 1)->type == TOK_IDENT) {
        /* Sonraki 2. token '(' ise fonksiyon bildirimi */
        if (peek_tok(P, 2)->type == TOK_LPAREN)
            return parse_func_decl(P);
        return parse_var_decl(P);
    }

    /* İfade deyimi: expr ';' */
    int  line = cur(P)->line;
    Node *e   = parse_expr(P);
    expect(P, TOK_SEMICOLON, "';' beklendi");
    Node *n   = new_node(NODE_EXPR_STMT, line);
    n->as.expr_stmt.expr = e;
    return n;
}

/* ─────────────────────────────────────────
 * PUBLIC API
 * ───────────────────────────────────────── */
void parser_init(Parser *P, Token *tokens, int count) {
    P->tokens    = tokens;
    P->count     = count;
    P->pos       = 0;
    P->had_error = 0;
}

Node *parse_program(Parser *P) {
    int    line  = cur(P)->line;
    int    cap   = 16;
    Node **decls = malloc(cap * sizeof(Node *));
    int    count = 0;

    while (!check(P, TOK_EOF)) {
        if (count >= cap) { cap *= 2; decls = realloc(decls, cap * sizeof(Node*)); }
        decls[count++] = parse_stmt(P);
        if (P->had_error) { synchronize(P); P->had_error = 0; }
    }

    Node *root = new_node(NODE_PROGRAM, line);
    root->as.program.decls = decls;
    root->as.program.count = count;
    return root;
}

/* ─────────────────────────────────────────
 * BELLEK SERBEST BIRAKMA
 * ───────────────────────────────────────── */
void node_free(Node *n) {
    if (!n) return;
    switch (n->type) {
        case NODE_BINOP:
            node_free(n->as.binop.left);
            node_free(n->as.binop.right); break;
        case NODE_UNOP:
            node_free(n->as.unop.operand); break;
        case NODE_ASSIGN:
            node_free(n->as.assign.value); break;
        case NODE_CALL:
            for (int i = 0; i < n->as.call.arg_count; i++)
                node_free(n->as.call.args[i]);
            free(n->as.call.args); break;
        case NODE_VAR_DECL:
            node_free(n->as.var_decl.init); break;
        case NODE_EXPR_STMT:
            node_free(n->as.expr_stmt.expr); break;
        case NODE_BLOCK:
            for (int i = 0; i < n->as.block.count; i++)
                node_free(n->as.block.stmts[i]);
            free(n->as.block.stmts); break;
        case NODE_IF:
            node_free(n->as.if_stmt.cond);
            node_free(n->as.if_stmt.then_branch);
            node_free(n->as.if_stmt.else_branch); break;
        case NODE_WHILE:
            node_free(n->as.while_stmt.cond);
            node_free(n->as.while_stmt.body); break;
        case NODE_FOR:
            node_free(n->as.for_stmt.init);
            node_free(n->as.for_stmt.cond);
            node_free(n->as.for_stmt.step);
            node_free(n->as.for_stmt.body); break;
        case NODE_RETURN:
            node_free(n->as.ret.value); break;
        case NODE_FUNC_DECL:
            free(n->as.func_decl.params);
            free(n->as.func_decl.param_types);
            node_free(n->as.func_decl.body); break;
        case NODE_PROGRAM:
            for (int i = 0; i < n->as.program.count; i++)
                node_free(n->as.program.decls[i]);
            free(n->as.program.decls); break;
        default: break;
    }
    free(n);
}

/* ─────────────────────────────────────────
 * AST YAZICI (debug)
 * ───────────────────────────────────────── */
static void indent_print(int depth) {
    for (int i = 0; i < depth; i++) printf("  ");
}

void node_print(const Node *n, int depth) {
    if (!n) { indent_print(depth); printf("(null)\n"); return; }
    indent_print(depth);
    switch (n->type) {
        case NODE_INT_LIT:
            printf("IntLit(%ld)\n", n->as.int_lit.val); break;
        case NODE_FLOAT_LIT:
            printf("FloatLit(%g)\n", n->as.float_lit.val); break;
        case NODE_STRING_LIT:
            printf("StringLit(\"%s\")\n", n->as.string_lit.val); break;
        case NODE_IDENT:
            printf("Ident(%s)\n", n->as.ident.name); break;
        case NODE_BINOP:
            printf("BinOp(%s)\n", TOKEN_NAMES[n->as.binop.op]);
            node_print(n->as.binop.left,  depth+1);
            node_print(n->as.binop.right, depth+1); break;
        case NODE_UNOP:
            printf("UnOp(%s)\n", TOKEN_NAMES[n->as.unop.op]);
            node_print(n->as.unop.operand, depth+1); break;
        case NODE_ASSIGN:
            printf("Assign(%s)\n", n->as.assign.name);
            node_print(n->as.assign.value, depth+1); break;
        case NODE_CALL:
            printf("Call(%s, %d args)\n",
                   n->as.call.name, n->as.call.arg_count);
            for (int i = 0; i < n->as.call.arg_count; i++)
                node_print(n->as.call.args[i], depth+1);
            break;
        case NODE_VAR_DECL:
            printf("VarDecl(%s %s)\n",
                   TOKEN_NAMES[n->as.var_decl.var_type],
                   n->as.var_decl.name);
            if (n->as.var_decl.init)
                node_print(n->as.var_decl.init, depth+1);
            break;
        case NODE_EXPR_STMT:
            printf("ExprStmt\n");
            node_print(n->as.expr_stmt.expr, depth+1);
            break;
        case NODE_BLOCK:
            printf("Block(%d)\n", n->as.block.count);
            for (int i = 0; i < n->as.block.count; i++)
                node_print(n->as.block.stmts[i], depth+1);
            break;
        case NODE_IF:
            printf("If\n");
            indent_print(depth+1); printf("cond:\n");
            node_print(n->as.if_stmt.cond, depth+2);
            indent_print(depth+1); printf("then:\n");
            node_print(n->as.if_stmt.then_branch, depth+2);
            if (n->as.if_stmt.else_branch) {
                indent_print(depth+1); printf("else:\n");
                node_print(n->as.if_stmt.else_branch, depth+2);
            } break;
        case NODE_WHILE:
            printf("While\n");
            indent_print(depth+1); printf("cond:\n");
            node_print(n->as.while_stmt.cond, depth+2);
            indent_print(depth+1); printf("body:\n");
            node_print(n->as.while_stmt.body, depth+2); break;
        case NODE_FOR:
            printf("For\n");
            indent_print(depth+1); printf("init:\n"); node_print(n->as.for_stmt.init, depth+2);
            indent_print(depth+1); printf("cond:\n"); node_print(n->as.for_stmt.cond, depth+2);
            indent_print(depth+1); printf("step:\n"); node_print(n->as.for_stmt.step, depth+2);
            indent_print(depth+1); printf("body:\n"); node_print(n->as.for_stmt.body, depth+2); break;
        case NODE_RETURN:
            printf("Return\n");
            if (n->as.ret.value)
                node_print(n->as.ret.value, depth+1);
            break;
        case NODE_FUNC_DECL:
            printf("FuncDecl(%s %s, %d params)\n",
                   TOKEN_NAMES[n->as.func_decl.ret_type],
                   n->as.func_decl.name,
                   n->as.func_decl.param_count);
            node_print(n->as.func_decl.body, depth+1);
            break;
        case NODE_PROGRAM:
            printf("Program(%d)\n", n->as.program.count);
            for (int i = 0; i < n->as.program.count; i++)
                node_print(n->as.program.decls[i], depth+1);
            break;
        default:
            printf("Node(%d)\n", n->type); break;
    }
}
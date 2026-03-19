# MFZ Lang — Geliştirme Rehberi

Bu belge MFZ diline yeni özellik eklemenin adım adım yolunu gösterir. Her bölüm "nereye dokunacaksın" sorusunu somut kod örnekleriyle yanıtlar.

---

## İçindekiler

- [Genel Mimari](#genel-mimari)
- [Yeni Anahtar Kelime Eklemek](#1-yeni-anahtar-kelime-eklemek)
- [Yeni Operatör Eklemek](#2-yeni-operatör-eklemek)
- [Yeni İfade Türü Eklemek](#3-yeni-i̇fade-türü-eklemek)
- [Yeni Deyim Türü Eklemek](#4-yeni-deyim-türü-eklemek)
- [Yeni Built-in Fonksiyon Eklemek](#5-yeni-built-in-fonksiyon-eklemek)
- [Yeni Değer Tipi Eklemek](#6-yeni-değer-tipi-eklemek)
- [Hata Ayıklama](#hata-ayıklama)
- [Değişiklik Kontrol Listesi](#değişiklik-kontrol-listesi)

---

## Genel Mimari

Her yeni özellik genellikle 3 katmana aynı anda dokunur:

```
lexer.h / lexer.c       → "Bu kelime/sembol geçerli mi?"
parser.h / parser.c     → "Bu token dizisi anlamlı mı? → AST düğümü"
interpreter.c           → "Bu AST düğümü ne anlama geliyor? → değer"
```

Kural: **Her katmana sırasıyla dokunun.** Lexer'da tanınmayan bir şey parser'a ulaşamaz; parser'da tanınmayan bir şey interpreter'a ulaşamaz.

---

## 1. Yeni Anahtar Kelime Eklemek

**Örnek:** `break` deyimi eklemek.

### Adım 1 — `lexer.h`: TokenType enum

```c
typedef enum {
    // ... mevcut tokenlar ...
    TOK_BREAK,    // ← ekle
    // ...
} TokenType;
```

### Adım 2 — `lexer.h`: TOKEN_NAMES dizisi

```c
static const char *TOKEN_NAMES[TOK_COUNT] = {
    // ... mevcut isimler (enum ile AYNI sırada) ...
    "BREAK",      // ← ekle
    // ...
};
```

### Adım 3 — `lexer.c`: KEYWORDS tablosu

```c
static const Keyword KEYWORDS[] = {
    // ... mevcut kelimeler ...
    { "break", TOK_BREAK },   // ← ekle
    { NULL, TOK_ERROR }       // sentinel — her zaman sonda
};
```

Bu üç adım yeterli. Lexer artık `break` kelimesini `TOK_BREAK` token'ı olarak üretir.

---

## 2. Yeni Operatör Eklemek

### Tek karakter operatör — örnek: `@`

`lexer.c` → `match_op()` → `SINGLE[]` tablosu:

```c
static const SingleOp SINGLE[] = {
    // ... mevcut operatörler ...
    { '@', TOK_AT },   // ← ekle
    { 0, TOK_ERROR }
};
```

`lexer.h`'a da `TOK_AT` ekle (yukarıdaki gibi).

### Çift karakter operatör — örnek: `**` (üs alma)

`lexer.c` → `match_op()` → `DUAL[]` tablosu:

```c
static const DualOp DUAL[] = {
    // ... mevcut operatörler ...
    { '*', '*', TOK_STARSTAR },   // ← ekle
    { 0, 0, TOK_ERROR }
};
```

> **Önemli:** `**` için `DUAL`'ı `SINGLE`'dan önce kontrol edin — zaten öyle, dokunmaya gerek yok.

### Operatörü interpreter'a tanıtmak

`interpreter.c` → `eval_binop()`:

```c
case TOK_STARSTAR: {
    double base = to_float(L), exp = to_float(R);
    return val_float(pow(base, exp));
}
```

---

## 3. Yeni İfade Türü Eklemek

**Örnek:** Dizi indeksleme `x[i]`

### Adım 1 — `parser.h`: NodeType

```c
typedef enum {
    // ...
    NODE_INDEX,     // x[i]
    // ...
} NodeType;
```

### Adım 2 — `parser.h`: Node union

```c
struct Node {
    // ...
    union {
        // ...
        struct {
            Node *object;
            Node *index;
        } index_expr;   // ← ekle
        // ...
    } as;
};
```

### Adım 3 — `parser.c`: `parse_call()` içine ekle

```c
static Node *parse_call(Parser *P) {
    Node *left = parse_primary(P);

    while (1) {
        if (left->type == NODE_IDENT && check(P, TOK_LPAREN)) {
            // ... mevcut fonksiyon çağrısı kodu ...
        }
        else if (check(P, TOK_LBRACKET)) {
            // Dizi indeksleme
            advance_tok(P);  // '['
            Node *n = new_node(NODE_INDEX, left->line);
            n->as.index_expr.object = left;
            n->as.index_expr.index  = parse_expr(P);
            expect(P, TOK_RBRACKET, "']' beklendi");
            left = n;
        }
        else break;
    }
    return left;
}
```

### Adım 4 — `interpreter.c`: `interp_eval()`

```c
case NODE_INDEX: {
    Value obj = interp_eval(I, expr->as.index_expr.object, env);
    Value idx = interp_eval(I, expr->as.index_expr.index,  env);
    // kendi liste/dizi yapına göre uygula
    break;
}
```

---

## 4. Yeni Deyim Türü Eklemek

**Örnek:** `break` deyimi (döngüden çıkmak)

> Önce [Bölüm 1](#1-yeni-anahtar-kelime-eklemek)'deki gibi `TOK_BREAK` eklenmiş olmalı.

### Adım 1 — `parser.h`: NodeType

```c
NODE_BREAK,
```

### Adım 2 — `parser.c`: `parse_break()` fonksiyonu

```c
static Node *parse_break(Parser *P) {
    int line = cur(P)->line;
    advance_tok(P);  // 'break'
    expect(P, TOK_SEMICOLON, "';' beklendi");
    return new_node(NODE_BREAK, line);
}
```

### Adım 3 — `parser.c`: `parse_stmt()` içine dal ekle

```c
static Node *parse_stmt(Parser *P) {
    switch (cur(P)->type) {
        case TOK_IF:     return parse_if(P);
        case TOK_WHILE:  return parse_while(P);
        case TOK_BREAK:  return parse_break(P);   // ← ekle
        // ...
    }
}
```

### Adım 4 — `interpreter.h`: Sinyal mekanizmasına ekle

`break` için `ReturnSignal` benzeri bir `BreakSignal` veya mevcut sinyale `is_break` bayrağı eklenebilir:

```c
typedef struct {
    int   active;
    int   is_break;   // ← ekle
    Value value;
} ReturnSignal;
```

### Adım 5 — `interpreter.c`: `interp_exec()`

```c
case NODE_BREAK:
    if (g_ret) { g_ret->active = 1; g_ret->is_break = 1; }
    return val_void();
```

Ve `while` / `for` döngülerinde:

```c
if (ret.active) {
    if (!ret.is_break && g_ret) *g_ret = ret;  // return'ü yukarı taşı
    break;  // her durumda döngüden çık
}
```

---

## 5. Yeni Built-in Fonksiyon Eklemek

En kolay genişletme noktası. Sadece `interpreter.c`'ye dokunman yeterli.

**Örnek:** `max(a, b)` fonksiyonu

### Adım 1 — Fonksiyonu yaz

```c
static Value builtin_max(Interpreter *I, Value *args, int argc, int line) {
    if (argc != 2)
        return runtime_error(I, line, "max() 2 argüman ister");
    if (!is_numeric(args[0]) || !is_numeric(args[1]))
        return runtime_error(I, line, "max() sayısal değer ister");

    double a = to_float(args[0]), b = to_float(args[1]);
    if (args[0].type == VAL_INT && args[1].type == VAL_INT)
        return val_int(args[0].as.ival > args[1].as.ival
                       ? args[0].as.ival : args[1].as.ival);
    return val_float(a > b ? a : b);
}
```

### Adım 2 — Tabloya ekle

```c
static const Builtin BUILTINS[] = {
    { "print",   builtin_print   },
    { "println", builtin_println },
    // ...
    { "max",     builtin_max     },   // ← ekle
    { NULL, NULL }
};
```

Bitti. `.mfz` dosyalarında `max(3, 7)` kullanılabilir.

---

## 6. Yeni Değer Tipi Eklemek

**Örnek:** `bool` tipi

Bu en kapsamlı değişikliktir — her katmana dokunur.

### `interpreter.h`

```c
typedef enum {
    VAL_INT, VAL_FLOAT, VAL_STRING, VAL_VOID,
    VAL_BOOL,   // ← ekle
} ValueType;

// Oluşturucu:
static inline Value val_bool(int v) {
    Value r; r.type = VAL_BOOL; r.as.ival = v ? 1 : 0; return r;
}
```

### `lexer.h` / `lexer.c`

```c
TOK_BOOL,         // tip token'ı
TOK_TRUE,         // literal
TOK_FALSE,

// KEYWORDS:
{ "bool",  TOK_BOOL  },
{ "true",  TOK_TRUE  },
{ "false", TOK_FALSE },
```

### `parser.c`

`parse_primary()`'a ekle:

```c
if (check(P, TOK_TRUE))  { advance_tok(P); Node *n = new_node(NODE_INT_LIT, t.line); n->as.int_lit.val = 1; return n; }
if (check(P, TOK_FALSE)) { advance_tok(P); Node *n = new_node(NODE_INT_LIT, t.line); n->as.int_lit.val = 0; return n; }
```

### `interpreter.c`

`value_print()` ve `is_truthy()` güncelle, tip kontrollerini ekle.

---

## Hata Ayıklama

### Token çıktısı

```bash
./mfz/mfz program.mfz --tokens
```

Lexer'ın bir kelimeyi tanıyıp tanımadığını kontrol etmek için kullan.

### AST çıktısı

```bash
./mfz/mfz program.mfz --ast
```

Parser'ın ağacı doğru kurduğunu doğrula. Yanlış öncelik veya eksik düğüm burada görünür.

### Yaygın hatalar

| Belirti | Olası sebep |
|---|---|
| `Bilinmeyen karakter` | Lexer o sembolü tanımıyor |
| `Beklenmedik token` | Parser o token'ı beklemiyor |
| `Tanımsız değişken` | Kapsam sorunu veya yazım hatası |
| `Geçersiz ikili işlem` | Tip uyumsuzluğu (`eval_binop`) |
| `Segfault` | AST düğümü `NULL` — parser'da eksik hata kontrolü |

---

## Değişiklik Kontrol Listesi

Yeni bir özellik eklerken bu listeyi takip et:

- [ ] `lexer.h` — `TokenType` enum'a sabit eklendi
- [ ] `lexer.h` — `TOKEN_NAMES` dizisi güncellendi (aynı sıra!)
- [ ] `lexer.c` — `KEYWORDS[]` veya `DUAL[]` / `SINGLE[]` güncellendi
- [ ] `parser.h` — `NodeType` güncellendi (gerekiyorsa)
- [ ] `parser.h` — `Node` union'ı güncellendi (gerekiyorsa)
- [ ] `parser.c` — `parse_*()` fonksiyonu yazıldı
- [ ] `parser.c` — `parse_stmt()` veya ilgili fonksiyona dal eklendi
- [ ] `interpreter.c` — `interp_eval()` veya `interp_exec()`'e case eklendi
- [ ] `interpreter.c` — `node_free()` güncellendi (yeni düğüm için)
- [ ] `parser.c` — `node_print()` güncellendi (debug için)
- [ ] Derleme: `gcc -Wall -Wextra` ile sıfır uyarı
- [ ] Test: `.mfz` dosyasıyla yeni özellik doğrulandı
- [ ] `README.md` güncellendi

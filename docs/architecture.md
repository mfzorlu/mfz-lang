# MFZ — Mimari Belgesi

Bu belge MFZ interpreter'ının iç yapısını ve bileşenler arası veri akışını açıklar.

---

## Genel Akış

```
.mfz dosyası
     │  (ham metin)
     ▼
┌─────────┐
│  Lexer  │  lexer.h / lexer.c
└─────────┘
     │  Token[]
     ▼
┌──────────┐
│  Parser  │  parser.h / parser.c
└──────────┘
     │  Node* (AST)
     ▼
┌─────────────┐
│ Interpreter │  interpreter.h / interpreter.c
└─────────────┘
     │  Çıktı / Yan etkiler
     ▼
  stdout
```

---

## Lexer

**Dosyalar:** `lexer.h`, `lexer.c`

### Sorumluluk

Ham kaynak metni `Token` dizisine dönüştürür. Boşlukları ve yorumları atar.

### Veri Yapıları

```c
typedef struct {
    TokenType type;    // TOK_INT_LIT, TOK_IDENT, ...
    char      lexeme[256];
    int       line, col;
    union { long ival; double fval; } value;
} Token;

typedef struct {
    const char *src;
    int pos, line, col;
} Lexer;
```

### Genişletme Noktaları

| Konum | Açıklama |
|---|---|
| `TokenType` enum | Yeni token sabiti |
| `TOKEN_NAMES[]` | Debug ismi |
| `KEYWORDS[]` | Yeni anahtar kelime |
| `DUAL[]` | Çift karakter operatör |
| `SINGLE[]` | Tek karakter operatör |
| `next_token()` | Yeni literal türü için dal |

---

## Parser

**Dosyalar:** `parser.h`, `parser.c`

### Sorumluluk

Token dizisini özyinelemeli iniş (recursive descent) yöntemiyle işleyerek AST üretir. Her gramer kuralı bir fonksiyona karşılık gelir.

### AST Düğümü

```c
typedef struct Node Node;
struct Node {
    NodeType type;
    int      line;
    union {
        // INT_LIT, FLOAT_LIT, STRING_LIT, IDENT,
        // BINOP, UNOP, ASSIGN, CALL,
        // VAR_DECL, EXPR_STMT, BLOCK,
        // IF, WHILE, FOR, RETURN,
        // FUNC_DECL, PROGRAM
    } as;
};
```

### Fonksiyon Hiyerarşisi

```
parse_program
└── parse_stmt
    ├── parse_if
    ├── parse_while
    ├── parse_for
    ├── parse_return
    ├── parse_block
    ├── parse_func_decl
    ├── parse_var_decl
    └── parse_expr
        └── parse_assign
            └── parse_or
                └── parse_and
                    └── parse_eq
                        └── parse_rel
                            └── parse_add
                                └── parse_mul
                                    └── parse_unary
                                        └── parse_call
                                            └── parse_primary
```

### Hata Kurtarma

`synchronize()` fonksiyonu ile panik mod: hata sonrası `;`, `}`, `if`, `while` gibi bilinen senkronizasyon noktalarına atlar.

---

## Interpreter

**Dosyalar:** `interpreter.h`, `interpreter.c`

### Sorumluluk

AST'yi doğrudan ziyaret ederek (tree-walk) çalıştırır. Derleme aşaması yoktur.

### Değer Temsili

```c
typedef struct {
    ValueType type;   // VAL_INT, VAL_FLOAT, VAL_STRING, VAL_VOID
    union {
        long   ival;
        double fval;
        char   sval[256];
    } as;
} Value;
```

### Kapsam Zinciri (Ortam)

```
global env
    │
    └── fonksiyon env
            │
            └── blok env
                    │
                    └── iç blok env
```

`env_get()` zinciri yukarı doğru tarar. `env_set()` mevcut kaydı bulursa günceller, bulamazsa yeni kayıt ekler.

### Return Mekanizması

Global `g_ret` işaretçisi aktif `ReturnSignal`'ı taşır. `return` deyimi bunu tetikler; `exec_block()` her adımda kontrol eder.

Özyinelemeli çağrılarda `eval_call()`, `g_ret`'i kaydedip yeni bir sinyal kurar, döndükten sonra eski değeri geri yükler:

```c
ReturnSignal  ret      = {0, val_void()};
ReturnSignal *prev_ret = g_ret;
g_ret = &ret;
exec_block(...);
g_ret = prev_ret;   // çağıranın sinyalini koru
```

### Built-in Fonksiyonlar

`BUILTINS[]` tablosu isim → fonksiyon işaretçisi eşlemesi tutar. `eval_call()` önce bu tabloyu arar; bulamazsa kullanıcı fonksiyonu tablosuna bakar.

---

## Bellek Yönetimi

| Nesne | Sahip | Serbest Bırakma |
|---|---|---|
| `Token[]` | `main.c` | `free(tokens)` |
| `char *src` | `main.c` | `free(src)` |
| `Node*` (AST) | `main.c` | `node_free(ast)` |
| `Env*` | Her blok/fonksiyon | `env_free(env)` |
| `Node.as.call.args` | `node_free()` | `free(args)` |
| `Node.as.func_decl.params` | `node_free()` | `free(params)` |

AST düğümlerinin sahipliği her zaman üst düğümdedir; `node_free()` özyinelemeli olarak tüm alt ağacı serbest bırakır.

# MFZ Dil Spesifikasyonu

Bu belge MFZ dilinin resmi gramer ve davranış tanımıdır.

---

## Gramer (BNF)

```
program    → decl* EOF

decl       → func_decl
           | var_decl
           | stmt

func_decl  → type IDENT '(' params ')' block
params     → ( type IDENT ( ',' type IDENT )* )?

var_decl   → type IDENT ( '=' expr )? ';'

stmt       → if_stmt
           | while_stmt
           | for_stmt
           | return_stmt
           | block
           | expr_stmt

if_stmt    → 'if' '(' expr ')' stmt ( 'else' stmt )?
while_stmt → 'while' '(' expr ')' stmt
for_stmt   → 'for' '(' expr? ';' expr? ';' expr? ')' stmt
return_stmt→ 'return' expr? ';'
block      → '{' stmt* '}'
expr_stmt  → expr ';'

expr       → assign
assign     → IDENT '=' assign | or_expr
or_expr    → and_expr  ( '||' and_expr  )*
and_expr   → eq_expr   ( '&&' eq_expr   )*
eq_expr    → rel_expr  ( ('=='|'!=') rel_expr )*
rel_expr   → add_expr  ( ('<'|'>'|'<='|'>=') add_expr )*
add_expr   → mul_expr  ( ('+'|'-') mul_expr )*
mul_expr   → unary     ( ('*'|'/'|'%') unary )*
unary      → ( '!' | '-' ) unary | call_expr
call_expr  → primary ( '(' args ')' )?
args       → ( expr ( ',' expr )* )?
primary    → INT_LIT | FLOAT_LIT | STRING_LIT | IDENT | '(' expr ')'

type       → 'int' | 'float' | 'void'
```

---

## Token Tanımları

### Literaller

| Token        | Desen                          | Örnek        |
|--------------|-------------------------------|--------------|
| `INT_LIT`    | `[0-9]+`                       | `42`, `0`    |
| `FLOAT_LIT`  | `[0-9]+ '.' [0-9]+`           | `3.14`       |
| `STRING_LIT` | `'"' karakter* '"'`           | `"merhaba"`  |
| `IDENT`      | `[a-zA-Z_][a-zA-Z0-9_]*`     | `x`, `toplam`|

### Anahtar Kelimeler

`if` `else` `while` `for` `return` `int` `float` `void`

### Operatörler

`+` `-` `*` `/` `%` `==` `!=` `<` `>` `<=` `>=` `=` `&&` `||` `!`

### Noktalama

`(` `)` `{` `}` `[` `]` `;` `:` `,` `.` `->`

---

## Operatör Önceliği

Düşükten yükseğe:

| Seviye | Operatör(ler)            | İlişkilendirme |
|--------|--------------------------|----------------|
| 1      | `=`                      | Sağ            |
| 2      | `\|\|`                   | Sol            |
| 3      | `&&`                     | Sol            |
| 4      | `==` `!=`                | Sol            |
| 5      | `<` `>` `<=` `>=`        | Sol            |
| 6      | `+` `-`                  | Sol            |
| 7      | `*` `/` `%`              | Sol            |
| 8      | `!` `-` (tekli)          | Sağ            |
| 9      | `()` (fonksiyon çağrısı) | Sol            |

---

## Tip Sistemi

MFZ zayıf tipli ve dinamik değerlendirme yapan statik bildirimli bir dildir.

### Otomatik Dönüşümler

| İşlem             | Sonuç   |
|-------------------|---------|
| `int OP int`      | `int`   |
| `int OP float`    | `float` |
| `float OP float`  | `float` |
| `string + string` | `string`|

### Doğruluk (Truthiness)

`is_truthy()` kuralları:

- `int`: `0` → yanlış, diğer → doğru
- `float`: `0.0` → yanlış, diğer → doğru
- `string`: `""` → yanlış, diğer → doğru
- `void`: her zaman yanlış

---

## Kapsam Kuralları

- Her `{...}` bloğu yeni bir kapsam oluşturur.
- Değişken arama: önce mevcut kapsam, sonra üst kapsamlar.
- Fonksiyonlar kendi izole kapsamında çalışır — global değişkenlere doğrudan erişemez.
- Fonksiyon parametreleri fonksiyonun yerel kapsamına bağlanır.

```mfz
int x = 10;

int f() {
    int x = 20;   // Yeni yerel x — dıştaki x'i gölgeler
    return x;     // 20
}

println(f());  // 20
println(x);    // 10 — dıştaki x değişmedi
```

---

## Yerleşik Fonksiyonlar

### `print(x)`
Değeri yeni satır eklemeden yazdırır.

### `println(x)`
Değeri yeni satırla yazdırır.

### `sqrt(x)`
`x`'in karekökünü `float` olarak döndürür.

```mfz
float s = sqrt(16);  // 4.0
```

### `abs(x)`
Mutlak değer. `int` için `int`, `float` için `float` döner.

### `int(x)`
`float`'ı `int`'e dönüştürür (tamsayıya yuvarlar, kesmez demek daha doğru — ondalık kısmı atar).

```mfz
int n = int(3.99);  // 3
```

### `float(x)`
`int`'i `float`'a dönüştürür.

---

## Yorumlar

```mfz
// Tek satır yorum — satır sonuna kadar

/* Çok satır yorum
   birden fazla satıra yayılabilir */
```

---

## Hata Türleri

| Aşama       | Hata Türü             | Örnek                        |
|-------------|----------------------|------------------------------|
| Lexer       | Bilinmeyen karakter  | `#`, `$` gibi semboller      |
| Lexer       | Bitmemiş string      | `"kapanmamış`                |
| Parser      | Beklenmedik token    | `int = 5;` (isim eksik)      |
| Parser      | Eksik `;`            | `int x = 5`                  |
| Interpreter | Tanımsız değişken    | Bildirilmeden kullanılan isim|
| Interpreter | Tanımsız fonksiyon   | Bildirilmeden çağrılan isim  |
| Interpreter | Sıfıra bölme         | `x / 0`                      |
| Interpreter | Tip uyumsuzluğu      | String'e aritmetik uygulama  |

# MFZ Lang

MFZ, C ile yazılmış sıfırdan geliştirilmiş bir programlama dili ve interpreter'ıdır. Lexer, parser ve tree-walk interpreter'dan oluşan modüler bir yapıya sahiptir. `.mfz` uzantılı dosyaları çalıştırır.

> Geliştirme sürecinde [Claude Sonnet 4.6](https://www.anthropic.com/claude) ile birlikte çalışılmıştır.

---

## İçindekiler

- [Kurulum](#kurulum)
- [Kullanım](#kullanım)
- [Dil Referansı](#dil-referansı)
- [Proje Yapısı](#proje-yapısı)
- [Geliştirme](#geliştirme)

---

## Kurulum

**Gereksinimler:** GCC, Make

```bash
git clone https://github.com/kullanici/mfz-lang.git
cd mfz-lang
make
```

Derleme sonucunda `mfz/mfz` çalıştırılabilir dosyası oluşur.

---

## Kullanım

```bash
# Bir .mfz dosyasını çalıştır
./mfz/mfz program.mfz

# Token listesini göster (debug)
./mfz/mfz program.mfz --tokens

# AST ağacını göster (debug)
./mfz/mfz program.mfz --ast

# Her ikisini de göster
./mfz/mfz program.mfz --all

# Makefile üzerinden çalıştır
make run FILE=program.mfz
```

---

## Dil Referansı

### Tipler

| Tip     | Açıklama          | Örnek           |
|---------|-------------------|-----------------|
| `int`   | Tam sayı          | `int x = 42;`   |
| `float` | Ondalıklı sayı    | `float pi = 3.14;` |
| `void`  | Dönüş değeri yok  | `void f() { }` |

### Değişken Tanımlama

```mfz
int sayi = 10;
float oran = 0.75;
int toplam = sayi + 5;
```

### Operatörler

| Kategori     | Operatörler                        |
|--------------|------------------------------------|
| Aritmetik    | `+` `-` `*` `/` `%`               |
| Karşılaştırma| `==` `!=` `<` `>` `<=` `>=`       |
| Mantıksal    | `&&` `\|\|` `!`                   |
| Atama        | `=`                                |

String birleştirme `+` ile yapılır:

```mfz
void mesaj = "Merhaba" + " Dünya";
```

### Koşullar

```mfz
if (x > 0) {
    println(x);
} else {
    println(0);
}
```

### Döngüler

```mfz
// while
int i = 0;
while (i < 10) {
    i = i + 1;
}

// for
for (int j = 0; j < 5; j = j + 1) {
    println(j);
}
```

### Fonksiyonlar

```mfz
int topla(int a, int b) {
    return a + b;
}

int sonuc = topla(3, 4);
println(sonuc);
```

Özyinelemeli fonksiyonlar desteklenir:

```mfz
int faktoriyel(int n) {
    if (n <= 1) { return 1; }
    return n * faktoriyel(n - 1);
}

println(faktoriyel(7));  // 5040
```

### Yerleşik Fonksiyonlar

| Fonksiyon       | Açıklama                        |
|-----------------|---------------------------------|
| `print(x)`      | Değeri yazdır                   |
| `println(x)`    | Değeri yazdır + yeni satır      |
| `sqrt(x)`       | Karekök                         |
| `abs(x)`        | Mutlak değer                    |
| `int(x)`        | Float → int dönüşümü            |
| `float(x)`      | Int → float dönüşümü            |

### Yorumlar

```mfz
// Tek satır yorum

/*
   Çok satır
   yorum
*/
```

### Örnek Program

```mfz
// fibonacci.mfz
int fibonacci(int n) {
    if (n <= 1) { return n; }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

int i = 0;
while (i <= 10) {
    println(fibonacci(i));
    i = i + 1;
}
```

---

## Proje Yapısı

```
mfz-lang/
├── mfz/
│   ├── lexer.h          # Token tipleri, Lexer yapısı, API
│   ├── lexer.c          # Tokenizer implementasyonu
│   ├── parser.h         # AST düğüm tipleri, Parser yapısı, API
│   ├── parser.c         # Recursive descent parser
│   ├── interpreter.h    # Değer tipleri, Env, Interpreter yapısı, API
│   ├── interpreter.c    # Tree-walk interpreter
│   └── main.c           # Giriş noktası, .mfz dosya çalıştırıcı
├── docs/                # Detaylı belgeler
├── development_guide.md # Yeni özellik ekleme rehberi
├── Makefile
├── .gitignore
└── README.md
```

### Mimari

```
Kaynak (.mfz)
    │
    ▼
[Lexer]          lexer.c — karakterleri token'lara ayırır
    │
    ▼  Token[]
[Parser]         parser.c — token'lardan AST üretir
    │
    ▼  Node* (AST)
[Interpreter]    interpreter.c — AST'yi ziyaret ederek çalıştırır
    │
    ▼
Çıktı
```

---

## Geliştirme

Yeni özellik eklemek için `development_guide.md` dosyasına bakın.

### Hızlı Derleme & Test

```bash
make
./mfz/mfz test.mfz --all
```

### Katkıda Bulunma

1. Fork'la
2. Feature branch oluştur (`git checkout -b feature/yeni-ozellik`)
3. Değişikliklerini commit'le
4. Pull request aç

---

## Lisans

MIT

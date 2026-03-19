# MFZ — Örnek Programlar

---

## 1. Merhaba Dünya

```mfz
println("Merhaba, Dünya!");
```

---

## 2. Değişkenler ve Aritmetik

```mfz
int a = 10;
int b = 3;

println(a + b);   // 13
println(a - b);   // 7
println(a * b);   // 30
println(a / b);   // 3  (tam bölme)
println(a % b);   // 1

float x = 10.0;
float y = 3.0;
println(x / y);   // 3.33333
```

---

## 3. Koşullar

```mfz
int puan = 75;

if (puan >= 90) {
    println("AA");
} else {
    if (puan >= 80) {
        println("BA");
    } else {
        if (puan >= 70) {
            println("BB");
        } else {
            println("FF");
        }
    }
}
```

---

## 4. Döngüler

```mfz
// while ile 1'den 10'a kadar topla
int toplam = 0;
int i = 1;
while (i <= 10) {
    toplam = toplam + i;
    i = i + 1;
}
println(toplam);  // 55

// for ile çarpım tablosu (5)
for (int j = 1; j <= 10; j = j + 1) {
    println(5 * j);
}
```

---

## 5. Fonksiyonlar

```mfz
int kare(int n) {
    return n * n;
}

int kup(int n) {
    return n * kare(n);
}

println(kare(4));   // 16
println(kup(3));    // 27
```

---

## 6. Özyineleme — Faktöriyel

```mfz
int faktoriyel(int n) {
    if (n <= 1) { return 1; }
    return n * faktoriyel(n - 1);
}

int i = 1;
while (i <= 10) {
    print(i);
    print(" -> ");
    println(faktoriyel(i));
    i = i + 1;
}
```

---

## 7. Özyineleme — Fibonacci

```mfz
int fib(int n) {
    if (n <= 1) { return n; }
    return fib(n - 1) + fib(n - 2);
}

for (int i = 0; i <= 12; i = i + 1) {
    println(fib(i));
}
```

---

## 8. String İşlemleri

```mfz
void selamla(void isim) {
    println("Merhaba, " + isim + "!");
}

selamla("Ahmet");
selamla("Ayşe");
```

---

## 9. Matematiksel Hesaplamalar

```mfz
float pi = 3.14159265;

// Çember alanı ve çevresi
float r = 7.0;
float alan   = pi * r * r;
float cevre  = 2.0 * pi * r;

print("Alan: ");
println(alan);
print("Cevre: ");
println(cevre);

// Hipotenüs
float a = 3.0;
float b = 4.0;
float c = sqrt(a * a + b * b);
println(c);  // 5.0
```

---

## 10. Asal Sayı Kontrolü

```mfz
int asal_mi(int n) {
    if (n < 2) { return 0; }
    int i = 2;
    while (i * i <= n) {
        if (n % i == 0) { return 0; }
        i = i + 1;
    }
    return 1;
}

// 2'den 50'ye kadar asal sayılar
int sayi = 2;
while (sayi <= 50) {
    if (asal_mi(sayi)) {
        println(sayi);
    }
    sayi = sayi + 1;
}
```

---

## 11. GCD (En Büyük Ortak Bölen)

```mfz
int gcd(int a, int b) {
    while (b != 0) {
        int gecici = b;
        b = a % b;
        a = gecici;
    }
    return a;
}

println(gcd(48, 18));   // 6
println(gcd(100, 75));  // 25
println(gcd(17, 13));   // 1
```

---

## 12. Üs Alma (Özyinelemeli)

```mfz
float us(float taban, int ust) {
    if (ust == 0) { return 1.0; }
    if (ust < 0)  { return 1.0 / us(taban, -ust); }
    return taban * us(taban, ust - 1);
}

println(us(2.0, 10));   // 1024.0
println(us(3.0, 5));    // 243.0
println(us(2.0, -3));   // 0.125
```

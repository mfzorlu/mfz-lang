# MFZ Lang

MFZ is a small programming language written in C — built from scratch with a lexer, recursive descent parser, and tree-walk interpreter. Source files use the `.mfz` extension.

> Developed with the assistance of [Claude Sonnet 4.6](https://www.anthropic.com/claude).

---

## Quick Start

```bash
git clone https://github.com/mfzorlu/mfz-lang.git
cd mfz-lang
make
./mfz/mfz program.mfz
```

---

## Installation

### Linux

Installs the `mfz` command system-wide, registers the `.mfz` MIME type, and sets up the file icon and double-click support.

**Requirements:** GCC, Make. Optional: `imagemagick` (for multi-resolution icons).

```bash
make
sudo make install
```

To uninstall:

```bash
sudo make uninstall
```

> If the `.mfz` file icon does not appear immediately, log out and back in.

### Windows

**Requirements:** GCC (e.g. [WinLibs](https://winlibs.com/) or [MSYS2](https://www.msys2.org/))

Right-click `install.bat` → **Run as administrator**

This compiles the binary, adds it to PATH, and registers `.mfz` files in the registry (icon + double-click support).

To uninstall: right-click `uninstall.bat` → **Run as administrator**

---

## Usage

```bash
mfz program.mfz            # run
mfz program.mfz --tokens   # show token list (debug)
mfz program.mfz --ast      # show AST (debug)
mfz program.mfz --all      # tokens + AST + run
```

Or use Make:

```bash
make run FILE=program.mfz
```

---

## Language Reference

### Types

| Type    | Example              |
|---------|----------------------|
| `int`   | `int x = 42;`        |
| `float` | `float pi = 3.14;`   |
| `void`  | `void f() { }`       |

### Operators

| Category      | Operators                          |
|---------------|------------------------------------|
| Arithmetic    | `+` `-` `*` `/` `%`               |
| Comparison    | `==` `!=` `<` `>` `<=` `>=`       |
| Logical       | `&&` `\|\|` `!`                   |
| Assignment    | `=`                                |

String concatenation with `+`:
```mfz
println("Hello" + ", " + "World!");
```

### Control Flow

```mfz
if (x > 0) {
    println(x);
} else {
    println(0);
}

while (i < 10) {
    i = i + 1;
}

for (int j = 0; j < 5; j = j + 1) {
    println(j);
}
```

### Functions

```mfz
int factorial(int n) {
    if (n <= 1) { return 1; }
    return n * factorial(n - 1);
}

println(factorial(7));  // 5040
```

Functions can access global variables:

```mfz
int counter = 0;

int increment(int amount) {
    return counter + amount;
}
```

### Built-in Functions

| Function      | Description                     |
|---------------|---------------------------------|
| `print(x)`    | Print value (no newline)        |
| `println(x)`  | Print value + newline           |
| `sqrt(x)`     | Square root                     |
| `abs(x)`      | Absolute value                  |
| `int(x)`      | Cast float → int                |
| `float(x)`    | Cast int → float                |

### Comments

```mfz
// single line

/* multi
   line */
```

---

## Project Structure

```
mfz-lang/
├── mfz/
│   ├── lexer.h / lexer.c          # Tokenizer
│   ├── parser.h / parser.c        # Recursive descent parser
│   ├── interpreter.h / interpreter.c  # Tree-walk interpreter
│   └── main.c                     # Entry point, .mfz runner
├── docs/
│   ├── language_spec.md           # Grammar, type system, operator precedence
│   ├── architecture.md            # Internal design, data flow
│   └── examples.md                # Example programs
├── development_guide.md           # How to add new syntax/features
├── install.sh                     # Linux installer (called by make install)
├── uninstall.sh                   # Linux uninstaller
├── install.bat                    # Windows installer
├── uninstall.bat                  # Windows uninstaller
├── Makefile
├── mfz.png                        # File icon (Linux)
├── mfz.ico                        # File icon (Windows)
└── README.md
```

---

## Extending the Language

See [`development_guide.md`](development_guide.md) for step-by-step instructions on adding new keywords, operators, statements, built-in functions, and value types.

---

## License

MIT

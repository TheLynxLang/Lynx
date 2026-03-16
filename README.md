# 🐾 Lynx
> v1.3.3

Lynx is a lightweight, animal-themed scripting language built in C. It supports variables, loops, functions, and modular packs for rapid tool development.

## 🚀 Quick Start

1. **Download & Install:**
   - Download `Lynx-1.3.3-installer.exe` from [Releases](https://github.com/justdev-chris/Lynx/releases)
   - Run the installer (adds Lynx to PATH automatically)

2. **Run a script:**

```
lynx myscript.lnx
```

3. **Enter Interactive Mode (REPL):**

```
lynx
```

---

## 📖 Syntax & Commands

### Variables

* `Set x = 10` : Store a numeric value
* `Set name = "hello"` : Store a string value
* `Hunt` : Display all variables in the Den
* `Roar x` : Print the value of a variable
* `Pounce x` : Delete variable x

### Arithmetic & Logic

Lynx supports full expression evaluation:

* `Set area = radius * radius`
* `Set total = a + b - c`
* `Set result = x % 3` : Modulo operator
* `Set bigger = x > y And y > z` : Logic operators (And, Or, Not)

### Control Flow

**Conditionals:**

```
If x > 5 {
Roar x
}
```

**For Loops:**

```
For i = 0 To 10 {
Roar i
}
```

**While Loops:**

```
While x > 0 {
Set x = x - 1
}
```

### Functions

Define and call functions:


```
Func greet(name) {
Roar name
}
greet(“world”)
```

### Operators

* `++, --` : Increment/Decrement (Set x++)
* `+, -, *, /, %` : Arithmetic
* `>, <, >=, <=, ==, !=` : Comparison
* `And, Or, Not` : Logic

### Modules & Persistence

* `Stalk_Pack "filename.lnx"` : Import another Lynx file (searches root and `std/` folder)

### Library Loading

* `LoadLib "mylib"` : Load C DLL from `./lib/` folder

---

## 🛠️ Project Structure

* `main.c` : Entry point and file loading
* `scanner.c` : Tokenization
* `parser.c` : Parsing and execution
* `memory.c` : Variable and function storage
* `std/` : Standard library (math.lnx, colors.lnx)
* `installer/` : ISS installer script

---

## 🤖 Automation

This project includes GitHub Actions workflows (`.github/workflows/`) that automatically:

1. Compile the engine on Windows
2. Run integration tests
3. Build the ISS installer
4. Upload artifacts and create releases

---

## 📝 Example Script


```
Calculate factorial
Func factorial(n) {
If n <= 1 {
Roar 1
}
Else {
Set result = n * factorial(n - 1)
Roar result
}
}
factorial(5)
```

---

## About

Lynx is a programming language built from C.

### Resources

[Releases](https://github.com/justdev-chris/Lynx/releases) • [Issues](https://github.com/justdev-chris/Lynx/issues)

### License

[MIT license](LICENSE)

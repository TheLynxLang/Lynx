# 🐾 Lynx

Lynx is a lightweight, animal-themed scripting language with a built-in package manager, file I/O, and native DLL support.

---

## 📦 Install

Download the latest `lynx.exe` from the [releases page](https://github.com/justdev-chris/Lynx/releases).

Add it to your `PATH` or run it from its folder.

---

## 🚀 Quick Start

### 1. Create a project

```bash
lynx init
```

This creates:

```
my_project/
├── lynx.toml
└── src/
    └── main.lnx
```

### 2. Write code

Edit `src/main.lnx`:

```lynx
Roar "Hello, Lynx!"
Set x = 10
Roar x
```

### 3. Run it

```bash
lynx build
```

Or directly:

```bash
lynx src/main.lnx
```

---

## 📚 Language Basics

### Variables

```lynx
Set x = 10
Set name = "Lynx"
Set x = x + 1
```

### Functions

```lynx
Func greet(name) {
    Roar "Hello, " + name
}

greet("Alpha")
```

### Conditionals

```lynx
If x > 5 {
    Roar "x is large"
} Else {
    Roar "x is small"
}
```

### Loops

```lynx
For i = 0 To 10 {
    Roar i
}

Set i = 0
While i < 10 {
    Roar i
    Set i = i + 1
}
```

### Operators

**Arithmetic**: `+`, `-`, `*`, `/`, `%`  
**Comparison**: `>`, `<`, `>=`, `<=`, `==`, `!=`  
**Logic**: `And`, `Or`, `Not`  
**Assignment**: `=`, `++`, `--`

### Comments

```lynx
# This is a comment
Roar "Hello"   # inline comment
```

---

## 🛠 Built-in Commands

| Command          | Description                        |
|------------------|------------------------------------|
| Set              | Assigns a variable                 |
| Roar             | Prints a value                     |
| Hunt             | Lists all variables                |
| Pounce           | Deletes a variable                 |
| Stalk_Pack       | Runs another `.lnx` file           |
| LoadLib          | Loads a C DLL                      |
| Run              | Executes a system command          |

---

## 📁 File I/O

| Command            | Description                     |
|--------------------|---------------------------------|
| KittyWriteFile     | Writes to a file                |
| KittyReadFile      | Reads a file                    |
| KittyFileExists    | Checks if a file exists         |
| Paw                | Creates a directory             |
| KittyRemoveFile    | Deletes a file                  |
| KittyListFiles     | Lists files in a directory      |

**Example:**

```lynx
KittyWriteFile "data.txt" "Hello, file!"
KittyReadFile "data.txt"
```

---

## 📦 Package Manager

| Command            | Description                              |
|--------------------|------------------------------------------|
| `lynx init`        | Creates a new project                    |
| `lynx add <pkg>`   | Adds a dependency                        |
| `lynx install`     | Installs all dependencies                |
| `lynx publish`     | Packs your project into a tarball        |
| `lynx build`       | Runs `src/main.lnx`                      |

**`lynx.toml` example:**

```toml
[package]
name = "my_project"
version = "0.1.0"
authors = ["Your Name"]
description = "A Lynx project"

[dependencies]
json = "1.0.0"
```

**Registry**: Packages are hosted at [https://github.com/justdev-chris/lynx-registry](https://github.com/justdev-chris/lynx-registry)

---

## 🔌 Modules

### Stalk_Pack (Legacy)
Runs a file directly. Variables leak globally.

```lynx
Stalk_Pack "libs/json/main.lnx"
```

### KittyPort (Recommended)
Loads a package from `libs/<name>/main.lnx` in a scoped, cached environment.

```lynx
KittyPort "json"
```

### LoadLib (Native DLLs)
Loads a C DLL from `lib/<name>.dll`.

```lynx
LoadLib "curl"
```

---

## 📚 Standard Library

Lynx ships with standard libraries in `std/`:

### math.lnx

```lynx
Stalk_Pack "math.lnx"

abs(x)       # absolute value
min(a, b)    # smaller of two
max(a, b)    # larger of two
clamp(x, lo, hi)  # limit x to range
```

### colors.lnx

```lynx
Stalk_Pack "colors.lnx"
Roar color.red + "Error!" + color.reset
```

---

## 🧪 Examples

### Hello World

```lynx
Roar "Hello, Lynx!"
```

### FizzBuzz

```lynx
For i = 1 To 100 {
    If i % 15 == 0 {
        Roar "FizzBuzz"
    } Else If i % 3 == 0 {
        Roar "Fizz"
    } Else If i % 5 == 0 {
        Roar "Buzz"
    } Else {
        Roar i
    }
}
```

### Read a File

```lynx
KittyReadFile "data.txt"
Roar __file_content
```

### System Command

```lynx
Run "echo Lynx rocks!"
```

---

## 🔧 Contributing

### Building from Source

```bash
gcc -o lynx.exe main.c parser.c scanner.c memory.c libloader.c -lm -luser32 -lurlmon
```

### Adding a Built-in
1. Add token to `lynx.h`
2. Add keyword to `scanner.c`
3. Add logic to `parser.c`

### Writing a DLL

```c
__declspec(dllexport) void hello() {
    printf("Hello from DLL!\n");
}
```

Compile to `lib/hello.dll` and load with `LoadLib "hello"`.

---

## 📄 License

MIT — see LICENSE for details.

---

## 🔗 Links

**Lynx Language Repo**
- https://github.com/justdev-chris/Lynx
- https://github.com/justdev-chris/Lynx/releases
- https://github.com/justdev-chris/Lynx/wiki

**Lynx Registry**
- https://github.com/justdev-chris/lynx-registry

**Installer**
- https://github.com/justdev-chris/Lynx/releases/latest/download/LynxInstaller.exe

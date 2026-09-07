# 🐾 Lynx Language Skill

## Overview
Lynx is a lightweight, animal-themed scripting language with a built-in package manager, file I/O, and native DLL support. This skill helps you write, run, and manage Lynx projects.

## When to Use
- Writing Lynx scripts (`.lnx` files)
- Managing Lynx projects with `lynx.toml`
- Installing Lynx packages from the registry
- Debugging Lynx code

---

## Quick Reference

### Command Line
```bash
lynx init [project_name] [author]   # Create new project
lynx add <package>                   # Add dependency
lynx install                         # Install dependencies
lynx build                           # Run src/main.lnx
lynx run <file.lnx>                  # Run script
lynx search <term>                   # Search registry
lynx update                          # Update packages
lynx remove <package>                # Remove dependency
lynx fmt <file.lnx>                  # Auto-format
lynx check <file.lnx>                # Check syntax
lynx debug                           # Toggle debug mode
lynx --version                       # Show version
lynx help                            # Show help
```

### Language Basics

**Variables:**
```lynx
Set x = 10
Set name = "Lynx"
Set x = x + 1
```

**Printing:**
```lynx
Roar "Hello, Lynx!"
Roar x
Roar "Value: " + x
```

**Functions:**
```lynx
Func greet(name) {
    Roar "Hello, " + name
}
greet("World")
```

**Conditionals:**
```lynx
If x > 5 {
    Roar "x is large"
} Else {
    Roar "x is small"
}
```

**Loops:**
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

**Operators:**
- Arithmetic: `+`, `-`, `*`, `/`, `%`
- Comparison: `>`, `<`, `>=`, `<=`, `==`, `!=`
- Logic: `And`, `Or`, `Not`
- Assignment: `=`, `++`, `--`

**Comments:**
```lynx
# This is a comment
Roar "Hello"   # inline comment
```

---

### Built-in Commands

| Command | Description |
|---------|-------------|
| `Set` | Assigns a variable |
| `Roar` | Prints a value |
| `Hunt` | Lists all variables |
| `Pounce` | Deletes a variable |
| `Stalk_Pack` | Runs another `.lnx` file |
| `LoadLib` | Loads a C DLL |
| `Run` | Executes a system command |
| `Return` | Returns from a function |
| `Break` | Breaks out of a loop |
| `Continue` | Skips to next loop iteration |

---

### File I/O

| Command | Description |
|---------|-------------|
| `KittyWriteFile` | Writes to a file |
| `KittyReadFile` | Reads a file |
| `KittyFileExists` | Checks if a file exists |
| `Paw` | Creates a directory |
| `KittyRemoveFile` | Deletes a file |

**Example:**
```lynx
KittyWriteFile "data.txt" "Hello, file!"
KittyReadFile "data.txt"
If __result == 1 {
    Roar __file_content
}
```

---

### String Functions

| Command | Description |
|---------|-------------|
| `KittySplitString` | Splits a string by delimiter |
| `KittyCheckIfStringContains` | Checks if string contains substring |
| `KittyReplaceString` | Replaces substring |
| `Trim` | Removes whitespace |
| `Len` | Gets string length |

**Example:**
```lynx
Set words = KittySplitString("a,b,c", ",")
Set count = __split_count
For i = 0 To count - 1 {
    Roar __split_[i]
}
```

---

### Package Manager

**`lynx.toml` format:**
```toml
[package]
name = "my_project"
version = "0.1.0"
authors = ["Your Name"]
description = "A Lynx project"

[dependencies]
json = "1.0.0"
```

**Commands:**
```bash
lynx add meowsys          # Adds meowsys = "0.1.0" to dependencies
lynx install              # Downloads and extracts all dependencies
lynx update               # Updates all packages to latest
lynx remove meowsys       # Removes from dependencies
lynx search json          # Searches registry for packages
lynx publish              # Creates package.tar.gz for publishing
```

---

### Error Handling

**Try/Catch:**
```lynx
Try {
    KittyReadFile "missing.txt"
} Catch {
    Roar "File not found!"
}
```

**Get Error:**
```lynx
Try {
    KittyReadFile "missing.txt"
} Catch {
    Roar "Error: " + GetError()
}
```

---

### Debug Mode

```bash
lynx debug                # Toggle debug messages ON/OFF
```

When enabled, Lynx prints detailed debug output including:
- Variable creation and lookup
- File operations
- Parser state
- Function calls

---

### Common Patterns

**Read a file:**
```lynx
KittyReadFile "config.txt"
If __result == 1 {
    Set content = __file_content
    Roar "Config: " + content
} Else {
    Roar "Config file not found"
}
```

**Parse JSON:**
```lynx
KittyReadFile "data.json"
Set json = __file_content
KittyParseJSON json
KittyReadFile __json_file
Roar __file_content
```

**Loop through lines:**
```lynx
KittyReadFile "data.txt"
Set content = __file_content
KittySplitString content "\n"
Set count = __split_count

For i = 0 To count - 1 {
    Set line = __split_[i]
    If line != "" {
        Roar "Line " + i + ": " + line
    }
}
```

---

### Project Structure

```
my_project/
├── lynx.toml          # Package manifest
├── src/
│   └── main.lnx       # Entry point
├── libs/              # Installed packages
├── std/               # Standard library
└── scripts/           # Internal scripts
```

---

### Tips

1. **Use `Roar` for debugging** - Print variables to see their values
2. **Check `__result`** - Most operations set `__result` to 1 on success, 0 on failure
3. **Use `__file_content`** - Stores content of last read file
4. **Split strings** - Results go to `__split_0`, `__split_1`, etc.
5. **Toggle debug** - Run `lynx debug` to see detailed execution logs
6. **Hardcode URLs** - Don't use `+` in `Run` commands

---

### Troubleshooting

| Issue | Solution |
|-------|----------|
| `lynx: command not found` | Add Lynx to PATH or use full path |
| `Run: Command failed` | Check command syntax and PATH |
| `Could not read entire file` | File may have trailing newline issue - ignore, it's fine |
| `Bad hostname` | Add `-k` flag to curl for SSL issues |
| `tar: Unrecognized archive format` | Package URL may be wrong or package doesn't exist |

---

### Links

- **Repository:** https://github.com/TheLynxLang/Lynx
- **Registry:** https://github.com/justdev-chris/lynx-registry
- **Releases:** https://github.com/TheLynxLang/Lynx/releases

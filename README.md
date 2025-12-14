# Mini-C
Interpreter in C, implementing a subset of C instructions

## Overview
Mini-C is a lightweight C interpreter that executes a subset of the C programming language. It parses and interprets C source code at runtime, making it useful for educational purposes, scripting, and rapid prototyping without the need for compilation.

The interpreter uses a recursive descent parser to analyze expressions and implements a virtual machine-like execution model with support for function calls, local/global variables, and control flow structures.

## How It Works

### Architecture
1. **Lexical Analyzer**: Tokenizes the source code into identifiers, keywords, operators, and literals
2. **Parser**: Builds an abstract representation of the code structure using recursive descent parsing
3. **Prescan Phase**: Identifies all function definitions and global variables before execution
4. **Interpreter**: Executes the parsed code using a stack-based approach for function calls and local variables
5. **Expression Evaluator**: Handles arithmetic, relational, and logical expressions with proper operator precedence

### Execution Flow
```
Source Code → Tokenization → Parsing → Prescan → Interpretation → Output
```

### Memory Model
- **Global Variables**: Stored in a fixed-size array accessible throughout the program
- **Local Variables**: Managed using a stack that grows/shrinks with function calls
- **Function Table**: Maps function names to their entry points in the source code
- **Call Stack**: Tracks nested function calls and maintains return addresses

## Usage
```bash
minic <filename.c>
```

The interpreter will load and execute the specified C source file, starting from the `main()` function.

## Example Programs

### Example 1: Hello World
```c
int main() {
    print("Hello, Mini-C!");
    return 0;
}
```

### Example 2: Factorial (Recursive)
```c
int factorial(int n) {
    if(n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

int main() {
    int result;
    result = factorial(5);
    print(result);
    return 0;
}
```

### Example 3: Fibonacci Sequence
```c
int fib(int n) {
    int a, b, temp, i;
    a = 0;
    b = 1;
    
    if(n <= 1) {
        return n;
    }
    
    for(i = 2; i <= n; i = i + 1) {
        temp = a + b;
        a = b;
        b = temp;
    }
    return b;
}

int main() {
    int i;
    for(i = 0; i < 10; i = i + 1) {
        print(fib(i));
    }
    return 0;
}
```

### Example 4: Sum of Array Elements
```c
int sum;

int calculate_sum(int count) {
    int i;
    sum = 0;
    for(i = 1; i <= count; i = i + 1) {
        sum = sum + i;
    }
    return sum;
}

int main() {
    int result;
    result = calculate_sum(10);
    print("Sum of 1 to 10: ");
    print(result);
    return 0;
}
```

### Example 5: Loop with Conditionals
```c
int main() {
    int i, even, odd;
    even = 0;
    odd = 0;
    
    for(i = 1; i <= 20; i = i + 1) {
        if(i % 2 == 0) {
            even = even + 1;
        }
        else {
            odd = odd + 1;
        }
    }
    
    print("Even numbers: ");
    print(even);
    print("Odd numbers: ");
    print(odd);
    return 0;
}
```

## Built-in Functions
- `print(value)` - Output a value to the console
- `puts(string)` - Output a string followed by newline
- `getnum()` - Read an integer from keyboard
- `getche()` - Read a character from console
- `putch(char)` - Output a single character

## Feature List
- [x] Parameterized functions with local variables
- [x] Recursion support
- [x] IF-ELSE statements
- [x] DO-WHILE, WHILE and FOR loops
- [x] Int and char variables
- [x] Global variables
- [x] Int and char constants
- [x] String constants (limited implementation)
- [x] Return statement with or without value
- [x] Limited standard library functions
- [x] Operators: +, -, *, %, <, >, <=, >=, ==, !=, unary - and unary +
- [x] Parentheses support
- [x] Functions returning integers
- [x] C-style comments (/* */)
- [ ] Refactor syntax and lexical analyzer
- [ ] Additional example programs

## Restrictions
- The Switch statement is not implemented
- IF, WHILE, DO and FOR statements **must** be enclosed in braces `{ }`
- Arrays are not supported
- Pointers are not supported
- Floating-point numbers are not supported
- Only single-dimensional variable declarations
- Limited string handling capabilities

## Implementation Details
- Maximum 100 functions
- Maximum 100 global variables
- Maximum 200 local variables per execution context
- Maximum program size: 10,000 characters
- Maximum identifier length: 31 characters
- Maximum 31 nested function calls

## Error Handling
The interpreter provides error messages for common syntax errors including:
- Unbalanced parentheses or braces
- Missing expressions or semicolons
- Undefined functions or variables
- Type mismatches
- Excessive local variables or nested calls

Error messages include line numbers to help locate issues in the source code.

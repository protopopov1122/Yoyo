WARNING
=============
I have written this project during the summer of 2016. The code and architecture
is very bad. The main purpose of project was self-educating: I learned C
(however now I admit that project's code style is crap), learned how to write
LL-parser, virtual machine and runtime, port it on different operating systems.
Project was purely educational, however, as far as I have not removed the repo,
I do not recomend to watch or use this repo.
I will use my knowledge to write something better.
Have a good day!

Yoyo
==============
# What is Yoyo?
--------------
Yoyo - a simple small dynamic programming language designed to be consistent and flexible
using traditional syntax concepts (C-style syntax). The main part of the project is written in C. However it
is written in OOP style to make code extension easier. Yoyo works on Linux, Windows, FreeBSD,
but the main development and testing platform is Linux. Whole code complies with C11 language standart
and comples with extra warning flags on gcc, clang and mingw-gcc.
Architecture of project contains several issues that limit performance and increase memory use,
but these issues originate in the project's concept of simplicity.
The main inspiration was Python and Javascript languages.
** Yoyo is only project to educate myself, gain expirence and some fun, so
I don't guarantee that project may be useful in any purposes. **

# Examples
--------------
See 'examples' directory. It contains several examples for different aspects of
language.

# Instalation
----------------
Project requires `libxslt` to build and POSIX-compatible system, libm, pthreads, dlfcn to work. On Windows
all runtime dependencies are statically linked with executable, on Linux & Unix they usually are provided with the OS.
Project contains simple Makefile. It has several options to build Yoyo:
* arch: x86, x64 (default)
* platform win, linux (default)
* target: release, debug (default)
* misc - specific compilation options and defines.
```bash
cd build
make # For Linux x64
make arch="x86" # For Linux x86
make platform="win" # For Windows x64
make platform"win" arch="x86" # For Windows x86
# You can pass "native=yes" to Makefile to build binary optimized for current CPU architecture 

sudo make install # Install and uninstall targets, works only on Linux
sudo make uninstall # It's bad to use direct install, but I've not built packages for Yoyo yet and it's the only way to install yoyo

# In project directory you can launch
yoyo ../examples/guide.yoyo # Launch interactive guide
yoyo ../YStd ../examples/repl.yoyo # Launch REPL
```
You can also build with GCC Profile-Guided Optimizations (tested only on Linux GCC). It will take some time but resulting executable will be much faster.
```bash
./PGO.sh
./PGO.sh "arch=x86"
./PGO.sh "arch=x86" "-DIntPool=5000"	# You can specify make and test launch options (e.g. switch garbage collectors or object implementations)
```
Project contains 'yoyo.vim' file that provides Yoyo syntax highlighting for Vim.
Project uses [dlfcn-win32](https://github.com/dlfcn-win32/dlfcn-win32) to compile on MinGW.


## Tutorial
-------------

Since Yoyo designed to be as simple as it's possible, it uses dynamic strong type system but provides tools to perform type checking in runtime.
It contains well-known control-flow structures: if-else, loops (while, do-while, for, foreach) with labels, break (with labels), continue (with labels),
return, throw, try-catch-else-finally, switch, but also some specific structures. It implements prototype-oriented programming paradigm, first-class functions,
dynamic arrays and other dynamic language tools.
It will be explained below.

### Type model
---------------
Yoyo has several data types:
* Integer - 64-bit signed integer
* Float - 64-bit floating-point number
* Boolean
* String - wide-char string
* Array - has several implementations (vector or list).
* Lambda - first-class function (may represent C procedure or even a few overloaded functions)
* Object - provides dictionary interface with a couple of implementations (AVL-tree and hash map)
* Declaration - defines a type or interface to provide built-in typecheck
* Null - represents nothing
Yoyo allows to create custom data types(used to create iterators)
Some of data types (Integer, Boolean, Null) are cached to reduce memory usage and boost performance.

### Syntax
-------------
Generally Yoyo uses C-like syntax but many syntax elements are optional.
Yoyo uses expression-based approach instead of statement-based. Statement is an expression with ';' at the end (optionally).
All syntax structures are expressions and return values.
```
a = if true 2
		else 3
```
Yoyo allows multiple assignments and even conditional assignments:
```
a,b = 1,2;
if (someCond) a
				 else b = 5;
```
Comments are C-like:
```
someCode; // One-line comment
/* Multiline
		comment*/
```
Parentheses in control-flow structures are optional, but they may be useful to to reduce ambiguities:
```
for i=0;i<100;i++
	print(i);
for (i=0;i<100;i++)
	print(i);
```
Code blocks return a value of the last statement:
```
v = {
	a = 1
	b = 2 // These variables are available only in this scope
	a+b
};
```

Functions may be defined in any scope and will be available in this scope or it sub-scopes:
```
def sum(a, b) return a+b // Braces are optional
if someCond {
	def sub(a,b) {
		a-b		// Return keyword is optional
	}
}
```
Since functions are first-class objects, you can use this function like a simple lambda:
```
fn1 = sum; // Now you can call fn1
```

You can define lambdas with '$' operator:
```
l1 = $(a,b,c) a+b*c
l2 = $(a,b,?c) {	// Braces are purely optional.
									// By the way, it's a variable-argument lambda: first two arguments are stored in a and b, but others are strored in c as the array
	return a+b+c[0]
}
```

You can call functions and lambdas and access arrays simply:
```
print(fun1()+arr[2]);
```
There is a small amount of keywords, so all runtime functions are represented as the native lambdas (and can be invoked as an ordinary function). E.g. print is the native lambda.

You can create objects with `object` keyword:
```
o = object; // Empty object, but it has some predefined methods like toString() and hashCode()
o1 = object : o; // Objects can have one parent
o2 = object { // You can define fields and methods this way
	x: 5
	def fun1() x++	// Simple function
	method fun2() self.x++ // Object method. Used to create polymorph methods
	l1: $() x++ // Or simply assign a lambda
}
```
Arrays are created simply:
```
arr = [1,2,3];
arr2 = []; // Empty array
arr3 = [1;3] // Equals to [1,1,1]
```

Operators are polymorph and can be used on different types. Operators '&&' and '||' are lazy.


## Contributors
--------------
Jevgenijs Protopopovs <jprotopopov1122@gmail.com>
## License
--------------
All project code, examples, documentation and other related files are licensed
under the terms or GNU GPLv3

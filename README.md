Yoyo
==============
# What is Yoyo?
--------------
Yoyo - simple small dynamic programming language designed to consistent and flexible
using traditional syntax concepts(C-style syntax). The main part of the project is written on C. However it
is written in OOP style to make code extension easier. Yoyo works on Linux, Windows, FreeBSD,
but the main development and testing platform is Linux. Whole code fits C11 language standart
and comples with extra warning flags on gcc, clang and mingw-gcc.
Architecture of project contains several issues that limit performance and increase memory use,
but these issues origins in project concept of simplicity.
The main inspiration was Python and Javascript.
** Yoyo is only project to educate myself, gain expirence and some fun, so
I don't guarantee that project may be useful in any purposes. **

# Examples
--------------
See 'examples' directory. It contains several examples of different aspects of
language.

# Goals
* Make project usable, remove bugs and refractor code.
* Add build and installation system.
* Write standart language library
* Add code examples and tests
Main goal is to make project useful and stable.

# Instalation
----------------
Project requires libxslt to build and POSIX, libm, pthreads, dlfcn to work. On Windows all runtime dependencies are statically linked with executable, on Linux & Unix they usually are provided with OS.
Project contains simple Makefile. It has several options to build Yoyo:
* arch: x86, x64(default)
* platform win, linux(default)
* target: release, debug(default)
* misc - specific compilation options and defines.
```bash
cd build
make # For Linux x64
make arch="x86" # For Linux x86
make platform="win" # For Windows x64
make platform"win" arch="x86" # For Windows x86

sudo make install # Install and uninstall targets work only on Linux
sudo make uninstall # It's bad to use direct install, but I've not builded packages for Yoyo yet and it's the only way to install yoyo

# In project directory you can launch
yoyo ../examples/guide.yoyo # Launch interactive guide
yoyo ../YStd ../examples/repl.yoyo # Launch REPL
```
You can also build with GCC Profile-Guided Optimizations(tested only on Linux GCC). It will take some time but resulting executable will be much faster.
```bash
./PGO.sh
./PGO.sh "arch=x86"
./PGO.sh "arch=x86" "-DIntPool=5000"	# You can specify make and test lauch options(e.g. switch garbage collectors or object implementations
```
Project contains 'yoyo.vim' file that provides Yoyo syntax highlighting in Vim.
In build directory will appear executable.
Project uses [dlfcn-win32](https://github.com/dlfcn-win32/dlfcn-win32) to compile on MinGW.


## Tutorial
-------------

Since Yoyo designed to be as simple as it possible, it uses dynamic strong type system but provides tools to perform type checking in runtime.
It contains well-known control-flow structures: if-else, loops(while, do-while, for, foreach) with labels, break(with labels), continue(with labels), return, throw, try-catch-else-finally, switch.
And also some specific structures. It provides prototype-based programming, first-class functions, dynamic arrays and other dynamic language tools.
It will be explained below.

### Type model
---------------
Yoyo has several data types:
* Integer - 64-bit signed integer
* Float - 64-bit floating-point number
* Boolean
* String - wide-char string
* Array - has several implementations, because provide abstract interface to implement
* Lambda - first-class function(may represent C procedure or even few different functions)
* Object - provides dictionary interface with couple realesations
* Declaration - defines type or interface to provide built-in typecheck
* Null - represents nothing
Yoyo allows to create custom data types(used to create iterators)
Some data of types(Integer, Boolean, Null) are cached to reduce memory use and boost performance.

### Syntax
-------------
Generally Yoyo use C-like syntax but many syntax elements are optional.
Yoyo uses expression-based approach instead of statement-based. Statement is an expression with ';' at end(optional). All syntax structures are expressions and return values.
```
a = if true 2
		else 3
```
Yoyo allow multiple assignment or even conditional assignment:
```
a,b = 1,2;
if (someCond) a
				 else b = 5;
```
Comments are C-like:
```
someCode; // One line comment
/* Multiple line
		comment*/
```
Parentheses in control-flow structures are optional, but they may be useful to to remove syntax variety:
```
for i=0;i<100;i++
	print(i);
for (i=0;i<100;i++)
	print(i);
```
Code blocks return value of last statement:
```
v = {
	a = 1
	b = 2 // These variables are available only in this scope
	a+b
};
```

Functions may be defined in any scope and will be available in this scope or it subscopes:
```
def sum(a, b) return a+b // Braces are optional
if someCond {
	def sub(a,b) {
		a-b		// Return keyword is optional
	}
}
```
Since functions are first-class objects, you can access this function like simple lambda:
```
fn1 = sum; // Now you can call fn1
```

You can define lambdas with '$' operator:
```
l1 = $(a,b,c) a+b*c
l2 = $(a,b,?c) {	// Braces are purely optional.
									// By the way, it's variable argument lambda: first two arguments are stored in a and b, but other strored in c as array
	return a+b+c[0]
}
```

You can call functions and lambdas and access arrays simply:
```
print(fun1()+arr[2]);
```
There are small amount of keywords, so all runtime functions are represented as native lambdas(can be called as simple lambda). E.g. print is native lambda.

You can create objects with 'object' keyword:
```
o = object; // Clean object, but it has some predefined methods like toString() and hashCode()
o1 = object : o; // Objects can have one parent.
o2 = object { // You can define fields and methods this way
	x: 5
	def fun1() x++	// Simple function
	method fun2() self.x++ // Object method. Used to create polymorph methods
	l1: $() x++ // Or simply create lambda
}
```
Arrays are created simply:
```
arr = [1,2,3];
arr2 = []; // Empty array
arr3 = [1;3] // Equal to [1,1,1]
```

Operators are polymorph and can be used on different types. Operators '&&' and '||' are lazy.


## Contributors
--------------
Eugene Protopopov(Jevgenijs Protopopovs) <protopopov1122@yandex.ru>
## License
--------------
All project code, examples, documentation and other related files are licensed
under the terms or GNU GPLv3

# Yoyo language manual
---------------------

## General
---------------

This document contains most general documentation on Yoyo build, usage and language manual.

## Installation
---------------

Yoyo runs on Linux, Unix-like OS and Windows. It doesn’t use any build system, but has custom Makefile with several options to build it on Linux(builds by gcc and clang) or MinGW on both x86 and x64 architectures.
Build notes:
* By default it builds debug developer version so you should specify target version.
* It depends on libxslt while build.
* Yoyo can be build on Linux, Unix or Cygwin(using MinGW)
```bash
git clone https://github.com/protopopov1122/Yoyo
cd Yoyo/build

make yoyo target=release # For 64-bit Linux
make yoyo target=release arch=x86 # For 32-bit Linux
make yoyo target=release platform=win # For 64-bit Windows
make yoyo target=release platform=win arch=x86 # For 32-bit Windows

sudo make install # To install Yoyo on Linux
sudo make uninstall # To uninstall Yoyo on Linux
```
By now Yoyo has not installator on Windows.

## Basic usage
----------------

All further instructions will work on Linux. Instructions for Windows will be published later.
To run Yoyo REPL just type:
```bash
yoyo
```
 Yoyo source files has .yoyo extension. To launch Yoyo file and pass arguments type:
 ```bash
 yoyo file.yoyo arg1 arg2 ...
 ```
 Yoyo has --help and --version options that provide basic information.

## Language manual
-------------------

### Basics

Tradional Hello world looks like:
```
print("Hello, world!");
```
Save it as hello.yoyo and lauch:
```bash
yoyo hello.yoyo
```
** Yoyo data types **
| Yoyo data type	| C data type	| Description					   |
| --------------	|:-----------:|-----------------------:|
| Integer					| int64_t			| Signed 64-bit integer  |
| Float						| double			| 64-bit floating-point  |
| Boolean					| bool				| Boolean value				   |
| String					| wchar_t\*	  | Wide character string	 |
| Array						|							| Abstract array			 	 | 
| Lambda					|							| Callable lambda			   |
| Object					|							| Dictionary based object|
| Null						|							| Nothing/Undefined value|

Not every Yoyo type has direct C equvalent and some of them represent abstract interfaces but implementations are chosed in runtime.
All binary and unary operations are polymorph. Yoyo has dynamic and strong duck typing.
** Operator priority table **
| Priority	| Operators				|
| --------- |:---------------:|
| 1					| [] . ()     		|
| 2					| ++ -- ~ + - 		|
| 3					| \*\*			  		|
| 4					| * / %       		|
| 5					| + -							|
| 6					| >> <<						|
| 7					| & |							|
| 8					| == != >= <= > < |
| 9 				| !								|
| 10				| && ||						|


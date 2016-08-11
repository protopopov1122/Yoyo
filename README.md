Yoyo
==============
## What is Yoyo?
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
## Examples
--------------
See 'examples' directory. It contains several examples of different aspects of
language.
## Goals
* Make project usable, remove bugs and refractor code.
* Add build and installation system.
* Write standart language library
* Add code examples and tests
Main goal is to make project useful and stable.
## Instalation
--------------
Project contains simple Makefile. It has several options to build Yoyo:
* arch: x86, x64(default)
* platform win, linux(default)
* target: release, debug(default)
* misc - specific compilation options and defines.
```bash
cd build
make yoyo target="release"
```
In build directory will appear executable.
Project uses dlfcn-win32 to compile on MinGW.
## Contributors
--------------
Eugene Protopopov(Jevgenijs Protopopovs) <protopopov1122@yandex.ru>
## License
--------------
All project code, examples, documentation and other related files are licensed
under the terms or GNU GPLv3

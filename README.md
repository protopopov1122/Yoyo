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
** Yoyo is only project to educate myself, gain expirence and some fun **, so
** I don't think it may be serious in any purposes **
## Examples
--------------
See 'examples' directory. Now it's empty but my main goal now is to fill it.
## Goals
Main goal is create examples, refractor code and add some comments to it.
## Instalation
--------------
The project is in the development stage so the only build system it has is simple Makefile.
This Makefile provides tools to build project on gcc, clang and mingw-gcc for Unix and
Windows. The main part of project builds in single executable that depends on small
amount of libraries(also can build as shared library).
Project used dlfcn library(see build/Win32)
## Contributors
--------------
Eugene Protopopov(Jevgenijs Protopopovs) <protopopov1122@yandex.ru>
## License
--------------
All project code, examples, documentation and other related files are licensed
under the terms or GNU GPLv3

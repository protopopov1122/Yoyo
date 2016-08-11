## Yoyo Virtual Machine
----------
Yoyo virtual machine includes several parts:
* compiler - translates AST to VM bytecode and performs simple optimizations. Uses Yoyo parser.
* interpreter - executes VM bytecode using Yoyo runtime.
* debugger - provides CLI debugger that allow control execution, create breakpoints, view runtime information and eval code
* Object that provide to program basic system functions like code eval, library loading, C FFI
This module depends on Yoyo parser and Yoyo runtime. Provides cli interface to system functions.

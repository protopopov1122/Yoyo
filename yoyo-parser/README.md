## Yoyo parser
------------
Yoyo parser includes several parts:
* Lexer - read from input stream and returns next token
* Parser - custom LL-parser gets tokens from input stream and returns AST for each parsed file
* Optimizer - performs simple optimizations over AST(mostly constant folding)
This module is used by Yoyo VM, but it is independent and can be compiled
separately.

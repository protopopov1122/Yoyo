This directory contains source of new Yoyo parser. It still will be
custom LL-parser, but it will not depend on other Yoyo components
and code will be cleaner. Motivation is to write few independent
components(parser, runtime, virtual machine) that can be used
different ways(e.g. interpretation, compilation with LLVM)
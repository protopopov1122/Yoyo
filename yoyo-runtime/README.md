## Yoyo runtime
-------------
Yoyo runtime contains data structures and procedures that
main system functionality:
* garbage collection
* data types: int64, double, wide string, boolean, array, object, lambda, null
* abstract interfaces to implement custom realizations of them
* exception handling
* type checking
* debugging support
* multithreading support
* different runtime optimizations
* interfaces that implements VM to access Yoyo compiler, interpreter and OS
* different implementations of arrays, objects, lambdas and collections
Yoyo is used by virtual machine, but it also may be used differently(e.g. by AOT
compiler). Yoyo runtime is independent module so it can be compiled separately
from other system parts and use without them. This module provides some interfaces
that must be implemented in other system parts.

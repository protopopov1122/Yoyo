## Yoyo Standart library
--------------
Standart library contain common code that can be used by each Yoyo application.
Only 'core.yoyo' is necessarily beacause it is loaded with VM init. It initializes
main functions to work with Yoyo, functions to launch threads, control types.
Path to standart library may be specified in compile-time or on application start.
By now standart library is poor, it contains only procedures that directly call Yoyo
runtime or C Standart library. 'native' directory contains code on C that is compiled
and linked with VM but it can also be loaded as a shared library in runtime, YStd code
directly it.

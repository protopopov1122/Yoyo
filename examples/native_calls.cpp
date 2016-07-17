// Yoyo headers must be in include path
#include <stdio.h>
#include <iostream>
#include "yoyo.h"	// Contains Yoyo API

/*Example of external library for Yoyo*/

using namespace std;

YOYO_FUNCTION(YOYO_EXAMPLES_WRITE) {		// All functions used from Yoyo
						// must be delarated this way.
						// There are 4 arguments:
						// lambda - NativeLambda created by Yoyo runtime
						//	to handle this function
						// argc - argument count
						// args - arguments passed to this function
						// th - current Yoyo thread
    wchar_t* wstr = toString(args[0], th);
    cout << "C++ library: ";
    wcout << wstr << endl;
    return getNull(th);
}
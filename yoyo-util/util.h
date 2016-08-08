#ifndef YOYO_UTIL_UTIL_H
#define YOYO_UTIL_UTIL_H

#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <wctype.h>
#include <stdbool.h>
#include <inttypes.h>

typedef struct InputStream {
	wint_t (*read)(struct InputStream*);
	void (*unread)(wint_t, struct InputStream*);
	void (*reset)(struct InputStream*);
	void (*close)(struct InputStream*);
} InputStream;

InputStream* file_input_stream(FILE*);
InputStream* string_input_stream(wchar_t*);
bool parse_int(wchar_t*, int64_t*);

#endif

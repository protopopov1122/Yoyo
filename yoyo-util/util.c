#include "util.h"

typedef struct FileInputStream {
	InputStream is;
	FILE* fd;
} FileInputStream;

wint_t FIS_read(InputStream* is) {
	FileInputStream* fis = (FileInputStream*) is;
	return fgetwc(fis->fd);
}
void FIS_unread(wint_t ch, InputStream* is) {
	FileInputStream* fis = (FileInputStream*) is;
	ungetwc(ch, fis->fd);
}
void FIS_reset(InputStream* is) {
	FileInputStream* fis = (FileInputStream*) is;
	rewind(fis->fd);
}
void FIS_close(InputStream* is) {
	FileInputStream* fis = (FileInputStream*) is;
	fclose(fis->fd);
	free(is);
}

InputStream* file_input_stream(FILE* fd) {
	FileInputStream* fis = malloc(sizeof(FileInputStream));
	fis->fd = fd;
	fis->is.read = FIS_read;
	fis->is.unread = FIS_unread;
	fis->is.reset = FIS_reset;
	fis->is.close = FIS_close;
	return (InputStream*) fis;
}

typedef struct StringInputStream {
	InputStream is;
	wchar_t* wcs;
	size_t index;
} StringInputStream;

wint_t SIS_read(InputStream* is) {
	StringInputStream* sis = (StringInputStream*) is;
	if (sis->index >= wcslen(sis->wcs))
		return WEOF;
	return sis->wcs[sis->index++];
}
void SIS_unread(wint_t ch, InputStream* is) {
	StringInputStream* sis = (StringInputStream*) is;
	if (sis->index > 0)
		sis->index--;
}
void SIS_reset(InputStream* is) {
	StringInputStream* sis = (StringInputStream*) is;
	sis->index = 0;
}
void SIS_close(InputStream* is) {
	StringInputStream* sis = (StringInputStream*) is;
	free(sis->wcs);
	free(sis);
}

InputStream* string_input_stream(wchar_t* wcs) {
	StringInputStream* sis = malloc(sizeof(StringInputStream));
	sis->wcs = calloc(1, sizeof(wchar_t) * (wcslen(wcs) + 1));
	wcscpy(sis->wcs, wcs);
	sis->index = 0;
	sis->is.read = SIS_read;
	sis->is.unread = SIS_unread;
	sis->is.reset = SIS_reset;
	sis->is.close = SIS_close;
	return (InputStream*) sis;
}

bool parse_int(wchar_t* wcs, int64_t* ptr) {
	int64_t number = 0;
	int8_t sign = 1;
	if (wcslen(wcs)==0)
		return false;
	if (wcs[0] == L'-') {
		sign = -1;
		wcs++;
	}
	if (wcslen(wcs) > 2 && wcsncmp(wcs, L"0x", 2) == 0) {
		for (size_t i = 2; i < wcslen(wcs); i++) {
			if (wcs[i] == L'_')
				continue;
			number *= 16;
			if (wcs[i] >= L'0' && wcs[i] <= '9')
				number += wcs[i] - L'0';
			else if (wcs[i] >= L'a' && wcs[i] <= 'f')
				number += wcs[i] - L'a' + 10;
			else if (wcs[i] >= L'A' && wcs[i] <= 'F')
				number += wcs[i] - L'A' + 10;
			else {
				number = -1;
				break;
			}
		}
	} else if (wcslen(wcs) > 2 && wcsncmp(wcs, L"0b", 2) == 0) {
		for (size_t i = 2; i < wcslen(wcs); i++) {
			if (wcs[i] == L'_')
				continue;
			number *= 2;
			if (wcs[i] == L'1')
				number++;
			else if (wcs[i] == L'0') {
			} else {
				number = -1;
				break;
			}
		}
	} else
		for (size_t i = 0; i < wcslen(wcs); i++) {
			if (wcs[i] == L'_')
				continue;
			if (!iswdigit(wcs[i])) {
				number = -1;
				break;
			} else {
				number *= 10;
				number += wcs[i] - L'0';
			}
		}
	if (number == -1)
		return false;
	*ptr = number * sign;
	return true;
}

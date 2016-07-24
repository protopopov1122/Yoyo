/*
 * Copyright (C) 2016  Jevgenijs Protopopovs <protopopov1122@yandex.ru>
 */
/*This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.*/

#include "yoyo-runtime.h"

/*File contains procedures to unify
 * Yoyo exception building.*/

YValue* Exception_toString(YLambda* l, YObject* scp, YValue** args, size_t argc, YThread* th) {
	YObject* exc = (YObject*) ((NativeLambda*) l)->object;
	((YoyoObject*) exc)->linkc++;
	YValue* baseVal = exc->get(exc, getSymbolId(&th->runtime->symbols, L"base"),
			th);
	wchar_t* base_s = toString(baseVal, th);
	YValue* lineVal = exc->get(exc, getSymbolId(&th->runtime->symbols, L"line"),
			th);
	wchar_t* line_s = toString(lineVal, th);
	YValue* chPosVal = exc->get(exc,
			getSymbolId(&th->runtime->symbols, L"charPosition"), th);

	wchar_t* chPos_s = toString(chPosVal, th);
	YValue* fileVal = exc->get(exc, getSymbolId(&th->runtime->symbols, L"file"),
			th);
	wchar_t* file_s = toString(fileVal, th);
	((YoyoObject*) exc)->linkc--;

	const char* fmt = "Exception<%ls> at %ls(%ls : %ls)";
	size_t size = snprintf(NULL, 0, fmt, base_s, file_s, line_s, chPos_s);
	char* cstr = malloc(sizeof(char) * (size + 1));
	sprintf(cstr, fmt, base_s, file_s, line_s, chPos_s);
	wchar_t* wstr = malloc(sizeof(wchar_t) * (strlen(cstr) + 1));
	mbstowcs(wstr, cstr, sizeof(wchar_t) * (strlen(cstr)));
	free(cstr);

	free(base_s);
	free(line_s);
	free(chPos_s);
	free(file_s);

	YValue* str = newString(wstr, th);
	free(wstr);
	return str;
}
YValue* TraceFrame_toString(YLambda* l, YObject* scp, YValue** args, size_t argc, YThread* th) {
	YObject* exc = (YObject*) ((NativeLambda*) l)->object;
	((YoyoObject*) exc)->linkc++;
	YValue* lineVal = exc->get(exc, getSymbolId(&th->runtime->symbols, L"line"),
			th);
	wchar_t* line_s = toString(lineVal, th);
	YValue* chPosVal = exc->get(exc,
			getSymbolId(&th->runtime->symbols, L"charPosition"), th);
	wchar_t* chPos_s = toString(chPosVal, th);
	YValue* fileVal = exc->get(exc, getSymbolId(&th->runtime->symbols, L"file"),
			th);
	wchar_t* file_s = toString(fileVal, th);
	((YoyoObject*) exc)->linkc--;

	const char* fmt = "%ls(%ls : %ls)";
	size_t size = snprintf(NULL, 0, fmt, file_s, line_s, chPos_s);
	char* cstr = malloc(sizeof(char) * size);
	sprintf(cstr, fmt, file_s, line_s, chPos_s);
	wchar_t* wstr = malloc(sizeof(wchar_t) * (strlen(cstr) + 1));
	mbstowcs(wstr, cstr, sizeof(wchar_t) * (strlen(cstr)));
	free(cstr);

	free(line_s);
	free(chPos_s);
	free(file_s);

	YValue* str = newString(wstr, th);
	free(wstr);
	return str;
}
wchar_t* Trace_toString(YArray* arr, YThread* th) {
	StringBuilder* sb = newStringBuilder(L"Call trace:\n");
	for (size_t i = 0; i < arr->size(arr, th); i++) {
		YValue* val = arr->get(arr, i, th);
		wchar_t* valstr = toString(val, th);
		sb->append(sb, L"\t");
		sb->append(sb, valstr);
		sb->append(sb, L"\n");
		free(valstr);
	}
	wchar_t* wstr = malloc(sizeof(wchar_t) * (wcslen(sb->string) + 1));
	wcscpy(wstr, sb->string);
	wstr[wcslen(sb->string) - 1] = L'\0';
	sb->free(sb);
	return wstr;
}

// Used by Yoyo runtime to throw exceptions
void throwException(wchar_t* msg, wchar_t** args, size_t argc, YThread* th) {
	StringBuilder* sb = newStringBuilder(msg);
	if (argc > 0) {
		sb->append(sb, L": ");
		for (size_t i = 0; i < argc; i++) {
			sb->append(sb, args[i]);
			if (i + 1 < argc)
				sb->append(sb, L", ");
		}
	}
	th->exception = newException(newString(sb->string, th), th);
	sb->free(sb);
}
/* Creates exception and fills information
 * about source file, code line and char position,
 * stack trace(if available)*/
YValue* newException(YValue* base, YThread* th) {
	if (th->frame==NULL) {
		return base;
	}
	MUTEX_LOCK(&th->runtime->runtime_mutex);
	SourceIdentifier sid = th->frame->get_source_id(th->frame);
	if (sid.file!=-1) {
		YObject* obj = th->runtime->newObject(NULL, th);
		((YoyoObject*) obj)->linkc++;
		obj->put(obj, getSymbolId(&th->runtime->symbols, L"base"), base, true,
				th);
		obj->put(obj, getSymbolId(&th->runtime->symbols, L"line"),
				newInteger(sid.line, th), true, th);
		obj->put(obj, getSymbolId(&th->runtime->symbols, L"charPosition"),
				newInteger(sid.charPosition, th), true, th);
		obj->put(obj, getSymbolId(&th->runtime->symbols, L"file"),
				newString(getSymbolById(&th->runtime->symbols, sid.file),
						th), true, th);
		obj->put(obj, getSymbolId(&th->runtime->symbols,
		TO_STRING),
				(YValue*) newNativeLambda(0, Exception_toString,
						(YoyoObject*) obj, th), true, th);

		YArray* trace = newArray(th);
		((YoyoObject*) trace)->linkc++;
		obj->put(obj, getSymbolId(&th->runtime->symbols, L"trace"),
				(YValue*) trace, true, th);
		LocalFrame* frame = th->frame;
		while (frame != NULL) {
			SourceIdentifier sid = frame->get_source_id(frame);
			if (sid.file==-1) {
				frame = frame->prev;
				continue;
			}
			YObject* frobj = th->runtime->newObject(NULL, th);
			((YoyoObject*) frobj)->linkc++;
			trace->add(trace, (YValue*) frobj, th);
			frobj->put(frobj, getSymbolId(&th->runtime->symbols, L"line"),
					newInteger(sid.line, th), true, th);
			frobj->put(frobj,
					getSymbolId(&th->runtime->symbols, L"charPosition"),
					newInteger(sid.charPosition, th), true, th);
			frobj->put(frobj, getSymbolId(&th->runtime->symbols, L"file"),
					newString(
							getSymbolById(&th->runtime->symbols,
									sid.file), th), true, th);
			frobj->put(frobj, getSymbolId(&th->runtime->symbols, TO_STRING),
					(YValue*) newNativeLambda(0, TraceFrame_toString,
							(YoyoObject*) frobj, th), true, th);
			((YoyoObject*) frobj)->linkc--;
			
			frame = frame->prev;
		}
		((YoyoObject*) trace)->linkc--;
		trace->toString = Trace_toString;

		((YoyoObject*) obj)->linkc--;
		MUTEX_UNLOCK(&th->runtime->runtime_mutex);
		return (YValue*) obj;
	}
	MUTEX_UNLOCK(&th->runtime->runtime_mutex);
	return base;
}

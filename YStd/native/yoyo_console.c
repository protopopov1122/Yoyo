/*
 * Copyright (C) 2016  Jevgenijs Protopopovs <protopopov1122@yandex.ru>
 */
/*This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License version 3 as published by
 the Free Software Foundation.


 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.*/

#include "yoyo-runtime.h"

YOYO_FUNCTION(YSTD_CONSOLE_PRINTLN) {
	YRuntime* runtime = th->runtime;
	wchar_t* wstr = toString(args[0], th);
	fprintf(runtime->env->out_stream, "%ls\n", wstr);
	fflush(runtime->env->out_stream);
	free(wstr);
	return getNull(th);
}
YOYO_FUNCTION(YSTD_CONSOLE_PRINT) {
	YRuntime* runtime = th->runtime;
	wchar_t* wstr = toString(args[0], th);
	fprintf(runtime->env->out_stream, "%ls", wstr);
	fflush(runtime->env->out_stream);
	free(wstr);
	return getNull(th);
}
YOYO_FUNCTION(YSTD_CONSOLE_READ) {
	YRuntime* runtime = th->runtime;
	wchar_t wstr[] = { getwc(runtime->env->in_stream), L'\0' };
	return newString(wstr, th);
}
YOYO_FUNCTION(YSTD_CONSOLE_READLINE) {
	YRuntime* runtime = th->runtime;
	wchar_t* wstr = readLine(runtime->env->in_stream);
	YValue* ystr = newString(wstr, th);
	free(wstr);
	return ystr;
}


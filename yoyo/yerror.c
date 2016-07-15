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
#include "../headers/yoyo/yerror.h"

/*
 * yerror procedure is used to unify error print in program.
 * Accepts error type and message, then formats message according
 * to the type and prints to error stream*/

void yerror(ErrorType err, wchar_t* msg, YThread* th) {
	switch (err) {
	case CompilationError:
		fprintf(th->runtime->env->err_stream, "%ls\n", msg);
		break;
	case ErrorFileNotFound:
		fprintf(th->runtime->env->err_stream, "File '%ls' not found\n", msg);
		break;
	default:
		fprintf(th->runtime->env->err_stream, "Error: %ls\n", msg);
		break;
	}
}

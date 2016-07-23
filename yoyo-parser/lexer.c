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

#include "headers/parser.h"

struct {
	wchar_t ch;
	yoperator_t op;
} OPERATORS[] = { 
	{L'*', MultiplyOperator},
	{L'/', DivideOperator},
	{L'%', ModuloOperator},
	{L'+', PlusOperator},
	{L'-', MinusOperator},
	{L'(', OpeningParentheseOperator},
	{L')', ClosingParentheseOperator},
	{L'>', GreaterOperator},
	{L'<', LesserOperator},
    {L'&', AndOperator},
    {L'|', OrOperator},
    {L'^', XorOperator},
    {L'=', AssignOperator},
    {L'!', LogicalNotOperator},
    {L'.', DotOperator},
    {L'[', OpeningBracketOperator},
    {L']', ClosingBracketOperator},
    {L':', ColonOperator},
	{L',', CommaOperator},
	{L'~', NotOperator},
	{L'{', OpeningBraceOperator},
	{L'}', ClosingBraceOperator},
	{L'$', DollarSignOperator},
	{L'?', QueryOperator},
	{L';', SemicolonOperator}	
};
const size_t OPERATORS_LEN = sizeof(OPERATORS) / sizeof(OPERATORS[0]);
/*Keywords and their string representatives*/
struct {
	wchar_t* wstr;
	ykeyword_t kw;
} KEYWORDS[] = { {L"null", NullKeyword},
		{ L"interface", InterfaceKeyword }, { L"if", IfKeyword }, {
		L"else", ElseKeyword }, { L"while", WhileKeyword },
		{ L"do", DoKeyword }, { L"for", ForKeyword }, { L"foreach",
				ForeachKeyword }, { L"pass", PassKeyword }, { L"continue",
				ContinueKeyword }, { L"break", BreakKeyword }, { L"object",
				ObjectKeyword }, { L"var", VarKeyword }, { L"del", DelKeyword },
		{ L"return", ReturnKeyword }, { L"throw", ThrowKeyword }, { L"try",
				TryKeyword }, { L"catch", CatchKeyword }, { L"finally",
				FinallyKeyword }, { L"switch", SwitchKeyword }, { L"case",
				CaseKeyword }, { L"default", DefaultKeyword }, { L"overload",
				OverloadKeyword }, { L"using", UsingKeyword }, { L"with",
				WithKeyword }, { L"def", FunctionKeyword }, };
const size_t KEYWORDS_LEN = sizeof(KEYWORDS) / sizeof(KEYWORDS[0]);

bool isOperator(wchar_t ch) {
	for (size_t i=0;i<OPERATORS_LEN;i++)
		if (OPERATORS[i].ch==ch)
			return true;
	return false;
}

wint_t readwc(ParseHandle* handle) {
	wint_t ch = handle->input->read(handle->input);
	if (ch=='\n') {
		handle->lastCharPos = handle->charPos;
		handle->charPos = 0;
		handle->line++;
	} else
		handle->charPos++;
	return ch;
}
void unreadwc(wint_t ch, ParseHandle* handle) {
	if (ch=='\n') {
		handle->charPos = handle->lastCharPos;
		handle->line--;
	} else
		handle->charPos--;
	handle->input->unread(ch, handle->input);
}

ytoken lex(ParseHandle* handle) {
	ytoken eof = {.type = TokenEOF};

	wint_t ch;
	do {
		ch = readwc(handle);
		if (ch==L'/') {
			wint_t nextc = readwc(handle);
			if (nextc==L'/') {
				do {
					ch = readwc(handle);
				} while (ch!=L'\n'&&ch!=WEOF);
			} else if (nextc=='*') {
				do {
					ch = nextc;
					nextc = readwc(handle);
				} while (ch!=WEOF&&!(ch==L'*'&&nextc==L'/'));
				ch = readwc(handle);
			} else {
				unreadwc(nextc, handle);
			}

		}
	} while (iswspace(ch));

	if (ch==WEOF) {
		return eof;
	}
	uint32_t line = handle->line;
	uint32_t charPos = handle->charPos;

	if (isOperator(ch)) {
		for (size_t i=0;i<OPERATORS_LEN;i++)
			if (OPERATORS[i].ch==ch) {
				ytoken tok = {.type = TokenOperator};
				tok.value.op = OPERATORS[i].op;
				tok.line = line;
				tok.charPos = charPos;
				return tok;
			}
	}

	wchar_t* wstr = NULL;
	size_t wlen = 0;
	if (ch==L'\"') {
		ch = readwc(handle);
		do {
				if (ch==L'\"')
					break;
				wstr = realloc(wstr, sizeof(wchar_t) * (++wlen));
				wstr[wlen - 1] = ch;
				ch = readwc(handle);
			} while (ch!=L'\"'&&ch!=WEOF);
		wstr = realloc(wstr, sizeof(wchar_t) * (++wlen));
		wstr[wlen - 1] = L'\0';
		yconstant_t cnst = {.type = WcsConstant};
		cnst.value.wcs = getSymbol(handle, wstr);
		addConstant(handle, cnst);
		free(wstr);
		ytoken tok = {.type = TokenConstant};
		tok.value.cnst = cnst;
		tok.line = line;
		tok.charPos = charPos;
		return tok;
	} else {
		do {
				wstr = realloc(wstr, sizeof(wchar_t) * (++wlen));
				wstr[wlen - 1] = ch;
				ch = readwc(handle);
				if (isOperator(ch)) {
					unreadwc(ch, handle);
					break;
				}
			} while (!iswspace(ch)&&ch!=WEOF);
		wstr = realloc(wstr, sizeof(wchar_t) * (++wlen));
		wstr[wlen - 1] = L'\0';
	}

	for (size_t i=0;i<KEYWORDS_LEN;i++)
		if (wcscmp(KEYWORDS[i].wstr, wstr)==0) {
			free(wstr);
			ytoken tok = {.type = TokenKeyword};
			tok.value.kw = KEYWORDS[i].kw;
			tok.line = line;
			tok.charPos = charPos;
			return tok;
		}

	int64_t number = 0;
	if (wcslen(wstr)>2&&wcsncmp(wstr, L"0x", 2)==0) {
		for (size_t i=2;i<wcslen(wstr);i++) {
			if (wstr[i]==L'_')
				continue;
			number *= 16;
			if (wstr[i]>=L'0'&&wstr[i]<='9')
				number += wstr[i] - L'0';
			else if (wstr[i]>=L'a'&&wstr[i]<='f')
				number += wstr[i] - L'a';
			else if (wstr[i]>=L'A'&&wstr[i]<='F')
				number += wstr[i] - L'A';
			else {
				number = -1;
				break;
			}
		}
	}
	else if (wcslen(wstr)>2&&wcsncmp(wstr, L"0b", 2)==0) {
		for (size_t i=2;i<wcslen(wstr);i++) {
			if (wstr[i]==L'_')
				continue;
			number *= 2;
			if (wstr[i]==L'1')
				number++;
			else if (wstr[i]==L'0') {}
			else {
				number = -1;
				break;
			}
		}
	}
	else for (size_t i=0;i<wcslen(wstr);i++) {
		if (wstr[i]==L'_')
			continue;
		if (!iswdigit(wstr[i])) {
			number = -1;
			break;
		} else {
			number *= 10;
			number += wstr[i] - L'0';
		}
	}
	if (number!=-1) {
		yconstant_t cnst = {.type = Int64Constant};
		cnst.value.i64 = number;
		addConstant(handle, cnst);
		free(wstr);
		ytoken tok = {.type = TokenConstant};
		tok.value.cnst = cnst;
		tok.line = line;
		tok.charPos = charPos;
		return tok;
	}

	if (wcscmp(wstr, L"true")==0) {
		yconstant_t cnst = {.type = BoolConstant};
		cnst.value.boolean = true;
		addConstant(handle, cnst);
		free(wstr);
		ytoken tok = {.type = TokenConstant};
		tok.value.cnst = cnst;
		tok.line = line;
		tok.charPos = charPos;
		return tok;
	}

	if (wcscmp(wstr, L"false")==0) {
			yconstant_t cnst = {.type = BoolConstant};
			cnst.value.boolean = false;
			addConstant(handle, cnst);
			free(wstr);
			ytoken tok = {.type = TokenConstant};
			tok.value.cnst = cnst;
			tok.line = line;
			tok.charPos = charPos;
			return tok;
		}

	wchar_t* symbol = getSymbol(handle, wstr);
	free(wstr);
	ytoken tok = {.type = TokenIdentifier};
	tok.value.id = symbol;
	tok.line = line;
	tok.charPos = charPos;
	return tok;
}

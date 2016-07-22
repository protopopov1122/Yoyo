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

#include "stringbuilder.h"
#include "yoyoc/token.h"

/*File contains procedures that generate token stream.
 * Lexer remove from input any comments and whitespaces.
 * All other symbols may be: operators, keywords,
 * constants and identifiers.*/

/*Structure that contains chars and operators that
 * match these chars*/
struct {
	wchar_t opchar;
	Operator operator;
} OPERATORS[] = { { L',', CommaToken }, { L'.', DotToken }, { L'{',
		OpeningBraceToken }, { L'}', ClosingBraceToken }, { L'(',
		OpeningParentheseToken }, { L')', ClosingParentheseToken }, { L'[',
		OpeningBracketToken }, { L']', ClosingBracketToken }, { L'>',
		GreaterToken }, { L'<', LesserToken }, { L'+', PlusToken }, { L'-',
		MinusToken }, { L'*', MultiplyToken }, { L'/', DivideToken }, { L'%',
		ModuloToken }, { L'=', AssignToken }, { L'|', OrToken }, { L'&',
		AndToken }, { L'^', XorToken }, { L';', SemicolonToken }, { L':',
		ColonToken }, { L'?', QueryToken }, { L'!', LogicalNotToken }, { L'~',
		NotToken }, { L'$', DollarSignToken }, { L'@', AtToken } };
/*Keywords and their string representatives*/
struct {
	wchar_t* string;
	Keyword keyword;
} KEYWORDS[] = { { L"interface", InterfaceKeyword }, { L"if", IfKeyword }, {
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
/*Check if char is operator*/
bool isCtrlChar(wchar_t ch) {

	size_t length = sizeof(OPERATORS) / sizeof(OPERATORS[0]);
	for (size_t i = 0; i < length; i++)
		if (ch == OPERATORS[i].opchar)
			return true;
	return false;
}

void YToken_free(YToken* token) {
	free(token->string);
	free(token);
}

typedef struct DefaultTokenStream {
	TokenStream stream;
	InputStream* input;
	int32_t line;
	int32_t charPos;
	int32_t lastCharPos;
} DefaultTokenStream;

/*Useful methods to read code and control current line
 * and char position in code*/
wint_t stream_getwc(DefaultTokenStream* ts) {
	wint_t out = ts->input->get(ts->input);
	if (out == L'\n') {
		ts->line++;
		ts->lastCharPos = ts->charPos;
		ts->charPos = 0;
	} else
		ts->charPos++;
	return out;
}
void stream_ungetwc(wint_t ch, DefaultTokenStream* ts) {
	if (ch == L'\n') {
		ts->line--;
		ts->charPos = ts->lastCharPos;
	} else
		ts->charPos--;
	ts->input->unget(ts->input, ch);
}

/*Main lexer merhod. Return next token from input stream.
 * If there isn't any tokens then return NULL*/
YToken* DefaultTokenStream_next(TokenStream* ts, YoyoCEnvironment* env) {
	DefaultTokenStream* stream = (DefaultTokenStream*) ts;
	wint_t ch;

	/*Skip all whitespaces and comments*/
	do {
		ch = stream_getwc(stream);
		if (ch == L'/') {
			wint_t nxtc = stream_getwc(stream);
			if (nxtc == L'/') {
				while (ch != L'\n' && ch != WEOF)
					ch = stream_getwc(stream);
			} else if (nxtc == L'*') {
				while (nxtc != WEOF) {
					ch = nxtc;
					nxtc = stream_getwc(stream);
					if (ch == L'*' && nxtc == L'/') {
						ch = stream_getwc(stream);
						break;
					}
				}
			} else {
				stream_ungetwc(nxtc, stream);
				break;
			}
		}
		if (ch == WEOF)
			return NULL;
	} while (iswspace(ch));

	// Each token store its line and char position
	int32_t tLine = stream->line;
	int32_t tCharPos = stream->charPos;

	StringBuilder* sb = newStringBuilder(L"");
	wchar_t wstr[2];
	wstr[1] = L'\0';

	// If token starts with " then it's string constant
	if (ch == L'\"') {
		do {
			wstr[0] = ch;
			if (ch == L'\\') {
				// Escape characters
				switch (stream_getwc(stream)) {
				case 'n':
					sb->append(sb, L"\n");
					break;
				case 't':
					sb->append(sb, L"\t");
					break;
				case '\"':
					sb->append(sb, L"\"");
					break;
				case '\\':
					sb->append(sb, L"\\");
					break;
				}
			} else
				sb->append(sb, wstr);
			ch = stream_getwc(stream);
		} while (ch != L'\"' && ch != WEOF);
		sb->append(sb, L"\"");
	} /*If token starts with digit then it's number */
	else if (iswdigit(ch)) {
		bool hex = false;
		if (ch == L'0') {
			/*Number may have prefixes:
			 * 0x, 0b or no prefix*/
			wint_t nxtc = stream_getwc(stream);
			if (nxtc == L'x') {
				sb->append(sb, L"0x");
				ch = stream_getwc(stream);
				hex = true;
			} else if (nxtc == L'b') {
				sb->append(sb, L"0b");
				ch = stream_getwc(stream);
			} else {
				stream_ungetwc(nxtc, stream);
			}
		}
		// Accepted hex, dec and bin numbers
		// As well as floating-point
		bool needed = true;
		while (ch != WEOF
				&& (iswdigit(ch) || (hex && ch <= L'f' && ch >= L'a')
						|| (hex && ch <= L'F' && ch >= L'A') || ch == L'_')) {
			wstr[0] = ch;
			sb->append(sb, wstr);
			ch = stream_getwc(stream);
			if (ch == L'.') {
				wint_t next = stream_getwc(stream);
				if (iswdigit(next)) {
					wstr[0] = next;
					sb->append(sb, L".");
					sb->append(sb, wstr);
					ch = stream_getwc(stream);
				} else {
					stream_ungetwc(next, stream);
					stream_ungetwc(ch, stream);
					needed = false;
					break;
				}
			}
		}
		if (needed)
			stream_ungetwc(ch, stream);
	}
	/*Else it's operator, keyword or identifier*/
	else
		while (ch != WEOF && !iswspace(ch)) {
			wstr[0] = ch;
			sb->append(sb, wstr);
			if (isCtrlChar(ch)) {
				break;
			}
			ch = stream_getwc(stream);
			if (isCtrlChar(ch)) {
				stream_ungetwc(ch, stream);
				break;
			}
		}

	/*Now token is read and must be translated to C struct.
	 * Each struct contains original token.*/
	YToken* token = NULL;
	wchar_t* wstr2 = sb->string;
	if (wcslen(wstr2) == 1) {
		wchar_t wchr = wstr2[0];
		size_t length = sizeof(OPERATORS) / sizeof(OPERATORS[0]);
		for (size_t i = 0; i < length; i++)
			if (OPERATORS[i].opchar == wchr) {
				/*Token length is 1 char and
				 * OPERATORS contain this char -> it's operator*/
				YOperatorToken* optkn = malloc(sizeof(YOperatorToken));
				optkn->op = OPERATORS[i].operator;
				optkn->token.type = OperatorToken;
				token = (YToken*) optkn;
			}
	}
	if (token == NULL) {
		size_t length = sizeof(KEYWORDS) / sizeof(KEYWORDS[0]);
		for (size_t i = 0; i < length; i++)
			if (wcscmp(KEYWORDS[i].string, wstr2) == 0) {
				/*Keywords contains this string*/
				Keyword kwrd = KEYWORDS[i].keyword;
				YKeywordToken* kwtkn = malloc(sizeof(YKeywordToken));
				kwtkn->keyword = kwrd;
				kwtkn->token.type = KeywordToken;
				token = (YToken*) kwtkn;
			}
	}
	if (token == NULL) {
		/*Check if it's predefined constant:
		 * true, false or null*/
		if (wcscmp(wstr2, L"true") == 0) {
			YConstantToken* cntkn = malloc(sizeof(YConstantToken));
			cntkn->id = env->bytecode->addBooleanConstant(env->bytecode, true);
			cntkn->token.type = ConstantToken;
			token = (YToken*) cntkn;
		} else if (wcscmp(wstr2, L"false") == 0) {
			YConstantToken* cntkn = malloc(sizeof(YConstantToken));
			cntkn->id = env->bytecode->addBooleanConstant(env->bytecode, false);
			cntkn->token.type = ConstantToken;
			token = (YToken*) cntkn;
		} else if (wcscmp(wstr2, L"null") == 0) {
			YConstantToken* cntkn = malloc(sizeof(YConstantToken));
			cntkn->id = env->bytecode->getNullConstant(env->bytecode);
			cntkn->token.type = ConstantToken;
			token = (YToken*) cntkn;
		}
	}

	if (token == NULL) {
		/*Check if it's integer and process it*/
		uint64_t number = 0;
		bool isnum = true;
		if (wcsncmp(wstr2, L"0x", 2) == 0)
			for (size_t i = 2; i < wcslen(wstr2); i++) {
				if (wstr2[i] == L'_')
					continue;
				if (wstr2[i] >= L'0' && wstr2[i] <= L'9')
					number = number * 16 + (wstr2[i] - L'0');
				else if (wstr2[i] >= L'a' && wstr2[i] <= L'f')
					number = number * 16 + (wstr2[i] - L'a' + 10);
				else if (wstr2[i] >= L'A' && wstr2[i] <= L'F')
					number = number * 16 + (wstr2[i] - L'A' + 10);
				else {
					isnum = false;
					break;
				}
			}
		else if (wcsncmp(wstr2, L"0b", 2) == 0)
			for (size_t i = 2; i < wcslen(wstr2); i++) {
				if (wstr2[i] == L'_')
					continue;
				if (wstr2[i] == L'0')
					number <<= 1;
				else if (wstr2[i] == L'1')
					number = (number << 1) + 1;
				else {
					isnum = false;
					break;
				}
			}

		else
			for (size_t i = 0; i < wcslen(wstr2); i++) {
				if (wstr2[i] == L'_')
					continue;
				else if (!iswdigit(wstr2[i])) {
					isnum = false;
					break;
				} else {
					char num = wstr2[i] - L'0';
					number *= 10;
					number += num;
				}
			}
		if (isnum) {
			/*If it's integer then save result as constant*/
			YConstantToken* cntkn = malloc(sizeof(YConstantToken));
			cntkn->id = env->bytecode->addIntegerConstant(env->bytecode,
					number);
			cntkn->token.type = ConstantToken;
			token = (YToken*) cntkn;
		}
	}
	if (token == NULL && wstr2[0] == L'"') {
		/*Check if it's string*/
		wchar_t* strcn = malloc(sizeof(wchar_t) * (wcslen(wstr2) - 1));
		memcpy(strcn, &wstr2[1], sizeof(wchar_t) * (wcslen(wstr2) - 2));
		strcn[wcslen(wstr2) - 2] = L'\0';

		YConstantToken* cntkn = malloc(sizeof(YConstantToken));
		cntkn->id = env->bytecode->addStringConstant(env->bytecode, strcn);
		cntkn->token.type = ConstantToken;
		token = (YToken*) cntkn;

		free(strcn);

	}
	if (token == NULL) {
		/*Check if it's floating-point number*/
		bool isfp = true;
		for (size_t i = 0; i < wcslen(wstr2); i++)
			if (!iswdigit(wstr2[i]) && wstr2[i] != L'.') {
				isfp = false;
				break;
			}
		if (isfp) {
			/*If it's fp then process and save it*/
			YConstantToken* cntkn = malloc(sizeof(YConstantToken));
			char* mbsstr = calloc(1, sizeof(wchar_t) * (wcslen(wstr2) + 1));
			wcstombs(mbsstr, wstr2, wcslen(wstr2));
			for (size_t i = 0; i < strlen(mbsstr); i++)
				if (mbsstr[i] == '.')
					mbsstr[i] = ',';
			cntkn->id = env->bytecode->addFloatConstant(env->bytecode,
					atof(mbsstr));
			free(mbsstr);
			cntkn->token.type = ConstantToken;
			token = (YToken*) cntkn;
		}
	}
	if (token == NULL) {
		/*Else it's identifier*/
		YIdentifierToken* itoken = malloc(sizeof(YIdentifierToken));
		itoken->token.type = IdentifierToken;
		itoken->id = env->bytecode->getSymbolId(env->bytecode, wstr2);
		token = (YToken*) itoken;
	}

	/*Add to token all information:
	 * line, start char position, raw string*/
	token->string = calloc(1, sizeof(wchar_t) * (wcslen(sb->string) + 1));
	wcscpy(token->string, sb->string);
	token->free = YToken_free;
	token->line = tLine;
	token->charPos = tCharPos;

	sb->free(sb);

	return token;
}

/*Methods that implement TokenStream interface*/
void DefaultTokenStream_close(TokenStream* ts, YoyoCEnvironment* env) {
	DefaultTokenStream* stream = (DefaultTokenStream*) ts;
	stream->input->close(stream->input);
	free(ts);
}

int32_t DefaultTokenStream_line(TokenStream* ts) {
	DefaultTokenStream* stream = (DefaultTokenStream*) ts;
	return stream->line;
}

int32_t DefaultTokenStream_charPos(TokenStream* ts) {
	DefaultTokenStream* stream = (DefaultTokenStream*) ts;
	return stream->charPos;
}

/*Create token stream from input stream*/
TokenStream* ylex(YoyoCEnvironment* env, InputStream* is, wchar_t* fileName) {
	if (is == NULL)
		return NULL;
	DefaultTokenStream* stream = malloc(sizeof(DefaultTokenStream));
	stream->stream.next = DefaultTokenStream_next;
	stream->stream.close = DefaultTokenStream_close;
	stream->input = is;
	stream->line = 1;
	stream->charPos = 1;
	stream->lastCharPos = 1;
	stream->stream.fileName = fileName;
	stream->stream.line = DefaultTokenStream_line;
	stream->stream.charPosition = DefaultTokenStream_charPos;
	return (TokenStream*) stream;
}

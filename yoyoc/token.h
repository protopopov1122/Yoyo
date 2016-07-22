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

#ifndef YOYOC_TOKEN_H
#define YOYOC_TOKEN_H

#include "yoyo-runtime.h"
#include "yoyoc.h"

typedef struct YToken {
	enum {
		IdentifierToken, ConstantToken, OperatorToken, KeywordToken
	} type;
	wchar_t* string;
	void (*free)(struct YToken*);
	int32_t line;
	int32_t charPos;
} YToken;

typedef struct YIdentifierToken {
	YToken token;
	int32_t id;
} YIdentifierToken;

typedef struct YConstantToken {
	YToken token;
	int32_t id;
} YConstantToken;

typedef enum {
	OpeningBraceToken,
	ClosingBraceToken,
	OpeningParentheseToken,
	ClosingParentheseToken,
	OpeningBracketToken,
	ClosingBracketToken,
	PlusToken,
	MinusToken,
	MultiplyToken,
	DivideToken,
	ModuloToken,
	AndToken,
	OrToken,
	XorToken,
	NotToken,
	LogicalNotToken,
	LesserToken,
	GreaterToken,
	DotToken,
	QueryToken,
	ColonToken,
	SemicolonToken,
	AssignToken,
	CommaToken,
	DollarSignToken,
	AtToken
} Operator;
typedef struct YOperatorToken {
	YToken token;
	Operator op;
} YOperatorToken;

typedef enum {
	InterfaceKeyword,
	IfKeyword,
	ElseKeyword,
	WhileKeyword,
	DoKeyword,
	ForKeyword,
	ForeachKeyword,
	BreakKeyword,
	ContinueKeyword,
	PassKeyword,
	ObjectKeyword,
	VarKeyword,
	DelKeyword,
	ReturnKeyword,
	ThrowKeyword,
	TryKeyword,
	CatchKeyword,
	FinallyKeyword,
	SwitchKeyword,
	CaseKeyword,
	DefaultKeyword,
	OverloadKeyword,
	UsingKeyword,
	WithKeyword,
	FunctionKeyword
} Keyword;
typedef struct YKeywordToken {
	YToken token;
	Keyword keyword;
} YKeywordToken;

typedef struct TokenStream {
	YToken* (*next)(struct TokenStream*, YoyoCEnvironment*);
	void (*close)(struct TokenStream*, YoyoCEnvironment*);
	wchar_t* fileName;
	int32_t (*line)(struct TokenStream*);
	int32_t (*charPosition)(struct TokenStream*);
} TokenStream;

TokenStream* ylex(YoyoCEnvironment*, InputStream*, wchar_t*);

#endif

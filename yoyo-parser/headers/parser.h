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

#ifndef HEADERS_PARSER_H_
#define HEADERS_PARSER_H_

#include "base.h"
#include "node.h"

typedef enum {
	MultiplyOperator,
	DivideOperator,
	ModuloOperator,
	PlusOperator,
	MinusOperator,
	OpeningParentheseOperator,
	ClosingParentheseOperator,
	GreaterOperator,
	LesserOperator,
	AndOperator,
	OrOperator,
	XorOperator,
	AssignOperator,
	LogicalNotOperator,
	DotOperator,
	OpeningBracketOperator,
	ClosingBracketOperator,
	CommaOperator,
	ColonOperator,
	NotOperator,
	OpeningBraceOperator,
	ClosingBraceOperator,
	DollarSignOperator,
	QueryOperator,
	SemicolonOperator
} yoperator_t;

typedef enum {
	NullKeyword,
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
	FunctionKeyword,
	MethodKeyword
} ykeyword_t;

typedef struct ytoken {
	enum {
		TokenIdentifier, TokenConstant, TokenOperator, TokenKeyword, TokenEOF
	} type;
	union {
		wchar_t* id;
		yconstant_t cnst;
		yoperator_t op;
		ykeyword_t kw;
	} value;
	uint32_t line;
	uint32_t charPos;
} ytoken;

typedef struct ParseHandle ParseHandle;

typedef struct GrammarRule {
	bool (*validate)(struct ParseHandle*);
	YNode* (*reduce)(struct ParseHandle*);
} GrammarRule;

typedef struct Grammar {
	GrammarRule constant;
	GrammarRule identifier;
	GrammarRule array;
	GrammarRule overload;
	GrammarRule lambda;
	GrammarRule object;
	GrammarRule interface;
	GrammarRule factor;
	GrammarRule reference;
	GrammarRule unary;
	GrammarRule power;
	GrammarRule mul_div;
	GrammarRule add_sub;
	GrammarRule bitshift;
	GrammarRule bitwise;
	GrammarRule comparison;
	GrammarRule logical_not;
	GrammarRule logical_ops;
	GrammarRule expr;
	GrammarRule expression;
	GrammarRule statement;
	GrammarRule function;
	GrammarRule root;
} Grammar;

typedef struct ParseHandle {
	InputStream* input;
	FILE* error_stream;
	wchar_t* fileName;

	wchar_t** symbols;
	size_t symbols_size;

	yconstant_t* constants;
	size_t constants_size;

	uint32_t line;
	uint32_t charPos;
	uint32_t lastCharPos;

	ytoken tokens[4];

	Grammar grammar;

	bool error_flag;
} ParseHandle;

wchar_t* getSymbol(ParseHandle*, wchar_t*);
void addConstant(ParseHandle*, yconstant_t);
ytoken shift(ParseHandle*);
ytoken lex(ParseHandle*);
YNode* parse(ParseHandle*);

YNode* optimize_node(YNode*);

#define NewValidate(name) bool name(ParseHandle* handle)
#define NewReduce(name) YNode* name(ParseHandle* handle)
#define NewRule(grammar, name, v, r) grammar->name.validate = v;\
									 grammar->name.reduce = r;
#define Validate(name, handle) (handle->grammar.name.validate(handle))
#define AssertToken(token, t) (token.type==t)
#define AssertOperator(token, oper) (AssertToken(token, TokenOperator)&&\
									token.value.op==oper)
#define AssertKeyword(token, k) (AssertToken(token, TokenKeyword)&&\
									token.value.kw==k)
#define PrintError(mess, handle) {\
                                            fprintf(handle->error_stream, "%ls", mess);\
                                            if (handle->tokens[0].type!=TokenEOF)\
                                                fprintf(handle->error_stream, " at '%ls'(%" PRIu32 ":%" PRIu32 ")",\
                                                	handle->fileName,\
                                                	handle->tokens[0].line,\
                                                    handle->tokens[0].charPos);\
                                            else fprintf(handle->error_stream," at '%ls'(%" PRId32 ":%" PRId32 ")",\
                                            		handle->fileName,\
                                            		handle->line,\
                                                    handle->charPos);\
                                            fprintf(handle->error_stream, "\n");\
                                }
#define ParseError(mess, stmt, handle) {\
                                            handle->error_flag = true;\
                                            PrintError(mess, handle);\
                                            stmt;\
                                            return NULL;\
                                        }
#define ExtractCoords(file, line, charPos, parser) uint32_t line = parser->tokens[0].line;\
											 uint32_t charPos = parser->tokens[0].charPos;\
											 wchar_t* file = parser->fileName;
#define SetCoords(node, f, l, cp) if (node!=NULL) { if (node->fileName == NULL) node->fileName = f;\
									if (node->line == -1) node->line = l;\
									if (node->charPos == -1) node->charPos = cp;}
#define Expect(cond, mess, stmt, parser) if (!(cond)) ParseError(mess, stmt, parser);
#define ExpectToken(token, tp, mess, stmt, parser) Expect(token.type==tp,\
                                                          mess, stmt, parser);
#define ExpectOperator(token, o, mess, stmt, parser) Expect(token.type==TokenOperator&&\
														token.value.op== o,\
                                                       mess, stmt, parser);
#define ExpectKeyword(token, k, mess, stmt, handle) Expect(token.type==TokenKeyword&&\
                                                        	token.value.kw == k,\
                                                         mess, stmt, handle);
#define ExpectNode(name, mess, stmt, handle) Expect(handle->grammar.name.validate(handle),\
                                                         mess, stmt, handle);
#define Reduce(dest, name, stmt, handle) {\
                                                *(dest) = handle->grammar.name.reduce(handle);\
                                                if (*(dest)==NULL) {\
                                                    stmt;\
                                                    return NULL;\
                                                }\
                                            }
#define ExpectReduce(dest, name, mess, stmt, handle) {\
                                                        ExpectNode(name, mess, stmt, handle);\
                                                        Reduce(dest, name, stmt, handle);\
                                                        }

#endif

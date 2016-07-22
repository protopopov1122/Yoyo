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

#ifndef YOYOC_YPARSE_H
#define YOYOC_YPARSE_H

#include "yoyoc.h"
#include "token.h"
#include "node.h"

#define NewValidate(name) bool name(YParser* parser, YoyoCEnvironment* env)
#define NewReduce(name) YNode* name(YParser* parser, YoyoCEnvironment* env)
#define NewRule(grammar, name, v, r) grammar->name.validate = v;\
															 grammar->name.reduce = r;
#define AssertToken(token, t) (token!=NULL&&token->type==t)
#define AssertOperator(token, o) (AssertToken(token, OperatorToken)&&((YOperatorToken*) token)->op==o)
#define AssertKeyword(token, kw) (AssertToken(token, KeywordToken)&&((YKeywordToken*) token)->keyword==kw)
#define GetOperator(token) (AssertToken(token, OperatorToken) ? ((YOperatorToken*) token)->op : -1)

#define NewNode(ptr, nodeStruct, nodeType, freePtr) *(ptr) = malloc(sizeof(nodeStruct));\
                                                    (*(ptr))->node.type = nodeType;\
													(*(ptr))->node.free = (void (*)(YNode*)) freePtr;\
													(*(ptr))->node.line = -1;\
													(*(ptr))->node.charPos = -1;\
													(*(ptr))->node.fname = -1;

#define PrintError(mess, parser) {\
                                            fprintf(parser->err_file, "%ls", mess);\
                                            if (parser->tokens[0]!=NULL)\
                                                fprintf(parser->err_file, " at %ls(%" PRIu32 ":%" PRIu32 ")",\
                                                	parser->stream->fileName,\
                                                	parser->tokens[0]->line,\
                                                    parser->tokens[0]->charPos);\
                                            else fprintf(parser->err_file," at %ls(%" PRId32 ":%" PRId32 ")",\
                                            		parser->stream->fileName,\
                                            		parser->stream->line(parser->stream),\
                                                        parser->stream->charPosition(parser->stream));\
                                            fprintf(parser->err_file, "\n");\
                                }
#define ParseError(mess, stmt, parser) {\
                                            parser->err_flag = true;\
                                            PrintError(mess, parser);\
                                            stmt;\
                                            return NULL;\
                                        }
#define Expect(cond, mess, stmt, parser) if (!(cond)) ParseError(mess, stmt, parser);
#define ExpectToken(token, tp, mess, stmt, parser) Expect(token!=NULL&&token->type==tp,\
                                                          mess, stmt, parser);
#define ExpectOperator(token, o, mess, stmt, parser) Expect(token!=NULL&&token->type==OperatorToken&&\
                                                       ((YOperatorToken*) token)->op == o,\
                                                       mess, stmt, parser);
#define ExpectKeyword(token, kw, mess, stmt, parser) Expect(token!=NULL&&token->type==KeywordToken&&\
                                                        ((KeywordToken*) token)->keyword == kw,\
                                                         mess, stmt, parser);
#define ExpectNode(name, mess, stmt, parser, env) Expect(parser->grammar.name.validate(parser, env),\
                                                         mess, stmt, parser);
#define Reduce(dest, name, stmt, parser, env) {\
                                                *(dest) = parser->grammar.name.reduce(parser, env);\
                                                if (*(dest)==NULL) {\
                                                    stmt;\
                                                    return NULL;\
                                                }\
                                            }
#define ExpectReduce(dest, name, mess, stmt, parser, env) {\
                                                        ExpectNode(name, mess, stmt, parser, env);\
                                                        Reduce(dest, name, stmt, parser, env);\
                                                        }

#define ExtractCoords(file, line, charPos, parser, env) uint32_t line = parser->tokens[0]->line;\
											 uint32_t charPos = parser->tokens[0]->charPos;\
											 int32_t file = \
											 	 env->bytecode->getSymbolId(\
											 			 env->bytecode,\
														 parser->stream->fileName);
#define SetCoords(node, f, l, cp) if (node!=NULL) {node->fname = f;\
									node->line = l;\
									node->charPos = cp;}

typedef struct YParser YParser;

typedef struct GrammarRule {
	bool (*validate)(YParser*, YoyoCEnvironment*);
	YNode* (*reduce)(YParser*, YoyoCEnvironment*);
} GrammarRule;

typedef struct Grammar {
	GrammarRule constant;
	GrammarRule identifier;

	GrammarRule object;
	GrammarRule array;
	GrammarRule overload;
	GrammarRule lambda;
	GrammarRule interface;
	GrammarRule declaration;
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

typedef struct YParser {
	Grammar grammar;
	TokenStream* stream;

	YToken* tokens[4];
	YToken* (*next)(YParser*, YoyoCEnvironment*);

	bool err_flag;
	YNode* root;

	FILE* err_file;
} YParser;

YNode* yparse(YoyoCEnvironment*, TokenStream*, YParser*);

#endif

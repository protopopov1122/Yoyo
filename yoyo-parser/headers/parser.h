#ifndef HEADERS_PARSER_H_
#define HEADERS_PARSER_H_

#include "base.h"
#include "node.h"

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
} yoperator_t;

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
} ykeyword_t;

typedef struct ytoken {
	enum {
		TokenIdentifier, TokenConstant,
		TokenOperator, TokenKeyword,
		TokenEOF
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
	GrammarRule logical;
	GrammarRule expr;
	GrammarRule expression;
	GrammarRule statement;
	GrammarRule function;
	GrammarRule root;
} Grammar;

typedef struct ParseHandle {
	FILE* input;
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
ytoken lex(ParseHandle*);
YNode* parse(ParseHandle*);

#define NewValidate(name) bool name(ParseHandle* handle)
#define NewReduce(name) YNode* name(ParseHandle* handle)
#define NewRule(grammar, name, v, r) grammar->name.validate = v;\
									 grammar->name.reduce = r;
#define AssertToken(token, t) (token.type==t)
#define AssertOperator(token, oper) (AssertToken(token, TokenOperator)&&\
									token.value.op==oper)
#define AssertKeyword(token, k) (AssertToken(token, TokenKeyword)&&\
									token.value.kw==k)
#define PrintError(mess, handle) {\
                                            fprintf(handle->error_stream, "%ls", mess);\
                                            if (handle->tokens[0].type!=TokenEOF)\
                                                fprintf(handle->error_stream, " at %ls(%" PRIu32 ":%" PRIu32 ")",\
                                                	handle->fileName,\
                                                	handle->tokens[0].line,\
                                                    handle->tokens[0].charPos);\
                                            else fprintf(handle->error_stream," at %ls(%" PRId32 ":%" PRId32 ")",\
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
#define SetCoords(node, f, l, cp) if (node!=NULL) {node->fileName = f;\
									node->line = l;\
									node->charPos = cp;}
#define Expect(cond, mess, stmt, parser) if (!(cond)) ParseError(mess, stmt, parser);
#define ExpectToken(token, tp, mess, stmt, parser) Expect(token.type==tp,\
                                                          mess, stmt, parser);
#define ExpectOperator(token, o, mess, stmt, parser) Expect(token.type==TokenOperator&&\
														token.value.op== o,\
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

#endif

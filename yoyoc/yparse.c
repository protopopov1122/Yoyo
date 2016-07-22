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

#include "yoyoc/yparse.h"
#include "bytecode.h"

/*File contains procedures to  parse token stream and create
 * AST. It's LL-parser. Main parser structure contains
 * Grammar structure which has two function pointers
 * on each grammar rule:
 * 	validate - checks visible tokens and returns
 * 		if this rule fits current state
 * 	reduce - reduces tokens from token stream
 * 		to new AST node and returns it
 * 	Each node contains information about
 * 	source code file, line and char position
 * 	where it's located. There are a few predefined
 * 	macros to reduce size of code(See 'yparse.h').
 * 	If during reduction there is an error then
 * 	procedure frees allocated resources and returns NULL.*/

/*If next token is constant.*/
NewValidate(Constant_validate) {
	return AssertToken(parser->tokens[0], ConstantToken);
}
NewReduce(Constant_reduce) {
	int32_t cid = ((YConstantToken*) parser->tokens[0])->id;
	ExtractCoords(file, line, charPos, parser, env);
	parser->next(parser, env);
	YNode* node = newConstantNode(cid);
	SetCoords(node, file, line, charPos);
	return node;
}
/*If next token is identifier*/
NewValidate(Identifier_validate) {
	return AssertToken(parser->tokens[0], IdentifierToken);
}
NewReduce(Identifier_reduce) {
	int32_t id = ((YIdentifierToken*) parser->tokens[0])->id;
	ExtractCoords(file, line, charPos, parser, env);
	parser->next(parser, env);
	YNode* node = newIdentifierReferenceNode(id);
	SetCoords(node, file, line, charPos);
	return node;
}

/*If next token is '$' then it's lambda.*/
NewValidate(Lambda_validate) {
	return AssertOperator(parser->tokens[0], DollarSignToken);
}
NewReduce(Lambda_reduce) {
	ExtractCoords(file, line, charPos, parser, env);
	parser->next(parser, env);
	size_t length = 0;
	int32_t* args = NULL;
	YNode** argTypes = NULL;
	bool vararg = false;
	ExpectOperator(parser->tokens[0], OpeningParentheseToken, L"Expected '(", ;,
			parser);
	parser->next(parser, env);
	/*Parsing lambda arguments*/
	while (!AssertOperator(parser->tokens[0], ClosingParentheseToken)) {
		if (AssertOperator(parser->tokens[0], QueryToken)) {
			/*It's vararg argument. It must be last*/
			vararg = true;
			parser->next(parser, env);
		}
		/*Get argument name*/
		ExpectToken(parser->tokens[0], IdentifierToken, L"Expected identifier",
				{ free(args); for (size_t i=0;i<length;i++) if (argTypes[i]!=NULL) argTypes[i]->free(argTypes[i]); free(argTypes); },
				parser);
		length++;
		args = realloc(args, sizeof(int32_t) * length);
		argTypes = realloc(argTypes, sizeof(YNode*) * length);
		args[length - 1] = ((YIdentifierToken*) parser->tokens[0])->id;
		argTypes[length - 1] = NULL;
		parser->next(parser, env);
		/*Check if argument has type*/
		if (AssertOperator(parser->tokens[0], MinusToken)&&
		AssertOperator(parser->tokens[1], GreaterToken)) {
			parser->next(parser, env);
			parser->next(parser, env);
			ExpectReduce(&argTypes[length - 1], expr, L"Expected expression",
					{ free(args); for (size_t i=0;i<length;i++) if (argTypes[i]!=NULL) argTypes[i]->free(argTypes[i]); free(argTypes); },
					parser, env);
		}

		/*Next token must be ',' or ')'*/
		Expect(
				AssertOperator(parser->tokens[0], CommaToken) ||AssertOperator(parser->tokens[0], ClosingParentheseToken),
				L"Expected ')' or ','",
				{ free(args); for (size_t i=0;i<length;i++) if (argTypes[i]!=NULL) argTypes[i]->free(argTypes[i]); free(argTypes); },
				parser);
		Expect(
				!(vararg&&!AssertOperator(parser->tokens[0], ClosingParentheseToken)),
				L"Vararg must be last, argument",
				{ free(args); for (size_t i=0;i<length;i++) if (argTypes[i]!=NULL) argTypes[i]->free(argTypes[i]); free(argTypes); },
				parser);
		if (AssertOperator(parser->tokens[0], CommaToken))
			parser->next(parser, env);
	}
	parser->next(parser, env);
	/*Check if lambda has return type*/
	YNode* retType = NULL;
	if (AssertOperator(parser->tokens[0], MinusToken)&&
	AssertOperator(parser->tokens[1], GreaterToken)) {
		parser->next(parser, env);
		parser->next(parser, env);
		ExpectReduce(&retType, expr, L"Expected expression",
				{ free(args); for (size_t i=0;i<length;i++) if (argTypes[i]!=NULL) argTypes[i]->free(argTypes[i]); free(argTypes); },
				parser, env);
	}
	/*Get lambda body*/
	YNode* body;
	ExpectReduce(&body, statement, L"Expected statement",
			{ free(args); for (size_t i=0;i<length;i++) if (argTypes[i]!=NULL) argTypes[i]->free(argTypes[i]); free(argTypes); if (retType!=NULL) retType->free(retType); },
			parser, env);
	/*Build lambda node*/
	YNode* out = newLambdaNode(args, argTypes, length, vararg, retType, body);
	SetCoords(out, file, line, charPos);
	return out;
}
/*If next token is keyword 'overload' then it's
 * overloaded lambda*/
NewValidate(Overload_validate) {
	return AssertKeyword(parser->tokens[0], OverloadKeyword);
}
NewReduce(Overload_reduce) {
	ExtractCoords(file, line, charPos, parser, env);
	parser->next(parser, env);
	parser->next(parser, env);
	size_t length = 0;
	YNode** lmbds = NULL;
	YNode* defLmbd = NULL;
	/*Code to free allocated resources if there will be an error*/
#define freestmt {\
                        for (size_t i=0;i<length;i++)\
                            lmbds[i]->free(lmbds[i]);\
                        free(lmbds);\
                        if (defLmbd!=NULL)\
                            defLmbd->free(defLmbd);\
                   }
	/*Get lambdas*/
	while (!AssertOperator(parser->tokens[0], ClosingParentheseToken)) {
		if (defLmbd == NULL && AssertKeyword(parser->tokens[0], DefaultKeyword)) {
			/*It's default lambda*/
			parser->next(parser, env);
			/*Get argument name*/
			ExpectToken(parser->tokens[0], IdentifierToken,
					L"Expects identifier", freestmt, parser);
			int32_t arg = ((YIdentifierToken*) parser->tokens[0])->id;
			parser->next(parser, env);
			int32_t* args = malloc(sizeof(int32_t));
			args[0] = arg;
			/*Get lambda body*/
			YNode* body;
			ExpectReduce(&body, statement, L"Expected statement",
					{freestmt; free(args);}, parser, env);
			YNode** argTypes = calloc(1, sizeof(YNode*));
			defLmbd = newLambdaNode(args, argTypes, 1, false, NULL, body);
		} else if (parser->grammar.expr.validate(parser, env)) {
			/*Get next lambda*/
			length++;
			lmbds = realloc(lmbds, sizeof(YNode*) * length);
			ExpectReduce(&lmbds[length - 1], expr, L"Expected expression",
					freestmt, parser, env);
		} else
			ParseError(L"Expected lambda or 'default'", freestmt, parser);
	}
	parser->next(parser, env);
#undef freestmt

	/*Build new overloaded lambda node*/
	YNode* out = newOverloadNode(lmbds, length, defLmbd);
	SetCoords(out, file, line, charPos);
	return out;
}
/*If next keyword is 'object' then it's object definition*/
NewValidate(Object_validate) {
	return AssertKeyword(parser->tokens[0], ObjectKeyword);
}
NewReduce(Object_reduce) {
	ExtractCoords(file, line, charPos, parser, env);
	YObjectNode* obj = (YObjectNode*) newObjectNode(NULL, NULL, 0, NULL, 0);
	parser->next(parser, env);
	/*Code to free allocated if error occurred*/
#define freestmt {\
        for (size_t i=0;i<obj->fields_length;i++) {\
            obj->fields[i].value->free(obj->fields[i].value);\
        }\
        free(obj->fields);\
        if (obj->parent!=NULL) obj->parent->free(obj->parent);\
        free(obj);\
    }
	if (AssertOperator(parser->tokens[0], ColonToken)) {
		/*Get object parent if available*/
		parser->next(parser, env);
		ExpectReduce(&obj->parent, expression, L"Exprected expression",
				freestmt, parser, env);
	}
	/*If object has body then parse it*/
	if (AssertOperator(parser->tokens[0], OpeningBraceToken)) {
		parser->next(parser, env);
		while (!AssertOperator(parser->tokens[0], ClosingBraceToken)) {
			if (parser->grammar.function.validate(parser, env)) {
				/*Parse object method*/
				YNode* nd;
				Reduce(&nd, function, freestmt, parser, env);
				YFunctionNode* func = (YFunctionNode*) nd;
				/*Check if object contains methods with
				 * the same name. If contains then add overload
				 * then with current*/
				for (size_t i = 0; i < obj->methods_length; i++) {
					if (obj->methods[i].id == func->name) {
						obj->methods[i].count++;
						obj->methods[i].lambda = realloc(obj->methods[i].lambda,
								sizeof(YLambdaNode*) * obj->methods[i].count);
						obj->methods[i].lambda[obj->methods[i].count - 1] =
								func->lambda;
						free(func);
						func = NULL;
						break;
					}
				}
				/*If there is no methods with the same name
				 * then add this*/
				if (func != NULL) {
					obj->methods_length++;
					obj->methods = realloc(obj->methods,
							sizeof(YFunctionBlock) * obj->methods_length);
					obj->methods[obj->methods_length - 1].id = func->name;
					obj->methods[obj->methods_length - 1].count = 1;
					obj->methods[obj->methods_length - 1].lambda = malloc(
							sizeof(YLambdaNode*));
					obj->methods[obj->methods_length - 1].lambda[0] =
							func->lambda;
					free(func);
					func = NULL;
				}
			} else {
				/*It must be field definition.
				 * Get field name.*/
				ExpectToken(parser->tokens[0], IdentifierToken,
						L"Expected identifier or '}'", freestmt, parser);
				int32_t id = ((YIdentifierToken*) parser->tokens[0])->id;
				parser->next(parser, env);
				YNode* type = NULL;
				/*If field has defined type then parse it*/
				if (AssertOperator(parser->tokens[0], MinusToken)&&
				AssertOperator(parser->tokens[1], GreaterToken)) {
					parser->next(parser, env);
					parser->next(parser, env);
					ExpectReduce(&type, expression, L"Expected expression",
							freestmt, parser, env);
				}
				ExpectOperator(parser->tokens[0], ColonToken, L"Expected ':'",
						freestmt, parser);
				parser->next(parser, env);
				/*Get field value and add field to object*/
				YNode* value;
				ExpectReduce(&value, expression, L"Expected expression",
						freestmt, parser, env);
				obj->fields_length++;
				obj->fields = realloc(obj->fields,
						obj->fields_length * sizeof(ObjectNodeField));
				obj->fields[obj->fields_length - 1].id = id;
				obj->fields[obj->fields_length - 1].value = value;
				obj->fields[obj->fields_length - 1].type = type;
			}
		}
		parser->next(parser, env);
	}
#undef freestmt
	/*Build object node*/
	YNode* out = (YNode*) obj;
	SetCoords(out, file, line, charPos);
	return out;
}
/*Procedures to build different types of arrays.
 * It's array definition if it starts with '['*/
NewValidate(Array_validate) {
	return AssertOperator(parser->tokens[0], OpeningBracketToken);
}
NewReduce(PArray_reduce) {
	ExtractCoords(file, line, charPos, parser, env);
	parser->next(parser, env);
	if (AssertOperator(parser->tokens[0], ClosingBracketToken)) {
		/*Array is empty*/
		parser->next(parser, env);
		YNode* node = newFilledArray(NULL, 0);
		SetCoords(node, file, line, charPos);
		return node;
	} else {
		/*Get first array element*/
		YNode* base;
		ExpectReduce(&base, expr, L"Expected expression", ;, parser, env);
		if (AssertOperator(parser->tokens[0], SemicolonToken)) {
			/*It's autogenerated runtime. First element
			 * is array element but next is element count*/
			parser->next(parser, env);
			/*Get element count*/
			YNode* count;
			ExpectReduce(&count, expr, L"Expected expression", base->free(base),
					parser, env);
			ExpectOperator(parser->tokens[0], ClosingBracketToken,
					L"Expected ']'", {
						base->free(base)
						;
						count->free(count)
						;
					}, parser);
			parser->next(parser, env);
			/*Return autogenerated array node*/
			YNode* out = newGeneratedArray(base, count);
			SetCoords(out, file, line, charPos);
			return out;
		} else {
			/*It's array definition*/
			size_t length = 1;
			YNode** array = malloc(sizeof(YNode*) * length);
			/*Save first element*/
			array[0] = base;
			/*Code to free allocated resources if error occurred*/
#define freestmt {\
                for (size_t i=0;i<length;i++) array[i]->free(array[i]);\
                free(array);\
			}
			Expect(
					AssertOperator(parser->tokens[0], CommaToken)||AssertOperator(parser->tokens[0], ClosingBracketToken),
					L"Expected ']' or ','", freestmt, parser);
			if (AssertOperator(parser->tokens[0], CommaToken))
				parser->next(parser, env);
			while (!AssertOperator(parser->tokens[0], ClosingBracketToken)) {
				/*Get next elements*/
				YNode* nd;
				ExpectReduce(&nd, expr, L"Expected expression", freestmt,
						parser, env);
				length++;
				array = realloc(array, sizeof(YNode*) * length);
				array[length - 1] = nd;
				Expect(
						AssertOperator(parser->tokens[0], CommaToken)||AssertOperator(parser->tokens[0], ClosingBracketToken),
						L"Expected ']' or ','", freestmt, parser);
				if (AssertOperator(parser->tokens[0], CommaToken))
					parser->next(parser, env);
			}
			parser->next(parser, env);
#undef freestmt
			/*Build AST node and return it*/
			YNode* out = newFilledArray(array, length);
			SetCoords(out, file, line, charPos);
			return out;
		}
	}
}

/*If next token is 'interface' then it's interface definition*/
NewValidate(Interface_validate) {
	return AssertKeyword(parser->tokens[0], InterfaceKeyword);
}
NewReduce(Interface_reduce) {
	ExtractCoords(file, line, charPos, parser, env);
	parser->next(parser, env);
	YNode** parents = NULL;
	size_t parent_count = 0;
	if (AssertOperator(parser->tokens[0], ColonToken)) {
		/*Get interface parent list*/
		parser->next(parser, env);
		YNode* p;
		/*Get first parent*/
		ExpectReduce(&p, expr, L"Expected expression", ;, parser, env);
		parents = realloc(parents, sizeof(YNode*) * (++parent_count));
		parents[0] = p;
		while (AssertOperator(parser->tokens[0], CommaToken)) {
			/*Get next parents if available*/
			parser->next(parser, env);
			ExpectReduce(&p, expr, L"Expected expression", {
				for (size_t i = 0; i < parent_count; i++)
					parents[i]->free(parents[i])
					;
				free(parents)
				;
			}, parser, env);
			parents = realloc(parents, sizeof(YNode*) * (++parent_count));
			parents[parent_count - 1] = p;
		}
	}

	int32_t* ids = NULL;
	YNode** types = NULL;
	size_t attr_count = 0;
	/*Get interface body if available*/
	if (AssertOperator(parser->tokens[0], OpeningBraceToken)) {
		parser->next(parser, env);
		/*Code to free allocated resources if error occurred*/
#define freestmt {\
				for (size_t i=0;i<parent_count;i++)\
					parents[i]->free(parents[i]);\
				free(parents);\
				free(ids);\
				for (size_t i=0;i<attr_count;i++)\
					types[i]->free(types[i]);\
				free(types);\
		}
		while (!AssertOperator(parser->tokens[0], ClosingBraceToken)) {
			/*Get field name*/
			ExpectToken(parser->tokens[0], IdentifierToken,
					L"Expected identifier or '}'", freestmt, parser);
			int32_t id = ((YIdentifierToken*) parser->tokens[0])->id;
			parser->next(parser, env);
			ExpectOperator(parser->tokens[0], ColonToken, L"Expected ':'",
					freestmt, parser);
			parser->next(parser, env);
			/*Get field type*/
			YNode* n;
			ExpectReduce(&n, expression, L"Expected expression", freestmt,
					parser, env);
			attr_count++;
			/*Save name and type*/
			ids = realloc(ids, sizeof(int32_t) * attr_count);
			types = realloc(types, sizeof(YNode*) * attr_count);
			ids[attr_count - 1] = id;
			types[attr_count - 1] = n;
			if (!(AssertOperator(parser->tokens[0], ClosingBraceToken)
					|| AssertToken(parser->tokens[0], IdentifierToken))) {
				ParseError(L"Expected '}' or identifier", freestmt, parser);
			}
		}
		parser->next(parser, env);
#undef freestmt
	}
	/*Build interface node and return it*/
	YNode* inode = newInterfaceNode(parents, parent_count, ids, types,
			attr_count);
	SetCoords(inode, file, line, charPos);
	return inode;
}
/*Just calls interface validate and reduce*/
NewValidate(Declaration_validate) {
	return parser->grammar.interface.validate(parser, env);
}
NewReduce(Declaration_reduce) {
	return parser->grammar.interface.reduce(parser, env);
}

/*Reduces factor: constants, object and interface definitions,
 * arrays, lambdas, overloads,
 * expressions in parentheses and braces*/
NewValidate(Factor_validate) {
	if (AssertOperator(parser->tokens[0], OpeningParentheseToken))
		return true;
	else if (AssertOperator(parser->tokens[0], OpeningBraceToken))
		return true;
	return parser->grammar.constant.validate(parser, env)
			|| parser->grammar.identifier.validate(parser, env)
			|| parser->grammar.lambda.validate(parser, env)
			|| parser->grammar.overload.validate(parser, env)
			|| parser->grammar.object.validate(parser, env)
			|| parser->grammar.declaration.validate(parser, env)
			|| parser->grammar.array.validate(parser, env);
}
NewReduce(Factor_reduce) {
	YNode* out = NULL;
	ExtractCoords(file, line, charPos, parser, env);
	/*Call suitable grammar methods on different rules*/
	if (parser->grammar.object.validate(parser, env))
		out = parser->grammar.object.reduce(parser, env);
	else if (parser->grammar.lambda.validate(parser, env))
		out = parser->grammar.lambda.reduce(parser, env);
	else if (parser->grammar.array.validate(parser, env))
		out = parser->grammar.array.reduce(parser, env);
	else if (parser->grammar.overload.validate(parser, env))
		out = parser->grammar.overload.reduce(parser, env);
	else if (parser->grammar.constant.validate(parser, env))
		out = parser->grammar.constant.reduce(parser, env);
	else if (parser->grammar.identifier.validate(parser, env))
		out = parser->grammar.identifier.reduce(parser, env);
	else if (parser->grammar.declaration.validate(parser, env))
		out = parser->grammar.declaration.reduce(parser, env);
	else if (AssertOperator(parser->tokens[0], OpeningParentheseToken)) {
		/*It's '(' expression ')'*/
		parser->next(parser, env);
		ExpectReduce(&out, expression, L"Expected expression", ;, parser, env);
		ExpectOperator(parser->tokens[0], ClosingParentheseToken,
				L"Expected ')'", ;, parser);
		parser->next(parser, env);
	} else if (AssertOperator(parser->tokens[0], OpeningBraceToken)) {
		/*It's block: '{' ... '}'
		 * Block may contain statements and function definitions*/
		YFunctionBlock* funcs = NULL;
		size_t funcs_c = 0;
		YNode** block = NULL;
		size_t length = 0;
		parser->next(parser, env);
		/*Code to free resources if error occurred*/
#define freestmt {\
            for (size_t i=0;i<length;i++)\
                block[i]->free(block[i]);\
            free(block);\
            for (size_t i=0;i<funcs_c;i++)\
                for (size_t j=0;j<funcs[i].count;j++)\
                    funcs[i].lambda[j]->node.free((YNode*) funcs[i].lambda[j]);\
            free(funcs);\
		}
		while (!AssertOperator(parser->tokens[0], ClosingBraceToken)) {
			if (parser->grammar.statement.validate(parser, env)) {
				/*Parse statement*/
				length++;
				block = realloc(block, sizeof(YNode*) * length);
				ExpectReduce(&block[length - 1], statement,
						L"Expected statement", freestmt, parser, env);
			} else if (parser->grammar.function.validate(parser, env)) {
				/*Parse function definition*/
				YNode* nd;
				Reduce(&nd, function, freestmt, parser, env);
				YFunctionNode* func = (YFunctionNode*) nd;
				/*Check if block contains function with that name.
				 * If contains then overload them.*/
				for (size_t i = 0; i < funcs_c; i++)
					if (funcs[i].id == func->name) {
						funcs[i].count++;
						funcs[i].lambda = realloc(funcs[i].lambda,
								sizeof(YLambdaNode*) * funcs[i].count);
						funcs[i].lambda[funcs[i].count - 1] = func->lambda;
						free(func);
						func = NULL;
						break;
					}
				/*Else add function to block*/
				if (func != NULL) {
					funcs_c++;
					funcs = realloc(funcs, sizeof(YFunctionBlock) * funcs_c);
					funcs[funcs_c - 1].id = func->name;
					funcs[funcs_c - 1].count = 1;
					funcs[funcs_c - 1].lambda = malloc(sizeof(YLambdaNode*));
					funcs[funcs_c - 1].lambda[0] = func->lambda;
					free(func);
					func = NULL;
				}
			} else
				ParseError(L"Expected statement or function", freestmt, parser);
		}
#undef freestmt
		parser->next(parser, env);
		/*Build block node*/
		out = newBlockNode(block, length, funcs, funcs_c);
	}
	/*Return result*/
	SetCoords(out, file, line, charPos);
	return out;
}
/*Try to parse reference or just return factor*/
NewValidate(Reference_validate) {
	return parser->grammar.factor.validate(parser, env);
}
NewReduce(Reference_reduce) {
	/*Get factor*/
	YNode* out;
	ExtractCoords(file, line, charPos, parser, env);
	ExpectReduce(&out, factor, L"Expected factor", ;, parser, env);
	while (true) {
		/*Try to parse reference. References may be recursive:
		 * one reference based on another.*/
		if (AssertOperator(parser->tokens[0], OpeningBracketToken)) {
			/*It's index or subsequence*/
			parser->next(parser, env);
			YNode* index;
			/*Get index*/
			ExpectReduce(&index, expression, L"Expected expression",
					out->free(out), parser, env);
			if (AssertOperator(parser->tokens[0], ClosingBracketToken)) {
				/*It's just index reference. Save it.*/
				parser->next(parser, env);
				out = newIndexReferenceNode(out, index);
			} else if (AssertOperator(parser->tokens[0], ColonToken)) {
				/*It's subsequence. Get end index.*/
				parser->next(parser, env);
				YNode* toIndex;
				ExpectReduce(&toIndex, expression, L"Expected expression", {
					out->free(out)
					;
					index->free(index)
					;
				},
				parser, env);
				ExpectOperator(parser->tokens[0], ClosingBracketToken,
						L"Expected ']'", {
							out->free(out)
							;
							index->free(index)
							;
						}, parser);
				parser->next(parser, env);
				/*Save start and end indecies.*/
				out = newSubseqReferenceNode(out, index, toIndex);
			} else
				ParseError(L"Expected ']' or ':'", {
					out->free(out)
					;
					index->free(index)
					;
				}, parser);
		} else if (AssertOperator(parser->tokens[0], DotToken)) {
			/*It's reference to object field*/
			parser->next(parser, env);
			ExpectToken(parser->tokens[0], IdentifierToken,
					L"Expected identifier", out->free(out), parser);
			/*Get field name*/
			int32_t id = ((YIdentifierToken*) parser->tokens[0])->id;
			parser->next(parser, env);
			/*Save it*/
			out = newFieldReferenceNode(out, id);
		} else if (AssertOperator(parser->tokens[0], OpeningParentheseToken)) {
			/*It's function call*/
			parser->next(parser, env);
			size_t length = 0;
			YNode** args = NULL;
			/*Get call arguments*/
			while (!AssertOperator(parser->tokens[0], ClosingParentheseToken)) {
				length++;
				args = realloc(args, sizeof(YNode*) * length);
				/*Get argument*/
				ExpectReduce(&args[length - 1], expr, L"Expected expression", {
					out->free(out)
					;
					for (size_t i = 0; i < length - 1; i++)
						args[i]->free(args[i])
						;
					free(args)
					;
				}, parser, env);
				if (AssertOperator(parser->tokens[0], ClosingParentheseToken))
					break;
				ExpectOperator(parser->tokens[0], CommaToken, L"Expected ','", {
					out->free(out)
					;
					for (size_t i = 0; i < length; i++)
						args[i]->free(args[i])
						;
					free(args)
					;
				}, parser);
				parser->next(parser, env);
			}
			parser->next(parser, env);
			/*Save call node*/
			out = newCallNode(out, args, length);
		} else {
			/*It's not reference*/
			break;
		}
	}
	/*Return result*/
	SetCoords(out, file, line, charPos);
	return out;
}
/*Check if it's unary operation or just return reference*/
NewValidate(Unary_validate) {
	return ((AssertOperator(parser->tokens[0], PlusToken)
			&& AssertOperator(parser->tokens[1], PlusToken))
			|| (AssertOperator(parser->tokens[0], MinusToken)
					&& AssertOperator(parser->tokens[1], MinusToken))
			|| AssertOperator(parser->tokens[0], PlusToken)
			|| AssertOperator(parser->tokens[0], MinusToken)
			|| AssertOperator(parser->tokens[0], NotToken))
			|| parser->grammar.reference.validate(parser, env);
}
NewReduce(Unary_reduce) {
	YNode* unary = NULL;
	ExtractCoords(file, line, charPos, parser, env);
	if (AssertOperator(parser->tokens[0],
			PlusToken) && AssertOperator(parser->tokens[1], PlusToken)) {
		/*It has '++' in prefix*/
		parser->next(parser, env);
		parser->next(parser, env);
		/*Get argument and save PreIncrement operation*/
		YNode* arg;
		ExpectReduce(&arg, reference, L"Expected reference", ;, parser, env);
		unary = newUnaryNode(PreIncrement, arg);
	}

	else if (AssertOperator(parser->tokens[0],
			MinusToken) && AssertOperator(parser->tokens[1], MinusToken)) {
		/*It has '--' in prefix*/
		parser->next(parser, env);
		parser->next(parser, env);
		/*Get argument and save PreDecrement operation*/
		YNode* arg;
		ExpectReduce(&arg, reference, L"Expected reference", ;, parser, env);
		unary = newUnaryNode(PreDecrement, arg);
	}

	else if (AssertOperator(parser->tokens[0], PlusToken)) {
		/* It's '+' operation
		 * Get argument and save it*/
		parser->next(parser, env);
		YNode* out;
		ExpectReduce(&out, reference, L"Expected reference", ;, parser, env);
		return out;
	}

	else if (AssertOperator(parser->tokens[0], MinusToken)) {
		/* It's '-' operation
		 * Get argument and save Negate operation*/
		parser->next(parser, env);
		YNode* arg;
		ExpectReduce(&arg, reference, L"Expected reference", ;, parser, env);
		unary = newUnaryNode(Negate, arg);
	}

	else if (AssertOperator(parser->tokens[0], NotToken)) {
		/* It's '~' operation
		 * Get argument and save Not operation*/
		parser->next(parser, env);
		YNode* arg;
		ExpectReduce(&arg, reference, L"Expected reference", ;, parser, env);
		unary = newUnaryNode(Not, arg);
	}

	else {
		/*It has no prefix. It may be just reference or
		 * PostIncrement or PostDecrement operation.*/
		YNode* out;
		ExpectReduce(&out, reference, L"Expected reference", ;, parser, env);

		if (AssertOperator(parser->tokens[0],
				PlusToken) && AssertOperator(parser->tokens[1], PlusToken)) {
			/* It's '++' operation
			 * Get argument and save PostIncrement operation*/
			parser->next(parser, env);
			parser->next(parser, env);
			unary = newUnaryNode(PostIncrement, out);
		} else if (AssertOperator(parser->tokens[0],
				MinusToken) && AssertOperator(parser->tokens[1], MinusToken)) {
			/* It's '--' operation
			 * Get argument and save PostDecrement operation*/
			parser->next(parser, env);
			parser->next(parser, env);
			unary = newUnaryNode(PostDecrement, out);
		}

		else {
			/*Return result*/
			SetCoords(out, file, line, charPos);
			return out;
		}

	}
	/*Return result*/
	SetCoords(unary, file, line, charPos);
	return unary;
}
/*Return unary or chain of power operations if
 * available*/
NewValidate(Power_validate) {
	return parser->grammar.unary.validate(parser, env);
}
NewReduce(Power_reduce) {
	/* Get unary*/
	YNode* out;
	ExtractCoords(file, line, charPos, parser, env);
	ExpectReduce(&out, unary, L"Expected expression", ;, parser, env);
	/*While there is power operation reduce it*/
	while (AssertOperator(parser->tokens[0], MultiplyToken)
			&& AssertOperator(parser->tokens[1], MultiplyToken)) {
		if (AssertOperator(parser->tokens[2], AssignToken))
			break;
		parser->next(parser, env);
		parser->next(parser, env);
		/*Get second arument and save operation as first argument*/
		YNode* right;
		ExpectReduce(&right, unary, L"Expected expression", out->free(out),
				parser, env);
		out = newBinaryNode(Power, out, right);
	}
	/*Return result*/
	SetCoords(out, file, line, charPos);
	return out;
}
/*Next rules are similar to Power rule, so see
 * Power_reduce comments mostly*/
NewValidate(Mul_div_validate) {
	return parser->grammar.power.validate(parser, env);
}
NewReduce(Mul_div_reduce) {
	YNode* out;
	ExtractCoords(file, line, charPos, parser, env);
	ExpectReduce(&out, power, L"Expected expression", ;, parser, env);
	while (AssertOperator(parser->tokens[0], MultiplyToken)
			|| AssertOperator(parser->tokens[0], DivideToken)
			|| AssertOperator(parser->tokens[0], ModuloToken)) {
		Operator op = GetOperator(parser->tokens[0]);
		if (AssertOperator(parser->tokens[1],
				AssignToken) || AssertOperator(parser->tokens[1], op))
			break;
		parser->next(parser, env);
		YNode* right;
		ExpectReduce(&right, power, L"Expected expression", out->free(out),
				parser, env);
		/*Switch suitable operation*/
		if (op == MultiplyToken)
			out = newBinaryNode(Multiply, out, right);
		else if (op == DivideToken)
			out = newBinaryNode(Divide, out, right);
		else if (op == ModuloToken)
			out = newBinaryNode(Modulo, out, right);
	}
	SetCoords(out, file, line, charPos);
	return out;
}
NewValidate(Add_sub_validate) {
	return parser->grammar.mul_div.validate(parser, env);
}
NewReduce(Add_sub_reduce) {
	YNode* out;
	ExtractCoords(file, line, charPos, parser, env);
	ExpectReduce(&out, mul_div, L"Expected expression", ;, parser, env);
	while (AssertOperator(parser->tokens[0], PlusToken)
			|| (AssertOperator(parser->tokens[0], MinusToken)
					&& !AssertOperator(parser->tokens[1], GreaterToken))) {
		if (AssertOperator(parser->tokens[1], AssignToken))
			break;
		Operator op = GetOperator(parser->tokens[0]);
		parser->next(parser, env);
		YNode* right;
		ExpectReduce(&right, mul_div, L"Expected expression", out->free(out),
				parser, env);
		/*Switch suitable operation*/
		if (op == PlusToken)
			out = newBinaryNode(Add, out, right);
		else if (op == MinusToken)
			out = newBinaryNode(Subtract, out, right);
	}
	SetCoords(out, file, line, charPos);
	return out;
}
NewValidate(Bitshift_validate) {
	return parser->grammar.add_sub.validate(parser, env);
}
NewReduce(Bitshift_reduce) {
	YNode* out;
	ExtractCoords(file, line, charPos, parser, env);
	ExpectReduce(&out, add_sub, L"Expected expression", ;, parser, env);
	while ((AssertOperator(parser->tokens[0], GreaterToken)
			&& AssertOperator(parser->tokens[1], GreaterToken))
			|| (AssertOperator(parser->tokens[0], LesserToken)
					&& AssertOperator(parser->tokens[1], LesserToken))) {
		if (AssertOperator(parser->tokens[2], AssignToken))
			break;
		Operator op = GetOperator(parser->tokens[0]);
		parser->next(parser, env);
		parser->next(parser, env);
		YNode* right;
		ExpectReduce(&right, add_sub, L"Expected expression", out->free(out),
				parser, env);
		/*Switch suitable operation*/
		if (op == LesserToken)
			out = newBinaryNode(ShiftLeft, out, right);
		else
			out = newBinaryNode(ShiftRight, out, right);
	}
	SetCoords(out, file, line, charPos);
	return out;
}
NewValidate(Bitwise_validate) {
	return parser->grammar.bitshift.validate(parser, env);
}
NewReduce(Bitwise_reduce) {
	YNode* out;
	ExtractCoords(file, line, charPos, parser, env);
	ExpectReduce(&out, bitshift, L"Expected expression", ;, parser, env);
	while ((AssertOperator(parser->tokens[0], OrToken)
			&& !AssertOperator(parser->tokens[1], OrToken))
			|| (AssertOperator(parser->tokens[0], AndToken)
					&& !AssertOperator(parser->tokens[1], AndToken))
			|| AssertOperator(parser->tokens[0], XorToken)) {
		if (AssertOperator(parser->tokens[1], AssignToken))
			break;
		Operator op = GetOperator(parser->tokens[0]);
		parser->next(parser, env);
		YNode* right;
		ExpectReduce(&right, bitshift, L"Expected expression", out->free(out),
				parser, env);
		/*Switch suitable operation*/
		if (op == OrToken)
			out = newBinaryNode(Or, out, right);
		else if (op == AndToken)
			out = newBinaryNode(And, out, right);
		else if (op == XorToken)
			out = newBinaryNode(Xor, out, right);
	}
	SetCoords(out, file, line, charPos);
	return out;
}
/*Comparison rule. It's not recursive as previous.
 * If there is no comparison returns just first node*/
NewValidate(Comparison_validate) {
	return parser->grammar.bitwise.validate(parser, env);
}
NewReduce(Comparison_reduce) {
	/*Get first node*/
	YNode* out;
	ExtractCoords(file, line, charPos, parser, env);
	ExpectReduce(&out, bitwise, L"Expected expression", ;, parser, env);
	if (AssertOperator(parser->tokens[0],
			AssignToken) && AssertOperator(parser->tokens[1], AssignToken)) {
		/*It's '==' comparison*/
		parser->next(parser, env);
		parser->next(parser, env);
		YNode* right;
		ExpectReduce(&right, bitwise, L"Expected expression", out->free(out),
				parser, env);
		out = newBinaryNode(BEquals, out, right);
	} else if (AssertOperator(parser->tokens[0],
			LogicalNotToken) && AssertOperator(parser->tokens[1], AssignToken)) {
		/*It's '!=' comparison*/
		parser->next(parser, env);
		parser->next(parser, env);
		YNode* right;
		ExpectReduce(&right, bitwise, L"Expected expression", out->free(out),
				parser, env);
		out = newBinaryNode(NotEquals, out, right);
	} else if (AssertOperator(parser->tokens[0],
			GreaterToken) && AssertOperator(parser->tokens[1], AssignToken)) {
		/*It's '>=' comparison*/
		parser->next(parser, env);
		parser->next(parser, env);
		YNode* right;
		ExpectReduce(&right, bitwise, L"Expected expression", out->free(out),
				parser, env);
		out = newBinaryNode(GreaterOrEquals, out, right);
	} else if (AssertOperator(parser->tokens[0],
			LesserToken) && AssertOperator(parser->tokens[1], AssignToken)) {
		/*It's '<=' comparison*/
		parser->next(parser, env);
		parser->next(parser, env);
		YNode* right;
		ExpectReduce(&right, bitwise, L"Expected expression", out->free(out),
				parser, env);
		out = newBinaryNode(LesserOrEquals, out, right);
	} else if (AssertOperator(parser->tokens[0], GreaterToken)&&
	!AssertOperator(parser->tokens[1], GreaterToken)) {
		/*It's '>' comparison*/
		parser->next(parser, env);
		YNode* right;
		ExpectReduce(&right, bitwise, L"Expected expression", out->free(out),
				parser, env);
		out = newBinaryNode(Greater, out, right);
	} else if (AssertOperator(parser->tokens[0], LesserToken)&&
	!AssertOperator(parser->tokens[1], LesserToken)) {
		/*It's '<' comparison*/
		parser->next(parser, env);
		YNode* right;
		ExpectReduce(&right, bitwise, L"Expected expression", out->free(out),
				parser, env);
		out = newBinaryNode(Lesser, out, right);
	}
	/*Return result*/
	SetCoords(out, file, line, charPos);
	return out;
}
/*Check logical not operation*/
NewValidate(Logical_not_validate) {
	return (AssertOperator(parser->tokens[0], LogicalNotToken))
			|| parser->grammar.comparison.validate(parser, env);
}
NewReduce(Logical_not_reduce) {
	ExtractCoords(file, line, charPos, parser, env);
	if (AssertOperator(parser->tokens[0], LogicalNotToken)) {
		/*It's something like '!'...*/
		parser->next(parser, env);
		YNode* arg;
		ExpectReduce(&arg, comparison, L"Expected expression", ;, parser, env);
		YNode* out = newUnaryNode(LogicalNot, arg);
		SetCoords(out, file, line, charPos);
		return out;
	}
	/*Just return parsed node*/
	YNode* nd;
	ExpectReduce(&nd, comparison, L"Expected expression", ;, parser, env);
	SetCoords(nd, file, line, charPos);
	return nd;
}
/*Rule similar to power. See it.*/
NewValidate(Logical_ops_validate) {
	return parser->grammar.logical_not.validate(parser, env);
}
NewReduce(Logical_ops_reduce) {
	YNode* out;
	ExtractCoords(file, line, charPos, parser, env);
	ExpectReduce(&out, logical_not, L"Expected expression", ;, parser, env);
	while ((AssertOperator(parser->tokens[0], AndToken)
			&& AssertOperator(parser->tokens[1], AndToken))
			|| (AssertOperator(parser->tokens[0], OrToken)
					&& AssertOperator(parser->tokens[1], OrToken))) {
		if (AssertOperator(parser->tokens[2], AssignToken))
			break;
		Operator op = GetOperator(parser->tokens[0]);
		parser->next(parser, env);
		parser->next(parser, env);
		YNode* right;
		ExpectReduce(&right, logical_not, L"Expected expression",
				out->free(out), parser, env);
		if (op == AndToken)
			out = newBinaryNode(LogicalAnd, out, right);
		else if (op == OrToken)
			out = newBinaryNode(LogicalOr, out, right);
	}
	SetCoords(out, file, line, charPos);
	return out;
}
/*Parse all control structures or just return parsed node*/
NewValidate(Expr_validate) {
	return AssertKeyword(parser->tokens[0], IfKeyword)
			|| AssertKeyword(parser->tokens[0], TryKeyword)
			|| AssertKeyword(parser->tokens[0], SwitchKeyword)
			|| AssertKeyword(parser->tokens[0], UsingKeyword)
			|| AssertKeyword(parser->tokens[0], WithKeyword)
			|| AssertKeyword(parser->tokens[0], BreakKeyword)
			|| AssertKeyword(parser->tokens[0], ContinueKeyword)
			|| AssertKeyword(parser->tokens[0], PassKeyword)
			|| AssertKeyword(parser->tokens[0], WhileKeyword)
			|| AssertKeyword(parser->tokens[0], DoKeyword)
			|| AssertKeyword(parser->tokens[0], ForKeyword)
			|| AssertKeyword(parser->tokens[0], ForeachKeyword)
			|| AssertKeyword(parser->tokens[0], ReturnKeyword)
			|| AssertKeyword(parser->tokens[0], ThrowKeyword)
			|| (AssertToken(parser->tokens[0], IdentifierToken)
					&& AssertOperator(parser->tokens[1], ColonToken)
					&& (AssertKeyword(parser->tokens[2], WhileKeyword)
							|| AssertKeyword(parser->tokens[2], ForeachKeyword)
							|| AssertKeyword(parser->tokens[2], DoKeyword)
							|| AssertKeyword(parser->tokens[2], ForKeyword)))
			|| parser->grammar.logical_ops.validate(parser, env);
}
NewReduce(Expr_reduce) {
	YNode* out;
	ExtractCoords(file, line, charPos, parser, env);
	if (AssertKeyword(parser->tokens[0], IfKeyword)) {
		/*Parses 'if' statement*/
		parser->next(parser, env);
		YNode* cond;
		YNode* body;
		ExpectReduce(&cond, expression, L"Expected expression", ;, parser, env);
		ExpectReduce(&body, statement, L"Expected statement", cond->free(cond),
				parser, env);
		YNode* elseBody = NULL;
		if (AssertKeyword(parser->tokens[0], ElseKeyword)) {
			/*Parses 'else' statement if available*/
			parser->next(parser, env);
			ExpectReduce(&elseBody, statement, L"Expected statement", {
				cond->free(cond)
				;
				body->free(body)
				;
			}, parser, env);
		}
		out = newConditionNode(cond, body, elseBody);
	} else if (AssertKeyword(parser->tokens[0], TryKeyword)) {
		parser->next(parser, env);
		YNode* tryBody;
		/*Parses 'try' body*/
		ExpectReduce(&tryBody, statement, L"Expected statement", ;, parser, env);
		YNode* catchRef = NULL;
		YNode* catchBody = NULL;
		YNode* elseBody = NULL;
		YNode* finBody = NULL;
		if (AssertKeyword(parser->tokens[0], CatchKeyword)) {
			/*Parses 'catch' argument and body*/
			parser->next(parser, env);
			ExpectReduce(&catchRef, expression, L"Expected expression",
					tryBody->free(tryBody), parser, env);
			ExpectReduce(&catchBody, statement, L"Expected statement", {
				tryBody->free(tryBody)
				;
				catchRef->free(catchRef)
				;
			}, parser, env);
		}
		if (AssertKeyword(parser->tokens[0], ElseKeyword)) {
			/*Parses 'else' body*/
			parser->next(parser, env);
			ExpectReduce(&elseBody, statement, L"Expected statement",
					{tryBody->free(tryBody); if (catchRef!=NULL) { catchRef->free(catchRef); catchBody->free(catchBody); } },
					parser, env);
		}
		if (AssertKeyword(parser->tokens[0], FinallyKeyword)) {
			/*Parses 'finally' body*/
			parser->next(parser, env);
			ExpectReduce(&finBody, statement, L"Expected statement",
					{tryBody->free(tryBody); if (catchRef!=NULL) { catchRef->free(catchRef); catchBody->free(catchBody); } if (elseBody!=NULL) elseBody->free(elseBody); },
					parser, env);
		}
		out = newTryNode(tryBody, catchRef, catchBody, elseBody, finBody);
	} else if (AssertKeyword(parser->tokens[0], SwitchKeyword)) {
		YNode* value = NULL;
		YNode* defCase = NULL;
		YCaseNode* cases = NULL;
		size_t case_count = 0;

#define freestmt {if (value!=NULL) value->free(value);\
					if (defCase!=NULL) defCase->free(defCase);\
					for (size_t i=0;i<case_count;i++) {\
						cases[i].value->free(cases[i].value);\
						cases[i].stmt->free(cases[i].stmt);\
					}\
					free(cases);}

		parser->next(parser, env);

		ExpectReduce(&value, expression, L"Expected expression", freestmt,
				parser, env);
		ExpectOperator(parser->tokens[0], OpeningBraceToken, L"Expected '{",
				freestmt, parser);
		parser->next(parser, env);
		while (!AssertOperator(parser->tokens[0], ClosingBraceToken)) {
			if (AssertKeyword(parser->tokens[0], CaseKeyword)) {
				parser->next(parser, env);
				YNode* cval = NULL;
				YNode* cstmt = NULL;
				ExpectReduce(&cval, expression, L"Expected expression",
						freestmt, parser, env);
				ExpectReduce(&cstmt, statement, L"Expected statement",
						{freestmt; cval->free(cval);}, parser, env);
				cases = realloc(cases, sizeof(YCaseNode) * (++case_count));
				cases[case_count - 1].value = cval;
				cases[case_count - 1].stmt = cstmt;
			} else if (AssertKeyword(parser->tokens[0], DefaultKeyword)) {
				if (defCase != NULL)
					ParseError(L"Default case already defined", freestmt,
							parser);
				parser->next(parser, env);
				ExpectReduce(&defCase, statement, L"Expected statement",
						freestmt, parser, env);
			} else
				ParseError(L"Expected 'case, 'default' or '}", freestmt, parser);
		}
		parser->next(parser, env);
#undef freestmt
		out = newSwitchNode(value, cases, case_count, defCase);
	} else if (AssertKeyword(parser->tokens[0], UsingKeyword)) {
		parser->next(parser, env);
		size_t length = 1;
		YNode** scopes = malloc(sizeof(YNode*));
		ExpectReduce(&scopes[0], expr, L"Expected expresion", free(scopes),
				parser, env);
#define freestmt {\
            for (size_t i=0;i<length;i++) {\
                scopes[i]->free(scopes[i]);\
            }\
            free(scopes);\
        }
		while (AssertOperator(parser->tokens[0], CommaToken)) {
			parser->next(parser, env);
			YNode* n;
			ExpectReduce(&n, expr, L"Expect expression", freestmt, parser, env);
			length++;
			scopes = realloc(scopes, sizeof(YNode*) * length);
			scopes[length - 1] = n;
		}
		YNode* body;
		ExpectReduce(&body, statement, L"Expected statement", freestmt, parser,
				env);
#undef freestmt
		out = newUsingNode(scopes, length, body);
	} else if (AssertKeyword(parser->tokens[0], WithKeyword)) {
		parser->next(parser, env);
		YNode* scope;
		YNode* body;
		ExpectReduce(&scope, expression, L"Expected expression", ;, parser, env);
		ExpectReduce(&body, statement, L"Expected statement",
				scope->free(scope), parser, env);
		out = newWithNode(scope, body);
	} else if (AssertKeyword(parser->tokens[0], PassKeyword)) {
		parser->next(parser, env);
		out = newPassNode();
	} else if (AssertKeyword(parser->tokens[0], BreakKeyword)) {
		parser->next(parser, env);
		int32_t id = -1;
		if (AssertToken(parser->tokens[0], IdentifierToken)) {
			id = ((YIdentifierToken*) parser->tokens[0])->id;
			parser->next(parser, env);
		}
		out = newBreakNode(id);
	} else if (AssertKeyword(parser->tokens[0], ContinueKeyword)) {
		parser->next(parser, env);
		int32_t id = -1;
		if (AssertToken(parser->tokens[0], IdentifierToken)) {
			id = ((YIdentifierToken*) parser->tokens[0])->id;
			parser->next(parser, env);
		}
		out = newContinueNode(id);
	} else if (AssertKeyword(parser->tokens[0], ReturnKeyword)) {
		parser->next(parser, env);
		YNode* value;
		ExpectReduce(&value, expression, L"Expected expression", ;, parser, env);
		out = newReturnNode(value);
	} else if (AssertKeyword(parser->tokens[0], ThrowKeyword)) {
		parser->next(parser, env);
		YNode* value;
		ExpectReduce(&value, expression, L"Expected expression", ;, parser, env);
		out = newThrowNode(value);
	} else if (AssertKeyword(parser->tokens[0], WhileKeyword)
			|| (AssertToken(parser->tokens[0], IdentifierToken)
					&& AssertOperator(parser->tokens[1], ColonToken)
					&& (AssertKeyword(parser->tokens[2], WhileKeyword)))) {
		int32_t id = -1;
		if (AssertToken(parser->tokens[0], IdentifierToken)) {
			id = ((YIdentifierToken*) parser->tokens[0])->id;
			parser->next(parser, env);
			parser->next(parser, env);
		}
		parser->next(parser, env);
		YNode* cond;
		YNode* body;
		ExpectReduce(&cond, expression, L"Expected expression", ;, parser, env);
		ExpectReduce(&body, statement, L"Expected statement", cond->free(cond),
				parser, env);
		return newWhileLoopNode(id, true, cond, body);
	} else if (AssertKeyword(parser->tokens[0], DoKeyword)
			|| (AssertToken(parser->tokens[0], IdentifierToken)
					&& AssertOperator(parser->tokens[1], ColonToken)
					&& (AssertKeyword(parser->tokens[2], DoKeyword)))) {
		int32_t id = -1;
		if (AssertToken(parser->tokens[0], IdentifierToken)) {
			id = ((YIdentifierToken*) parser->tokens[0])->id;
			parser->next(parser, env);
			parser->next(parser, env);
		}
		parser->next(parser, env);
		YNode* body;
		ExpectReduce(&body, statement, L"Expected statement", ;, parser, env);
		if (AssertKeyword(parser->tokens[0], WhileKeyword)) {
			parser->next(parser, env);
			YNode* cond;
			ExpectReduce(&cond, expression, L"Expected expression",
					body->free(body), parser, env);
			return newWhileLoopNode(id, false, cond, body);
		} else {
			return newLoopNode(id, body);
		}
	} else if (AssertKeyword(parser->tokens[0], ForKeyword)
			|| (AssertToken(parser->tokens[0], IdentifierToken)
					&& AssertOperator(parser->tokens[1], ColonToken)
					&& (AssertKeyword(parser->tokens[2], ForKeyword)))) {
		int32_t id = -1;
		if (AssertToken(parser->tokens[0], IdentifierToken)) {
			id = ((YIdentifierToken*) parser->tokens[0])->id;
			parser->next(parser, env);
			parser->next(parser, env);
		}
		parser->next(parser, env);
		YNode* init;
		YNode* cond;
		YNode* loop;
		YNode* body;
		ExpectReduce(&init, statement, L"Expects statement", ;, parser, env);
		ExpectReduce(&cond, statement, L"Expects statement", init->free(init),
				parser, env);
		ExpectReduce(&loop, statement, L"Expects statement", {
			init->free(init)
			;
			cond->free(cond)
			;
		}, parser, env);
		ExpectReduce(&body, statement, L"Expects statement", {
			init->free(init)
			;
			cond->free(cond)
			;
			loop->free(loop)
			;
		}, parser, env);
		return newForLoopNode(id, init, cond, loop, body);
	} else if (AssertKeyword(parser->tokens[0], ForeachKeyword)
			|| (AssertToken(parser->tokens[0], IdentifierToken)
					&& AssertOperator(parser->tokens[1], ColonToken)
					&& (AssertKeyword(parser->tokens[2], ForeachKeyword)))) {
		int32_t id = -1;
		if (AssertToken(parser->tokens[0], IdentifierToken)) {
			id = ((YIdentifierToken*) parser->tokens[0])->id;
			parser->next(parser, env);
			parser->next(parser, env);
		}
		parser->next(parser, env);
		YNode* refnode;
		YNode* col;
		YNode* body;

		ExpectReduce(&refnode, expression, L"Expected expression", ;, parser,
				env);
		ExpectOperator(parser->tokens[0], ColonToken, L"Expected ':'",
				refnode->free(refnode), parser);
		parser->next(parser, env);
		ExpectReduce(&col, expression, L"Expected expression",
				refnode->free(refnode), parser, env);
		ExpectReduce(&body, expression, L"Expected statement", {
			refnode->free(refnode)
			;
			col->free(col)
			;
		}, parser, env);
		out = newForeachLoopNode(id, refnode, col, body);
	} else
		ExpectReduce(&out, logical_ops, L"Expected expression", ;, parser, env);
	SetCoords(out, file, line, charPos);
	return out;
}
NewValidate(Expression_validate) {
	return AssertKeyword(parser->tokens[0], VarKeyword)
			|| AssertKeyword(parser->tokens[0], DelKeyword)
			|| parser->grammar.expr.validate(parser, env);
}
NewReduce(Expression_reduce) {
	ExtractCoords(file, line, charPos, parser, env);
	if (AssertKeyword(parser->tokens[0], DelKeyword)) {
		parser->next(parser, env);
		size_t length = 1;
		YNode** list = malloc(sizeof(YNode*) * length);
		ExpectReduce(&list[0], expr, L"Expected expression", free(list), parser,
				env);
#define freestmt {\
            for (size_t i=0;i<length;i++)\
                list[i]->free(list[i]);\
            free(list);\
		}
		while (AssertOperator(parser->tokens[0], CommaToken)) {
			parser->next(parser, env);
			YNode* n;
			ExpectReduce(&n, expr, L"Expected expression", freestmt, parser,
					env);
			length++;
			list = realloc(list, sizeof(YNode*) * length);
			list[length - 1] = n;
		}
#undef freestmt
		YNode* out = newDeleteNode(list, length);
		SetCoords(out, file, line, charPos);
		return out;
	}

	bool newVar = false;
	if (AssertKeyword(parser->tokens[0], VarKeyword)) {
		parser->next(parser, env);
		newVar = true;
	}
	YNode* type = NULL;
	if (AssertOperator(parser->tokens[0], MinusToken)&&
	AssertOperator(parser->tokens[1], GreaterToken)) {
		parser->next(parser, env);
		parser->next(parser, env);
		ExpectReduce(&type, expr, L"Expected expression", ;, parser, env);
	}
	YNode* node = NULL;
	ExpectReduce(&node, expr, L"Expected expression",
			if (type!=NULL) type->free(type);, parser, env);
	YNode** dests = NULL;
	size_t dest_count = 0;
	if (AssertOperator(parser->tokens[0], CommaToken)) {
		dests = malloc(sizeof(YNode*));
		dests[0] = node;
		dest_count++;
		while (AssertOperator(parser->tokens[0], CommaToken)) {
			parser->next(parser, env);
			YNode* n;
			ExpectReduce(&n, expr, L"Expected expression", {
				for (size_t i = 0; i < dest_count; i++)
					dests[i]->free(dests[i])
					;
				free(dests)
				;
			}, parser, env);
			dests = realloc(dests, sizeof(YNode*) * (++dest_count));
			dests[dest_count - 1] = n;
		}
	}
	if (AssertOperator(parser->tokens[0], AssignToken)
			|| ((AssertOperator(parser->tokens[0], PlusToken)
					|| AssertOperator(parser->tokens[0], MinusToken)
					|| AssertOperator(parser->tokens[0], MultiplyToken)
					|| AssertOperator(parser->tokens[0], DivideToken)
					|| AssertOperator(parser->tokens[0], ModuloToken)
					|| AssertOperator(parser->tokens[0], AndToken)
					|| AssertOperator(parser->tokens[0], OrToken)
					|| AssertOperator(parser->tokens[0], XorToken))
					&& AssertOperator(parser->tokens[1], AssignToken))
			|| (((AssertOperator(parser->tokens[0], MultiplyToken)
					&& AssertOperator(parser->tokens[1], MultiplyToken))
					|| (AssertOperator(parser->tokens[0], LesserToken)
							&& AssertOperator(parser->tokens[1], LesserToken))
					|| (AssertOperator(parser->tokens[0], GreaterToken)
							&& AssertOperator(parser->tokens[1], GreaterToken))
					|| (AssertOperator(parser->tokens[0], AndToken)
							&& AssertOperator(parser->tokens[1], AndToken))
					|| (AssertOperator(parser->tokens[0], OrToken)
							&& AssertOperator(parser->tokens[1], OrToken)))
					&& AssertOperator(parser->tokens[2], AssignToken))) {
		if (dests == NULL) {
			dests = malloc(sizeof(YNode*));
			dests[0] = node;
			dest_count++;
		}
		YAssignmentOperation op;
		if (AssertOperator(parser->tokens[0], PlusToken)) {
			op = AAddAssign;
			parser->next(parser, env);
		} else if (AssertOperator(parser->tokens[0], MinusToken)) {
			op = ASubAssign;
			parser->next(parser, env);
		} else if (AssertOperator(parser->tokens[0], MultiplyToken)&&
		AssertOperator(parser->tokens[1], MultiplyToken)) {
			op = APowerAssign;
			parser->next(parser, env);
			parser->next(parser, env);
		} else if (AssertOperator(parser->tokens[0], MultiplyToken)) {
			op = AMulAssign;
			parser->next(parser, env);
		} else if (AssertOperator(parser->tokens[0], DivideToken)) {
			op = ADivAssign;
			parser->next(parser, env);
		} else if (AssertOperator(parser->tokens[0], ModuloToken)) {
			op = AModAssign;
			parser->next(parser, env);
		} else if (AssertOperator(parser->tokens[0], GreaterToken)&&
		AssertOperator(parser->tokens[1], GreaterToken)) {
			op = AShiftRightAssign;
			parser->next(parser, env);
			parser->next(parser, env);
		} else if (AssertOperator(parser->tokens[0], LesserToken)&&
		AssertOperator(parser->tokens[1], LesserToken)) {
			op = AShiftLeftAssign;
			parser->next(parser, env);
			parser->next(parser, env);
		} else if (AssertOperator(parser->tokens[0], AndToken)&&
		AssertOperator(parser->tokens[1], AndToken)) {
			op = ALogicalAndAssign;
			parser->next(parser, env);
			parser->next(parser, env);
		} else if (AssertOperator(parser->tokens[0], OrToken)&&
		AssertOperator(parser->tokens[1], OrToken)) {
			op = ALogicalOrAssign;
			parser->next(parser, env);
			parser->next(parser, env);
		} else if (AssertOperator(parser->tokens[0], AndToken)) {
			op = AAndAssign;
			parser->next(parser, env);
		} else if (AssertOperator(parser->tokens[0], OrToken)) {
			op = AOrAssign;
			parser->next(parser, env);
		} else if (AssertOperator(parser->tokens[0], XorToken)) {
			op = AXorAssign;
			parser->next(parser, env);
		} else {
			op = AAssign;
		}
		if (op != AAssign)
			for (size_t i = 0; i < dest_count; i++) {
				if (dests[i]->type == FilledArrayN)
					ParseError(L"Can only assign to a tuple",
							{ for (size_t i=0; i<dest_count; i++) dests[i]->free(dests[i]); free(dests); if (type!=NULL) type->free(type); },
							parser);
			}
		parser->next(parser, env);

		size_t slength = 1;
		YNode** srcs = malloc(sizeof(YNode*) * slength);
		ExpectReduce(&srcs[0], expr, L"Expected expression",
				{ for (size_t i=0; i<dest_count; i++) dests[i]->free(dests[i]); free(dests); if (type!=NULL) type->free(type); free(srcs); },
				parser, env);
		while (AssertOperator(parser->tokens[0], CommaToken)) {
			parser->next(parser, env);
			YNode* nd;
			ExpectReduce(&nd, expr, L"Expected expression",
					{ for (size_t i=0; i<dest_count; i++) dests[i]->free(dests[i]); free(dests); for (size_t i=0; i<slength; i++) srcs[i]->free(srcs[i]); free(srcs); if (type!=NULL) type->free(type); node->free(node); },
					parser, env);
			slength++;
			srcs = realloc(srcs, sizeof(YNode*) * slength);
			srcs[slength - 1] = nd;
		}
		YNode* out = newAssignmentNode(op, newVar, type, srcs, slength, dests,
				dest_count);
		SetCoords(out, file, line, charPos);
		return out;
	}
	if (newVar || type != NULL) {
		if (dests == NULL) {
			dests = malloc(sizeof(YNode*));
			dests[0] = node;
			dest_count++;
		}
		YNode** srcs = malloc(sizeof(YNode*));
		srcs[0] = newConstantNode(
				env->bytecode->getNullConstant(env->bytecode));
		YNode* out = newAssignmentNode(AAssign, newVar, type, srcs, 1, dests,
				dest_count);
		SetCoords(out, file, line, charPos);
		return out;
	}
	SetCoords(node, file, line, charPos);
	return node;
}
NewValidate(Statement_validate) {
	return parser->grammar.expression.validate(parser, env);
}
NewReduce(Statement_reduce) {
	uint32_t line = parser->tokens[0]->line;
	uint32_t charPos = parser->tokens[0]->charPos;
	wchar_t* fname = parser->stream->fileName;
	YNode* out;
	ExpectReduce(&out, expression, L"Expect expression", ;, parser, env);
	if (AssertOperator(parser->tokens[0], SemicolonToken))
		parser->next(parser, env);
	out->line = line;
	out->charPos = charPos;
	out->fname = env->bytecode->getSymbolId(env->bytecode, fname);
	return out;
}

NewValidate(Function_validate) {
	return AssertKeyword(parser->tokens[0], FunctionKeyword);
}
NewReduce(Function_reduce) {
	ExtractCoords(file, line, charPos, parser, env);
	parser->next(parser, env);
	ExpectToken(parser->tokens[0], IdentifierToken, L"Expected identifier", ;,
			parser);
	int32_t id = ((YIdentifierToken*) parser->tokens[0])->id;
	parser->next(parser, env);
	size_t length = 0;
	int32_t* args = NULL;
	YNode** argTypes = NULL;
	bool vararg = false;
	ExpectOperator(parser->tokens[0], OpeningParentheseToken, L"Expected '('",
			;, parser);
	parser->next(parser, env);
	while (!AssertOperator(parser->tokens[0], ClosingParentheseToken)) {
		if (AssertOperator(parser->tokens[0], QueryToken)) {
			vararg = true;
			parser->next(parser, env);
		}
		ExpectToken(parser->tokens[0], IdentifierToken, L"Expected identifier",
				{ free(args); for (size_t i=0;i<length;i++) if (argTypes[i]!=NULL) argTypes[i]->free(argTypes[i]); free(argTypes); },
				parser);
		length++;
		args = realloc(args, sizeof(int32_t) * length);
		args[length - 1] = ((YIdentifierToken*) parser->tokens[0])->id;
		argTypes = realloc(argTypes, sizeof(YNode*) * length);
		argTypes[length - 1] = NULL;
		parser->next(parser, env);
		if (AssertOperator(parser->tokens[0], MinusToken)&&
		AssertOperator(parser->tokens[1], GreaterToken)) {
			parser->next(parser, env);
			parser->next(parser, env);
			ExpectReduce(&argTypes[length - 1], expr, L"Expected expression",
					{ free(args); for (size_t i=0;i<length;i++) if (argTypes[i]!=NULL) argTypes[i]->free(argTypes[i]); free(argTypes); },
					parser, env);
		}
		Expect(
				AssertOperator(parser->tokens[0], CommaToken)||AssertOperator(parser->tokens[0], ClosingParentheseToken),
				L"Expects ')' or ','",
				{ free(args); for (size_t i=0;i<length;i++) if (argTypes[i]!=NULL) argTypes[i]->free(argTypes[i]); free(argTypes); },
				parser);
		Expect(
				!(vararg&&!AssertOperator(parser->tokens[0], ClosingParentheseToken)),
				L"Vararg must be last, argument",
				{ free(args); for (size_t i=0;i<length;i++) if (argTypes[i]!=NULL) argTypes[i]->free(argTypes[i]); free(argTypes); },
				parser);
		if (AssertOperator(parser->tokens[0], CommaToken))
			parser->next(parser, env);
	}
	parser->next(parser, env);
	YNode* retType = NULL;
	if (AssertOperator(parser->tokens[0], MinusToken)&&
	AssertOperator(parser->tokens[1], GreaterToken)) {
		parser->next(parser, env);
		parser->next(parser, env);
		ExpectReduce(&retType, expr, L"Expected expression",
				{ free(args); for (size_t i=0;i<length;i++) if (argTypes[i]!=NULL) argTypes[i]->free(argTypes[i]); free(argTypes); },
				parser, env);
	}
	YNode* body;
	ExpectReduce(&body, statement, L"Expected statement",
			{ free(args); for (size_t i=0;i<length;i++) if (argTypes[i]!=NULL) argTypes[i]->free(argTypes[i]); free(argTypes); if (retType!=NULL) retType->free(retType); },
			parser, env);
	YNode* out = newFunctionNode(id,
			(YLambdaNode*) newLambdaNode(args, argTypes, length, vararg,
					retType, body));
	SetCoords(out, file, line, charPos);
	return out;
}

NewValidate(Root_validate) {
	return true;
}
NewReduce(Root_reduce) {
	ExtractCoords(file, line, charPos, parser, env);
	YFunctionBlock* funcs = NULL;
	size_t funcs_c = 0;

	YNode** block = NULL;
	size_t length = 0;

	while (parser->tokens[0] != NULL) {
		if (parser->grammar.statement.validate(parser, env)) {
			YNode* nd;
			nd = parser->grammar.statement.reduce(parser, env);
			if (nd == NULL)
				continue;
			length++;
			block = realloc(block, sizeof(YNode*) * length);
			block[length - 1] = nd;
		} else if (parser->grammar.function.validate(parser, env)) {
			YFunctionNode* func =
					(YFunctionNode*) parser->grammar.function.reduce(parser,
							env);
			if (func == NULL)
				continue;
			for (size_t i = 0; i < funcs_c; i++)
				if (funcs[i].id == func->name) {
					funcs[i].count++;
					funcs[i].lambda = realloc(funcs[i].lambda,
							sizeof(YLambdaNode*) * funcs[i].count);
					funcs[i].lambda[funcs[i].count - 1] = func->lambda;
					free(func);
					func = NULL;
					break;
				}
			if (func != NULL) {
				funcs_c++;
				funcs = realloc(funcs, sizeof(YFunctionBlock) * funcs_c);
				funcs[funcs_c - 1].id = func->name;
				funcs[funcs_c - 1].count = 1;
				funcs[funcs_c - 1].lambda = malloc(sizeof(YLambdaNode*));
				funcs[funcs_c - 1].lambda[0] = func->lambda;
				free(func);
				func = NULL;
			}
		} else {
			parser->next(parser, env);
		}
	}
	YNode* out = newBlockNode(block, length, funcs, funcs_c);
	out->type = RootN;
	SetCoords(out, file, line, charPos);
	return out;
}
void initGrammar(Grammar* g) {
	NewRule(g, constant, Constant_validate, Constant_reduce);
	NewRule(g, identifier, Identifier_validate, Identifier_reduce);

	NewRule(g, object, Object_validate, Object_reduce);
	NewRule(g, array, Array_validate, PArray_reduce);
	NewRule(g, overload, Overload_validate, Overload_reduce);
	NewRule(g, lambda, Lambda_validate, Lambda_reduce);
	NewRule(g, interface, Interface_validate, Interface_reduce);
	NewRule(g, declaration, Declaration_validate, Declaration_reduce);
	NewRule(g, factor, Factor_validate, Factor_reduce);
	NewRule(g, reference, Reference_validate, Reference_reduce);
	NewRule(g, unary, Unary_validate, Unary_reduce);
	NewRule(g, power, Power_validate, Power_reduce);
	NewRule(g, mul_div, Mul_div_validate, Mul_div_reduce);
	NewRule(g, add_sub, Add_sub_validate, Add_sub_reduce);
	NewRule(g, bitshift, Bitshift_validate, Bitshift_reduce);
	NewRule(g, bitwise, Bitwise_validate, Bitwise_reduce);
	NewRule(g, comparison, Comparison_validate, Comparison_reduce);
	NewRule(g, logical_not, Logical_not_validate, Logical_not_reduce);
	NewRule(g, logical_ops, Logical_ops_validate, Logical_ops_reduce);

	NewRule(g, expr, Expr_validate, Expr_reduce);
	NewRule(g, expression, Expression_validate, Expression_reduce);
	NewRule(g, statement, Statement_validate, Statement_reduce);

	NewRule(g, function, Function_validate, Function_reduce);

	NewRule(g, root, Root_validate, Root_reduce);
}
YToken* YParser_next(YParser* parser, YoyoCEnvironment* env) {
	if (parser->tokens[0] != NULL)
		parser->tokens[0]->free(parser->tokens[0]);
	parser->tokens[0] = parser->tokens[1];
	parser->tokens[1] = parser->tokens[2];
	parser->tokens[2] = parser->tokens[3];
	parser->tokens[3] = parser->stream->next(parser->stream, env);
	return parser->tokens[0];
}

YNode* yparse(YoyoCEnvironment* env, TokenStream* stream, YParser* parser) {

	if (stream == NULL) {
		parser->err_flag = true;
		parser->root = NULL;
		return NULL;
	}
	parser->stream = stream;
	parser->next = YParser_next;
	parser->tokens[0] = stream->next(stream, env);
	parser->tokens[1] = stream->next(stream, env);
	parser->tokens[2] = stream->next(stream, env);
	parser->tokens[3] = stream->next(stream, env);
	initGrammar(&parser->grammar);
	parser->err_flag = false;

	parser->root = parser->grammar.root.reduce(parser, env);
	parser->stream = NULL;
	stream->close(stream, env);

	return parser->root;
}


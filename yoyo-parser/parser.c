#include "headers/parser.h"



wchar_t* getSymbol(ParseHandle* handle, wchar_t* wcs) {
	for (size_t i=0;i<handle->symbols_size;i++)
		if (wcscmp(handle->symbols[i], wcs)==0)
			return handle->symbols[i];
	size_t i = handle->symbols_size;
	handle->symbols = realloc(handle->symbols, sizeof(wchar_t*) * (++handle->symbols_size));
	handle->symbols[i] = calloc(wcslen(wcs)+1, sizeof(wchar_t));
	wcscpy(handle->symbols[i], wcs);
	return handle->symbols[i];
}

yconstant_t addConstant(ParseHandle* handle, yconstant_t c) {
	size_t i = handle->constants_size++;
	handle->constants = realloc(handle->constants, sizeof(yconstant_t) * handle->constants_size);
	handle->constants[i] = c;
	if (c.type==WcsConstant)
		c.value.wcs = getSymbol(handle, c.value.wcs);
	return c;
}

ytoken shift(ParseHandle* handle) {
	handle->tokens[0] = handle->tokens[1];
	handle->tokens[1] = handle->tokens[2];
	handle->tokens[2] = handle->tokens[3];
	handle->tokens[3] = lex(handle);
	return handle->tokens[0];
}

NewValidate(Constant_validate) {
	return AssertToken(handle->tokens[0], TokenConstant);
}
NewReduce(Constant_reduce) {
	yconstant_t cnst = handle->tokens[0].value.cnst;
	ExtractCoords(file, line, charPos, handle);
	shift(handle);
	YNode* node = newConstantNode(cnst);
	SetCoords(node, file, line, charPos);
	return node;
}

NewValidate(Identifier_validate) {
	return AssertToken(handle->tokens[0], TokenIdentifier);
}
NewReduce(Identifier_reduce) {
	wchar_t* id = handle->tokens[0].value.id;
	ExtractCoords(file, line, charPos, handle);
	shift(handle);
	YNode* node = newIdentifierReferenceNode(id);
	SetCoords(node, file, line, charPos);
	return node;
}

NewValidate(Parser_Array_validate) {
    return false;
}
NewReduce(Parser_Array_reduce) {
    return NULL;
}

NewValidate(Object_validate) {
    return false;
}
NewReduce(Object_reduce) {
    return NULL;
}

NewValidate(Lambda_validate) {
    return false;
}
NewReduce(Lambda_reduce) {
    return NULL;
}

NewValidate(Overload_validate) {
    return false;
}
NewReduce(Overload_reduce) {
    return NULL;
}

NewValidate(Interface_validate) {
    return false;
}
NewReduce(Interface_reduce) {
    return NULL;
}

NewValidate(Factor_validate) {
	return Validate(identifier, handle)||
         	Validate(constant, handle)||
					AssertOperator(handle->tokens[0], OpeningParentheseOperator);
}
NewReduce(Factor_reduce) {
	YNode* out = NULL;
	ExtractCoords(file, line, charPos, handle);
	if (Validate(identifier, handle))
		out = handle->grammar.identifier.reduce(handle);
	else if (Validate(constant, handle))
		out = handle->grammar.constant.reduce(handle);
	else if (AssertOperator(handle->tokens[0], OpeningParentheseOperator)) {
		shift(handle);
		ExpectReduce(&out, expression, L"Expected expression", ;, handle);
		ExpectOperator(handle->tokens[0], ClosingParentheseOperator, L"Expected ')'", ;, handle);
		shift(handle);
	} 
	else
		ParseError(L"Expected expression", ;, handle);
	SetCoords(out, file, line, charPos);
	return out;
}

NewValidate(Reference_validate) {
	return Validate(factor, handle);
}
NewReduce(Reference_reduce) {
	YNode* node;
	ExtractCoords(file, line, charPos, handle);
    ExpectReduce(&node, factor, L"Expected expression", ;, handle);
    while (AssertOperator(handle->tokens[0], DotOperator)||
            AssertOperator(handle->tokens[0], OpeningBracketOperator)||
						AssertOperator(handle->tokens[0], OpeningParentheseOperator)) {
        if (AssertOperator(handle->tokens[0], DotOperator)) {
            shift(handle);
            ExpectToken(handle->tokens[0], TokenIdentifier, L"Expected identifier", node->free(node);, handle);
            wchar_t* id = handle->tokens[0].value.id;
            shift(handle);
            node = newFieldReferenceNode(node, id);
        } else if (AssertOperator(handle->tokens[0], OpeningBracketOperator)) {
            shift(handle);
            YNode* left;
            ExpectReduce(&left, expression, L"Expected expression", node->free(node), handle);
            if (AssertOperator(handle->tokens[0], ColonOperator)) {
                shift(handle);
                YNode* right;
                ExpectReduce(&right, expression, L"Expected expression", {node->free(node); left->free(left);}, handle);
                ExpectOperator(handle->tokens[0], ClosingBracketOperator, L"Expected ']'", {node->free(node);
                    right->free(right); left->free(left);}, handle);
                shift(handle);
                node = newSubseqReferenceNode(node, left, right);
            } else {
                ExpectOperator(handle->tokens[0], ClosingBracketOperator, L"Expected ']'",
									{node->free(node); left->free(left);}, handle);
                shift(handle);
                node = newIndexReferenceNode(node, left);
            }
        } else if (AssertOperator(handle->tokens[0], OpeningParentheseOperator)) {
					shift(handle);
					YNode** args = NULL;
					size_t len = 0;
#define freestmt {\
					for (size_t i=0;i<len;i++) args[i]->free(args[i]);\
					free(args);\
					node->free(node);\
				}
					while (!AssertOperator(handle->tokens[0], ClosingParentheseOperator)) {
						YNode* n;
						ExpectReduce(&n, expr, L"Expected expression", freestmt, handle);
						args = realloc(args, sizeof(YNode*) * (++len));
						args[len-1] = n;
						if (AssertOperator(handle->tokens[0], CommaOperator))
							shift(handle);
						else if (!AssertOperator(handle->tokens[0], ClosingParentheseOperator))
							ParseError(L"Expected ')' or ','", freestmt, handle);
					}
					shift(handle);
					node = newCallNode(node, args, len);
#undef freestmt
				}
    }
	SetCoords(node, file, line, charPos);
    return node;
}

NewValidate(Unary_validate) {
	return Validate(reference, handle);
}
NewReduce(Unary_reduce) {
	YNode* node;
    ExpectReduce(&node, reference, L"Expected expression", ;, handle);
    return node;
}

NewValidate(Power_validate) {
	return Validate(unary, handle);
}
NewReduce(Power_reduce) {
	YNode* node;
    ExpectReduce(&node, unary, L"Expected expression", ;, handle);
    return node;
}

NewValidate(Mul_div_validate) {
	return Validate(power, handle);
}
NewReduce(Mul_div_reduce) {
	YNode* node;
	ExtractCoords(file, line, charPos, handle);
    ExpectReduce(&node, power, L"Expected expression", ;, handle);
		while (AssertOperator(handle->tokens[0], MultiplyOperator)||
						AssertOperator(handle->tokens[0], DivideOperator)||
						AssertOperator(handle->tokens[0], ModuloOperator)) {
					YBinaryOperation op;
					if (AssertOperator(handle->tokens[0], MultiplyOperator))
						op = Multiply;
					if (AssertOperator(handle->tokens[0], DivideOperator))
						op = Divide;
					if (AssertOperator(handle->tokens[0], ModuloOperator))
						op = Modulo;
					shift(handle);
					YNode* left;
					ExpectReduce(&left, power, L"Expected expression", node->free(node);, handle);
					node = newBinaryNode(op, node, left);
		}
		SetCoords(node, file, line, charPos);
    return node;
}

NewValidate(Add_sub_validate) {
	return Validate(mul_div, handle);
}
NewReduce(Add_sub_reduce) {
    YNode* node;
	ExtractCoords(file, line, charPos, handle);
    ExpectReduce(&node, mul_div, L"Expected expression", ;, handle);
		while (AssertOperator(handle->tokens[0], PlusOperator)||
                AssertOperator(handle->tokens[0], MinusOperator)) {
					YBinaryOperation op;
					if (AssertOperator(handle->tokens[0], PlusOperator))
						op = Add;
					if (AssertOperator(handle->tokens[0], MinusOperator))
						op = Subtract;
					shift(handle);
					YNode* left;
					ExpectReduce(&left, mul_div, L"Expected expression", node->free(node);, handle);
					node = newBinaryNode(op, node, left);
		}
		SetCoords(node, file, line, charPos);
    return node;
}

NewValidate(Bitshift_validate) {
	return Validate(add_sub, handle);
}
NewReduce(Bitshift_reduce) {
    YNode* node;
	ExtractCoords(file, line, charPos, handle);
    ExpectReduce(&node, add_sub, L"Expected expression", ;, handle);
		while ((AssertOperator(handle->tokens[0], GreaterOperator)&&
                AssertOperator(handle->tokens[1], GreaterOperator))||
              (AssertOperator(handle->tokens[0], LesserOperator)&&
                AssertOperator(handle->tokens[1], LesserOperator))) {
					YBinaryOperation op;
					if (AssertOperator(handle->tokens[0], GreaterOperator))
						op = ShiftRight;
					if (AssertOperator(handle->tokens[0], LesserOperator))
						op = ShiftLeft;
					shift(handle);
                    shift(handle);
					YNode* left;
					ExpectReduce(&left, add_sub, L"Expected expression", node->free(node);, handle);
					node = newBinaryNode(op, node, left);
		}
		SetCoords(node, file, line, charPos);
    return node;
}

NewValidate(Bitwise_validate) {
	return Validate(bitshift, handle);
}
NewReduce(Bitwise_reduce) {
    YNode* node;
	ExtractCoords(file, line, charPos, handle);
    ExpectReduce(&node, bitshift, L"Expected expression", ;, handle);
    while (AssertOperator(handle->tokens[0], AndOperator)||
                AssertOperator(handle->tokens[0], OrOperator)||
                AssertOperator(handle->tokens[0], XorOperator)) {
                    if (AssertOperator(handle->tokens[1], AndOperator)||
                        AssertOperator(handle->tokens[1], OrOperator))
                        break;
					YBinaryOperation op;
					if (AssertOperator(handle->tokens[0], AndOperator))
						op = And;
					if (AssertOperator(handle->tokens[0], OrOperator))
						op = Or;
					if (AssertOperator(handle->tokens[0], XorOperator))
						op = Xor;
					shift(handle);
					YNode* left;
					ExpectReduce(&left, bitshift, L"Expected expression", node->free(node);, handle);
					node = newBinaryNode(op, node, left);
		}
		SetCoords(node, file, line, charPos);
    return node;
}

NewValidate(Comparison_validate) {
	return Validate(bitwise, handle);
}
NewReduce(Comparison_reduce) {
	YNode* node;
	ExtractCoords(file, line, charPos, handle);
    ExpectReduce(&node, bitwise, L"Expected expression", ;, handle);
    if (AssertOperator(handle->tokens[0], AssignOperator)&&
        AssertOperator(handle->tokens[1], AssignOperator)) {
        shift(handle);
        shift(handle);
        YNode* left;
        ExpectReduce(&left, bitwise, L"Expected expression", node->free(node), handle);
        node = newBinaryNode(BEquals, left, node);
    } else if (AssertOperator(handle->tokens[0], LogicalNotOperator)&&
        AssertOperator(handle->tokens[1], AssignOperator)) {
        shift(handle);
        shift(handle);
        YNode* left;
        ExpectReduce(&left, bitwise, L"Expected expression", node->free(node), handle);
        node = newBinaryNode(NotEquals, left, node);
    } else if (AssertOperator(handle->tokens[0], GreaterOperator)&&
        AssertOperator(handle->tokens[1], AssignOperator)) {
        shift(handle);
        shift(handle);
        YNode* left;
        ExpectReduce(&left, bitwise, L"Expected expression", node->free(node), handle);
        node = newBinaryNode(GreaterOrEquals, left, node);
    } else if (AssertOperator(handle->tokens[0], LesserOperator)&&
        AssertOperator(handle->tokens[1], AssignOperator)) {
        shift(handle);
        shift(handle);
        YNode* left;
        ExpectReduce(&left, bitwise, L"Expected expression", node->free(node), handle);
        node = newBinaryNode(LesserOrEquals, left, node);
    } else if (AssertOperator(handle->tokens[0], LesserOperator)) {
        shift(handle);
        YNode* left;
        ExpectReduce(&left, bitwise, L"Expected expression", node->free(node), handle);
        node = newBinaryNode(Lesser, left, node);
    } else if (AssertOperator(handle->tokens[0], GreaterOperator)) {
        shift(handle);
        YNode* left;
        ExpectReduce(&left, bitwise, L"Expected expression", node->free(node), handle);
        node = newBinaryNode(Greater, left, node);
    }
    SetCoords(node, file, line, charPos);
    return node;
}

NewValidate(Logical_not_validate) {
	return Validate(comparison, handle)||
            AssertOperator(handle->tokens[0], LogicalNotOperator);
}
NewReduce(Logical_not_reduce) {
	YNode* node;
	ExtractCoords(file, line, charPos, handle);
    if (AssertOperator(handle->tokens[0], LogicalNotOperator)) {
        shift(handle);
        ExpectReduce(&node, comparison, L"Expected expression", ;, handle);
       node = newUnaryNode(LogicalNot, node);
    } else {
        ExpectReduce(&node, comparison, L"Expected expression", ;, handle);
    }
    SetCoords(node, file, line, charPos);
    return node;
}

NewValidate(Logical_ops_validate) {
	return Validate(logical_not, handle);
}
NewReduce(Logical_ops_reduce) {
    YNode* node;
	ExtractCoords(file, line, charPos, handle);
    ExpectReduce(&node, logical_not, L"Expected expression", ;, handle);
		while ((AssertOperator(handle->tokens[0], AndOperator)&&
                AssertOperator(handle->tokens[1], AndOperator))||
              (AssertOperator(handle->tokens[0], OrOperator)&&
                AssertOperator(handle->tokens[1], OrOperator))) {
					YBinaryOperation op;
					if (AssertOperator(handle->tokens[0], AndOperator))
						op = LogicalAnd;
					if (AssertOperator(handle->tokens[0], OrOperator))
						op = LogicalOr;
					shift(handle);
                    shift(handle);
					YNode* left;
					ExpectReduce(&left, logical_not, L"Expected expression", node->free(node);, handle);
					node = newBinaryNode(op, node, left);
		}
		SetCoords(node, file, line, charPos);
    return node;
}

NewValidate(Expr_validate) {
	return Validate(logical_ops, handle)||
					AssertKeyword(handle->tokens[0], IfKeyword);
}
NewReduce(Expr_reduce) {
	YNode* node;
	ExtractCoords(file, line, charPos, handle);
	if (AssertKeyword(handle->tokens[0], IfKeyword)) {
		shift(handle);
		YNode* cond;
		YNode* body;
		YNode* elseBody = NULL;
		ExpectReduce(&cond, expression, L"Expected expression", ;, handle);
		ExpectReduce(&body, statement, L"Expected statement", cond->free(cond);, handle);
		if (AssertKeyword(handle->tokens[0], ElseKeyword)) {
			shift(handle);
			ExpectReduce(&elseBody, statement, L"Expected statement", {cond->free(cond); body->free(body);}, handle);
		}
		node = newConditionNode(cond, body, elseBody);
	} else
		ExpectReduce(&node, logical_ops, L"Expected expression", ;, handle);
	SetCoords(node, file, line, charPos);
  return node;
}

NewValidate(Expression_validate) {
	return Validate(expr, handle);
}
NewReduce(Expression_reduce) {
	/*YNode* node;
    ExpectReduce(&node, expr, L"Expected expression", ;, handle);
    return node;*/
	ExtractCoords(file, line, charPos, handle);
	if (AssertKeyword(handle->tokens[0], DelKeyword)) {
		shift(handle);
		size_t length = 1;
		YNode** list = malloc(sizeof(YNode*) * length);
		ExpectReduce(&list[0], expr, L"Expected expression", free(list), handle);
#define freestmt {\
            for (size_t i=0;i<length;i++)\
                list[i]->free(list[i]);\
            free(list);\
		}
		while (AssertOperator(handle->tokens[0], CommaOperator)) {
			shift(handle);
			YNode* n;
			ExpectReduce(&n, expr, L"Expected expression", freestmt, handle);
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
	if (AssertKeyword(handle->tokens[0], VarKeyword)) {
		shift(handle);
		newVar = true;
	}
	YNode* type = NULL;
	if (AssertOperator(handle->tokens[0], MinusOperator)&&
			AssertOperator(handle->tokens[1], GreaterOperator)) {
		shift(handle);
		shift(handle);
		ExpectReduce(&type, expr, L"Expected expression", ;, handle);
	}
	YNode* node = NULL;
	ExpectReduce(&node, expr, L"Expected expression",
			if (type!=NULL) type->free(type);, handle);
	YNode** dests = NULL;
	size_t dest_count = 0;
	if (AssertOperator(handle->tokens[0], CommaOperator)) {
		dests = malloc(sizeof(YNode*));
		dests[0] = node;
		dest_count++;
		while (AssertOperator(handle->tokens[0], CommaOperator)) {
			shift(handle);
			YNode* n;
			ExpectReduce(&n, expr, L"Expected expression", {
				for (size_t i = 0; i < dest_count; i++)
					dests[i]->free(dests[i])
					;
				free(dests)
				;
			}, handle);
			dests = realloc(dests, sizeof(YNode*) * (++dest_count));
			dests[dest_count - 1] = n;
		}
	}
	if (AssertOperator(handle->tokens[0], AssignOperator)
			|| ((AssertOperator(handle->tokens[0], PlusOperator)
					|| AssertOperator(handle->tokens[0], MinusOperator)
					|| AssertOperator(handle->tokens[0], MultiplyOperator)
					|| AssertOperator(handle->tokens[0], DivideOperator)
					|| AssertOperator(handle->tokens[0], ModuloOperator)
					|| AssertOperator(handle->tokens[0], AndOperator)
					|| AssertOperator(handle->tokens[0], OrOperator)
					|| AssertOperator(handle->tokens[0], XorOperator))
					&& AssertOperator(handle->tokens[1], AssignOperator))
			|| (((AssertOperator(handle->tokens[0], MultiplyOperator)
					&& AssertOperator(handle->tokens[1], MultiplyOperator))
					|| (AssertOperator(handle->tokens[0], LesserOperator)
							&& AssertOperator(handle->tokens[1], LesserOperator))
					|| (AssertOperator(handle->tokens[0], GreaterOperator)
							&& AssertOperator(handle->tokens[1], GreaterOperator))
					|| (AssertOperator(handle->tokens[0], AndOperator)
							&& AssertOperator(handle->tokens[1], AndOperator))
					|| (AssertOperator(handle->tokens[0], OrOperator)
							&& AssertOperator(handle->tokens[1], OrOperator)))
					&& AssertOperator(handle->tokens[2], AssignOperator))) {
		if (dests == NULL) {
			dests = malloc(sizeof(YNode*));
			dests[0] = node;
			dest_count++;
		}
		YAssignmentOperation op;
		if (AssertOperator(handle->tokens[0], PlusOperator)) {
			op = AAddAssign;
			shift(handle);
		} else if (AssertOperator(handle->tokens[0], MinusOperator)) {
			op = ASubAssign;
			shift(handle);
		} else if (AssertOperator(handle->tokens[0], MultiplyOperator)&&
		AssertOperator(handle->tokens[1], MultiplyOperator)) {
			op = APowerAssign;
			shift(handle);
			shift(handle);
		} else if (AssertOperator(handle->tokens[0], MultiplyOperator)) {
			op = AMulAssign;
			shift(handle);
		} else if (AssertOperator(handle->tokens[0], DivideOperator)) {
			op = ADivAssign;
			shift(handle);
		} else if (AssertOperator(handle->tokens[0], ModuloOperator)) {
			op = AModAssign;
			shift(handle);
		} else if (AssertOperator(handle->tokens[0], GreaterOperator)&&
		AssertOperator(handle->tokens[1], GreaterOperator)) {
			op = AShiftRightAssign;
			shift(handle);
			shift(handle);
		} else if (AssertOperator(handle->tokens[0], LesserOperator)&&
		AssertOperator(handle->tokens[1], LesserOperator)) {
			op = AShiftLeftAssign;
			shift(handle);
			shift(handle);
		} else if (AssertOperator(handle->tokens[0], AndOperator)&&
		AssertOperator(handle->tokens[1], AndOperator)) {
			op = ALogicalAndAssign;
			shift(handle);
			shift(handle);
		} else if (AssertOperator(handle->tokens[0], OrOperator)&&
		AssertOperator(handle->tokens[1], OrOperator)) {
			op = ALogicalOrAssign;
			shift(handle);
			shift(handle);
		} else if (AssertOperator(handle->tokens[0], AndOperator)) {
			op = AAndAssign;
			shift(handle);
		} else if (AssertOperator(handle->tokens[0], OrOperator)) {
			op = AOrAssign;
			shift(handle);
		} else if (AssertOperator(handle->tokens[0], XorOperator)) {
			op = AXorAssign;
			shift(handle);
		} else {
			op = AAssign;
		}
		if (op != AAssign)
			for (size_t i = 0; i < dest_count; i++) {
				if (dests[i]->type == FilledArrayN)
					ParseError(L"Can only assign to a tuple",
							{ for (size_t i=0; i<dest_count; i++) dests[i]->free(dests[i]); free(dests); if (type!=NULL) type->free(type); },
							handle);
			}
		shift(handle);

		size_t slength = 1;
		YNode** srcs = malloc(sizeof(YNode*) * slength);
		ExpectReduce(&srcs[0], expr, L"Expected expression",
				{ for (size_t i=0; i<dest_count; i++) dests[i]->free(dests[i]); free(dests); if (type!=NULL) type->free(type); free(srcs); },
				handle);
		while (AssertOperator(handle->tokens[0], CommaOperator)) {
			shift(handle);
			YNode* nd;
			ExpectReduce(&nd, expr, L"Expected expression",
					{ for (size_t i=0; i<dest_count; i++) dests[i]->free(dests[i]); free(dests); for (size_t i=0; i<slength; i++) srcs[i]->free(srcs[i]); free(srcs); if (type!=NULL) type->free(type); node->free(node); },
					handle);
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
		srcs[0] = newNullNode();
		YNode* out = newAssignmentNode(AAssign, newVar, type, srcs, 1, dests,
				dest_count);
		SetCoords(out, file, line, charPos);
		return out;
	}
	SetCoords(node, file, line, charPos);
	return node;
}

NewValidate(Statement_validate) {
	return Validate(expression, handle);
}
NewReduce(Statement_reduce) {
	YNode* node;
	ExpectReduce(&node, expression, L"Expected expression", ;, handle);
	if (AssertOperator(handle->tokens[0], SemicolonOperator))
		shift(handle);
	return node;
}

NewValidate(Root_validate) {
	return true;
}
NewReduce(Root_reduce) {
	ExtractCoords(file, line, charPos, handle);
	YNode** root = NULL;
	size_t length = 0;
#define freestmt {\
	for (size_t i=0;i<length;i++) root[i]->free(root[i]);\
	free(root);\
}
	while (!AssertToken(handle->tokens[0], TokenEOF)) {
		if (Validate(statement, handle)) {
			YNode* node;
			ExpectReduce(&node, statement, L"Expected statement",
				freestmt, handle);
			root = realloc(root, sizeof(YNode*) * (++length));
			root[length-1] = node;
		}
		else
			ParseError(L"Expected statement or function", freestmt, handle);
	}
	YNode* rootNode = newBlockNode(root, length, NULL, 0 );
	SetCoords(rootNode, file, line, charPos);
	return rootNode;
}

void initGrammar(Grammar* g) {
	NewRule(g, constant, Constant_validate, Constant_reduce);
	NewRule(g, identifier, Identifier_validate, Identifier_reduce);
	NewRule(g, lambda, Lambda_validate, Lambda_reduce);
	NewRule(g, object, Object_validate, Object_reduce);
	NewRule(g, array, Parser_Array_validate, Parser_Array_reduce);
	NewRule(g, overload, Overload_validate, Overload_reduce);
	NewRule(g, interface, Interface_validate, Interface_reduce);
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
	NewRule(g, root, Root_validate, Root_reduce);
}

YNode* parse(ParseHandle* handle) {
	initGrammar(&handle->grammar);
	return handle->grammar.root.reduce(handle);
}
/*
int main(int argc, char** argv) {
	argc--; argv++;
	if (argc==0) {
		printf("Specify input file!");
		exit(-1);
	}
	FILE* fd = fopen(argv[0], "r");
	ParseHandle handle;
	handle.input = fd;
	handle.error_stream = stderr;
	handle.fileName = L"test";
	handle.constants = NULL;
	handle.constants_size = 0;
	handle.symbols = NULL;
	handle.symbols_size = 0;
	handle.charPos = 0;
	handle.line = 1;
	handle.error_flag = false;

	shift(&handle);
	shift(&handle);
	shift(&handle);
	shift(&handle);
	YNode* node = parse(&handle);

	fclose(fd);

	pseudocode(node, stdout);
	node->free(node);
	return 0;
}
*/

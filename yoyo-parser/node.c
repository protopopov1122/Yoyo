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

#include "headers/node.h"

void Binary_free(YNode* n) {
	YBinaryNode* node = (YBinaryNode*) n;
	node->left->free(node->left);
	node->right->free(node->right);
	free(node);
}
void Unary_free(YNode* n) {
	YUnaryNode* un = (YUnaryNode*) n;
	un->argument->free(un->argument);
	free(un);
}
void Lambda_free(YNode* n) {
	YLambdaNode* node = (YLambdaNode*) n;
	node->body->free(node->body);
	free(node->args);
	for (size_t i = 0; i < node->argc; i++)
		if (node->argTypes[i] != NULL)
			node->argTypes[i]->free(node->argTypes[i]);
	free(node->argTypes);
	if (node->retType != NULL)
		node->retType->free(node->retType);
	free(node);
}
void Object_free(YNode* n) {
	YObjectNode* node = (YObjectNode*) n;
	if (node->parent != NULL)
		node->parent->free(node->parent);
	for (size_t i = 0; i < node->fields_length; i++) {
		node->fields[i].value->free(node->fields[i].value);
		if (node->fields[i].type != NULL)
			node->fields[i].type->free(node->fields[i].type);
	}
	free(node->fields);
	for (size_t i = 0; i < node->methods_length; i++) {
		for (size_t j = 0; j < node->methods[i].count; j++)
			((YNode*) node->methods[i].lambda[j])->free(
					(YNode*) node->methods[i].lambda[j]);
		free(node->methods[i].lambda);
	}
	free(node->methods);
	free(node);
}
void IndexReference_free(YNode* n) {
	YArrayReferenceNode* node = (YArrayReferenceNode*) n;
	node->array->free(node->array);
	node->index->free(node->index);
	free(node);
}
void SubsequenceReference_free(YNode* n) {
	YSubsequenceReferenceNode* node = (YSubsequenceReferenceNode*) n;
	node->array->free(node->array);
	node->from->free(node->from);
	node->to->free(node->to);
	free(node);
}
void FieldReference_free(YNode* n) {
	YFieldReferenceNode* node = (YFieldReferenceNode*) n;
	node->object->free(node->object);
	free(node);
}
void Call_free(YNode* n) {
	YCallNode* node = (YCallNode*) n;
	for (size_t i = 0; i < node->argc; i++) {
		node->args[i]->free(node->args[i]);
	}
	free(node->args);
	node->function->free(node->function);
	free(node);
}
void GeneratedArray_free(YNode* n) {
	YGeneratedArrayNode* node = (YGeneratedArrayNode*) n;
	node->element->free(node->element);
	node->count->free(node->count);
	free(node);
}
void FilledArray_free(YNode* n) {
	YFilledArrayNode* node = (YFilledArrayNode*) n;
	for (size_t i = 0; i < node->length; i++)
		node->array[i]->free(node->array[i]);
	free(node->array);
	free(node);
}
void InterfaceNode_free(YNode* n) {
	YInterfaceNode* node = (YInterfaceNode*) n;
	for (size_t i = 0; i < node->parent_count; i++)
		node->parents[i]->free(node->parents[i]);
	free(node->parents);
	for (size_t i = 0; i < node->attr_count; i++)
		node->types[i]->free(node->types[i]);
	free(node->ids);
	free(node->types);
	free(node);
}
void Assignment_free(YNode* n) {
	YAssignmentNode* node = (YAssignmentNode*) n;
	for (size_t i = 0; i < node->dest_len; i++)
		node->dest[i]->free(node->dest[i]);
	for (size_t i = 0; i < node->src_len; i++)
		node->src[i]->free(node->src[i]);
	free(node->src);
	free(node->dest);
	if (node->type != NULL)
		node->type->free(node->type);
	free(node);
}
void Delete_free(YNode* n) {
	YDeleteNode* node = (YDeleteNode*) n;
	for (size_t i = 0; i < node->length; i++)
		node->list[i]->free(node->list[i]);
	free(node->list);
	free(node);
}
void Condition_free(YNode* n) {
	YConditionNode* node = (YConditionNode*) n;
	node->cond->free(node->cond);
	node->body->free(node->body);
	if (node->elseBody != NULL)
		node->elseBody->free(node->elseBody);
	free(node);
}
void While_free(YNode* n) {
	YWhileLoopNode* node = (YWhileLoopNode*) n;
	node->cond->free(node->cond);
	node->body->free(node->body);
	free(node);
}
void For_free(YNode* n) {
	YForLoopNode* node = (YForLoopNode*) n;
	node->init->free(node->init);
	node->cond->free(node->cond);
	node->loop->free(node->loop);
	node->body->free(node->body);
	free(node);
}
void Foreach_free(YNode* n) {
	YForeachLoopNode* node = (YForeachLoopNode*) n;
	node->refnode->free(node->refnode);
	node->col->free(node->col);
	node->body->free(node->body);
	free(node);
}
void Loop_free(YNode* n) {
	YLoopNode* node = (YLoopNode*) n;
	node->body->free(node->body);
	free(node);
}
void ValueReturn_free(YNode* n) {
	YValueReturnNode* node = (YValueReturnNode*) n;
	node->value->free(node->value);
	free(node);
}
void Try_free(YNode* n) {
	YTryNode* node = (YTryNode*) n;
	node->tryBody->free(node->tryBody);
	if (node->catchRef != NULL) {
		node->catchRef->free(node->catchRef);
		node->catchBody->free(node->catchBody);
	}
	if (node->elseBody != NULL)
		node->elseBody->free(node->elseBody);
	if (node->finBody != NULL)
		node->finBody->free(node->finBody);
	free(node);
}
void Switch_free(YNode* n) {
	YSwitchNode* sw = (YSwitchNode*) n;
	sw->value->free(sw->value);
	for (size_t i = 0; i < sw->length; i++) {
		sw->cases[i].value->free(sw->cases[i].value);
		sw->cases[i].stmt->free(sw->cases[i].stmt);
	}
	free(sw->cases);
	if (sw->defCase != NULL)
		sw->defCase->free(sw->defCase);
	free(sw);
}
void Overload_free(YNode* n) {
	YOverloadNode* node = (YOverloadNode*) n;
	for (size_t i = 0; i < node->count; i++)
		node->lambdas[i]->free(node->lambdas[i]);
	free(node->lambdas);
	if (node->defLambda != NULL)
		node->defLambda->free(node->defLambda);
	free(node);
}
void Using_free(YNode* n) {
	YUsingNode* node = (YUsingNode*) n;
	for (size_t i = 0; i < node->count; i++)
		node->scopes[i]->free(node->scopes[i]);
	free(node->scopes);
	node->body->free(node->body);
	free(node);
}
void With_free(YNode* n) {
	YWithNode* node = (YWithNode*) n;
	node->scope->free(node->scope);
	node->body->free(node->body);
	free(node);
}
void Block_free(YNode* n) {
	YBlockNode* node = (YBlockNode*) n;
	for (size_t i = 0; i < node->length; i++) {
		node->block[i]->free(node->block[i]);
	}
	free(node->block);
	for (size_t i = 0; i < node->funcs_count; i++) {
		for (size_t j = 0; j < node->funcs[i].count; j++)
			((YNode*) node->funcs[i].lambda[j])->free(
					(YNode*) node->funcs[i].lambda[j]);
		free(node->funcs[i].lambda);
	}
	free(node->funcs);
	free(node);
}
void Function_free(YNode* n) {
	YFunctionNode* node = (YFunctionNode*) n;
	((YNode*) node->lambda)->free((YNode*) node->lambda);
	free(node);
}
YNode* newNullNode() {
	YNode* node = malloc(sizeof(YNode));
	node->type = NullN;
	node->free = (void (*)(YNode*)) free;
	node->fileName = NULL;
	node->line = -1;
	node->charPos = -1;
	return node;
}
YNode* newBinaryNode(YBinaryOperation op, YNode* left, YNode* right) {
	YBinaryNode* bin;
	NewNode(&bin, YBinaryNode, BinaryN, Binary_free);
	bin->left = left;
	bin->right = right;
	bin->operation = op;
	return (YNode*) bin;
}
YNode* newLambdaNode(bool method, wchar_t** args, YNode** argTypes,
		ssize_t argc,
		bool vararg, YNode* retType, YNode* body) {
	YLambdaNode* ln;
	NewNode(&ln, YLambdaNode, LambdaN, Lambda_free);
	ln->method = method;
	ln->args = args;
	ln->argTypes = argTypes;
	ln->argc = argc;
	ln->body = body;
	ln->vararg = vararg;
	ln->retType = retType;
	return (YNode*) ln;
}
YNode* newObjectNode(YNode* parent, ObjectNodeField* fields, size_t flen,
		YFunctionBlock* meths, size_t mlen) {
	YObjectNode* obj;
	NewNode(&obj, YObjectNode, ObjectN, Object_free);
	obj->parent = parent;
	obj->fields = fields;
	obj->fields_length = flen;
	obj->methods = meths;
	obj->methods_length = mlen;
	return (YNode*) obj;
}
YNode* newUnaryNode(YUnaryOperation op, YNode* arg) {
	YUnaryNode* un;
	NewNode(&un, YUnaryNode, UnaryN, Unary_free);
	un->operation = op;
	un->argument = arg;
	return (YNode*) un;
}
YNode* newConstantNode(yconstant_t c) {
	YConstantNode* out;
	NewNode(&out, YConstantNode, ConstantN, free);
	out->id = c;
	return (YNode*) out;
}
YNode* newIdentifierReferenceNode(wchar_t* id) {
	YIdentifierReferenceNode* ref;
	NewNode(&ref, YIdentifierReferenceNode, IdentifierReferenceN, free);
	ref->id = id;
	return (YNode*) ref;
}
YNode* newIndexReferenceNode(YNode* arr, YNode* ind) {
	YArrayReferenceNode* ref;
	NewNode(&ref, YArrayReferenceNode, ArrayReferenceN, IndexReference_free);
	ref->array = arr;
	ref->index = ind;
	return (YNode*) ref;
}
YNode* newSubseqReferenceNode(YNode* arr, YNode* from, YNode* to) {
	YSubsequenceReferenceNode* ref;
	NewNode(&ref, YSubsequenceReferenceNode, SubsequenceN,
			SubsequenceReference_free);
	ref->array = arr;
	ref->from = from;
	ref->to = to;
	return (YNode*) ref;
}
YNode* newFieldReferenceNode(YNode* obj, wchar_t* fid) {
	YFieldReferenceNode* ref;
	NewNode(&ref, YFieldReferenceNode, FieldReferenceN, FieldReference_free);
	ref->object = obj;
	ref->field = fid;
	return (YNode*) ref;
}
YNode* newCallNode(YNode* scope, YNode* fun, YNode** args, size_t argc) {
	YCallNode* call;
	NewNode(&call, YCallNode, CallN, Call_free);
	call->scope = scope;
	call->function = fun;
	call->args = args;
	call->argc = argc;
	return (YNode*) call;
}
YNode* newFilledArray(YNode** array, size_t length) {
	YFilledArrayNode* arr;
	NewNode(&arr, YFilledArrayNode, FilledArrayN, FilledArray_free);
	arr->array = array;
	arr->length = length;
	return (YNode*) arr;
}
YNode* newGeneratedArray(YNode* el, YNode* count) {
	YGeneratedArrayNode* arr;
	NewNode(&arr, YGeneratedArrayNode, GeneratedArrayN, GeneratedArray_free);
	arr->element = el;
	arr->count = count;
	return (YNode*) arr;
}
YNode* newInterfaceNode(YNode** parents, size_t pcount, wchar_t** ids,
		YNode** types, size_t acount) {
	YInterfaceNode* node;
	NewNode(&node, YInterfaceNode, InterfaceN, InterfaceNode_free);
	node->parents = parents;
	node->parent_count = pcount;
	node->ids = ids;
	node->types = types;
	node->attr_count = acount;
	return (YNode*) node;
}
YNode* newAssignmentNode(YAssignmentOperation op, bool newVar, YNode* type,
		YNode** src, size_t src_s, YNode** dst, size_t dst_s) {
	YAssignmentNode* assn;
	NewNode(&assn, YAssignmentNode, AssignmentN, Assignment_free);
	assn->operation = op;
	assn->dest = dst;
	assn->dest_len = dst_s;
	assn->src = src;
	assn->src_len = src_s;
	assn->newVar = newVar;
	assn->type = type;
	return (YNode*) assn;
}
YNode* newDeleteNode(YNode** list, size_t len) {
	YDeleteNode* del;
	NewNode(&del, YDeleteNode, DeleteN, Delete_free);
	del->list = list;
	del->length = len;
	return (YNode*) del;
}

YNode* newConditionNode(YNode* cnd, YNode* body, YNode* elseBody) {
	YConditionNode* cond;
	NewNode(&cond, YConditionNode, ConditionN, Condition_free);
	cond->cond = cnd;
	cond->body = body;
	cond->elseBody = elseBody;
	return (YNode*) cond;
}
YNode* newLoopNode(wchar_t* id, YNode* body) {
	YLoopNode* loop;
	NewNode(&loop, YLoopNode, LoopN, Loop_free);
	loop->id = id;
	loop->body = body;
	return (YNode*) loop;
}
YNode* newWhileLoopNode(wchar_t* id, bool eos, YNode* cnd, YNode* body) {
	YWhileLoopNode* loop;
	NewNode(&loop, YWhileLoopNode, WhileLoopN, While_free);
	loop->id = id;
	loop->evalOnStart = eos;
	loop->cond = cnd;
	loop->body = body;
	return (YNode*) loop;
}
YNode* newForLoopNode(wchar_t* id, YNode* init, YNode* cnd, YNode* after,
		YNode* body) {
	YForLoopNode* loop;
	NewNode(&loop, YForLoopNode, ForLoopN, For_free);
	loop->id = id;
	loop->init = init;
	loop->cond = cnd;
	loop->loop = after;
	loop->body = body;
	return (YNode*) loop;
}
YNode* newForeachLoopNode(wchar_t* id, YNode* ref, YNode* col, YNode* body) {
	YForeachLoopNode* loop;
	NewNode(&loop, YForeachLoopNode, ForeachLoopN, Foreach_free);
	loop->id = id;
	loop->refnode = ref;
	loop->col = col;
	loop->body = body;
	return (YNode*) loop;
}
YNode* newPassNode() {
	YPassNode* pass;
	NewNode(&pass, YPassNode, PassN, free);
	return (YNode*) pass;
}
YNode* newBreakNode(wchar_t* id) {
	YLoopControlNode* brk;
	NewNode(&brk, YLoopControlNode, BreakN, free);
	brk->label = id;
	return (YNode*) brk;
}
YNode* newContinueNode(wchar_t* id) {
	YLoopControlNode* brk;
	NewNode(&brk, YLoopControlNode, ContinueN, free);
	brk->label = id;
	return (YNode*) brk;
}
YNode* newThrowNode(YNode* v) {
	YValueReturnNode* ret;
	NewNode(&ret, YValueReturnNode, ThrowN, ValueReturn_free);
	ret->value = v;
	return (YNode*) ret;
}
YNode* newReturnNode(YNode* v) {
	YValueReturnNode* ret;
	NewNode(&ret, YValueReturnNode, ReturnN, ValueReturn_free);
	ret->value = v;
	return (YNode*) ret;
}
YNode* newTryNode(YNode* tryB, YNode* cRef, YNode* cB, YNode* elB, YNode* finB) {
	YTryNode* node;
	NewNode(&node, YTryNode, TryN, Try_free);
	node->tryBody = tryB;
	node->catchRef = cRef;
	node->catchBody = cB;
	node->elseBody = elB;
	node->finBody = finB;
	return (YNode*) node;
}
YNode* newSwitchNode(YNode* val, YCaseNode* cases, size_t cases_len,
		YNode* defCase) {
	YSwitchNode* sw;
	NewNode(&sw, YSwitchNode, SwitchN, Switch_free);
	sw->value = val;
	sw->cases = cases;
	sw->length = cases_len;
	sw->defCase = defCase;
	return (YNode*) sw;
}
YNode* newOverloadNode(YNode** lmbds, size_t len, YNode* defL) {
	YOverloadNode* ov;
	NewNode(&ov, YOverloadNode, OverloadN, Overload_free);
	ov->count = len;
	ov->lambdas = lmbds;
	ov->defLambda = defL;
	return (YNode*) ov;
}
YNode* newUsingNode(YNode** scopes, size_t scopec, YNode* body) {
	YUsingNode* us;
	NewNode(&us, YUsingNode, UsingN, Using_free);
	us->scopes = scopes;
	us->count = scopec;
	us->body = body;
	return (YNode*) us;
}
YNode* newWithNode(YNode* scope, YNode* body) {
	YWithNode* wth;
	NewNode(&wth, YWithNode, WithN, With_free);
	wth->scope = scope;
	wth->body = body;
	return (YNode*) wth;
}
YNode* newFunctionNode(wchar_t* id, YLambdaNode* l) {
	YFunctionNode* func;
	NewNode(&func, YFunctionNode, FunctionN, Function_free);
	func->name = id;
	func->lambda = l;
	return (YNode*) func;
}
YNode* newBlockNode(YNode** bl, size_t len, YFunctionBlock* fcs, size_t fcc) {
	YBlockNode* block;
	NewNode(&block, YBlockNode, BlockN, Block_free);
	block->block = bl;
	block->length = len;
	block->funcs = fcs;
	block->funcs_count = fcc;
	return (YNode*) block;
}

/*void pseudocode(YNode* nd, FILE* out) {
 //fprintf(out, "\t\t\tstart: %"PRIu32" : %"PRIu32"\n", nd->line, nd->charPos);
 switch (nd->type) {
 case ConstantN: {
 yconstant_t cnst = ((YConstantNode*) nd)->id;
 switch (cnst.type) {
 case Int64Constant:
 fprintf(out, "push_integer %"PRId64"\n", cnst.value.i64);
 break;
 default:
 break;
 }
 }
 break;
 case IdentifierReferenceN: {
 wchar_t* id = ((YIdentifierReferenceNode*) nd)->id;
 fprintf(out, "push_local_variable %ls\n", id);
 }
 break;
 case UnaryN: {
 YUnaryNode* un = (YUnaryNode*) nd;
 pseudocode(un->argument, out);
 switch (un->operation) {
 case LogicalNot:
 fprintf(out, "logical-not\n");
 break;
 default:
 break;
 }
 }
 break;
 case BinaryN: {
 YBinaryNode* bin = (YBinaryNode*) nd;
 pseudocode(bin->right, out);
 pseudocode(bin->left, out);
 switch (bin->operation) {
 case Multiply:
 fprintf(out, "multiply\n");
 break;
 case Divide:
 fprintf(out, "divide\n");
 break;
 case Modulo:
 fprintf(out, "modulo\n");
 break;
 case Add:
 fprintf(out, "add\n");
 break;
 case Subtract:
 fprintf(out, "subtract\n");
 break;
 case ShiftLeft:
 fprintf(out, "shl\n");
 break;
 case ShiftRight:
 fprintf(out, "shr\n");
 break;
 case And:
 fprintf(out, "and\n");
 break;
 case Or:
 fprintf(out, "or\n");
 break;
 case Xor:
 fprintf(out, "xor\n");
 break;
 case BEquals:
 fprintf(out, "equals\n");
 break;
 case NotEquals:
 fprintf(out, "not-equals\n");
 break;
 case GreaterOrEquals:
 fprintf(out, "greater-or-equals\n");
 break;
 case LesserOrEquals:
 fprintf(out, "lesser-or-equals\n");
 break;
 case Greater:
 fprintf(out, "greater\n");
 break;
 case Lesser:
 fprintf(out, "lesser\n");
 break;
 case LogicalAnd:
 fprintf(out, "logical-and\n");
 break;
 case LogicalOr:
 fprintf(out, "logical-or\n");
 break;
 default:
 break;
 }
 }
 break;
 case FieldReferenceN: {
 YFieldReferenceNode* fn = (YFieldReferenceNode*) nd;
 pseudocode(fn->object, out);
 printf("field %ls\n", fn->field);
 }
 break;
 case ArrayReferenceN: {
 YArrayReferenceNode* in = (YArrayReferenceNode*) nd;
 pseudocode(in->array, out);
 pseudocode(in->index, out);
 printf("index\n");
 }
 break;
 case SubsequenceN: {
 YSubsequenceReferenceNode* sn = (YSubsequenceReferenceNode*) nd;
 pseudocode(sn->array, out);
 pseudocode(sn->from, out);
 pseudocode(sn->to, out);
 printf("subseq\n");
 }
 break;

 case ConditionN: {
 YConditionNode* cn = (YConditionNode*) nd;
 pseudocode(cn->cond, out);
 pseudocode(cn->body, out);
 fprintf(out, "if\n");
 if (cn->elseBody!=NULL) {
 pseudocode(cn->elseBody, out);
 fprintf(out, "else\n");
 }
 }
 break;
 case BlockN: {
 YBlockNode* block = (YBlockNode*) nd;
 fprintf(out, "begin\n");
 for (size_t i=0;i<block->length;i++)
 pseudocode(block->block[i], out);
 fprintf(out, "end\n");
 }
 break;
 default:
 break;
 }
 //fprintf(out, "\t\t\tend: %"PRIu32" : %"PRIu32"\n", nd->line, nd->charPos);
 }*/


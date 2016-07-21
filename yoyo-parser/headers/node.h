#ifndef HEADERS_NODE_H_
#define HEADERS_NODE_H_

#include "base.h"

typedef struct YNode {
	enum {
		ConstantN,
		LambdaN,
		ObjectN,
		InterfaceN,
		IdentifierReferenceN,
		ArrayReferenceN,
		SubsequenceN,
		FieldReferenceN,
		CallN,
		FilledArrayN,
		GeneratedArrayN,
		BinaryN,
		UnaryN,
		AssignmentN,
		DeleteN,
		SetTypeN,
		ConditionN,
		WhileLoopN,
		LoopN,
		ForLoopN,
		ForeachLoopN,
		BreakN,
		ContinueN,
		PassN,
		ReturnN,
		ThrowN,
		TryN,
		SwitchN,
		OverloadN,
		UsingN,
		WithN,
		FunctionN,
		BlockN,
		RootN
	} type;
	void (*free)(struct YNode*);
	int32_t line;
	int32_t charPos;
	wchar_t* fileName;
} YNode;

typedef struct YConstantNode {
	YNode node;
	yconstant_t id;
} YConstantNode;

typedef struct YLambdaNode {
	YNode node;
	int32_t* args;
	YNode** argTypes;
	ssize_t argc;
	bool vararg;
	YNode* retType;
	YNode* body;
} YLambdaNode;

typedef struct YFunctionBlock {
	int32_t id;
	YLambdaNode** lambda;
	size_t count;
} YFunctionBlock;

typedef struct ObjectNodeField {
	int32_t id;
	YNode* value;
	YNode* type;
} ObjectNodeField;
typedef struct YObjectNode {
	YNode node;
	YNode* parent;
	ObjectNodeField* fields;
	size_t fields_length;
	struct YFunctionBlock* methods;
	size_t methods_length;
} YObjectNode;

typedef struct YInterfaceNode {
	YNode node;
	YNode** parents;
	size_t parent_count;
	int32_t* ids;
	YNode** types;
	size_t attr_count;
} YInterfaceNode;

typedef struct YIdentifierReferenceNode {
	YNode node;
	wchar_t* id;
} YIdentifierReferenceNode;

typedef struct YArrayReferenceNode {
	YNode node;
	YNode* array;
	YNode* index;
} YArrayReferenceNode;

typedef struct YSubsequenceReferenceNode {
	YNode node;
	YNode* array;
	YNode* from;
	YNode* to;
} YSubsequenceReferenceNode;

typedef struct YFieldReferenceNode {
	YNode node;
	YNode* object;
	wchar_t* field;
} YFieldReferenceNode;

typedef struct YCallNode {
	YNode node;
	YNode* function;
	YNode** args;
	size_t argc;
} YCallNode;

typedef struct YFilledArrayNode {
	YNode node;
	YNode** array;
	size_t length;
} YFilledArrayNode;

typedef struct YGeneratedArrayNode {
	YNode node;
	YNode* element;
	YNode* count;
} YGeneratedArrayNode;

typedef enum {
	Add,
	Subtract,
	Multiply,
	Divide,
	Modulo,
	Power,
	ShiftLeft,
	ShiftRight,
	And,
	Or,
	Xor,
	LogicalAnd,
	LogicalOr,
	BEquals,
	NotEquals,
	LesserOrEquals,
	GreaterOrEquals,
	Lesser,
	Greater
} YBinaryOperation;

typedef struct YBinaryNode {
	YNode node;
	YBinaryOperation operation;
	YNode* right;
	YNode* left;
} YBinaryNode;

typedef enum {
	Not,
	Negate,
	LogicalNot,
	PreDecrement,
	PreIncrement,
	PostDecrement,
	PostIncrement
} YUnaryOperation;
typedef struct YUnaryNode {
	YNode node;
	YUnaryOperation operation;
	YNode* argument;
} YUnaryNode;

typedef enum {
	AAssign,
	AAddAssign,
	ASubAssign,
	AMulAssign,
	ADivAssign,
	AModAssign,
	APowerAssign,
	AShiftLeftAssign,
	AShiftRightAssign,
	AAndAssign,
	AOrAssign,
	AXorAssign,
	ALogicalAndAssign,
	ALogicalOrAssign
} YAssignmentOperation;
typedef struct YAssignmentNode {
	YNode node;
	YAssignmentOperation operation;
	YNode** dest;
	size_t dest_len;
	YNode** src;
	size_t src_len;
	bool newVar;
	YNode* type;
} YAssignmentNode;

typedef struct YDeleteNode {
	YNode node;
	YNode** list;
	size_t length;
} YDeleteNode;

typedef struct YConditionNode {
	YNode node;
	YNode* cond;
	YNode* body;
	YNode* elseBody;
} YConditionNode;

typedef struct YWhileLoopNode {
	YNode node;
	bool evalOnStart;
	YNode* cond;
	YNode* body;
	int32_t id;
} YWhileLoopNode;

typedef struct YForeachLoopNode {
	YNode node;
	int32_t id;
	YNode* refnode;
	YNode* col;
	YNode* body;
} YForeachLoopNode;

typedef struct YLoopNode {
	YNode node;
	YNode* body;
	int32_t id;
} YLoopNode;

typedef struct YForLoopNode {
	YNode node;
	YNode* init;
	YNode* cond;
	YNode* loop;
	YNode* body;
	int32_t id;
} YForLoopNode;

typedef struct YPassNode {
	YNode node;
} YPassNode;

typedef struct YLoopControlNode {
	YNode node;
	int32_t label;
} YLoopControlNode;

typedef struct YValueReturnNode {
	YNode node;
	YNode* value;
} YValueReturnNode;

typedef struct YTryNode {
	YNode node;
	YNode* tryBody;
	YNode* catchRef;
	YNode* catchBody;
	YNode* elseBody;
	YNode* finBody;
} YTryNode;

typedef struct YCaseNode {
	YNode* value;
	YNode* stmt;
} YCaseNode;
typedef struct YSwitchNode {
	YNode node;
	YNode* value;
	size_t length;
	YCaseNode* cases;
	YNode* defCase;
} YSwitchNode;

typedef struct YOverloadNode {
	YNode node;
	size_t count;
	YNode** lambdas;
	YNode* defLambda;
} YOverloadNode;

typedef struct YUsingNode {
	YNode node;
	YNode** scopes;
	size_t count;
	YNode* body;
} YUsingNode;

typedef struct YWithNode {
	YNode node;
	YNode* scope;
	YNode* body;
} YWithNode;

typedef struct YFunctionNode {
	YNode node;
	int32_t name;
	YLambdaNode* lambda;
} YFunctionNode;

typedef struct YBlockNode {
	YNode node;
	YNode** block;
	size_t length;
	YFunctionBlock* funcs;
	size_t funcs_count;
} YBlockNode;

YNode* newConstantNode(yconstant_t);
YNode* newIdentifierReferenceNode(wchar_t*);
YNode* newIndexReferenceNode(YNode*, YNode*);
YNode* newSubseqReferenceNode(YNode*, YNode*, YNode*);
YNode* newLambdaNode(int32_t*, YNode**, ssize_t, bool, YNode*, YNode*);
YNode* newFieldReferenceNode(YNode*, wchar_t*);
YNode* newCallNode(YNode*, YNode**, size_t);
YNode* newFilledArray(YNode**, size_t);
YNode* newGeneratedArray(YNode*, YNode*);
YNode* newObjectNode(YNode*, ObjectNodeField*, size_t, YFunctionBlock*, size_t);
YNode* newInterfaceNode(YNode**, size_t, int32_t*, YNode**, size_t);
YNode* newBinaryNode(YBinaryOperation, YNode*, YNode*);
YNode* newUnaryNode(YUnaryOperation, YNode*);
YNode* newAssignmentNode(YAssignmentOperation, bool, YNode*, YNode**, size_t,
		YNode**, size_t);
YNode* newDeleteNode(YNode**, size_t);
YNode* newConditionNode(YNode*, YNode*, YNode*);
YNode* newLoopNode(int32_t, YNode*);
YNode* newWhileLoopNode(int32_t, bool, YNode*, YNode*);
YNode* newForLoopNode(int32_t, YNode*, YNode*, YNode*, YNode*);
YNode* newForeachLoopNode(int32_t, YNode*, YNode*, YNode*);
YNode* newPassNode();
YNode* newBreakNode(int32_t);
YNode* newContinueNode(int32_t);
YNode* newThrowNode(YNode*);
YNode* newReturnNode(YNode*);
YNode* newTryNode(YNode*, YNode*, YNode*, YNode*, YNode*);
YNode* newSwitchNode(YNode*, YCaseNode*, size_t, YNode*);
YNode* newOverloadNode(YNode**, size_t, YNode*);
YNode* newUsingNode(YNode**, size_t, YNode*);
YNode* newWithNode(YNode*, YNode*);
YNode* newFunctionNode(int32_t, YLambdaNode*);
YNode* newBlockNode(YNode**, size_t, YFunctionBlock*, size_t);

#define NewNode(ptr, nodeStruct, nodeType, freePtr) *(ptr) = malloc(sizeof(nodeStruct));\
                                                    (*(ptr))->node.type = nodeType;\
													(*(ptr))->node.free = (void (*)(YNode*)) freePtr;\
													(*(ptr))->node.line = -1;\
													(*(ptr))->node.charPos = -1;\
													(*(ptr))->node.fileName = NULL;

void pseudocode(YNode*, FILE*);

#endif

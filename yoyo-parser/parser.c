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

void addConstant(ParseHandle* handle, yconstant_t c) {
	size_t i = handle->constants_size++;
	handle->constants = realloc(handle->constants, sizeof(yconstant_t) * handle->constants_size);
	handle->constants[i] = c;
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

void initGrammar(Grammar* g) {
	NewRule(g, constant, Constant_validate, Constant_reduce);
	NewRule(g, identifier, Identifier_validate, Identifier_reduce);
}

YNode* parse(ParseHandle* handle) {
	initGrammar(&handle->grammar);
	while (handle->tokens[0].type!=TokenEOF) {
		ytoken tok = handle->tokens[0];
		shift(handle);
		printf("%x %"PRIu32":%"PRIu32"\n", tok.type, tok.line, tok.charPos);
	}
	return NULL;
}

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

	parse(&handle);

	fclose(fd);
	return 0;
}

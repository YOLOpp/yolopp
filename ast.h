#pragma once
#include <vector>
#include <string>

using namespace std;

enum ast_type_t{
	AT_ASSIGNMENT,
	AT_FUNCTIONCALL,
	AT_CONDITIONAL,
	AT_LOOP,
	AT_EXPR,
	AT_ARRAY,
	//AT_SET,
	//AT_LIST,
	AT_SYNCBLOCK,
	AT_ASYNCBLOCK,
	AT_FUNCTIONDEF
};

class AST{
	ast_type_t type;
	vector<AST*> children;
	string val;
	AST(const Tokens&);
	string translate(void);
};

class Tokens: public vector<string>{
	Tokens(const string&);
};

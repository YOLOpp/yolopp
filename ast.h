#pragma once
#include <vector>
#include <string>

using namespace std;

enum ast_type_t{
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

class Tokens : public vector<string> {
public:
	Tokens(const string&);
	Tokens()=default;
};

class AST{
	ast_type_t type;
	vector<AST*> children;
	string val;
public:
	AST(const Tokens&);
	~AST(void);
	string translate(void);
};

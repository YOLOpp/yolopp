#pragma once
#include <vector>
#include <string>

using namespace std;

typedef int ast_type_t;

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

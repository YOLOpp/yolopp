#pragma once
#include <vector>
#include <string>

using namespace std;

typedef int ast_type_t;

class AST {
	ast_type_t type;
	vector<AST*> children;
	string ext;
	AST( const string& s );
	void create( const string& s, int a, int b );
};

class Tokens: public vector<string> {
	Tokens( const string& );
};

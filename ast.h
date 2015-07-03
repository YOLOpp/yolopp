#pragma once
#include <vector>
#include <string>

using namespace std;

enum ast_type_t{
	AT_FUNCTIONCALL, //val is function name, children are arguments
	AT_CONDITIONAL, //if statement; first child is condition (AT_EXPR), second child is block (AT_(A)SYNCBLOCK)
	AT_LOOP, //loop var, from, to, block
	AT_EXPR, //val gives the node string; it's an operator
	AT_WORD, //variable
	AT_NUMBER, //number
	AT_STRING, //string
	AT_ARRAY, //children are members
	//AT_SET,
	//AT_LIST,
	AT_SYNCBLOCK, //children are the lines/statements of the block
	AT_ASYNCBLOCK, //children are the lines/statements of the block
	AT_FUNCTIONDEF, //function name, return value, AT_ARRAY of arguments, body block (AT_(A)SYNCBLOCK)
	AT_VARIABLEDEF, //variable name, type
};

enum token_type {
	LEFT_BRACKET,
	RIGHT_BRACKET,
	INTEGER,
	FLOAT,
	STRING,
	OPERATOR,
	KEYWORD,
	TYPENAME,
	FUNCTION,
	VARIABLE,
	COMMA
};

enum associativity {
	LEFT=false,
	RIGHT=true
};

struct Token : public string {
	token_type type;
	string& str() { return *this; }
	const string& str() const { return *this; }
};

class Tokens : public vector<Token> {
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

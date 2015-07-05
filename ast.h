#pragma once
#include <vector>
#include <string>

using namespace std;

enum ast_type_t{
	AT_FUNCTIONCALL, //val is function name, children are arguments
	AT_CONDITIONAL, //if statement; first child is condition (AT_EXPR), second child is block (AT_(A)SYNCBLOCK)
	AT_LOOP, //loop var, from, to, block
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
	AT_DATATYPE,
	AT_FLOW // return, break, continue
};



enum token_type {
	LEFT_BRACKET,
	RIGHT_BRACKET,
	INTEGER,			// will be preserved by shunthing yard
	FLOAT,				//
	STRING,				//
	OPERATOR,			//
	KEYWORD,			//
	TYPENAME,
	FUNCTION,			//
	VARIABLE,			//
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

struct AST{
	ast_type_t type;
	string val;
	vector<AST*> children;
	AST() = default;
	AST( ast_type_t t );
	AST( ast_type_t t, string v, vector<AST*> c );
	AST(const Tokens&);
	~AST(void);
	string translate(void);
};

int bracketIterator( const Tokens& tokens, int i, int n );
int resolveTypename( const Tokens& tokens, AST*& typename_result, int i, int n );
AST* loopYard( const Tokens& postfix, int& n );
AST* junkYard( const Tokens& tokens, int i, int n );
vector<AST*> graveYard( const Tokens& tokens, int i, int n );
vector<AST*> scotlandYard( const Tokens& tokens, int i, int n );
Tokens shuntingYard( const Tokens& tokens, int i, int n );
ostream& operator<<( ostream& os, const AST& ast );
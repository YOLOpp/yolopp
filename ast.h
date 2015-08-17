#pragma once
#include <vector>
#include <string>
#include <exception>
#include <stack>
#include <sstream>

using namespace std;

enum ast_type_t{
	AT_FUNCTIONCALL, // val is function name; children are arguments
	AT_CONDITIONAL, //if statement; first child is condition, second child is block (AT_(A)SYNCBLOCK)
	AT_LOOP, // val is (while|for); while: condition, block
	AT_WORD, //variable
	AT_NUMBER, //number
	AT_STRING, //string
	AT_ARRAY, //children are members
	//AT_SET,
	//AT_LIST,
	AT_SYNCBLOCK, //children are the lines/statements of the block
	AT_ASYNCBLOCK, //children are the lines/statements of the block
	AT_FUNCTIONDEF, // val is function name; children are return value, AT_ARRAY of arguments, body block (AT_(A)SYNCBLOCK)
	AT_VARIABLEDEF, // val is variable name; child is type
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

struct Token;
class Tokens;
class AST;

typedef stack<AST*> TranslatePath;


ostream& operator<<( ostream& os, const AST& ast );

struct Token : public string {
	token_type type;
	string& str() { return *this; }
	const string& str() const { return *this; }
};

class Tokens : public vector<Token> {
public:
	Tokens(const string&);
	Tokens()=default;
private:
	int bracketIterator( int i, int n ) const;
	int resolveTypename( AST*& typename_result, int i, int n ) const;
	int commaIterator( int i , int n ) const;
	AST* loopYard( int& n ) const;
	AST* junkYard( int i, int n ) const;
	vector<AST*> graveYard( int i, int n ) const;
	vector<AST*> scotlandYard( int i, int n, int& block_id ) const;
	Tokens shuntingYard( int i, int n ) const;
	friend class AST;
};

class AST {
private:
	ast_type_t type;
	string val;
	vector<AST*> children;
public:
	AST() = default;
	AST( ast_type_t t );
	AST( ast_type_t t, string v, vector<AST*> c );
	AST( const Tokens& );
	~AST();
	string translate(void);
	void translateBlock( stringstream& ss, TranslatePath&, int indent );
	void translateItem( stringstream& ss, TranslatePath&, int indent, bool allow_block = true );
	void printFunctionHeader( stringstream& ss, string x );
	void pullFunctions( stringstream& ss, TranslatePath& translatePath );
	string decodeTypename();
	static string findFunctionName( string name, TranslatePath translatePath );
	friend class Tokens;
	friend ostream& operator<<( ostream& os, const AST& ast );
};

class compile_exception : public exception {
	string err_str;
	int token_id;
	virtual const char* what() const noexcept;
public:
	compile_exception( string err, int i );
};

class translate_exception : public exception {
	string err_str;
	virtual const char* what() const noexcept;
public:
	translate_exception( string err );
};

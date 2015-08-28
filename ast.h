#pragma once
#include <vector>
#include <string>
#include <exception>
#include <stack>
#include <sstream>
#include <map>
#include <set>
#include <iostream>

using namespace std;

#define ASYNC_BRACKET	"{"
#define SYNC_BRACKET	"["

enum ast_type_t{
	AT_FUNCTIONCALL, // val is function name; children are arguments
	AT_CONDITIONAL, //if statement; first child is condition, second child is if block (AT_(A)SYNCBLOCK), third is else block
	AT_LOOP, // val is (while|for); while: condition, block
	AT_WORD, //variable
	AT_NUMBER, //number
	AT_STRING, //string
	AT_ARRAY, // (tuple) children are members
	AT_INLINE_SET, // {set}
	AT_INLINE_LIST, // [list]
	AT_SYNCBLOCK, //children are the lines/statements of the block
	AT_ASYNCBLOCK, //children are the lines/statements of the block
	AT_FUNCTIONDEF, // val is function name; children are return value, AT_ARRAY of arguments, body block (AT_(A)SYNCBLOCK)
	AT_VARIABLEDEF, // val is variable name; child is type
	AT_SPACEDEF, // val is space name; child is AT_ARRAY of member variables
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
	TYPENAME,			// *
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

typedef stack<const AST*> TranslatePath;

extern vector<AST*> blockRegister;
extern const map< string, tuple< int, associativity, bool > > operator_precedence;
extern const set<string> keywords;
extern const set<string> nonFinalTypenames;
extern const set<string> finalTypenames;

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
	bool isTypename( int i, const set<string>& typenames, bool tuples = true ) const;
	bool isBlock( int i ) const;
	bool isKeyword( int i ) const;
	bool isVariable( int i, const set<string>& typenames ) const;
	int bracketIterator( int i, int n ) const;
	int commaIterator( int i, int n ) const;
	int resolveTypename( AST*& typename_result, int i, int n, const set<string>& typenames ) const;
	AST* getNamedTuple( int& i, int k, const set<string>& typenames ) const;
	AST* statementTranslate( int i, int n ) const;
	void spaceTranslateFirst( set<string>& spaceNames, int i, int n ) const;
	void spaceTranslateSecond( vector<AST*>& block, int i, int n, const set<string>& typenames ) const;
	void functionTranslate( vector<AST*>& block, int i, int n, const set<string>& typenames ) const;
	void variableTranslate( vector<AST*>& block, int i, int n, const set<string>& typenames ) const;
	AST* pseudoBlock( int& k, int i, int n, const set<string>& typenames, bool forceBlock = false ) const;
	void keywordTranslate( vector<AST*>& block, int i, int n, const set<string>& typenames ) const;
	void segmentTranslate( vector<AST*>& block, int i, int n, const set<string>& typenames ) const;
	vector<AST*> blockTranslate( int i, int n, const set<string>& old_typenames ) const;
	Tokens shuntingYard( int i, int n ) const;
	AST* postfixTranslate( int& n ) const;
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
	AST( const AST& ) = default;
	~AST(); // non-recursive delete
	string translate(void) const;
	void translateBlock( stringstream& ss, TranslatePath&, int indent ) const;
	void translateItem( stringstream& ss, TranslatePath&, int indent, bool allow_block = true ) const;
	void printFunctionHeader( stringstream& ss, string x, const TranslatePath& translatePath ) const;
	void pullFunctions( stringstream& ss, TranslatePath& translatePath ) const;
	void pullSpaces( stringstream& ss, TranslatePath& translatePath ) const;
	string decodeTypename( const TranslatePath& ) const;
	bool compare( const AST* other ) const;
	AST* getType( const TranslatePath& ) const;
	AST* cascade(); // recursive delete
	static string findFunctionName( string name, TranslatePath translatePath );
	static string findSpaceName( string name, TranslatePath translatePath );
	static AST* findFunctionType( string name, TranslatePath translatePath );
	static AST* findVariableType( string name, TranslatePath translatePath );
	friend class Tokens;
	friend ostream& operator<<( ostream& os, const AST& ast );
};

class compile_exception : public exception {
public:
	string err_str;
	int token_id;
	virtual const char* what() const noexcept;
	compile_exception( string err, int i );
};

class translate_exception : public exception {
public:
	string err_str;
	virtual const char* what() const noexcept;
	translate_exception( string err );
};

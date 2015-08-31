#include <iostream>
#include <stack>
#include <string>
#include <vector>
#include <map>
#include <tuple>
#include <climits>
#include <sstream>
#include <exception>
#include <set>

#include "ast.h"

const set<string> finalTypenames {
	"int",
	"string", 
	"rat",
	"real",
	"bool"
};

const set<string> nonFinalTypenames { "set", "list" };

const map< string, tuple< int, associativity, bool > > operator_precedence {
	{"->",make_tuple(-1,LEFT,false)},	// member variable by name
	{".",make_tuple(-1,LEFT,false)},	// VSO to SVO
	{"!",make_tuple(0,RIGHT,true)},		// sort list
	{"?",make_tuple(0,RIGHT,true)},		// shuffle list
	{"#",make_tuple(0,RIGHT,true)},		// count elements list/set
	{"@",make_tuple(1,LEFT,false)},		// list indexing
	{"\u220A",make_tuple(1,LEFT,false)},	// set contains
	{"<->",make_tuple(2,LEFT,false)},	// swap elements
	{"<~>",make_tuple(2,RIGHT,false)},	//
	{"^",make_tuple(3,RIGHT,false)},	// exponentiation
	{"-u",make_tuple(4,RIGHT,true)},	// unary minus
	{"*",make_tuple(5,LEFT,false)},		// multiplication
	{"/",make_tuple(5,LEFT,false)},		// division
	{"\u00AC",make_tuple(6,RIGHT,true)},	// not
	{"+",make_tuple(6,LEFT,false)},		// addition
	{"-",make_tuple(6,LEFT,false)},		// subtraction
	{":",make_tuple(7,LEFT,false)},		// range construction
	{"~",make_tuple(12,LEFT,false)},	// cast (temp)
	{"<",make_tuple(13,LEFT,false)},	// less-than
	{">",make_tuple(13,LEFT,false)},	// more-than
	{"<=",make_tuple(13,LEFT,false)},	// at most
	{"≤",make_tuple(13,LEFT,false)},	// at most
	{">=",make_tuple(13,LEFT,false)},	// at least
	{"≥",make_tuple(13,LEFT,false)},	// at least
	{"==",make_tuple(14,LEFT,false)},	// equal to
	{"!=",make_tuple(14,LEFT,false)},	// less-than or more-than ( not equal to )
	{"\u2260",make_tuple(14,LEFT,false)},	// less-than or more-than ( not equal to )
	{"\u2227",make_tuple(15,LEFT,false)},	// and
	{"\u22BB",make_tuple(16,LEFT,false)},	// xor
	{"\u2228",make_tuple(17,LEFT,false)},	// or
	{"=",make_tuple(18,RIGHT,false)}	// asignment
};

const set<string> keywords {
	"import",
	"space",
	"if",
	"else",
	"while",
	"for",
	"in",
	"break",
	"continue",
	"function",
	"return"
};

vector<AST*> blockRegister;

bool AST::compare( const AST* other ) const {
	if( type != other->type || val != other->val || children.size() != other->children.size() )
		return false;
	for( size_t i = 0; i < children.size(); i++ )
		if( !children.at(i)->compare( other->children.at(i) ) )
			return false;
	return true;
}

AST::AST( ast_type_t t ) {
	type = t;
}

AST::AST( ast_type_t t, string v, vector<AST*> c ) {
	type = t;
	val = move( v );
	children = move( c );
}

AST::AST( const Tokens& tokens ) {
	type = AT_SYNCBLOCK;
	val = "0";
	try {
		children = tokens.blockTranslate( 0, tokens.size(), finalTypenames );
	} catch( compile_exception& e ) {
		if( e.token_id >= 0 )
			cerr << "(" << tokens.at(e.token_id).str() << ")";
		cerr << e.what() << endl;
		throw e;
	}
}

AST::~AST() { // non-recursive delete

}

AST* AST::cascade() {
	for( auto child: children )
		delete child->cascade();
	return this;
}

const char* compile_exception::what() const noexcept {
	string* s = new string( "(" + to_string( token_id ) + ") Error: " + err_str + "!" );
	return s->c_str();
}

compile_exception::compile_exception( string err, int i ) : err_str(err), token_id(i) {
}

ostream& operator<<( ostream& os, const AST& ast ) {
	os << "(" << ast.type << "," << ast.val << ",";
	for( const AST* c: ast.children ) {
		if( c == nullptr )
			os << "INTERNAL ERROR";
		else
			os << *c;
	}
	return os << ")";
}
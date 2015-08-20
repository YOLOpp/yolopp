#include <iostream>
#include <stack>
#include <string>
#include <vector>
#include <map>
#include <tuple>
#include <climits>
#include <sstream>
#include <exception>

#include "ast.h"

const map< string, tuple< int, associativity, bool > > operator_precedence {
	{"!",make_tuple(0,RIGHT,true)},		// sort list
	{"?",make_tuple(0,RIGHT,true)},		// shuffle list
	{"<->",make_tuple(1,LEFT,false)},	// swap elements
	{"<~>",make_tuple(1,RIGHT,false)},	//
	{"@",make_tuple(2,LEFT,false)},		// list indexing
	{"^",make_tuple(3,RIGHT,false)},	// exponentiation
	{"-u",make_tuple(4,RIGHT,true)},	// unary minus
	{"*",make_tuple(5,LEFT,false)},		// multiplication
	{"/",make_tuple(5,LEFT,false)},		// division
	{"+",make_tuple(6,LEFT,false)},		// addition
	{"-",make_tuple(6,LEFT,false)},		// subtraction
	{"<",make_tuple(8,LEFT,false)},		// less-than
	{">",make_tuple(8,LEFT,false)},		// more-than
	{"<=",make_tuple(8,LEFT,false)},	// at most
	{">=",make_tuple(8,LEFT,false)},	// at least
	{"==",make_tuple(9,LEFT,false)},	// equal to
	{"<>",make_tuple(9,LEFT,false)},	// less-than or more-than ( not equal to )
	{"&&",make_tuple(12,LEFT,false)},	// and
	{"^^",make_tuple(13,LEFT,false)},	// xor
	{"||",make_tuple(14,LEFT,false)},	// or
	{"=",make_tuple(15,RIGHT,false)}	// asignment
};

// name, number of parameters
// n>=0 : n parameters, n<0  : -n optional parameters
const map<string, int> keywords {
	{ "import", INT_MIN },
	{ "if", 1 },
	{ "else", 0 },
	{ "while", 1 },
	{ "forever", 0 },
	{ "break", -1 },
	{ "continue", 0 },
	{ "function", 3 },
	{ "return", 1 },
	{ "array-init", INT_MIN }
};

// name, parameters, is prefix
const map<string, pair<int, bool> > datatypes {
	{ "null", make_pair(0,false) }, // for functions returning nothing
	{ "int", make_pair(-1, false) },
	{ "string", make_pair(0, false) },
	{ "rat", make_pair(-1, false) },
	{ "float", make_pair(-1, false) },
	{ "tuple", make_pair(INT_MIN, true) },
	{ "complex", make_pair(-1, true) },
	{ "list", make_pair(0, true ) },
	{ "set", make_pair(0, true ) }
};

const string ASYNC_BRACKET = "{";
const string SYNC_BRACKET = "[";

Tokens::Tokens( const string& s ) {
	int n = s.size();
	enum { MODE_NONE, MODE_INT, MODE_FLOAT, MODE_STRING, MODE_NAME } mode = MODE_NONE;
	bool terminate = false;
	bool escape = false;
	Token t, u;

	for( int i = 0; i < n; i++ ) {
		if( mode == MODE_NONE ) {
			if( isalpha( s[i] ) )
				mode = MODE_NAME;
			else if( isdigit( s[i] ) )
				mode = MODE_INT;
			else if( s[i] == '.' )
				mode = MODE_FLOAT;
			else if( s[i] == '"' )
				mode = MODE_STRING;
		}
		if( mode == MODE_STRING ) { // inside string
			if( s[i] == '"' && !t.empty() && !escape ) {
				t.type = STRING;
				t.push_back( s[i] );
				push_back(t);
				mode = MODE_NONE;
				t.clear();
			} else if( s[i] == '\\' && !escape ) {
				escape = true;
			} else {
				if( escape ) {
					escape = false;
					t.push_back( '\\' );
				}
				t.push_back( s[i] );
			}
		} else {
			if( !isspace( s[i] ) ) {
				terminate = true;
				if( s[i] == '-' ) { // minus
					u.push_back( '-' ); 
					if( !( back().type == RIGHT_BRACKET || back().type == VARIABLE || back().type == FLOAT || back().type == INTEGER || back().type == STRING ) ) // unary minus
						u.push_back( 'u' ); 
					u.type = OPERATOR;
				} else if( s[i] == '<' ) {
					if( i + 2 < n && s[i+2] == '>' && ( s[i+1] == '-' || s[i+1] == '~' ) ) { // swap operator
						u.push_back( s[i] );
						u.push_back( s[i+1] );
						u.push_back( s[i+2] );
						u.type = OPERATOR;
						i += 2;
					} else { // less-than
						u.push_back( '<' );
						if( i + 1 < n && s[i+1] == '=' ) { // at most
							u.push_back( '=' );
							i++;
						} else if( i + 1 < n && s[i+1] == '>' ) { // not equal to
							u.push_back( '>' );
							i++;
						}
						u.type = OPERATOR;
					}
				} else if( s[i] == '|' || s[i] == '&' || s[i] == '^' || s[i] == '=' ) { // logical operators
					u.push_back( s[i] );
					u.type = OPERATOR;
					if( i + 1 < n && s[i] == s[i+1] ) {
						u.push_back( s[i] );
						i++;
					} else if( s[i] != '=' )
						throw compile_exception( "Invalid operator '" + string( 1, s[i] ) + "'", i );
				} else if( s[i] == '>' ) {
					u.push_back( s[i] );
					u.type = OPERATOR;
					if( i + 1 < n && s[i] == '=' ) {
						u.push_back( '=' );
						i++;
					}
				} else if( s[i] == '{' || s[i] == '[' || s[i] == '(' ) { // opening brackets
					u.push_back( s[i] );
					u.type = LEFT_BRACKET;
				} else if( s[i] == '}' || s[i] == ']' || s[i] == ')' /*|| s[i] == '>'*/ ) { // closing brackets
					if( s[i] == '}' )
						u.push_back( '{' );
					else if( s[i] == ']' )
						u.push_back( '[' );
					else if( s[i] == ')')
						u.push_back( '(' );
					/*else
						u.push_back( '<' );*/
					u.type = RIGHT_BRACKET;
				} else if( s[i] == ',' ) { // argument seperator
					u.push_back( ',' );
					u.type = COMMA;
				} else if( s[i] == '.' ) { // float
					if( mode == MODE_INT )
						mode = MODE_FLOAT;
					else
						throw compile_exception( "Unexpected '.'", i );
					t.push_back( s[i] );
					terminate = false;
				} else if( operator_precedence.find( string( 1, s[i] ) ) != operator_precedence.end() ) { // single-character operator
					u.push_back( s[i] );
					u.type = OPERATOR;
				} else if( !isdigit( s[i] ) && ( mode == MODE_INT || mode == MODE_FLOAT ) ) // filter letters from numbers
					throw compile_exception( "Unexpected '" + string( 1, s[i] ) + "' in number", i );
				else {
					t.push_back( s[i] );
					terminate = false;
				}
			}
			if( isspace( s[i] ) || terminate ) { // termination
				if( mode == MODE_NAME ) {
					if( keywords.find( string(t) ) != keywords.end() )
						t.type = KEYWORD;
					else if( datatypes.find( string(t) ) != datatypes.end() )
						t.type = TYPENAME;
					else
						t.type = VARIABLE;
				}
				else if( mode == MODE_FLOAT )
					t.type = FLOAT;
				else if( mode == MODE_INT )
					t.type = INTEGER;
				if( mode != MODE_NONE ) {
					push_back( t );
					mode = MODE_NONE;
					t.clear();
				}
			}
			if( terminate ) {
				if( back().type == VARIABLE && u.type == LEFT_BRACKET )
					back().type = FUNCTION;
				push_back( u );
				u.clear();
				terminate = false;
			}
		}
	}
	if( !t.empty() ) {
		if( mode == MODE_INT )
			t.type = INTEGER;
		else if( mode == MODE_FLOAT )
			t.type = FLOAT;
		else if( mode == MODE_NAME )
			t.type = VARIABLE;
		else if( mode == MODE_STRING )
			throw compile_exception( "Missing closing-'\"'", n );
		else
			throw compile_exception( "Unexpected tokenize mode", n );
		push_back( t );
	}
}

int Tokens::bracketIterator( int i, int n ) const {
	if( at(i).type == LEFT_BRACKET) {
		stack<char> bracket_stack;
		bracket_stack.push( at(i).str().front() );
		while( ++i < n && !bracket_stack.empty() ) {
			if( at(i).type == LEFT_BRACKET )
				bracket_stack.push( at(i).str().front() );
			else if( at(i).type == RIGHT_BRACKET ) {
				if( bracket_stack.top() != at(i).str().front() )
					throw compile_exception( "Mismatched parenthesis", i );
				bracket_stack.pop();
			}
		}
		if( !bracket_stack.empty() )
			throw compile_exception( "Missing parenthesis", i );
	} else
		throw compile_exception( "'" + at(i).str() + "' is not a left-parenthesis", i ); 
	return i; // return index past closing bracket
}

int Tokens::commaIterator( int i , int n ) const {
	int brackets = 0;
	while( i < n ) {
		if( at(i).type == LEFT_BRACKET )
			brackets++;
		else if( at(i).type == RIGHT_BRACKET ) {
			brackets--;
			if( brackets < 0 )
				return -1;
		} else if( at(i).type == COMMA && brackets == 0 )
			return i;
		i++;
	}
	return n;
}

int Tokens::resolveTypename( AST*& typename_result, int i, int n ) const { // does not support datatype parameters yet
	bool isFinal = false;
	typename_result = new AST( AT_DATATYPE );
	AST* head = typename_result;
	while( i < n && at(i).type == TYPENAME ) {
		if( isFinal ) 
			throw compile_exception( "Not expecting '" + at(i).str() + "'", i );
		if( datatypes.find( at(i).str() ) != datatypes.end() ) {
			head->val = at(i);
			if( datatypes.at(at(i).str()).second == false ) {
				isFinal = true;
			} else {
				head->children.push_back( new AST( AT_DATATYPE ) );
				head = head->children.back();
			}
		} else 
			throw compile_exception( "Unknown datatype '" + at(i).str() + "'", i );
		i++;
	}
	if( !isFinal ) 
		throw compile_exception( "Expected datatype", i );
	return i; // return index past last datatype specifier
}

AST* Tokens::loopYard( int& n ) const {
	AST* h = new AST;
	int parameters;
	stack<AST*> reverse;
	n--;
	if( n < 0 ) 
		throw compile_exception( "Expecting more tokens ( Hint: += / -= / ... are no legal operators )", -1 );

	switch( at(n).type ) { /*KEYWORDS NOT YET SUPPORTED*/
		case OPERATOR:
			if( get<2>( operator_precedence.at(at(n).str()) ) == true ) // is unary
				parameters = 1;
			else
				parameters = 2;
			h->val = "@operator" + at(n); // temporary
			h->type = AT_FUNCTIONCALL;
			break;
		case FUNCTION: {
			stringstream ss( at(n).str(), ios_base::in );
			ss >> (h->val) >> parameters;
			h->type = AT_FUNCTIONCALL;
			break;
		}
		case INTEGER:
		case FLOAT:
			h->type = AT_NUMBER;
			h->val = at(n);
			parameters = 0;
			break;
		case STRING:
			h->type = AT_STRING;
			h->val = at(n);
			parameters = 0;
			break;
		case VARIABLE:
			h->type = AT_WORD;
			h->val = at(n);
			parameters = 0;
			break;
		case KEYWORD: {
			stringstream ss( at(n) );
			string a;
			ss >> a;
			if( a.compare( 0, 4, "init" ) == 0 ) {
				if( a == "init-list" )
					h->type = AT_INLINE_LIST;
				else if( a == "init-set" )
					h->type = AT_INLINE_SET;
				else
					throw compile_exception( "Unknown initaliser list", -1 );
				h->val = at(n);
				ss >> parameters;
				break;
			}
		}
		default:
			throw compile_exception( "Unknown/Unsupported token type", -1 );
			h = nullptr;
	}
	for( int i = 0; i < parameters; i++ )
		reverse.push( loopYard( n ) );
	while( !reverse.empty() ) {
		h->children.push_back( reverse.top() );
		reverse.pop();
	}
	return h;
}

AST* Tokens::junkYard( int i, int n ) const { // converts statement tokens to AST using shuntingYard
	Tokens postfix = shuntingYard( i, n );
	int k = postfix.size();
	return postfix.loopYard( k );
}

/*vector<AST*> graveYard( const Tokens& tokens, int i, int n ) { // applies junkYard to multiple statements seperated by commas
	vector<AST*> r;
	int j = i;
	while( j < n ) {
		while( tokens[j].type != COMMA && j < n )
			j++;
		r.push_back( junkYard( tokens, i, j ) );
		i = j + 1;
	}
	return r;
}*/

vector<AST*> Tokens::scotlandYard( int i, int n, int& block_id ) const {
	vector<AST*> r;
	AST *a = nullptr, *b = nullptr, *c = nullptr, *d = nullptr;
	int j, k, l, m;
	while( i < n ) {
		switch( at(i).type ) {
			case KEYWORD:
				if( at(i).str() == "function" ) {
					j = resolveTypename( d, i + 1, n );
					if( j < n && at(j).type == FUNCTION ) {
						k = bracketIterator( j+1, n );
						m = j + 2;
						b = new AST( AT_ARRAY );
						while( m < k ) {
							if( at(m).type == TYPENAME ) {
								l = resolveTypename( c, m, k - 1 );
								if( l + 1 < n && at(l).type == VARIABLE && ( at(l+1).type == COMMA || at(l+1).type == RIGHT_BRACKET ) ) {
									b->children.push_back( new AST( AT_VARIABLEDEF, at(l).str() /*variable-name*/, { c /*data-type*/} ) );
									m = l+2;
								} else 
									throw compile_exception( "Variable name required in function definition", i );
							} else 
								throw compile_exception( "Expected typename in function definition", i );
						}
						l = bracketIterator( k, n );
						if( at(j+1).str() == "(" && ( at(k).str() == SYNC_BRACKET || at(k).str() == ASYNC_BRACKET ) ) {
					 		a = new AST( AT_FUNCTIONDEF, at(j).str(), { d /*return-type*/, b /*parameters*/, 
					 				new AST( at(i).str() == SYNC_BRACKET ? AT_SYNCBLOCK : AT_ASYNCBLOCK, to_string(block_id++), scotlandYard( k+1, l - 1, block_id ) ) } );
					 		i = l;
						} else
							throw compile_exception( "Unexpected parenthesis-type near function definition", i );
					} else 
						throw compile_exception( "Expected function name", i );
				} else if( at(i).str() == "if" ) {
					if( i+1 < n && at(i+1).type == LEFT_BRACKET && at(i+1).str() == "(" ) {
						j = bracketIterator( i+1, n );
						b = junkYard( i+2, j-1 );
						if( j < n ) {
							if( at(j).type == LEFT_BRACKET && ( at(j).str() == ASYNC_BRACKET || at(j).str() == SYNC_BRACKET ) ) {
								k = bracketIterator( j, n );
								c = new AST( at(j).str() == SYNC_BRACKET ? AT_SYNCBLOCK : AT_ASYNCBLOCK, to_string(block_id++), { scotlandYard( j+1, k-1, block_id ) } );
								a = new AST( AT_CONDITIONAL, "", { b, c } );
								if( k < n && at(k).type == KEYWORD && at(k).str() == "else" ) {
									if( k + 1 < n && at(k+1).type == LEFT_BRACKET && ( at(k+1).str() == ASYNC_BRACKET || at(k+1).str() == SYNC_BRACKET ) ) { // TODO: allow elseif without brackets
										l = bracketIterator( k+1, n );
										d = new AST( at(j+1).str() == SYNC_BRACKET ? AT_SYNCBLOCK : AT_ASYNCBLOCK, to_string(block_id++), { scotlandYard( k+2, l-1, block_id ) } );
										a->children.push_back( d );
										i = l;
									} else
										throw compile_exception( "Brackets after else-statement are required", i );
								} else
									i = k;
							} else 
								throw compile_exception( "Empty if-statement", i );
						} else 
							throw compile_exception( "Brackets after if-statement are required", i );
					} else
						throw compile_exception( "Expected conditional after if-statement", i ); 
				} else if( at(i).str() == "while" ) {
					if( i + 1 < n && at(i+1).type == LEFT_BRACKET && at(i+1) == "(" ) {
						j = bracketIterator( i+1, n );
						if( j < n ) {
							b = junkYard( i+2, j-1 );
							if( at(j).type == LEFT_BRACKET && ( at(j).str() == ASYNC_BRACKET || at(j).str() == SYNC_BRACKET ) ) {
								k = bracketIterator( j, n );
								a = new AST( AT_LOOP, "while", { b, new AST( at(j+1).str() == SYNC_BRACKET ? AT_SYNCBLOCK : AT_ASYNCBLOCK, to_string(block_id++), { scotlandYard( j+1, k-1, block_id ) } ) } );
								i = k;
							} else 
								throw compile_exception( "Brackets after while-statement are required", i );
						} else 
							throw compile_exception( "Empty if-statement", i );
					} else 
						throw compile_exception( "Expected conditional after while-statement", i );
				} else if( at(i).str() == "return" ) {
					if( i + 1 < n && at(i+1).type != COMMA ) {
						l = commaIterator( i + 1, n );
						if( l == -1 )
							l = n;
						a = new AST( AT_FLOW, "return", { junkYard( i + 1, l ) } );
						i = l;
					} else 
						throw compile_exception( "Empty return statement", i );
				} else 
					throw compile_exception( "Unexpected '" + at(i).str() + "' keyword", i );
				break;
			case LEFT_BRACKET:
				if( at(i).str() == SYNC_BRACKET || at(i).str() == ASYNC_BRACKET ) {
			 		a = new AST( at(i).str() == SYNC_BRACKET ? AT_SYNCBLOCK : AT_ASYNCBLOCK );
					j = bracketIterator( i, n );
			 		a->children = scotlandYard( i, j - 1, block_id );
					i = j;
					break;
				} else if(  at(i).str() == "<" ) 
					throw compile_exception( "Unexpected '" + at(i).str() + "'" , i );
			case VARIABLE: case INTEGER: case FLOAT: case STRING: case FUNCTION:
				k = commaIterator( i, n );
				if( k == -1 )
					k = n;
				a = junkYard( i, k );
				i = k;
			break;
			case TYPENAME:
				k = resolveTypename( b, i, n );
				if( k < n && at(k).type == VARIABLE ) {
					c = new AST( AT_VARIABLEDEF, at(k).str(), { b } );
					if( k + 1 < n ) {
						if( at(k+1) == "=" ) {
							l = commaIterator( k, n );
							if( l == -1 )
								l = n;
							a = junkYard( k, l );
							r.push_back( c );
							i = l;
						} else if( at(k+1).type == COMMA ) {
							a = c;
							i = k + 1;
						} else 
							throw compile_exception( "Unexpected '" + at(k+1).str() + "' in variable definition" , k+1 );
					}
				} else 
					throw compile_exception( "Expected variable name after datatype specifier", i );
			break;
			default:
				throw compile_exception( "Unhandled token type", i );
		}
		r.push_back( a );
		if( i < n ) {
			if( at(i).type != COMMA ) 
				throw compile_exception( "Expected comma", i );
			else
				i++;
		}
	}
	return r;
}

// converts a vector of tokens in infix notation to postfix notation
Tokens Tokens::shuntingYard( int i, int n ) const {
	stack<Token> operator_stack;
	stack<int> comma_stack;
	Tokens output;
	for( Tokens::const_iterator t = begin() + i; t != ( begin() + n ); t++ ) {
		const Token& token = *t;
		if( !operator_stack.empty() && operator_stack.top().type == LEFT_BRACKET && comma_stack.top() == 0 && token.type != RIGHT_BRACKET )
			comma_stack.top() = 1;
		if( token.type == INTEGER || token.type == FLOAT || token.type == STRING || token.type == VARIABLE )
			output.push_back( token );
		else if( token.type == FUNCTION )
			operator_stack.push( token );
		else if( token.type == COMMA ) {
			comma_stack.top()++;
			if( operator_stack.empty() ) 
				throw compile_exception( "No left-parenthesis before ','", i );
			while( operator_stack.top().type != LEFT_BRACKET ) {
				output.push_back( operator_stack.top() );
				operator_stack.pop();
				if( operator_stack.empty() ) 
					throw compile_exception( "No left-parenthesis before ','", i );
			}
		} else if( token.type == OPERATOR ) {
			while( !operator_stack.empty() && operator_stack.top().type == OPERATOR && (
					( get<1>( operator_precedence.at(token.str()) ) == LEFT && get<0>(operator_precedence.at(token.str())) >= get<0>(operator_precedence.at(operator_stack.top().str())) ) || 
					( get<1>( operator_precedence.at(token.str()) ) == RIGHT && get<0>(operator_precedence.at(token.str())) > get<0>(operator_precedence.at(operator_stack.top().str())) ) ) ) {
				output.push_back( operator_stack.top() );
				operator_stack.pop();
			}
			operator_stack.push( token );
		} else if( token.type == LEFT_BRACKET ) {
			operator_stack.push( token );
			comma_stack.push(0);
		} else if( token.type == RIGHT_BRACKET ) {
			if( operator_stack.empty() ) 
				throw compile_exception( "Missing left-parenthesis", i );
			while( operator_stack.top().type != LEFT_BRACKET ) {
				output.push_back( operator_stack.top() );
				operator_stack.pop();
				if( operator_stack.empty() ) 
					throw compile_exception( "Missing left-parenthesis", i );
			}
			operator_stack.pop();
			if( token.str() == "[" ) {
				Token t;
				t.str() = "init-list " + to_string( comma_stack.top() );
				t.type = KEYWORD;
				output.push_back( t );
			} else if( token.str() == "{" ) {
				Token t;
				t.str() = "init-set " + to_string( comma_stack.top() );
				t.type = KEYWORD;
				output.push_back( t );
			} else if( !operator_stack.empty() && operator_stack.top().type == FUNCTION ) {
				operator_stack.top().str() += " " + to_string( comma_stack.top() );
				output.push_back( operator_stack.top() );
				operator_stack.pop();
			}
			comma_stack.pop();
		}
	}
	while( !operator_stack.empty() ) {
		if( operator_stack.top().type == LEFT_BRACKET ) 
			throw compile_exception( "Missing right-parenthesis", i );
		output.push_back( operator_stack.top() );
		operator_stack.pop();
	}
	return output;
}

bool AST::compare( const AST* other ) const {
	if( type != other->type || val != other->val || children.size() != other->children.size() )
		return false;
	for( size_t i = 0; i < children.size(); i++ )
		if( !children.at(i)->compare( other->children.at(i) ) )
			return false;
	return true;
}

AST* AST::getType() const {
	switch( type ) {
		case AT_WORD: case AT_FUNCTIONCALL:
			throw compile_exception( "Yet to implement: Type deduction for variables and functions", -1 );
			break;
		case AT_NUMBER: 
			return new AST( AT_DATATYPE, val.find('.') == string::npos ? "int": "float" , {} );
			break;
		case AT_STRING:
			return new AST( AT_DATATYPE, "string", {} );
			break;
		case AT_INLINE_SET: case AT_INLINE_LIST: 
			if( children.size() == 0 )
				throw compile_exception( "Type Error: Could not deduce type of empty set", -1 );
			else {
				AST *r = children.at(0)->getType(), *c;
				bool b;
				for( size_t i = 1; i < children.size(); i++ ) {
					c = children.at(1)->getType();
					b = r->compare( c );
					delete c->cascade();
					if( !b )
						throw compile_exception( "Type Error: Could not deduce type of inline list/set", -1 );
				}
				return new AST( AT_DATATYPE, type == AT_INLINE_LIST ? "list" : "set", {r} );
			}
			break;
		default:
			throw compile_exception( "Type Error: Node " + to_string(type) + " has no datatype", -1 );
	}
	return nullptr;
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
	int block_id = 1;
	type = AT_ASYNCBLOCK;
	val = "0";
	try {
		children = tokens.scotlandYard( 0, tokens.size(), block_id );
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
	string s = "(" + to_string( token_id ) + ") Error: " + err_str + "!";
	return s.c_str();
}

compile_exception::compile_exception( string err, int i ) {
	err_str = move( err );
	token_id = i;
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

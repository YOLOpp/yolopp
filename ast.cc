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
	{"!",make_tuple(1,RIGHT,true)},
	{"?",make_tuple(1,RIGHT,true)},
	{"<->",make_tuple(2,LEFT,false)},
	{"<~>",make_tuple(2,RIGHT,false)},
	{"^",make_tuple(3,RIGHT,false)},
	{"-u",make_tuple(4,RIGHT,true)},
	{"*",make_tuple(5,LEFT,false)},
	{"/",make_tuple(5,LEFT,false)},
	{"+",make_tuple(6,LEFT,false)},
	{"-",make_tuple(6,LEFT,false)},
	{"&&",make_tuple(12,LEFT,false)},
	{"^^",make_tuple(13,LEFT,false)},
	{"||",make_tuple(14,LEFT,false)},
	{"=",make_tuple(15,RIGHT,false)}
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
	{ "frac", make_pair(-1, false) },
	{ "array", make_pair(INT_MIN, true) },
	{ "complex", make_pair(-1, true) }
};

const string ASYNC_BRACKET = "{";
const string SYNC_BRACKET = "[";

/*Tokens::Tokens( const string & s ) {
	int n = s.size();
	bool func_fix = false;
	string t;
	for( int i = 0; i < n; i++ ) {
		if( !isalpha( s[i] ) && !isdigit( s[i] ) ) {
			if( !isspace( s[i] ) )
				func_fix = false;
			if( t != "" ) {
				push_back( t );
				t.clear();
				func_fix = true;
			}
			if( (s[i] == '|' || s[i] == '&' || s[i] == '^') && i+1 < n && s[i] == s[i+1] ) {
				push_back( s.substr( i, 2 ) );
				i++;
			} else if( i + 2 < n && s[i] == '<' && s[i+2] == '>' && ( s[i+1] == '-' || s[i+1] == '~' ) ) {
				push_back( s.substr( i, 3 ) );
				i+=2;
			} else if( s[i] == '-' && ( back() == ">" || back() == ")" || isdigit(back()[0]) || isalpha(back()[0]) ) ) {
				push_back( "-u" );
			} else if( s[i] == '(' ) {
				if( func_fix )
					back() += " ";
				push_back( "(" );
			} else if( !isspace( s[i] ) ) {
				push_back( string( 1, s[i] ) );
			}
		} else
			t += s[i];
	}
	if( t != "" )
		push_back( t );
}*/

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
				} else if( operator_precedence.find( string( 1, s[i] ) ) != operator_precedence.end() ) { // single-character operator
					u.push_back( s[i] );
					u.type = OPERATOR;
				} else if( s[i] == '<' ) {
					if( i + 2 < n && s[i+2] == '>' && ( s[i+1] == '-' || s[i+1] == '~' ) ) { // swap operator
						u.push_back( s[i] );
						u.push_back( s[i+1] );
						u.push_back( s[i+2] );
						u.type = OPERATOR;
						i += 2;
					} else { // opening '<'-bracket
						u.push_back( '<' );
						u.type = LEFT_BRACKET;
					}
				} else if( s[i] == '|' || s[i] == '&' || s[i] == '^' ) { // logical operators
					if( i + 1 < n && s[i] == s[i+1] ) {
						u.push_back( s[i] );
						u.push_back( s[i] );
						u.type = OPERATOR;
						i++;
					} else
						throw compile_exception( "Invalid operator '" + string( 1, s[i] ) + "'", i );
				} else if( s[i] == '{' || s[i] == '[' || s[i] == '(' ) { // opening brackets
					u.push_back( s[i] );
					u.type = LEFT_BRACKET;
				} else if( s[i] == '}' || s[i] == ']' || s[i] == ')' || s[i] == '>' ) { // closing brackets
					if( s[i] == '}' )
						u.push_back( '{' );
					else if( s[i] == ']' )
						u.push_back( '[' );
					else if( s[i] == ')')
						u.push_back( '(' );
					else
						u.push_back( '<' );
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
		//cout << "t:\"" << t << "\" mode:" << int(mode) << endl;
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

int bracketIterator( const Tokens& tokens, int i, int n ) {
	if( tokens[i].type == LEFT_BRACKET) {
		stack<char> bracket_stack;
		bracket_stack.push( tokens[i].str().front() );
		while( ++i < n && !bracket_stack.empty() ) {
			if( tokens[i].type == LEFT_BRACKET )
				bracket_stack.push( tokens[i].str().front() );
			else if( tokens[i].type == RIGHT_BRACKET ) {
				if( bracket_stack.top() != tokens[i].str().front() )
					throw compile_exception( "Mismatched parenthesis", i );
				bracket_stack.pop();
			}
		}
		if( !bracket_stack.empty() )
			throw compile_exception( "Missing parenthesis", i );
	} else
		throw compile_exception( "'" + tokens[i].str() + "' is not a left-parenthesis", i ); 
	return i; // return index past closing bracket
}

int commaIterator( const Tokens& tokens, int i , int n ) {
	int brackets = 0;
	while( i < n ) {
		if( tokens[i].type == LEFT_BRACKET )
			brackets++;
		else if( tokens[i].type == RIGHT_BRACKET ) {
			brackets--;
			if( brackets < 0 )
				return -1;
		} else if( tokens[i].type == COMMA )
			return i;
		i++;
	}
	return -1;
}

int resolveTypename( const Tokens& tokens, AST*& typename_result, int i, int n ) { // does not support datatype parameters yet
	bool isFinal = false;
	typename_result = new AST( AT_DATATYPE );
	AST* head = typename_result;
	while( i < n && tokens[i].type == TYPENAME ) {
		if( isFinal ) 
			throw compile_exception( "Not expecting '" + tokens[i].str() + "'", i );
		if( datatypes.find( tokens[i].str() ) != datatypes.end() ) {
			head->val = tokens[i];
			if( datatypes.at(tokens[i].str()).second == false ) {
				isFinal = true;
			} else {
				head->children.push_back( new AST( AT_DATATYPE ) );
				head = head->children.back();
			}
		} else 
			throw compile_exception( "Unknown datatype '" + tokens[i].str() + "'", i );
		i++;
	}
	if( !isFinal ) 
throw compile_exception( " Expected datatype", i );

	return i; // return index past last datatype specifier
}

AST* loopYard( const Tokens& postfix, int& n ) {
	AST* h = new AST;
	int parameters;
	stack<AST*> reverse;
	n--;
	if( n < 0 ) 
		throw compile_exception( "You done goofed", -1 );
	switch( postfix[n].type ) { /*KEYWORDS NOT YET SUPPORTED*/
		case OPERATOR:
			if( get<2>( operator_precedence.at(postfix[n].str()) ) == true ) // is unary
				parameters = 1;
			else
				parameters = 2;
			h->val = "operator" + postfix[n]; // temporary
			h->type = AT_FUNCTIONCALL;
			break;
		case FUNCTION: {
			stringstream ss( postfix[n].str(), ios_base::in );
			ss >> (h->val) >> parameters;
			h->type = AT_FUNCTIONCALL;
			break;
		}
		case INTEGER:
		case FLOAT:
			h->type = AT_NUMBER;
			h->val = postfix[n];
			parameters = 0;
			break;
		case STRING:
			h->type = AT_STRING;
			h->val = postfix[n];
			parameters = 0;
			break;
		case VARIABLE:
			h->type = AT_WORD;
			h->val = postfix[n];
			parameters = 0;
			break;
		default:
			throw compile_exception( "Unknown/Unsupported token type", -1 );
			h = nullptr;
	}
	for( int i = 0; i < parameters; i++ )
		reverse.push( loopYard( postfix, n ) );
	while( !reverse.empty() ) {
		h->children.push_back( reverse.top() );
		reverse.pop();
	}
	return h;
}

AST* junkYard( const Tokens& tokens, int i, int n ) { // converts statement tokens to AST using shuntingYard
	Tokens postfix = shuntingYard( tokens, i, n );
	int k = postfix.size();
	return loopYard( postfix, k );
}

vector<AST*> graveYard( const Tokens& tokens, int i, int n ) { // applies junkYard to multiple statements seperated by commas
	vector<AST*> r;
	AST* temp = nullptr;
	int j = i;
	while( j < n ) {
		while( tokens[j].type != COMMA && j < n )
			j++;
		junkYard( tokens, i, j );
		i = j;
	}
}

vector<AST*> scotlandYard( const Tokens& tokens, int i, int n ) {
	vector<AST*> r;
	AST *a = nullptr, *b = nullptr, *c = nullptr, *d = nullptr;
	int j, k, l, m;
	while( i < n ) {
		switch( tokens[i].type ) {
			case KEYWORD:
				if( tokens[i].str() == "function" ) {
					j = resolveTypename( tokens, d, i + 1, n );
					if( j < n && tokens[j].type == FUNCTION ) {
						k = bracketIterator( tokens, j+1, n );
						m = j + 2;
						b = new AST( AT_ARRAY );
						while( m < k ) {
							if( tokens[m].type == TYPENAME ) {
								l = resolveTypename( tokens, c, m, k - 1 );
								if( l + 1 < n && tokens[l].type == VARIABLE && ( tokens[l+1].type == COMMA || tokens[l+1].type == RIGHT_BRACKET ) ) {
									b->children.push_back( new AST( AT_VARIABLEDEF, tokens[l].str() /*variable-name*/, { c /*data-type*/} ) );
									m = l+2;
								} else 
									throw compile_exception( "Variable name required in function definition", i );
							} else 
								throw compile_exception( "Expected typename in function definition", i );
						}
						l = bracketIterator( tokens, k, n );
						if( tokens[j+1].str() == "(" && ( tokens[k].str() == SYNC_BRACKET || tokens[k].str() == ASYNC_BRACKET ) ) {
					 		a = new AST( AT_FUNCTIONDEF, tokens[j].str(), { d /*return-type*/, b /*parameters*/, 
					 				new AST( tokens[i].str() == SYNC_BRACKET ? AT_SYNCBLOCK : AT_ASYNCBLOCK, "", scotlandYard( tokens, k+1, l - 1 ) ) } );
					 		i = l + 1;
						} else
							throw compile_exception( "Unexpected parenthesis-type near function definition", i );
					} else 
						throw compile_exception( "Expected function name", i );
				} else if( tokens[i].str() == "if" ) {
					if( i+1 < n && tokens[i+1].type == LEFT_BRACKET && tokens[i+1].str() == "(" ) {
						j = bracketIterator( tokens, i+1, n );
						b = junkYard( tokens, i+2, j-1 );
						if( j < n ) {
							if( tokens[j].type == LEFT_BRACKET && ( tokens[j].str() == ASYNC_BRACKET || tokens[j].str() == SYNC_BRACKET ) ) {
								k = bracketIterator( tokens, j, n );
								c = new AST( tokens[j].str() == SYNC_BRACKET ? AT_SYNCBLOCK : AT_ASYNCBLOCK, "", { scotlandYard( tokens, j+1, k-1 ) } );
								a = new AST( AT_CONDITIONAL, "", { b, c } );
								if( k < n && tokens[k].type == KEYWORD && tokens[k].str() == "else" ) {
									if( k + 1 < n && tokens[k+1].type == LEFT_BRACKET && ( tokens[k+1].str() == ASYNC_BRACKET || tokens[k+1].str() == SYNC_BRACKET ) ) {
										l = bracketIterator( tokens, k+1, n );
										d = new AST( tokens[j+1].str() == SYNC_BRACKET ? AT_SYNCBLOCK : AT_ASYNCBLOCK, "", { scotlandYard( tokens, k+2, l-1 ) } );
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
				} else if( tokens[i].str() == "while" ) {
					if( i + 1 < n && tokens[i+1].type == LEFT_BRACKET && tokens[i+1] == "(" ) {
						j = bracketIterator( tokens, i+1, n );
						if( j < n ) {
							b = junkYard( tokens, i+2, j-1 );
							if( tokens[j].type == LEFT_BRACKET && ( tokens[j].str() == ASYNC_BRACKET || tokens[j].str() == SYNC_BRACKET ) ) {
								k = bracketIterator( tokens, j, n );
								a = new AST( AT_LOOP, "while", { b, new AST( tokens[j+1].str() == SYNC_BRACKET ? AT_SYNCBLOCK : AT_ASYNCBLOCK, "", { scotlandYard( tokens, j+1, k-1 ) } ) } );
								i = k;
							} else 
								throw compile_exception( "Brackets after while-statement are required", i );
						} else 
							throw compile_exception( "Empty if-statement", i );
					} else 
						throw compile_exception( "Expected conditional after while-statement", i );
				} else if( tokens[i].str() == "return" ) {
					if( i + 1 < n && tokens[i+1].type != COMMA ) {
						l = commaIterator( tokens, i + 1, n );
						if( l == -1 )
							l = n;
						a = new AST( AT_FLOW, "return", { junkYard( tokens, i + 1, l ) } );
						i = l;
					} else 
						throw compile_exception( "Empty return statement", i );
				} else 
					throw compile_exception( "Unexpected '" + tokens[i].str() + "' keyword", i );
				break;
			case LEFT_BRACKET:
				if( tokens[i].str() == SYNC_BRACKET || tokens[i].str() == ASYNC_BRACKET ) {
			 		a = new AST( tokens[i].str() == SYNC_BRACKET ? AT_SYNCBLOCK : AT_ASYNCBLOCK );
					j = bracketIterator( tokens, i, n );
			 		a->children = scotlandYard( tokens, i, j - 1 );
					i = j;
					break;
				} else if(  tokens[i].str() == "<" ) 
					throw compile_exception( "Unexpected '" + tokens[i].str() + "'" , i );
			case VARIABLE: case INTEGER: case FLOAT: case STRING: case FUNCTION:
				k = commaIterator( tokens, i, n );
				if( k == -1 )
					k = n;
				a = junkYard( tokens, i, k );
				i = k;
			break;
			case TYPENAME:
				k = resolveTypename( tokens, b, i, n );
				if( k < n && tokens[k].type == VARIABLE ) {
					c = new AST( AT_VARIABLEDEF, tokens[k].str(), { b } );
					if( k + 1 < n ) {
						if( tokens[k+1] == "=" ) {
							l = commaIterator( tokens, k, n );
							if( l == -1 )
								l = n;
							a = junkYard( tokens, k, l );
							r.push_back( c );
							i = l;
						} else if( tokens[k+1].type == COMMA ) {
							a = c;
							i = k + 1;
						} else 
							throw compile_exception( "Unexpected '" + tokens[k+1].str() + "' in variable definition" , k+1 );
					}
				} else 
					throw compile_exception( "Expected variable name after datatype specifier", i );
			break;
		}
		r.push_back( a );
		if( i < n ) {
			if( tokens[i].type != COMMA ) 
				throw compile_exception( "Expected comma", i );
			else
				i++;
		}
	}
	return r;
}

Tokens shuntingYard( const Tokens& tokens, int i, int n ) {
	stack<Token> operator_stack;
	stack<int> comma_stack;
	Tokens output;
	for( Tokens::const_iterator t = tokens.begin() + i; t != ( tokens.begin() + n ); t++ ) {
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
			if( token.str() == "<" ) {
				Token t;
				t.str() = "array-init " + to_string( comma_stack.top() );
				t.type = KEYWORD;
				output.push_back( t );
			} else if( !operator_stack.empty() && operator_stack.top().type == FUNCTION ) {
				operator_stack.top().str() += " " + to_string( comma_stack.top() );
				output.push_back( operator_stack.top() );
				operator_stack.pop();
			}
			comma_stack.pop();
		}
		/*cout << "(" << token << "): " ;
		for( auto& x : output )
			cout << x << "@";
		cout << endl;*/

	}
	while( !operator_stack.empty() ) {
		if( operator_stack.top().type == LEFT_BRACKET ) 
			throw compile_exception( "Missing right-parenthesis", i );
		output.push_back( operator_stack.top() );
		operator_stack.pop();
	}
	return output;
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
	type = AT_ASYNCBLOCK;
	children = scotlandYard( tokens, 0, tokens.size() );
}

AST::~AST() { // geen recursieve delete

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

int main() {
	//string s = "yolo = x <-> y / 5.0 * f(3,\"6\")";
	//string s = "yolo = <4,5,6>*9";
	//string s = "func()";
	//string s = "function int abs( complex int z ) [ int a = sqrt( z * conj(z) ), return a ]";
	//string s = "if( swag ) [ yolo() ]";
	//string s = "if( foo && bar ) { func() } else [ bar() ]";
	string s = "int i = 0, while( less( i, 10 ) ) [ print(i), i = i + 1 ]";
	Tokens t(s);
	for( auto& i : t )
		cout << i << ";";
	cout << endl;

	cout << AST(t) << endl;
}
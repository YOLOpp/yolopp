#include <iostream>
#include <stack>
#include <string>
#include <vector>
#include <map>
#include <tuple>
#include <climits>

#include "ast.h"

map< string, tuple< int, associativity, bool > > operator_precedence {
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
map<string, int> keywords {
	{ "import", INT_MIN },
	{ "if", 1 },
	{ "else", 0 },
	{ "forever", 0 },
	{ "break", -1 },
	{ "continue", 0 },
	{ "function", 3 },
	{ "array-init", INT_MIN }
};

// name, parameters, is prefix
map<string, pair<int, bool> > datatypes {
	{ "int", make_pair(-1, false) },
	{ "string", make_pair(0, false) },
	{ "frac", make_pair(-1, false) },
	{ "array", make_pair(INT_MIN, true) },
	{ "complex", make_pair(-1, true) }
};

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
					} else {
						cerr << "Error: Invalid operator '" << s[i] << "'!" << endl;
						throw;
					}
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
					else {
						cerr << "Error: Unexpected '.' !" << endl;
						throw;
					}
					t.push_back( s[i] );
					terminate = false;
				} else if( !isdigit( s[i] ) && ( mode == MODE_INT || mode == MODE_FLOAT ) ) { // filter letters from numbers
					cerr << "Error: Unexpected '" << s[i] << "' in number!" << endl;
					throw;
				} else {
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
		else if( mode == MODE_STRING ) {
			cerr << "Error: Missing closing-'\"'!" << endl;
			throw;
		} else {
			cerr << "Internal Error: Unexpected tokenize mode!" << endl;
			throw;
		}
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
				if( bracket_stack.top() != tokens[i].str().front() ) {
					cerr << "Error: Mismatched parenthesis!" << endl;
					throw;
				}
				bracket_stack.pop();
			}
		}
		if( !bracket_stack.empty() ) {
			cerr << "Error: Missing parenthesis!" << endl;
			throw;
		}
	} else {
		cerr << "Internal Error: '" << tokens[i] << "' is not a left-parenthesis!" << endl;
		throw; 
	}
	return i;
}

int resolveTypename( const Tokens& tokens, int i, int n ) {
	bool isFinal = false;
	while( i < n && tokens[i].type == TYPENAME ) {
		if( isFinal ) {
			cerr << "Error: Not expecting '" << tokens[i] << "'!" << endl;
			throw;
		}
		if( datatypes.find( tokens[i].str() ) != datatypes.end() ) {
			if( datatypes[tokens[i].str()].second == false )
				isFinal = true;
		} else {
			cerr << "Error: Unknown datatype '" << tokens[i] << "'!" << endl;
			throw;
		}
		i++;
	}
	if( !isFinal ) {
		cerr << "Error: Expected datatype!" << endl;
		throw;
	}
	return i;
}

AST* scotlandYard( const Tokens& tokens, int i = 0, int n = -1 ) {
	int brackets;
	int j, k;
	if( n == -1 )
		n = tokens.size();
	if( tokens[i].type == KEYWORD ) {
		if( tokens[i].str() == "if" ) {
			if( i+1 < n && tokens[i+1].type == LEFT_BRACKET && tokens[i+1].str() == "(" ) {
				j = bracketIterator( tokens, i+1, n );
				scotlandYard( tokens, i+1, j );
				scotlandYard( tokens, j, n );
			} else {
				cerr << "Error: Expected conditional after if-statement!" << endl;
				throw;
			}
		} else if( tokens[i].str() == "function" ) {
			j = resolveTypename( tokens, i+1, n );
			if( tokens[j].type == FUNCTION ) {
				k = bracketIterator( tokens, j+1, n );
				scotlandYard( tokens, k, n );
			} else {
				cerr << "Error: Expected function name!" << endl;
				throw;
			}
		}
	} else if( tokens[i].type == LEFT_BRACKET) {
		brackets = 1;
		while( ++i < n && brackets > 0 ) {
			if( tokens[i].type == LEFT_BRACKET )
				brackets++;
			else if( tokens[i].type == RIGHT_BRACKET )
				brackets--;
		}
	} else {
		throw;
	}
}

Tokens shuntingYard( const Tokens& tokens ) {
	stack<Token> operator_stack;
	stack<int> comma_stack;
	Tokens output;
	for( const Token& token : tokens ) {
		if( !operator_stack.empty() && operator_stack.top().type == LEFT_BRACKET && comma_stack.top() == 0 && token.type != RIGHT_BRACKET )
			comma_stack.top() = 1;
		if( token.type == INTEGER || token.type == FLOAT || token.type == STRING || token.type == VARIABLE )
			output.push_back( token );
		else if( token.type == FUNCTION )
			operator_stack.push( token );
		else if( token.type == COMMA ) {
			comma_stack.top()++;
			if( operator_stack.empty() ) {
				cerr << "Error: No left-parenthesis before ','!" << endl;
				throw;
			}
			while( operator_stack.top().type != LEFT_BRACKET ) {
				output.push_back( operator_stack.top() );
				operator_stack.pop();
				if( operator_stack.empty() ) {
					cerr << "Error: No left-parenthesis before ','!" << endl;
					throw;
				}
			}
		} else if( token.type == OPERATOR ) {
			while( !operator_stack.empty() && operator_stack.top().type == OPERATOR && (
					( get<1>( operator_precedence[token.str()] ) == LEFT && get<0>(operator_precedence[token.str()]) >= get<0>(operator_precedence[operator_stack.top().str()]) ) || 
					( get<1>( operator_precedence[token.str()] ) == RIGHT && get<0>(operator_precedence[token.str()]) > get<0>(operator_precedence[operator_stack.top().str()]) ) ) ) {
				output.push_back( operator_stack.top() );
				operator_stack.pop();
			}
			operator_stack.push( token );
		} else if( token.type == LEFT_BRACKET ) {
			operator_stack.push( token );
			comma_stack.push(0);
		} else if( token.type == RIGHT_BRACKET ) {
			if( operator_stack.empty() ) {
				cerr << "Error: Missing left-parenthesis!" << endl;
				throw;
			}
			while( operator_stack.top().type != LEFT_BRACKET ) {
				output.push_back( operator_stack.top() );
				operator_stack.pop();
				if( operator_stack.empty() ) {
					cerr << "Error: Missing left-parenthesis!" << endl;
					throw;
				}
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
		if( operator_stack.top().type == LEFT_BRACKET ) {
			cerr << "Error: Missing right-parenthesis!" << endl;
			throw;
		}
		output.push_back( operator_stack.top() );
		operator_stack.pop();
	}
	return output;
}


/*Tokens shuntingYard( const Tokens& tokens ) {
	stack<Token> operator_stack;
	stack<int> comma_stack;
	Tokens output;
	for( int i = 0; i < tokens.size(); i++ ) {
		Token s = tokens.at(i);
		int x;
		if( !operator_stack.empty() && operator_stack.top().type == LEFT_BRACKET && comma_stack.top() == 0 && s.type != RIGHT_BRACKET )
			comma_stack.top() = 1;

		cout << "(" << s << ")";
		if( s.type == OPERATOR) {
			if( s.type == FLOAT || s.type == STRING || s.type == INTEGER ) // constants
				output.push_back( s );
			} else if( s.type == LEFT_BRACKET ) { // left parenthesis
				operator_stack.push( s );
				comma_stack.push( 0 );
			} else if( s.type == RIGHT_BRACKET ) {
				if( operator_stack.empty() ) { // no left-parenthesis
					cerr << "Error: Unmatched right parenthesis!" << endl;
					throw;
				}
				while( operator_stack.top().type() != LEFT_BRACKET ) { // find left-parenthesis
					output.push_back( operator_stack.top() );
					operator_stack.pop();
					if( operator_stack.empty() ) { // no left-parenthesis
						cerr << "Error: Unmatched right parenthesis!" << endl;
						throw;
					}
				}
				if( operator_stack.top().str() != s.str() ) { // unmatched parenthesis
					cerr << "Error: Closing \'" << operator_stack.top() << "' with '" << s << "'!" << endl;
					throw;
				}
				operator_stack.pop();
				if( s.str() == ">" ) {
					Token t;
					t.str() = "array " + to_string(comma_stack.top());
					t.type = KEYWORD;
					output.push_back( t );
				} else {
					if( operator_stack.empty() ) {

					} else if( operator_stack.top().type ==  ) {
						try {
							stoi( operator_stack.top() );
						} catch( const invalid_argument& ia ) {
							if( operator_stack.top() != "(" && operator_stack.top() != "{" && operator_stack.top() != "[" &&
									operator_stack.top() != "<" && operator_stack.top() != "," ) {
								output.push_back( operator_stack.top() + to_string( comma_stack.top() ) );
								operator_stack.pop();
							}
						}
					}
				}
				comma_stack.pop();
			} else if( s == "," ) {
				if( operator_stack.empty() ) {
					cerr << "Error: Unmatched right parenthesis!" << endl;
					throw;
				}
				while( operator_stack.top() != "(" && operator_stack.top() != "{" && operator_stack.top() != "[" &&
						operator_stack.top() != "<" ) {
					output.push_back( operator_stack.top() );
					operator_stack.pop();
					if( operator_stack.empty() ) {
						cerr << "Error: Unmatched right parenthesis!" << endl;
						throw;
					}
				}
				comma_stack.top()++;
			} else if( s.back() == ' ' ) {
				operator_stack.push( s );
			} else if( keywords.find( s ) == keywords.end() ) {
				output.push_back( s );
			}
		} else {
			while( !operator_stack.empty() && ( ( get<1>(ref->second) == LEFT && get<0>(ref->second) >= get<0>( operator_precedence[ operator_stack.top()] ) ) || 
					(get<1>(ref->second) == RIGHT && get<0>(ref->second) > get<0>( operator_precedence[ operator_stack.top() ] ) ) ) )  {
				output.push_back( operator_stack.top() );
				operator_stack.pop();
			}
			operator_stack.push( s );
		}
		for( auto& i : output )
			cout << i << "#";
		cout << endl;
	}
	while( !operator_stack.empty() ) {
		if( operator_stack.top() == "(" || operator_stack.top() == "{" || operator_stack.top() == "{" || operator_stack.top() == "<" ) {
			cerr << "Error: Unmatched left parenthesis!" << endl;
			throw;
		}
		output.push_back( operator_stack.top() );
		operator_stack.pop();
	}
	return output;
}*/



int main() {
	//string s = "yolo = x <-> y / 5.0 * f(3,\"6\")";
	string s = "yolo = <4,5,6>*9";
	Tokens t(s);
	for( auto& i : t )
		cout << i << " " << int(i.type) << "#";
	cout << endl;
	Tokens r = shuntingYard( t );
	for( auto& i : r )
		cout << i << "#";
}
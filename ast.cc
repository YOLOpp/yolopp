#include <iostream>
#include <stack>
#include <string>
#include <vector>
#include <map>
#include <tuple>

#include "ast.h"

Tokens::Tokens( const string & s ) {
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
}

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

map<string, int> keywords;

Tokens shuntingYard( const Tokens& tokens ) {
	stack<string> operator_stack;
	stack<int> comma_stack;
	Tokens output;
	for( int i = 0; i < tokens.size(); i++ ) {
		string s = tokens.at(i);
		auto ref = operator_precedence.find( s );
		int x;
		if( !operator_stack.empty() && ( operator_stack.top() == "(" || operator_stack.top() == "[" || operator_stack.top() == "{" || operator_stack.top() == "<" ) && comma_stack.top() == 0 && ( s != ")" && s != "]" && s != ">" && s != "}" ) )
			comma_stack.top() = 1;

		cout << "(" << s << ")";
		if( ref == operator_precedence.end() ) {
			try {
				x = stoi( s );
				output.push_back( s );
			} catch( const invalid_argument& ia ) {
				if( s == "(" || s == "[" || s == "{" || s == "<" ) {
					operator_stack.push( s );
					comma_stack.push( 0 );
				} else if( s == ")" || s == "]" || s == "}" || s == ">" ) {
					if( operator_stack.empty() ) {
						cerr << "Error: Unmatched right parenthesis!" << endl;
						throw;
					}
					while( operator_stack.top() != "(" && operator_stack.top() != "{" && operator_stack.top() != "[" && operator_stack.top() != "<" ) {
						output.push_back( operator_stack.top() );
						operator_stack.pop();
						if( operator_stack.empty() ) {
							cerr << "Error: Unmatched right parenthesis!" << endl;
							throw;
						}
					}
					operator_stack.pop();
					if( s == ">" )
						output.push_back( "array " + to_string(comma_stack.top()) );
					else {
						if( operator_stack.empty() ) {

						} else if( operator_precedence.find( operator_stack.top() ) == operator_precedence.end() ) {
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
}

#include <set>
#include "ast.h"

const map<string, string> operator_synonyms {
	{"at","@"},
	{"shuffle","?"},
	{"sort","!"},
	{"sizeof","#"},
	{"swap","<->"},
	{"contains","\\"},	// these symbols are temporary, to be replaced with unicode
	{"as","~"}			//
};

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
					if( i+1<n && s[i+1] == '>' ) { // -> operator
						u.push_back( '>' );
						i++;
					} else if( !( back().type == RIGHT_BRACKET || back().type == VARIABLE || back().type == FLOAT || back().type == INTEGER || back().type == STRING ) ) // unary minus
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
				} else if( s[i] == '.' ) { // float or dot operator
					if( mode == MODE_INT && i+1<n && !isalpha( s[i+1] ) ) {
						mode = MODE_FLOAT;
						t.push_back( s[i] );
						terminate = false;
					} else if( mode == MODE_FLOAT && t.empty() ) {
						if( i+1<n && !isdigit( s[i+1] ) ) {
							u.push_back('.');
							u.type = OPERATOR;
							mode = MODE_NONE;
						} else {
							t.push_back( s[i] );
							terminate = false;
						}
					} else {
						u.push_back('.');
						u.type = OPERATOR;
					}
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
					else if( finalTypenames.find( string(t) ) != finalTypenames.end() || nonFinalTypenames.find( string(t) ) != nonFinalTypenames.end() )
						t.type = TYPENAME;
					else if( operator_synonyms.find( string(t) ) != operator_synonyms.end() ) {
						t.type = OPERATOR;
						t.str() = operator_synonyms.at( t.str() );
					} else
						t.type = VARIABLE;
				} else if( mode == MODE_FLOAT )
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
				if( size() > 0 && back().type == VARIABLE && u.type == LEFT_BRACKET )
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
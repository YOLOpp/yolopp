#include <set>
#include <cassert>
#include "ast.h"

const map<string, string> operator_synonyms {
	{"or","\u2228"},
	{"and","\u2227"},
	{"xor","\u22BB"},
	{"not","\u00AC"},
	{"at","@"},
	{"shuffle","?"},
	{"sort","!"},
	{"sizeof","#"},
	{"swap","<->"},
	{"contains","\u220A"},
	{"as","~"} // temp
};

void Tokens::tokenizeString( const string& s, int& i, int n ) {
	assert( s[i] == '"' );
	int j = i++;
	bool escape = false;

	while( i < n ) {
		if( escape )
			escape = false;
		else if( s[i] == '"' ) {
			emplace_back( STRING, "u8" + s.substr( j, (++i)-j ) );
			return;
		} else if( s[i] == '\\' )
			escape = true;
		i++;
	}
	throw compile_exception( "Expected end of string", j );
}

void Tokens::tokenizeNumber( const string& s, int& i, int n ) {
	assert( isdigit( s[i] ) || s[i] == '.' );
	bool real = false;
	bool dot = false;
	int j = i;

	if( s[i] == '.' && ( i+1 == n || !isdigit( s[i+1] ) ) )
		tokenizeOperator( s, i, n );
	else {
		while( i < n ) {
			if( s[i] == '.' ) {
				if( real ) {
					dot = true;
					break;
				} else if( i+1 == n || !isalpha( s[i+1] ) )
					real = true;
				else {
					dot = true;
					break;
				}
			} else if( !isdigit( s[i] ) )
				break;
			i++;
		}
		if( dot )
			tokenizeOperator( s, i, n );
		emplace_back( real?FLOAT:INTEGER, s.substr( j, i-j-dot ) );
	}
}

bool isNonfirstWordSymbol( char c ) {
	return isalpha(c) || c == '_' || isdigit(c);
}

void Tokens::tokenizeWord( const string& s, int& i, int n ) {
	assert( isalpha( s[i] ) );
	int j = i;
	string w;
	token_type t = VARIABLE;
	map<string,string>::const_iterator x;

	while( i < n && isNonfirstWordSymbol( s[i] ) ) i++;
	w = s.substr( j, i-j );
	if( keywords.count( w ) )
		t = KEYWORD;
	else if( ( x = operator_synonyms.find( w ) ) != operator_synonyms.end() ) {
		t = OPERATOR;
		w = x->second;
	} else if( finalTypenames.find( w ) != finalTypenames.end() || nonFinalTypenames.find( w ) != nonFinalTypenames.end() )
		t = TYPENAME;
	emplace_back( t, w );
}

constexpr bool isBracket( char c ) {
	return c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}';
}

void Tokens::tokenizeBracket( const string& s, int& i, int n ) {
	assert( isBracket( s[i] ) );

	char c = s.at(i++);
	c += (c == ')'); 
	bool isLeft = (c == '(' || c == '[' || c == '{');
	if( c == '(' && size() > 0 && back().type == VARIABLE )
		back().type = FUNCTION;
	emplace_back( isLeft?LEFT_BRACKET:RIGHT_BRACKET, string(1,c-2*(1-isLeft)));
}

void Tokens::tokenizeOperator( const string& s, int& i, int n ) {
	if( s[i] == ',' ) {
		i++;
		emplace_back( COMMA, "," );
	} else {
		for( int k = min(4,n-i); k > 0; k-- ) {
			auto x = operator_precedence.find( s.substr(i,k) );
			if( x != operator_precedence.end() ) {
				if( x->first == "-" && ( size() == 0 || !( back().type == RIGHT_BRACKET || back().type == VARIABLE || back().type == FLOAT || back().type == INTEGER || back().type == STRING ) ) )
					emplace_back( OPERATOR, "-u" ); // unary minus
				else
					emplace_back( OPERATOR, x->first );
				i += k;
				return;
			}
		}
		throw compile_exception( "Not an operator", i );
	}
}

Tokens::Tokens( const string& s ) {
	int n = s.size();
	int i = 0;

	while( i < n ) {
		if( isspace( s[i] ) )
			i++;
		else if( s[i] == '"' )
			tokenizeString( s, i, n );
		else if( isdigit( s[i] ) || s[i] == '.' )
			tokenizeNumber( s, i, n );
		else if( isalpha( s[i] ) )
			tokenizeWord( s, i, n );
		else if( isBracket( s[i] ) )
			tokenizeBracket( s, i, n );
		else
			tokenizeOperator( s, i, n );
	}
}
#include "ast.h"

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
			if( token.str() == "~" ) {
				bool done = false;
				stack<Token> reverse;
				while( ++t != begin() + n ) {
					if( t->type == TYPENAME ) {
						reverse.push( *t );
						if( finalTypenames.count( t->str() ) ) { // ending typename
							done = true;
							break;
						}
					} else
						throw compile_exception( "Incomplete typename in typecast", i );
				}
				while( !reverse.empty() ) {
					output.push_back( reverse.top() );
					reverse.pop();
				}
				output.push_back( token );
				if( !done )
					throw compile_exception( "Unexpected end of statement", i );
			} else
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
			} else if( comma_stack.top() > 1 ) {
				Token t;
				t.str() = "init-tuple " + to_string( comma_stack.top() );
				t.type = KEYWORD;
				output.push_back( t ); 
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

AST* Tokens::postfixTranslate( int& n ) const {
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
			if( at(n) == "~" )
				h->val = "@static_cast";
			else
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
				else if( a == "init-tuple" )
					h->type = AT_ARRAY;
				else
					throw compile_exception( "Unknown initaliser list", -1 );
				h->val = at(n);
				ss >> parameters;
				break;
			}
		}
		case TYPENAME:
			h->type = AT_DATATYPE;
			h->val = at(n);
			parameters = nonFinalTypenames.count( at(n) );
		break;
		default:
			throw compile_exception( "Unknown/Unsupported token type " + to_string( at(n).type ), -1 );
			h = nullptr;
	}
	for( int i = 0; i < parameters; i++ )
		reverse.push( postfixTranslate( n ) );
	while( !reverse.empty() ) {
		h->children.push_back( reverse.top() );
		reverse.pop();
	}
	return h;
}

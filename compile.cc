#include <cassert>
#include "ast.h"

bool Tokens::isTypename( int i, const set<string>& typenames, bool tuples ) const {
	while( at(i).type == LEFT_BRACKET && at(i).str() == "(" && tuples )
		i++;
	return typenames.count( at(i) ) || nonFinalTypenames.count( at(i) );
}

bool Tokens::isVariable( int i, const set<string>& typenames ) const {
	return !isKeyword( i ) && !isTypename( i, typenames );
}

bool Tokens::isBlock( int i ) const {
	return at(i).type == LEFT_BRACKET && ( at(i).str() == SYNC_BRACKET || at(i).str() == ASYNC_BRACKET );
}

bool Tokens::isKeyword( int i ) const {
	return keywords.count( at(i) );
}

int Tokens::bracketIterator( int i, int n ) const { // i on first bracket, n on arbitrary bound
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
	return i-1; // return at closing bracket
}

int Tokens::commaIterator( int i, int n ) const { // i not on comma, n on arbitrary bound
	int brackets = 0;
	while( i < n ) {
		if( at(i).type == LEFT_BRACKET )
			brackets++;
		else if( at(i).type == RIGHT_BRACKET ) {
			brackets--;
			if( brackets < 0 )
				return i;
		} else if( at(i).type == COMMA && brackets == 0 )
			return i;
		i++;
	}
	return n;
} // return at comma, or at bracket for error

int Tokens::elseIterator( int i, int n ) const { // i on starting point, n on next comma
	int brackets = 0;
	while( --n >= i ) {
		if( at(n).type == LEFT_BRACKET )
			brackets--;
		else if( at(n).type == RIGHT_BRACKET ) {
			brackets++;
			if( brackets < 0 )
				return -1;
		} else if( at(n).type == KEYWORD && at(n).str() == "else" && brackets == 0 )
			return n;
	}
	return -1;
} // return at else, or -1 for no else

int Tokens::resolveTypename( AST*& typename_result, int i, int n, const set<string>& typenames ) const {
	bool isFinal = false;
	typename_result = new AST( AT_DATATYPE );
	AST* head = typename_result;
	while( i < n && ( isTypename( i, typenames, false ) || ( at(i).str() == "(" && at(i).type == LEFT_BRACKET ) ) ) {
		if( isFinal ) 
			throw compile_exception( "Unexpected token after typename", i );
		if( isTypename( i, typenames, false ) ) {
			head->val = at(i);
			if( nonFinalTypenames.count( at(i) ) ) {
				head->children.push_back( new AST( AT_DATATYPE ) );
				head = head->children.back();
			} else
				isFinal = true;
		} else if( at(i).str() == "(" ) { // tuples    // () is not considered a tuple
			head->type = AT_ARRAY;
			do {
				head->children.push_back( nullptr );
				i = resolveTypename( head->children.back(), i+1, n, typenames );
			} while( at(i).type == COMMA );
			if( at(i).type == RIGHT_BRACKET && at(i).str() == "(" )
				isFinal = true;
			else
				throw compile_exception( "Unexpected end of tuple", i );
		} else
			throw compile_exception( "Unknown datatype '" + at(i).str() + "'", i );
		i++;
	}
	if( !isFinal ) 
		throw compile_exception( "Expected datatype", i );
	return i; // return index past last datatype specifier
}

AST* Tokens::getNamedTuple( int& i, int k, const set<string>& typenames ) const { // i on first bracket, k on arbitrary bound
	assert( at(i).str() == "(" && at(i).type == LEFT_BRACKET );
	AST* tuple = new AST( AT_ARRAY );
	AST* datatype;
	int l;
	bool notFirst = false;

	i++;
	while( i < k && at(i-notFirst).type != RIGHT_BRACKET ) {
		if( !isTypename( i, typenames ) )
			throw compile_exception( "Expected typename in function/space definition", i );

		l = resolveTypename( datatype, i, k, typenames );
		
		if( l+1 >= k || at(l).type != VARIABLE || ( at(l+1).type != COMMA && at(l+1).type != RIGHT_BRACKET ) )
			throw compile_exception( "Variable name required in function/space definition", i );

		tuple->children.push_back( new AST( AT_VARIABLEDEF, at(l).str(), { datatype } ) );
		i = l+2;
		notFirst = true;		
	}
	i -= notFirst;
	if( at(i).type != RIGHT_BRACKET || at(i).str() != "(" )
		throw compile_exception( "Expected closing bracket in tuple", i );
	return tuple;
} // returns AST* tuple, sets i to closing bracket, throws on error

AST* Tokens::statementTranslate( int i, int n ) const {
	Tokens postfix = shuntingYard( i, n );
	int k = postfix.size();
	return postfix.postfixTranslate( k );
}

void Tokens::spaceTranslateFirst( set<string>& spaceNames, int i, int n ) const {
	assert( at(i).str() == "space" );
	
	if( at(i+1).type == FUNCTION )
		spaceNames.insert( at(i+1).str() );
	else
		throw compile_exception( "Expected typename after space", i );
} // adds typenames to space

void Tokens::spaceTranslateSecond( vector<AST*>& block, int i, int n, const set<string>& typenames ) const {
	assert( at(i).str() == "space" );
	AST* structure;
	string spaceName;

	spaceName = at(i+1).str();
	i += 2;
	structure = getNamedTuple( i, n, typenames );

	if( i+1 < n )
		throw compile_exception( "Unexpected end of space definition", i );
	block.push_back( new AST( AT_SPACEDEF, spaceName, {structure} ) );
}

AST* encapsulateBlock( vector<AST*>&& children, const string& block_type ) {
	blockRegister.push_back( new AST( block_type == ASYNC_BRACKET ? AT_ASYNCBLOCK : AT_SYNCBLOCK, to_string( blockRegister.size()+1 ), std::move( children ) ) );
	return blockRegister.back();
}

void Tokens::functionTranslate( vector<AST*>& block, int i, int n, const set<string>& typenames ) const {
	assert( at(i).str() == "function" );
	int k;
	AST *returnType, *parameters;
	string functionName;

	i = resolveTypename( returnType, i+1, n, typenames );

	if( at(i).type == OPERATOR ) {
		functionName = "@" + operator_function_name.at(at(i).str());
		if( !overloadable_operators.count( at(i) ) )
			throw compile_exception( "Can't overload this operator", i );
	} else if( at(i).type == FUNCTION )
		functionName = at(i).str();
	else
		throw compile_exception( "Expected function name", i );
		
	i += 1;

	parameters = getNamedTuple( i, n, typenames );
	i += 1;

	block.push_back( new AST( AT_FUNCTIONDEF, functionName, { returnType, parameters, pseudoBlock( k, i, n, typenames, true ) } ) );
}

void Tokens::variableTranslate( vector<AST*>& block, int i, int n, const set<string>& typenames ) const {
	AST *variableType;
	string variableName;

	i = resolveTypename( variableType, i, n, typenames );

	if( !isVariable( i, typenames ) ) 
		throw compile_exception( "Expected variable name in variable definition", i );
	variableName = at(i);

	block.push_back( new AST( AT_VARIABLEDEF, variableName, { variableType } ) );

	if( at(i+1).str() == "=" )
		block.push_back( statementTranslate( i, n ) );
}

AST* Tokens::pseudoBlock( int& k, int i, int n, const set<string>& typenames, bool forceBlock ) const {
	AST* block;
	if( at(i).type == LEFT_BRACKET && at(i).str() != "(" ) {
		k = bracketIterator( i, n );
		block = encapsulateBlock( blockTranslate( i+1, k, typenames ), at(i).str() );
	} else {
		k = commaIterator( i, n );
		if( isKeyword( i ) ) {
			vector<AST*> temp;
			keywordTranslate( temp, i, n, typenames );
			block = temp.back();
		} else if( !isTypename( i, typenames ) )
			block = statementTranslate( i, n );
		else
			throw compile_exception( "Illegal action in bracketless block", i );
		
		if( forceBlock )
			block = encapsulateBlock( { block }, SYNC_BRACKET );
	}
	return block;
}

void Tokens::keywordTranslate( vector<AST*>& block, int i, int n, const set<string>& typenames ) const {
	if( at(i).str() == "else" )
		throw compile_exception( "Unexpected else-statement", i );
	else if( at(i).str() == "return" ) {
		block.push_back( new AST( AT_FLOW, "return", { statementTranslate( i+1, n ) } ) );
	} else {
		string keyword = at(i);
		AST *condition, *container, *effect;
		int k, l = -1;

		if( at(i+1).str() != "(" )
			throw compile_exception( "Expected (-bracket", i+1 );
		int j = bracketIterator( i+1, n );
		i += 2;

		if( keyword == "for" ) {
			if( !isVariable( i, typenames ) )
				throw compile_exception( "Expected variable", i );
			container = statementTranslate( i, i+1 );
			if( at(i+1).str() != "in" )
				throw compile_exception( "Expected in-keyword in for-statement", i );
			i += 2;
		}
		condition = statementTranslate( i, j );

		if( keyword == "if" )
			l = elseIterator( j+1, n );
		if( l == -1 )
			effect = pseudoBlock( k, j+1, n, typenames, true );
		else
			effect = pseudoBlock( k, j+1, l, typenames, true );

		if( keyword == "for" )
			block.push_back( new AST( keyword == "if" ? AT_CONDITIONAL : AT_LOOP, keyword, { container, condition, effect } ) );
		else
			block.push_back( new AST( keyword == "if" ? AT_CONDITIONAL : AT_LOOP, keyword, { condition, effect } ) );
		if( keyword == "if" && l != -1 )
			block.back()->children.push_back( pseudoBlock( i, l+1, n, typenames, true ) );
	}
}

void Tokens::segmentTranslate( vector<AST*>& block, int i, int n, const set<string>& typenames ) const {
	if( i+1 >= n ) 
		block.push_back( new AST( AT_SYNCBLOCK ) );
	else if( isTypename( i, typenames ) )
		variableTranslate( block, i, n, typenames );
	else if( isBlock( i ) )
		block.push_back( encapsulateBlock( blockTranslate( i+1, n-1, typenames ), at(i) ) );
	else if( isKeyword( i ) )
		keywordTranslate( block, i, n, typenames );
	else
		block.push_back( statementTranslate( i, n ) );
}

vector<AST*> Tokens::blockTranslate( int i, int n, const set<string>& old_typenames ) const {
	vector<int> points;
	vector<int> spaces;
	vector<int> functions;
	vector<int> other;
	vector<AST*> block;
	set<string> typenames = old_typenames;

	points.push_back( i );
	while( i < n ) {
		if( at(i).str() == "function" )
			functions.push_back( points.size() );
		else if( at(i).str() == "space" )
			spaces.push_back( points.size() );
		else
			other.push_back( points.size() );
		i = commaIterator( i, n ) + 1;
		points.push_back( i );
		if( i >= n || at(i).type == RIGHT_BRACKET )
			break;
	}

	for( int i: spaces )
		spaceTranslateFirst( typenames, points.at(i-1), points.at(i)-1 );
	for( int i: spaces )
		spaceTranslateSecond( block, points.at(i-1), points.at(i)-1, typenames );
	for( int i: functions )
		functionTranslate( block, points.at(i-1), points.at(i)-1, typenames );
	for( int i: other )
		segmentTranslate( block, points.at(i-1), points.at(i)-1, typenames );

	return block;
}


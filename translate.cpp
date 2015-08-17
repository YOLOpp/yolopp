#include "ast.h"
#include <sstream>
#include <cassert>
#include <stack>
#include <set>
#include <iostream>
#include <fstream>

bool isBlock(ast_type_t type){
	return type==AT_SYNCBLOCK||type==AT_ASYNCBLOCK||type==AT_FUNCTIONDEF;
}

set<string> functionNames = { "f_0_print", "f_0_input", "f_0_to_string" };

string AST::translate(void){
	stringstream ss;
	TranslatePath translatePath;
	// more includes?
	ss << "#include <iostream>\n#include <string>\n#include <gmp.h>\n#include <gmpxx.h>\n";
	// global stuff
	ss << "typedef mpz_class t_int;\n";
	ss << "t_int v_argc;\nchar** v_argv;\n";
	ss << "void f_0_print(std::string s){std::cout<<s;}\nstd::string f_0_input(void){std::string s;std::getline(std::cin,s);return s;}\n";
	ss << "std::string f_0_to_string(t_int x){return x.get_str();}\n";

	// functions
	pullFunctions( ss, translatePath );
	// main
	ss << "int main( int argc, char **argv ) {\n";
	// do global stuff
	ss << "\tv_argc = argc;\n\tv_argv = argv;\n\t";
	// translate main
	translateBlock( ss, translatePath, 1 );
	ss<<"\n}\n";
	return ss.str();
}

string AST::decodeTypename() {
	assert( type == AT_DATATYPE );
	return "t_int"; // temp
}

void AST::printFunctionHeader( stringstream& ss, string x ) {
	assert( type == AT_FUNCTIONDEF );
	ss << children.at(0)->decodeTypename();
	ss << " f_" << x << "_" << val << "( ";
	bool comma = false;
	for( AST* argument : children.at(1)->children ) {
		if( comma )
			ss << ", ";
		comma = true;
		ss << " " << argument->children.at(0)->decodeTypename();
		ss << "v_" << argument->val;
	}
	ss << " )";
}

void AST::pullFunctions( stringstream& ss, TranslatePath& translatePath ) {
	assert( type == AT_SYNCBLOCK || type == AT_ASYNCBLOCK );
	translatePath.push( this );
	for( AST* node : children ) {
		if( node->type == AT_FUNCTIONDEF ) {
			functionNames.insert( "f_" + val + "_" + node->val );
			node->printFunctionHeader( ss, val );
			ss << ";" << endl;
			node->children.at(2)->pullFunctions( ss, translatePath );
		} else if( node->type == AT_SYNCBLOCK || node->type == AT_ASYNCBLOCK )
			node->pullFunctions( ss, translatePath );
	}
	for( AST* node : children ) {
		if( node->type == AT_FUNCTIONDEF ) {
			node->printFunctionHeader( ss, val );
			node->children.at(2)->translateBlock( ss, translatePath, 1 );
		}
	}
	translatePath.pop();
}

void AST::translateBlock( stringstream& ss, TranslatePath& translatePath, int indent ){
	assert( type == AT_SYNCBLOCK || type == AT_ASYNCBLOCK );
	ss << " {\n"; // add stuff for threading here
	translatePath.push( this );
	for( AST *node : children ) {
		if( node->type != AT_FUNCTIONDEF )
			node->translateItem( ss, translatePath, indent + 1 );
	}
	for( int i = 0; i < indent; i++ )
		ss << "\t";
	ss << "}";
	translatePath.pop();
}

string AST::findFunctionName( string name, TranslatePath translatePath ) { // empty string means not found
	string temp;
	while( !translatePath.empty() ) {
		temp = "f_" + translatePath.top()->val + "_" + name;
		if( functionNames.count( temp ) )
			return temp;
		translatePath.pop();
	}
	return "";
}

void AST::translateItem( stringstream& ss, TranslatePath& translatePath, int indent, bool typeless ){
	if( typeless )
		for( int i = 0; i < indent; i++ )
			ss << "\t";
	switch( type ) {
		case AT_FUNCTIONCALL: {
			string functionName;
			if( val.at(0) == '@' )
				functionName = val.substr( 1 );
			else
				functionName = findFunctionName( val, translatePath ); // this ruins function overloading
			if( functionName == "" )
				throw translate_exception( "Unknown function " + val );
			if( functionName == "operator=" ) {
				ss << "((";
				children.at(0)->translateItem( ss, translatePath, indent, false );
				ss << ")=(";
				children.at(1)->translateItem( ss, translatePath, indent, false );
				ss << "))";
			} else {
				ss << functionName << "( ";
				bool comma = false;
				for( AST* argument : children ) {
					if( comma )
						ss << ", ";
					comma = true;
					argument->translateItem( ss, translatePath, indent, false );
				}
				ss << " )";
			}
		}
		break;
		case AT_LOOP:
		    if( typeless ) {
				if( val == "while" ) {
					ss << "while( ";
					children.at(0)->translateItem( ss, translatePath, indent, false );
					ss << " )";
					children.at(1)->translateBlock( ss, translatePath, indent );
				} else {
					throw translate_exception( "Unknown loop " + val );
				}
			} else
				throw translate_exception( "Unexpected block" );
		break;
		case AT_VARIABLEDEF:
			ss << children.at(0)->decodeTypename() << " v_" << val;
		break;
		case AT_NUMBER: case AT_STRING:
			ss << val; //temp
		break;
		case AT_WORD:
			ss << "v_" << val;
		break;
		//case AT_CONDITIONAL:
		//case AT_LOOP:
		//case AT_ARRAY:
		case AT_SYNCBLOCK: case AT_ASYNCBLOCK:
			if( typeless )
				translateBlock( ss, translatePath, indent );
			else
				throw translate_exception( "Unexpected block" );
		break;
		default:
			throw translate_exception( "Unexpected node " + to_string( type ) + " as translation item" );
	}
	if( typeless )
		ss << ";\n";
}

const char* translate_exception::what() const noexcept {
	string s = "Translation Error: " + err_str + "!";
	return s.c_str();
}

translate_exception::translate_exception( string err ) {
	err_str = move( err );
}

int main() {
	string s = "int i = 0, while( i < 10 ) [ print(to_string(i)), i = i + 1 ]";
	Tokens t(s);
	AST ast(t);
	std::cout << ast<< std::endl;
	std::ofstream output("output.cc");
	output << ast.translate() << endl;
}
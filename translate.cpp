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

set<string> functionNames = { "f_0_print", "f_0_input", "f_0_to_string", "f_0_to_float", "f_0_to_rat" };

string AST::translate(void){
	stringstream ss;
	TranslatePath translatePath;
	ss << "#include \"y_lib.h\"\n";
	// global stuff
	/*ss << "typedef mpz_class t_int;\n";
	ss << "t_int v_argc;\nchar** v_argv;\n";
	ss << "void f_0_print(std::string s){std::cout<<s;}\nstd::string f_0_input(void){std::string s;std::getline(std::cin,s);return s;}\n";
	ss << "std::string f_0_to_string(t_int x){return x.get_str();}\n";*/

	// functions
	pullFunctions( ss, translatePath );
	// main
	ss << "int main( int argc, char **argv ) {\n";
	// do global stuff
	ss << "\tv_argc = argc;\n\tfor(int i = 0; i < argc; i++ ) v_argv.emplace_back( argv[i] );\n\trandom_generator.seed(std::chrono::high_resolution_clock::now().time_since_epoch().count());\n\t";
	// translate main
	translateBlock( ss, translatePath, 1 );
	ss<<"\n}\n";
	return ss.str();
}

string AST::decodeTypename() {
	string r;
	bool comma = false;
	if( type == AT_ARRAY ) {
		r = "std::tuple<";
		for( AST* child : children ) {
			if( comma )
				r += ",";
			comma = true;
			r += child->decodeTypename();
		}
		r += ">";
	} else if( type == AT_DATATYPE ) {
		if( val == "set" || val == "list" ) {
			r = "t_" + val + "<";
			for( AST* child : children ) {
				if( comma )
					r += ",";
				comma = true;
				r += child->decodeTypename();
			}
			r += ">";
		} else 
			r = "t_" + val;		
	} else 
		translate_exception( "Node " + to_string(type) + " is not a typename" );
	return r;
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
		ss << argument->children.at(0)->decodeTypename();
		ss << " v_" << argument->val;
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
			node->children.at(2)->translateBlock( ss, translatePath, 0 );
			ss << endl;
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
			} else if( functionName == "operator@" ) {
				ss << "("; 
				children.at(0)->translateItem( ss, translatePath, indent, false );
				ss << ").at((";
				children.at(1)->translateItem( ss, translatePath, indent, false );
				ss << ").get_ui())";
			} else if( functionName == "static_cast" ) {
				ss << "cast<";
				children.at(1)->translateItem( ss, translatePath, indent, false );
				ss << ">( ";
				children.at(0)->translateItem( ss, translatePath, indent, false );
				ss << " )";
			} else {
				if( functionName == "operator-u" )
					functionName = "operator-";
				else if( functionName == "operator<->" )
					functionName = "std::swap";
				else if( functionName == "operator?" )
					functionName = "shuffle";
				else if( functionName == "operator." )
					functionName = "contains";
				else if( functionName == "operator#" )
					functionName = "size_of";
				else if( functionName == "operator:" ) {
					functionName = "t_range<";
					AST* type = children.at(0)->getType();
					functionName += type->decodeTypename() + ">";
					delete type->cascade();
				} 
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
		case AT_CONDITIONAL:
			if( typeless ) {
				ss << "if( ";
				children.at(0)->translateItem( ss, translatePath, indent, false );
				ss << " )";
				children.at(1)->translateBlock( ss, translatePath, indent );
				if( children.size() > 2 ) { // exists an else condition
					ss << " else";
					children.at(2)->translateBlock( ss, translatePath, indent );
				}
			} else
				throw translate_exception( "Unexpected block" );
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
		case AT_NUMBER: 
			if( val.find('.') == string::npos )
				ss << "t_int";
			else
				ss << "t_float";
		case AT_STRING:
			ss << "(" << val << ")"; //temp
		break;
		case AT_WORD:
			ss << "v_" << val;
		break;
		case AT_INLINE_LIST: case AT_INLINE_SET: {
			AST* r = getType();
			ss << "std::move(" << r->decodeTypename() << "({ ";
			bool comma = false;
			for( AST* child: children ) {
				if( comma )
					ss << ", ";
				comma = true;
				child->translateItem( ss, translatePath, indent, false );
			}
			ss << " }))";
			delete r->cascade();
		}
		break;
		case AT_SYNCBLOCK: case AT_ASYNCBLOCK:
			if( typeless )
				translateBlock( ss, translatePath, indent );
			else
				throw translate_exception( "Unexpected block" );
		break;
		case AT_FLOW:
			if( typeless ) {
				if( val == "return" ) {
					ss << "return ";
					children.at(0)->translateItem( ss, translatePath, indent, false ); 
				}
			} else
				throw translate_exception( "Unexpected flow statement" );
		break;
		case AT_DATATYPE:
			ss << decodeTypename();
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

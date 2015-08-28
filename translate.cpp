#include "ast.h"
#include <sstream>
#include <cassert>
#include <stack>
#include <set>
#include <iostream>
#include <fstream>
#include <map>

set<string> builtinFunctionNames = { "print", "input", "push", "pop", "insert", "remove" };
//map<string,vector<pair<AST*,string>>> spaceNames;

string AST::translate() const {
	stringstream ss;
	TranslatePath translatePath;
	ss << "#include \"./../y_lib.h\"\n";  // WILL BE REPLACED WITH <> ONCE STUCTURE IS ADDED TO THE PATH

	// spaces
	pullSpaces( ss, translatePath );
	// functions
	pullFunctions( ss, translatePath );
	// main
	ss << "int main( int argc, char **argv ) {\n";
	// do global stuff
	ss << "\tv_argc = argc;\n\tfor(int i = 0; i < argc; i++ ) v_argv.push_back(std::move(t_string(argv[i])));\n\trandom_generator.seed(std::chrono::high_resolution_clock::now().time_since_epoch().count());\n\t";
	// translate main
	translateBlock( ss, translatePath, 1 );
	ss<<"\n}\n";
	return ss.str();
}

string AST::decodeTypename( const TranslatePath& translatePath ) const {
	string r;
	bool comma = false;
	if( type == AT_ARRAY ) {
		r = "t_tuple<";
		for( AST* child : children ) {
			if( comma )
				r += ",";
			comma = true;
			r += child->decodeTypename( translatePath );
		}
		r += ">";
	} else if( type == AT_DATATYPE ) {
		if( val == "set" || val == "list" ) {
			r = "t_" + val + "<";
			for( AST* child : children ) {
				if( comma )
					r += ",";
				comma = true;
				r += child->decodeTypename( translatePath );
			}
			r += ">";
		} else 
			r = findSpaceName( val, translatePath );
	} else 
		translate_exception( "Node " + to_string(type) + " is not a typename" );
	return r;
}

void AST::printFunctionHeader( stringstream& ss, string x, const TranslatePath& translatePath ) const {
	assert( type == AT_FUNCTIONDEF );
	ss << children.at(0)->decodeTypename( translatePath );
	ss << " f_" << x << "_" << val << "( ";
	bool comma = false;
	for( AST* argument : children.at(1)->children ) {
		if( comma )
			ss << ", ";
		comma = true;
		ss << argument->children.at(0)->decodeTypename( translatePath );
		ss << " v_" << argument->val;
	}
	ss << " )";
}

void AST::pullFunctions( stringstream& ss, TranslatePath& translatePath ) const {
	assert( type == AT_SYNCBLOCK || type == AT_ASYNCBLOCK );
	translatePath.push( this );
	for( AST* node : children ) {
		if( node->type == AT_FUNCTIONDEF ) {
			node->printFunctionHeader( ss, val, translatePath );
			ss << ";" << endl;
			node->children.at(2)->pullFunctions( ss, translatePath );
		} else if( node->type == AT_SYNCBLOCK || node->type == AT_ASYNCBLOCK )
			node->pullFunctions( ss, translatePath );
	}
	for( AST* node : children ) {
		if( node->type == AT_FUNCTIONDEF ) {
			node->printFunctionHeader( ss, val, translatePath );
			node->children.at(2)->translateBlock( ss, translatePath, 0 );
			ss << endl;
		}
	}
	translatePath.pop();
}

void AST::pullSpaces( stringstream& ss, TranslatePath& translatePath ) const {
	assert( type == AT_SYNCBLOCK || type == AT_ASYNCBLOCK );
	translatePath.push( this );
	for( AST* node : children ) {
		if( node->type == AT_SPACEDEF ) {
			ss << "struct s_" << val << "_" << node->val << ";\n"; 
		} else if( node->type == AT_SYNCBLOCK || node->type == AT_ASYNCBLOCK )
			node->pullFunctions( ss, translatePath );
	}
	for( AST* node : children ) {
		if( node->type == AT_SPACEDEF ) {
			vector<pair<AST*,string>> memberNames;
			assert( node->children.at(0)->type == AT_ARRAY );
			ss << "struct s_" << val << "_" << node->val << "{ ";
			for( AST* member : node->children.at(0)->children ) {
				assert( member->type == AT_VARIABLEDEF );
				ss << member->children.at(0)->decodeTypename( translatePath ) << " " << member->val << "; ";
				memberNames.push_back( make_pair( new AST( *member->children.at(0) ), member->val ) );
			}
			ss << "};\n";
		}
	}
	translatePath.pop();
}

void AST::translateBlock( stringstream& ss, TranslatePath& translatePath, int indent ) const {
	assert( type == AT_SYNCBLOCK || type == AT_ASYNCBLOCK );
	ss << " {\n"; // add stuff for threading here
	translatePath.push( this );
	for( AST *node : children ) {
		if( node->type != AT_FUNCTIONDEF && node->type != AT_SPACEDEF )
			node->translateItem( ss, translatePath, indent + 1 );
	}
	for( int i = 0; i < indent; i++ )
		ss << "\t";
	ss << "}";
	translatePath.pop();
}

string AST::findFunctionName( string name, TranslatePath translatePath ) {
	if( builtinFunctionNames.count( name ) )
		return "f_0_" + name;
	while( !translatePath.empty() ) {
		for( const AST* item: translatePath.top()->children ) {
			if( item->type == AT_FUNCTIONDEF && item->val == name )
				return "f_" + translatePath.top()->val + "_" + name;
		}
		translatePath.pop();
	}
	return "";
} // empty string means not found

string AST::findSpaceName( string name, TranslatePath translatePath ) {
	if( finalTypenames.count( name ) || nonFinalTypenames.count( name ) )
		return "t_" + name;
	while( !translatePath.empty() ) {
		for( const AST* item: translatePath.top()->children ) {
			if( item->type == AT_SPACEDEF && item->val == name )
				return "s_" + translatePath.top()->val + "_" + name;
		}
		translatePath.pop();
	}
	return "";
} // empty string means not found

AST* AST::findFunctionType( string name, TranslatePath translatePath ) { 
	while( !translatePath.empty() ) {
		for( const AST* item: translatePath.top()->children ) {
			if( item->type == AT_FUNCTIONDEF && item->val == name )
				return new AST( *item->children.at(0) );
		}
		translatePath.pop();
	}
	return nullptr;
} // nullptr means not found

AST* AST::findVariableType( string name, TranslatePath translatePath ) { // does not handle function arguments yet
	while( !translatePath.empty() ) {
		for( const AST* item: translatePath.top()->children ) {
			if( item->type == AT_VARIABLEDEF && item->val == name )
				return new AST( *item->children.at(0) );
		}
		translatePath.pop();
	}
	return nullptr;
} // nullptr means not found

void AST::translateItem( stringstream& ss, TranslatePath& translatePath, int indent, bool typeless ) const {
	if( typeless )
		for( int i = 0; i < indent; i++ )
			ss << "\t";
	bool comma = false;
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
			} else if( functionName == "operator@" && children.at(1)->type == AT_NUMBER ) { // TODO: allow expressions like 3*5+1
				ss << "special_at<" << children.at(1)->val << ">( "; 
				children.at(0)->translateItem( ss, translatePath, indent, false );
				ss << ")";
			} else if( functionName == "static_cast" ) {
				ss << "cast<";
				children.at(1)->translateItem( ss, translatePath, indent, false );
				ss << ">( ";
				children.at(0)->translateItem( ss, translatePath, indent, false );
				ss << " )";
			} else if( functionName == "operator." ) {
				if( children.at(1)->type == AT_FUNCTIONCALL ) {
					ss << findFunctionName( children.at(1)->val, translatePath ) << "( ";
					children.at(0)->translateItem( ss, translatePath, indent, false );
					for( AST* child: children.at(1)->children ) {
						ss << ", ";
						child->translateItem( ss, translatePath, indent, false );
					}
					ss << " )";
				} else
					throw translate_exception( "Expected function after '.'" );
			} else {
				if( functionName == "operator-u" )
					functionName = "operator-";
				else if( functionName == "operator<->" )
					functionName = "std::swap";
				else if( functionName == "operator?" )
					functionName = "shuffle";
				else if( functionName == "operator\\" )
					functionName = "contains";
				else if( functionName == "operator#" )
					functionName = "size_of";
				else if( functionName == "operator:" ) {
					functionName = "t_range<";
					AST* type = children.at(0)->getType( translatePath );
					functionName += type->decodeTypename( translatePath ) + ">";
					delete type->cascade();
				} else if( functionName == "operator@" )
					functionName = "at";
				ss << functionName << "( ";
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
				} else if( val == "for" ) {
					ss << "for( auto& ";
					children.at(0)->translateItem( ss, translatePath, indent, false );
					ss << ": ";
					children.at(1)->translateItem( ss, translatePath, indent, false );
					ss << " )";
					children.at(2)->translateBlock( ss, translatePath, indent );
				} else {
					throw translate_exception( "Unknown loop " + val );
				}
			} else
				throw translate_exception( "Unexpected block" );
		break;
		case AT_VARIABLEDEF:
			ss << children.at(0)->decodeTypename( translatePath ) << " v_" << val;
		break;
		case AT_NUMBER: 
			if( val.find('.') == string::npos )
				ss << "t_int";
			else
				ss << "t_float";
			ss << "(" << val << ")"; //temp
		break;
		case AT_STRING:
			ss << "t_string(" << val << ")"; //temp
		break;
		case AT_WORD:
			ss << "v_" << val;
		break;
		case AT_INLINE_LIST: case AT_INLINE_SET: {
			AST* r = getType( translatePath );
			ss << "std::move(" << r->decodeTypename( translatePath ) << "({ ";
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
		case AT_ARRAY:
			ss << "std::forward_as_tuple( ";
			for( AST* child: children ) {
				if( comma )
					ss << ", ";
				comma = true;
				child->translateItem( ss, translatePath, indent, false );
			}
			ss << " )";
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
			ss << decodeTypename( translatePath );
		break;
		default:
			throw translate_exception( "Unexpected node " + to_string( type ) + " as translation item" );
	}
	if( typeless )
		ss << ";\n";
}

AST* AST::getType( const TranslatePath& translatePath ) const {
	switch( type ) {
		case AT_WORD: 
			return findVariableType( val, translatePath );
			break;
		case AT_FUNCTIONCALL:
			return findFunctionType( val, translatePath );
			break;
		case AT_NUMBER: 
			return new AST( AT_DATATYPE, val.find('.') == string::npos ? "int": "float" , {} );
			break;
		case AT_STRING:
			return new AST( AT_DATATYPE, "string", {} );
			break;
		case AT_ARRAY: {
			std::vector<AST*> temp;
			for( auto x : children ) 
				temp.push_back( x->getType( translatePath ) );
			return new AST( AT_ARRAY, "tuple", std::move( temp ) );
		} break;
		case AT_INLINE_SET: case AT_INLINE_LIST: 
			if( children.size() == 0 )
				throw compile_exception( "Type Error: Could not deduce type of empty set", -1 );
			else {
				AST *r = children.at(0)->getType( translatePath ), *c;
				bool b;
				for( size_t i = 1; i < children.size(); i++ ) {
					c = children.at(1)->getType( translatePath );
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

const char* translate_exception::what() const noexcept {
	string s = "Translation Error: " + err_str + "!";
	return s.c_str();
}

translate_exception::translate_exception( string err ) {
	err_str = move( err );
}

#include "ast.h"
#include <sstream>
#include <cassert>

bool isBlock(ast_type_t type){
	return type==AT_SYNCBLOCK||type==AT_ASYNCBLOCK||type==AT_FUNCTIONDEF;
}

string translateblock(AST&);
string translateitem(AST&);

string AST::translate(void){
	vector<AST*> functions;
	for(AST *node : children){
		if(node->type==AT_FUNCTIONDEF)functions.push_back(node);
	}
	stringstream ss;
	ss<<"#include <iostream>\n\n";
	for(AST *node : functions){
		ss<<translateblock(*node);
	}
	ss<<"int main(int argc,char **argv){\n";
	for(AST *node : children){
		if(node->type!=AT_FUNCTIONDEF)ss<<translateblock(*node);
	}
	ss<<"}\n";
	return ss.str();
}

string translateblock(AST &root){
	stringstream ss;
	for(AST *node : children){
		ss<<translateitem(*node);
	}
	return ss.str();
}

string translateitem(AST &root){
	switch(node->type){
		case AT_FUNCTIONDEF:continue;
		case AT_FUNCTIONCALL:
		case AT_CONDITIONAL:
		case AT_LOOP:
		case AT_EXPR:
		case AT_ARRAY:
		case AT_SYNCBLOCK:
		case AT_ASYNCBLOCK:
		default:assert(false);
	}
}

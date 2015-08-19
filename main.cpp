#include <iostream>
#include <fstream>
#include <set>
#include <vector>
#include <string>
#include <unistd.h>
#include <sys/wait.h>

#include "ast.h"

using namespace std;


string tempnamefor(string nm){
	return nm+"._temp.cpp";
}

int main(int argc,char **argv){
	ios_base::sync_with_stdio(false);
	int i,j;
	set<char> options;
	vector<string> args;
	for(i=1;i<argc;i++){
		if(argv[i][0]=='-'){
			for(j=1;argv[i][j];j++)options.insert(argv[i][j]);
		} else {
			args.emplace_back(argv[i]);
		}
	}
	for(char it : options){
		switch(it){
			//case 'a':break;
			default:
				cerr<<"Option '"<<it<<"' not supported or unknown."<<endl;
				return 1;
		}
	}
	if(args.size()!=2){
		cerr<<"Requires an input and output file."<<endl;
		return 1;
	}
	string srcfname=args[0];
	string outfname=args[1];
	ifstream srcf(srcfname,ios::binary);
	if(!srcf){
		cerr<<"Could not open file '"<<srcfname<<"'"<<endl;
		return 1;
	}
	string tempfname=tempnamefor(outfname);
	ofstream outf(tempfname);
	if(!srcf){
		cerr<<"Could not open temporary file for '"<<outfname<<"'"<<endl;
		return 1;
	}
	srcf.seekg(0,ios::end);
	int len=srcf.tellg();
	srcf.seekg(0);
	string source;
	source.resize(len);
	srcf.read(&*source.begin(),len);
	srcf.close();

	string trans=AST(Tokens(source)).translate();
	outf.write(trans.data(),trans.size());

	outf.flush();

	pid_t pid=fork();
	if(pid==0){
		execlp("g++","-Wall","-Wextra","-pedantic","-O3","-std=c++11","-lgmp","-lgmpxx",tempfname.c_str(),"-o",outfname.c_str());
	}
	while(true){
		int status;
		pid_t w=waitpid(pid,&status,0);
		if(w==-1){
			perror("waitpid");
			return 2;
		}
		if(WIFEXITED(status))break;
	}

	outf.close(); //only now release the file to any other writers
	return 0;
}

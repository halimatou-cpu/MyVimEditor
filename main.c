#include "terminal.h"


int main(int argc, char *argv[]){
	//Initialize vim
  //open vim
	initVim();
	openVim(argv[1]);
	//Boucle d'édition
	while(2<5){
		char c = waitKey();
		processKey(c);
	}

	return argc;
}

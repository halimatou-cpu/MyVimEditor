#include "terminal.h"
struct row **crow;

// Activation du mode canonique (terminal par defaut)

void mode_canonique(){
  if( tcsetattr(STDIN_FILENO, TCSANOW,  &T.stdin) != 0) exit(0);
}

//Désactivation du mode canonique
void mode_non_canonique(){

	if( tcgetattr(STDIN_FILENO, &T.stdin) ) exit(0);

  if(T.stdin.c_lflag & ICANON){

		struct termios new = T.stdin;
		cfmakeraw(&new);
		new.c_cc[VMIN] = 1;
		new.c_cc[VTIME] = 0;
		if(tcsetattr(STDIN_FILENO, TCSANOW, &new) != 0) exit(0);
	}
}
// parser les arguments

int parse_line(char *s, char **argv[]){

	if(strlen(s) == 0) return 0;

	unsigned int i = 0; //index de la chaine s
	int end = 0;
	int j = 0;

	char prec = s[0];

	while((s[i] != '\0')){

		if(i > 0){
			if((prec == ' ') && (s[i] != prec))	end = i;

		}

		if( ( (s[i] == ' ') && (prec != s[i]) ) || ( (i == strlen(s)-1) && (s[i] != ' ') ) ){
			char *temp = malloc( (i-end+1)*sizeof(char) );
			int n = end;
			for(unsigned int k = 0; k < i-end+1; k++){
				if((k == i-end) && s[n] == ' '){
					n++;
					continue;
				}
				temp[k] = s[n];
				n++;
			}
			temp[i-end+1] = '\0';

			argv[j] = (char **) malloc( strlen(temp) +1);
			*(argv[j]) = temp;

			j++;
		}

		prec = s[i];//stocker le caractere precedent
		i++; //index du caracter suivant
	}

	argv[j] = NULL;

	return j;
}

//Changement de Mode du terminal

void changeMode(){
	T.mode = !(T.mode);
}

char waitKey(){
   char c;
   while(read(STDIN_FILENO, &c, 1) != 1);
   return c;
}

// Ferme et quitte

void out(){
	//free all mallocs
	for(int i = T.maxRow-1; i >= 0; i--)
		free(crow[i]);
	//
	write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
	//close all files descriptors
	fclose(T.file);
	close(T.fd_buffer);
	//
	mode_canonique();
    exit(0);
}

//Traitement du mode normal

void defaultMode(){
	changeMode();
	mode_canonique();

	char normal[T.numCol];
	// Affichage facultatif
	dprintf(STDOUT_FILENO,"\x1b[%d;1H",T.numRow);
	write(STDOUT_FILENO, " ", 1);
	dprintf(STDOUT_FILENO,"\x1b[%d;1H",T.numRow);

	while( fgets(normal, T.numCol, stdin) && (normal[0] != 'i')){
		dprintf(STDOUT_FILENO,"\x1b[%d;1H",T.numRow-1);
		write(STDOUT_FILENO,"\x1b[K",3);
		dprintf(STDOUT_FILENO,"\x1b[%d;1H",T.numRow);
		write(STDOUT_FILENO,"\x1b[K",3);

		normal[strlen(normal)-1] = '\0';

		char ***tab = malloc(sizeof(char ***));
		int size = parse_line(normal, tab);

		int k = 0;
		while(tab[k]!= NULL){
			printf(":%s\n", *(tab[k]));
			k++;
		}

		sleep(1);
		if(size == 1){
			if(strcmp(*(tab[0]),":q") == 0) out();
			else if(strcmp(*(tab[0]),":w") == 0){
				if(T.filename != NULL){
					int fd = open(T.filename, O_WRONLY | O_CREAT | O_NONBLOCK);
					char buff[BUFSIZ];
					lseek(T.fd_buffer, 0, SEEK_SET);
					int rd = read(T.fd_buffer, &buff, BUFSIZ);
					write(fd, &buff, rd);
					close(fd);
					T.notif = "File saved ";
				}else{
					T.notif = "Filename required";
				}
			}else{
				write(STDOUT_FILENO, tab[0], strlen(*(tab[0])));
				T.notif = "Wrong command, restart please!";
			}
		}else if(size == 2){
			if(strcmp(*(tab[0]),":w") == 0){
				if(T.filename == NULL){
					T.filename = malloc(strlen(*(tab[1])));
				}
				strcpy(T.filename,*(tab[1]));
				int fd = open(T.filename, O_WRONLY | O_CREAT | O_TRUNC | O_NONBLOCK);
				if(fd == -1){
					T.notif = "Access denied! ";
					strcat(T.notif, T.filename);
				}else{
					char buff[BUFSIZ];
					lseek(T.fd_buffer, 0, SEEK_SET);
					int rd = read(T.fd_buffer, &buff, BUFSIZ);
					write(fd, &buff, rd);
					close(fd);
					T.notif = "File saved! ";
				}
			}
			dprintf(STDOUT_FILENO,"\x1b[%d;1H",T.numRow);
			write(STDOUT_FILENO,"\x1b[K",3);
		}else
			T.notif = "Wrong command, restart please! ";

		if(strlen(T.notif) != 0){
			write(STDOUT_FILENO, T.notif, strlen(T.notif));
			sleep(1);
		}

		dprintf(STDOUT_FILENO,"\x1b[%d;1H",T.numRow);
		write(STDOUT_FILENO,"\x1b[K",3);
		dprintf(STDOUT_FILENO,"\x1b[%d;1H",T.numRow);

	}
	mode_non_canonique();
	changeMode();
}
//Gestion de la taille du terminal
void getTerminalSize(int *r, int *c){

	struct winsize w;
  //winsize dans la librairie <sys/ioctl.h>
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
// ioctl input and output control
    *r = w.ws_row;
    *c =  w.ws_col;
}

void initVim(){
	getTerminalSize( (&T.numRow), &(T.numCol) );
  T.fd_buffer = openFile("buffer.txt", true, 2);
	char s[50];
	sprintf(s, "numRow : %d numCol %d", T.numRow, T.numCol);
	T.file = fdopen(T.fd_buffer,"r");
	T.offset = 0;
	T.stop = 0;
	T.eof = false;
	T.mode = true;
	T.vu = false;
	T.filename = NULL;
	crow = (struct row **) malloc(sizeof(struct row **));
	T.maxRow = 0;
	T.termX = T.termY = 1;
	mode_non_canonique();
}


int openFile(char filename[], bool inCurrentDir, int right){
	int fd;
	if(inCurrentDir){
		char dir[256] = "";
		getcwd(dir, sizeof(dir));
		strcat(dir, "/");
		strcat(dir, filename);
		switch(right){
			case 0:
				fd = open(dir, O_RDONLY | O_CREAT | O_NONBLOCK, 0764);
				break;
			case 1:
				fd = open(dir, O_WRONLY | O_CREAT | O_APPEND | O_NONBLOCK, 0764);
				break;
			case 2:
				fd = open(dir, O_RDWR | O_CREAT | O_TRUNC | O_NONBLOCK, 0764);
				break;
		}
	}else{
		switch(right){
			case 0:
				fd = open(filename, O_RDONLY | O_CREAT | O_NONBLOCK, 0764);
				break;
			case 1:
				fd = open(filename, O_WRONLY | O_CREAT | O_APPEND | O_NONBLOCK, 0764);
				break;
			case 2:
				fd = open(filename, O_RDWR | O_CREAT | O_TRUNC | O_NONBLOCK, 0764);
				break;
		}
	}

	return fd;
}

void open_again_fd(){
	close(T.fd_buffer);
	T.fd_buffer = openFile("buffer.txt", true, 2);
}

int updateBuffer(){
    //Seek the cursor to the begining
    lseek(T.fd_buffer, 0, SEEK_SET);
    // read now
    char *buff[BUFSIZ];
    return read(T.fd_buffer, buff, BUFSIZ);
}

void addline(char str[]){
	int i = T.maxRow;
	void *p1 = (struct row*) malloc(sizeof(struct row *));
	void *p = malloc(strlen(str)+1);

	if(p == NULL){
		perror(" OUPS! P\n");
		exit(0);
	}

	if(p1 == NULL){
		perror(" OUPS! P1\n");
		exit(0);
	}

	crow[i] = p1;
	crow[i]->row = p;
	strcpy(crow[i]->row,str);

	crow[i]->size = strlen(str);

	T.maxRow++;
}


void rang(){
	fseek(T.file, T.offset, SEEK_SET);

	char str[T.numCol];
	char *len;
	while((len = fgets(str, T.numCol, T.file)) != NULL){
		str[strlen(len)] = '\0';
		addline(str);
	}
}

void showFile(int offset){
	write(STDOUT_FILENO, "\x1b[2J", 4); //clear tout l'écran
	write(STDOUT_FILENO, "\x1b[3J", 4); //reset scroll
	write(STDOUT_FILENO, "\x1b[H", 3); //set the cursor to home position
	if(T.maxChar > 0){

		if(offset < 0)
			offset = 0;

		for(int i = T.maxRow-1; i >= 0; i--)
			free(crow[i]);

		T.maxRow = 0;
		T.offset = offset;

		rang();

		int limit = offset+T.numRow-1;
		for(int i = offset; i < limit && i < T.maxRow; i++){

			char str[10];
			sprintf(str, "\x1b[%d;1H", (i-offset+1));
			write(STDOUT_FILENO, str, strlen(str));

			write(STDOUT_FILENO, crow[i]->row, strlen(crow[i]->row));
		}

		T.eof = false;
		if(limit >= T.maxRow)
		  T.eof = true;

		char str[10];
		sprintf(str, "\x1b[%d;%dH", T.termX, T.termY);
		write(STDOUT_FILENO, str, strlen(str));
	}
}

void openVim(char *filename){
	if(filename != NULL){
		int fd = openFile(filename, false, 0);
		char buff[BUFSIZ];
		int nr = read(fd, &buff, BUFSIZ);
		write(T.fd_buffer, &buff, nr);
		T.filename = malloc(sizeof(char)*T.numCol);
		strcpy(T.filename, filename);
		showFile(0);
		close(fd);
		read(T.fd_buffer, &T.buffer, BUFSIZ);

	}else{
		strcpy(T.buffer,"");
		//vider l'ecran
		write(STDOUT_FILENO, "\x1b[2J", 4);
		write(STDOUT_FILENO, "\x1b[3J", 4);
		write(STDOUT_FILENO, "\x1b[H", 3);
	}

	T.maxChar = updateBuffer();
	T.curX = T.curY = 0;
	T.termX = T.termY = 1;
	write(STDOUT_FILENO, "\x1b[1;1H", 6);
}


int getBufferPos(){
	int pos = 0;
	for(int i = 0; i < (T.offset + T.termX -1) && i < T.maxRow; i++){
		pos += crow[i]->size;
	}
	pos += (T.termY-1);

	return pos;
}
void updateCursorPos(int add){
//terminal cursor

	T.termY += add;

	if(T.termY < 1){
		if(T.termX <= 1){
			T.termY = 1;
		}else{
			T.termX--;
			T.termY = crow[T.offset + T.termX-1]->size;
		}
	}


	if(T.termY == T.numCol)
		T.vu = true;

	T.termX += (T.termY / T.numCol);
	T.termY %= (T.numCol);

	if(T.termX < 1)
		T.termX = 1;

	if(T.termY < 1)
		T.termY = 1;

	if( T.termX > (T.numRow-1) )
		T.termX = T.numRow-1;

	if( T.termX > (T.maxRow+1) && T.maxRow != 0)
		T.termX = T.maxRow+1;

}
void clearBackward(){
	T.stop = getBufferPos() -1;
	lseek(T.fd_buffer, 0, SEEK_SET);
	int n = 0;
	if(T.stop == T.maxChar-1){
		if(T.stop > -1){
			n = read(T.fd_buffer, &T.buffer, T.stop);
			open_again_fd();
			write(T.fd_buffer, &T.buffer, n);
		}
	}else if(T.stop > 0){
		n = read(T.fd_buffer, &T.buffer, T.stop);
		char c;
		read(T.fd_buffer, &c, 1);
		n += read(T.fd_buffer, &T.buffer1, BUFSIZ);
		open_again_fd();
		write(T.fd_buffer, &T.buffer, T.stop);
		write(T.fd_buffer, &T.buffer1, n-T.stop);
	}else{
		n = read(T.fd_buffer, &T.buffer, BUFSIZ);
		open_again_fd();
		write(T.fd_buffer, &T.buffer, BUFSIZ);
	}
	updateCursorPos(-1);
}

bool isInsert(){
	return T.mode;
}


void processKey(char c){
	T.stop = getBufferPos();

	if( c == 27 && isInsert() ){
    //if insert and echap
		char nc;
		if((read(STDIN_FILENO, &nc, 1) == 1) && (nc == '[')){
			read(STDIN_FILENO, &nc, 1);
			moveCursor((int) nc);
			ecran(T.offset);
		}else{
			defaultMode();
			updateTerminal();
		}
	}else{
		if(c == 127){
			clearBackward();
			T.maxChar--;
			if(T.maxChar < 0)
				T.maxChar = 0;
		}else{


				if((T.stop+1) < T.maxChar){//to insert some char
					lseek(T.fd_buffer, 0, SEEK_SET);
					read(T.fd_buffer, &T.buffer, T.stop);
					read(T.fd_buffer, &T.buffer1, BUFSIZ);
					open_again_fd();
					write(T.fd_buffer, &T.buffer, T.stop);
				}

				if(c == 10 || c == 13){//newLine key
					char str0[50] = "";
					sprintf(str0,"writing : [%d;%dH",T.termX,T.termY);
					T.termX += 1;
					T.termY = 1;
					char str[70] = "";
					sprintf(str,"Before updateTerminal [%d;%dH",T.termX,T.termY);
					updateCursorPos(0);
					char str1[70] = "";
					sprintf(str1,"After updateTerminal [%d;%dH",T.termX,T.termY);

					write(T.fd_buffer,"\r\n", 2);
				}else{
					write(T.fd_buffer, &c, 1);
					updateCursorPos(1);
				}

				if(T.stop < T.maxChar){
          //same for insert char
					write(T.fd_buffer, &T.buffer1, T.maxChar-T.stop);
				}
				T.maxChar++;
			}

		updateTerminal();
	}
	fseek(stdin,0,SEEK_END);
	char str[5] = "";
	sprintf(str,"%d",T.maxChar);
}


void moveCursor(int ascii){
	int val;
	switch(ascii){
		case UP:
			T.termX--;
			if((T.termX >= 1) && (T.termY > crow[T.offset + T.termX-1]->size))
				T.termY = crow[T.offset + T.termX-1]->size;

			M.direction = UP;
			M.x = T.termX;
			break;
		case DOWN:
			T.termX++;
			val = T.offset + T.termX -1;

			char str1[70] = "";
			sprintf(str1,"val : %d , max : %d\n",val, T.maxRow);

			if( val > -1 && val <  T.maxRow){
				if((T.termX >= 1) && (T.termY > crow[val]->size))
					T.termY = crow[val]->size;
			}
			M.direction = DOWN;
			M.x = T.termX;
			break;
		case RIGHT:
			val = T.offset + T.termX -1;
			if( val > -1 && val <  T.maxRow){
				if(T.termY <= crow[val]->size){
					updateCursorPos(1);
				}
			}
			break;
		case LEFT:
			if(T.termY > 1)
				updateCursorPos(-1);
			break;
	}
	updateCursorPos(0); //mettre à jour ymax
  setCursorFdBuffer();
}

void setCursorFdBuffer(){
	T.stop = getBufferPos();
	lseek(T.fd_buffer, T.stop, SEEK_SET);
	char str[10] = "";
	sprintf(str,"\033[%d;%dH",T.termX,T.termY);
	write(STDOUT_FILENO, str,strlen(str));
}

void updateTerminal(){
  showFile(T.offset);

}

void ecran(int offset){
	if(M.direction == UP && M.x < 1){
		showFile(offset - 1);
	}else if(M.direction == DOWN && M.x >= T.numRow){
		if(T.eof){
			showFile(offset);
		}else{
			showFile(offset+1);
		}
	}
	char str[10] = "";
	sprintf(str,"off : %d",T.offset);
	M.direction = NONE;
}

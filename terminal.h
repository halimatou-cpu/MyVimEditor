#ifndef __TERMINAL__
#define __TERMINAL__

#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <stdbool.h>
#include <sys/ioctl.h>


// Structures de données

typedef struct param{
	int numRow; // number of rows
	int numCol; // number of colomns in Terminal
	int termX;
	int termY;
	int curX;
	int curY;
	int offset; // offset-line in fd_buffer
	int stop; // cursor position in fd_buffer
	int maxChar; // number of characters editor
	int maxRow; //max rows
	int r; //for maxRow
	char *filename;
	char *notif;
  int fd_buffer;
	char buffer[BUFSIZ];
	char buffer1[BUFSIZ];
	bool mode;
	bool eof; // end of file
	bool vu;
  FILE* file;
	struct termios stdin;
	struct termios stdout;
} param;

struct row{
	char *row;
	int size;
};

typedef enum sens{
	UP = 65, // <ESC>[A  which is the ANSI code
	DOWN, // <ESC>[B
	RIGHT, // <ESC>[C
	LEFT, // <ESC>[D
	NONE = -1 //Nothing
} sens;

typedef struct move{
	sens direction;
	int x;
} move;


// VARIABLES GLOBAL
param T;
move M;

struct row **crow;

// Déclaration des fonctions
bool isInsert();
char waitKey();
void mode_canonique();
void mode_non_canonique();
void ecran(int offset);
int parse_line(char *s, char **argv[]);
void changeMode();
void out();
void clearBackward();
void defaultMode();
void initVim();
int openFile(char filename[], bool inCurrentDir, int right);
void open_again_fd();
int updateBuffer();
void openVim(char *filename);
void processKey(char c);
void showFile(int offset);
int getBufferPos();
void moveCursor(int ascii);
void setCursorFdBuffer();
void updateCursorPos(int add);
void updateTerminal();


#endif

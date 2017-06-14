#include <stdio.h>
#include <string.h>
#include <ncurses.h>
#include <stdlib.h>
#include <locale.h>
#include <time.h>
#include "sqlite3.h"
#include "sql_handler.h"

#define LINUX

/* display constants */
const int MAX_LINE_LEN = 512;
const char *BTM_ROW = "┌ h - hide this ─────────────────────────────────────────────────────┐\n"\
					  "│ q - quit ; n - new entry ; d - delete entry ; e - edit entry       │\n"\
					  "│ p - copy password ; u - copy username ; l - copy URL ; o - options │\n"\
					  "└────────────────────────────────────────────────────────────────────┘";

/* display function prototypes */
void createNewScreen(sqlite3 *db, int *id);
char *generatePasskey(int len);
int confirmationSreen(char *warning);
void importPasswords(sqlite3 *db, FILE *fp);


int main(int argc, char *argv[])
{
	char *locale; //for unicode
	locale = setlocale(LC_ALL, "");

	/* formating a tring for th e top row */
	char TOP_ROW[128],
		 fname[64],
		 passwd[128];
	sprintf(TOP_ROW, "%-16s\t%-16s\t%-16s\t%-8s", "Title", "URL", "Username", "Passkey");

	int select = 0, //current cursor selection
	 	hidden = 1, //do we hide the help? 0=no
	 	show = -1,  //used to not censor a specific password
	 	ch,			//input character
		size,		//size of the database
		x, y,		//dimensions of the terminal
		tries = 0;

	/* initialize ncurses */
	initscr();
	raw();
	keypad(stdscr, TRUE);

	sqlite3 *db2;

	 //for debug purposes
	if (argc > 1)
	{
		if (strcmp(argv[1], "init") == 0)
		{
			sqlite3 *db = init_database("tmp.db", "password");
			FILE *fp = fopen("tmp.passwords", "r");
			importPasswords(db, fp);
			// add(db, "Email", "gmail.com", "foo@bar.com", "password123");
			// add(db, "Amazon", "amazon.com", "foo@bar.com", "password321");
			sqlite3_close(db);
		}
	}
	else
		/* Validate encryption key */
	{
		echo();
		printw("File Name: ");
		getstr(fname);
		noecho();
		while (tries < 6)
		{
			printw("Passkey for %s:", fname);
			getstr(passwd);
			db2 = init_database("tmp.db", "password");
			if (db2 != NULL)
			{
				break;
			}
			clear();
			refresh();
			printw("Incorrect key\n");
			tries++;
		}
		if (tries > 5)
		{
			printw("Too many failed attempts, exiting..");
			exit(1);
		}
	}
	
	
	noecho();

	int menuSpace, offset = 0;

	/* main menu loop */
	size = tableSize(db2);
	int ids[size];
	char arr[size][MAX_LINE_LEN];
	toString(db2, size, arr, ids, &show);
	while (ch != 113) //q
	{
		//size = tableSize(db2);
		getmaxyx(stdscr, y, x);
		attron(A_UNDERLINE);
		printw("%s\n", TOP_ROW);
		attroff(A_UNDERLINE);

		menuSpace = y - 3;

		/* highlight selected menu item */
		for (int i = offset; i < size; i++)
		{
			if (select == i)
			{
				attron(A_STANDOUT);
				printw("%s", arr[i]);
				attroff(A_STANDOUT);
			}
			else
			{
				printw("%s", arr[i]);
			}
		}

		/* output help if we want */
		if (hidden == 0)
			mvprintw(y - 4, 0, "%s", BTM_ROW);
		else
			mvprintw(y - 1, 0, "(h - show help)");

		/* process user input */
		ch = getch();
		switch (ch)
		{
			case KEY_UP:
				if (select > 0)
				{
					select--;
					if (select < offset)
					{
						offset -= menuSpace;
					}
				}
				break;
			case KEY_DOWN:
				if (select < size - 1)
				{
					select++;
					if (select > menuSpace + offset)
					{
						offset += menuSpace;
					}
				}
				break;
			case 110: //'n' key
				createNewScreen(db2, NULL);
				size = tableSize(db2);
				toString(db2, size, arr, ids, &show);
				break;
			case 100: //'d' key
				if (confirmationSreen("Are you sure youu want to delete selected entry? (y/n): ") == 0)
					removeFromTable(db2, ids[select]);
				size = tableSize(db2);
				toString(db2, size, arr, ids, &show);
				break;
			case 104 : //h
				hidden = !hidden;
				break;
			case 101: //e
				createNewScreen(db2, & ids[select]);
				size = tableSize(db2);
				toString(db2, size, arr, ids, &show);
				break;
			case 10: //enter
				show = (show != select) ? select : -1;
				toString(db2, size, arr, ids, &show);
				break;
			case 112: //p
				toClipBoard(db2, ids[select], 0);
				break;
			case 117: //u
				toClipBoard(db2, ids[select], 1);
				break;
			case 108: //l
				toClipBoard(db2, ids[select], 2);
				break;
		}
		clear();
		refresh();
	}
	endwin();

	return 0;
}

void createNewScreen(sqlite3 *db, int *id)
/* multipurpose menu function for creasting new entries and editing existing ones */
{
	//new member attribs
	char newName[64],
		 newUrl[64],
		 newLogin[64],
		 newPassword[128];
	//array of things to display
	char *displays[7] = {"Title:\t\t", "URL:\t\t", "Username:\t", "Passkey:\t", "\tShow Password\t", "\tRandom 64-byte Password (32 Character)", "\n\t\t[Save]"};
	//pointers to those things
	char *arr[4]; 

	const short SHOW_PASS = 0,
				GENERATE_PASS = 1;
	short options[2] = {0, 0};

	int x, y, ch = 0, current = 0;

	/* if we want to create a new entry, initialize all attribs empty*/
	if (id == NULL)
	{	
		strcpy(newName, " ");
		strcpy(newUrl, " ");
		strcpy(newLogin, " ");
		strcpy(newPassword, " ");
	}
	/* get them from the table if we want to edit one */
	else
	{
		retreiveFromTable(db, *id, newName, newUrl, newLogin, newPassword);
	}

	//setting display pointers 
	arr[0] = &newName;
	arr[1] = &newUrl;
	arr[2] = &newLogin;
	arr[3] = &newPassword;

	/* main input loop */
	while (ch != 113)
	{
		clear();
		refresh();
		printw("\n");
		for (int i = 0; i <= 7; i++)
		{
			if (current == i)
			{
				attron(A_BOLD);
				if (i < 3 || i == 3 && options[SHOW_PASS] != 0)
				{
					printw("\t\t%s%s\n", displays[i] ,arr[i]);
				}
				else if (i > 3 && i < 6)
					printw("\t%s%s\n", ((options[(i == 4) ? SHOW_PASS : GENERATE_PASS] == 1) ? "[X] " : "[ ] "), displays[i]);
				else if (i == 6)
					printw("%s", displays[i]);
				else if (i > 6)
					printw("[Cancel]");
				else
					printw("\t\t%s **** \n", displays[i]);
				attroff(A_BOLD);
			}
			else
			{
				if (i < 3 || i == 3 && options[SHOW_PASS] != 0)
					printw("\t\t%s%s\n", displays[i] ,arr[i]);
				else if (i > 3 && i < 6)
					printw("\t%s%s\n", ((options[(i == 4) ? SHOW_PASS : GENERATE_PASS] == 1) ? "[X] " : "[ ] "),displays[i]);
				else if (i == 6)
					printw("%s", displays[i]);
				else if (i > 6)
					printw("[Cancel]");
				else
					printw("\t\t%s **** \n", displays[i]);
			}
		}
		ch = getch();
		switch(ch)
		{
			case KEY_UP:
				if (current > 0)
					current--;
				break;
			case KEY_DOWN:
				if (current <= 6)
					current++;
				break;
			case 10: //10 == acsii enter
				echo();
				switch(current)
				{
					case 0:
						move(1 ,32);
						strcpy(newName, " ");
						getstr(newName);
						break;
					case 1:
						move(2 ,32);
						strcpy(newUrl, " ");
						getstr(newUrl);
						break;
					case 2:
						move(3 ,32);
						strcpy(newLogin, " ");
						getstr(newLogin);
						break;
					case 3:
						options[GENERATE_PASS] = 0;
						move(4 ,32);
						noecho();
						strcpy(newPassword, " ");
						getstr(newPassword);
						break;
					case 4:
						options[SHOW_PASS] = !options[SHOW_PASS];
						break;
					case 5:
						options[GENERATE_PASS] = !options[GENERATE_PASS];
						if (options[GENERATE_PASS] == 1)
							strcpy(newPassword, generatePasskey(32));
						break;
					case 6:
						if (id != NULL)
							removeFromTable(db, *id);
						add(db, newName, newUrl, newLogin, newPassword);
						return;
					case 7:
						return;

				}
				current = current < 4 ? current+1 : current;
				noecho();
				break;
		}
	}
}

int confirmationSreen(char *warning)
{
	int y, x, ch = 100;
	getmaxyx(stdscr, y, x);
	while (1 < 2)
	{
		refresh();
		clear();
		mvprintw(y/2, x/8, warning);
		//printw("%c", ch);
		ch = getch();
		switch(ch)
		{
			case 121:
				return 0;
			case 110:
				return 1;
		}
	}
}

char *generatePasskey(int len)
{
	time_t t;
	srand(time(NULL));
	char key[len];
	char *ptr = &key;
	for (int i = 0; i < len; i++)
		key[i] = (char)rand() % 75 + 48;
	return ptr;
}

void importPasswords(sqlite3 *db, FILE *fp)
{
	char line[512],
		*attribs[4];
	int current = 0, j = 0;
	while (fgets(line, sizeof(line), fp))
	{
		current = 0; j = 0;
		attribs[current] = (char*)malloc(sizeof(char) * 256);
		memset(attribs[current], ' ', 256);
		for (int i = 0; i < sizeof(line); i++)
		{
			if (isValidChar((int)(line[i])) == 0)
			{
				if ((int)line[i] == 44 || (int)line[i] == 59)
				{
					attribs[current][j+1] = '\0';
					current++; j = 0;
					attribs[current] = (char*)malloc(sizeof(char) * 256);
					memset(attribs[current], ' ', 256);
					if ((int)line[i] == 59)
						break;
				}
				else
				{
					attribs[current][j] = (char)line[i];
					j++;
				}
			}
		}
		//printf("%s:%s:%s:%s\n", attribs[0], attribs[1], attribs[2], attribs[3]);
		add(db, attribs[0], attribs[1], attribs[2], attribs[3]);
	}
	for (int i = 0; i < 4; i++)
		free(attribs[i]);
	//exit(0);
}

int isValidChar(int ch)
{
	//0: yes, 1: no
	//return (ch >= 32 && ch <= 126) ? 0 : 1;
	if (ch >= 32 && ch <= 126)
		return 0;
	return 1;
}
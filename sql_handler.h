#ifndef SQL_HANDLER_H_INCLUDED
#define SQL_HANDLER_H_INCLUDED

static int callback(void *data, int argc, char **argv, char **azColName);
short check_key(char *key, sqlite3 *db);
sqlite3* init_database(char *path, char *key);
void toString(sqlite3 *db, int size, char arr[size][512], int ids[size], int *noShow);
void add(sqlite3 * db, char *name, char *url, char *login, char *password);
int tableSize(sqlite3 * db);
void removeFromTable(sqlite3 *db, int id);
void retreiveFromTable(sqlite3 *db, int id, char newName[64], char newUrl[64], char newLogin[64], char newPassword[128]);
void toClipBoard(sqlite3 *db, int id, int attrib);

#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "sqlite3.h"

#include <ncurses.h>

#define DEBUG 1

short check_key(char *key, sqlite3 *db)
{
	/* tests the validity of a key */
	if (sqlite3_exec(db, "select count(*) from sqlite_master;", NULL, NULL, NULL) == SQLITE_OK)
	{
		return 0;
	}
	return 1;
}

sqlite3* init_database(char *path, char *key)
{
	sqlite3 *db = NULL;
	int rc;
	char *errMsg = 0;
	sqlite3_stmt *stmt;

	rc = sqlite3_open(path, &db);

	if (rc)
	{
		fprintf(stderr, "Faild to open databse: %s\n", path);
		return NULL;
	}

	sqlite3_key(db, key, (int)strlen(key));

	if (check_key(key, db) == 1)
	{
		fprintf(stderr, "Invalid key/file is not a databse\n");
		return NULL;
	}

	rc = sqlite3_prepare_v2(db, "select * from entries;", -1, &stmt, NULL);
	if (rc)
	{
		const char *err = sqlite3_errmsg(db);
		if (strcmp(err, "no such table: entries") == 0)
		{
			rc = sqlite3_exec(db, 
							"create table entries ("\
							"id integer primary key autoincrement,"\
							"name text,"\
							"url text,"\
							"login text,"\
							"password text);",
							NULL, 
							0, 
							&errMsg);
			if (rc)
			{
				fprintf(stderr, "Could not initialize empty database: %s\n", errMsg);
				return NULL;
			}
		}
		else
		{
			fprintf(stderr, "sqlite3 error: %s", err);
		}
	}

	sqlite3_finalize(stmt);

	return db;

}

void toString(sqlite3 *db, int size, char arr[size][512], int ids[size], int *noShow, int order, char *tName)
{
	char sql[128],
		 output[512];
	memset(output, '\0', 512);
	for (int i = 0; i < size; i++)
		memset(arr[i], '\0', 512);
	if (order != 0)
	{
		switch(order)
		{
			case 1:
				sprintf(sql, "select * from '%s' order by name;", tName);
				break;
			case 2:
				sprintf(sql, "select * from '%s' order by url;", tName);
				break;
			case 3:
				sprintf(sql, "select * from '%s' order by login;", tName);
				break;
			case -1:
				sprintf(sql, "select * from '%s' order by name DESC;", tName);
				break;
			case -2:
				sprintf(sql, "select * from '%s' order by url DESC;", tName);
				break;
			case -3:
				sprintf(sql, "select * from '%s' order by login DESC;", tName);
				break;
		}
	}
	else
	{
		sprintf(sql, "select * from '%s';", tName);
	}
	sqlite3_stmt *stmt;
	int rc;

	//sqlite3_reset(stmt);
	//printf("SQL: %s\n", sql);
	rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	if (rc != SQLITE_OK)
	{
		fprintf(stderr, "Unable to read databse\n");
		fprintf(stderr, "SQL: %s\n", sql);
		exit(1);
	}
	int j = 0;
	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		if (*noShow == j)
		{
			sprintf(output, "%-16s\t%-16s\t%-16s\t%-16s\n", 
							sqlite3_column_text(stmt, 1), 
							sqlite3_column_text(stmt, 2),
							sqlite3_column_text(stmt, 3),
							sqlite3_column_text(stmt, 4));
		}
		else
		{
			sprintf(output, "%-16s\t%-16s\t%-16s\t%-8s\n",
							sqlite3_column_text(stmt, 1), 
							sqlite3_column_text(stmt, 2),
							sqlite3_column_text(stmt, 3),
							"******");
		}
		//printf("output: %s\n", output);
		strcpy(arr[j], output);
		ids[j] = sqlite3_column_int(stmt, 0);
		// printf("%d\t%s\t", j, sqlite3_column_text(stmt, 1));
		// printf("%s\t", sqlite3_column_text(stmt, 2));
		// printf("%s\t", sqlite3_column_text(stmt, 3));
		// printf("%s\t\n", sqlite3_column_text(stmt, 4));
		//printf("%d\t", j);
		//for (int i = 1; i < 4; i++)
		//{
		//	printf("%s\t", j, (char*)sqlite3_column_text(stmt, i));
		//}
		j++;
		memset(output, '\0', 512);
	}
	sqlite3_finalize(stmt);
	//free(output);
}

void add(sqlite3 * db, char *name, char *url, char *login, char *password, char *tName)
{
	char *errMsg = 0;
	char *sql = (char*)malloc(sizeof(char) * (strlen(name) + strlen(login) + strlen(password) + strlen(url)) + 128);
	sprintf(sql, "insert into %s (id, name, url, login, password) values (NULL, \"%s\", \"%s\",  \"%s\", \"%s\");", tName, name, url, login, password);
	if (sqlite3_exec(db, sql, NULL, NULL, &errMsg) == SQLITE_OK)
	{
		//printf("Appended to databse successfully\n");

	}
	else
	{
		printf("Unable to append to database\nError: %s\n", errMsg);
		printf("Statement: \"%s\"\n", sql);
	}
	free(sql);
}

void retreiveFromTable(sqlite3 *db, int id, char newName[64], char newUrl[64], char newLogin[64], char newPassword[128])
{
	char *errMsg = 0;
	char *sql = (char*) malloc(sizeof(char) * (sizeof "select * from entries where id=") + 32);
	printf("After malloc\n");
	int rc;
	sqlite3_stmt *stmt;
	sprintf(sql, "select * from entries where id=%d;", id);
	printf("Afer sprintf\n");
	rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	if (rc != SQLITE_OK)
	{
		fprintf(stderr, "Enable to prep statement: retrievFromTableCall\n");
		return;
	}
	printf("After statement preparation\n");
	if ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		strcpy(newName, sqlite3_column_text(stmt, 1));
		strcpy(newUrl, sqlite3_column_text(stmt, 2));
		strcpy(newLogin, sqlite3_column_text(stmt, 3));
		strcpy(newPassword, sqlite3_column_text(stmt, 4));
	}
	else
	{
		fprintf(stderr, "Could not read results from database\n");
	}
	free(sql);
	sqlite3_finalize(stmt);
}

void removeFromTable(sqlite3 *db, int id)
{
	char str_id[8], sql[64];
	int rc;
	char *errMsg = 0;
	sprintf(str_id, "%d", id);
	sprintf(sql, "delete from entries where id = %s;", str_id);
	rc = sqlite3_exec(db, sql, NULL, NULL, &errMsg);
	if (rc)
	{
		fprintf(stderr, "Unable to remove from database\nReason: %s", errMsg);
		printf("Unable to remove from database\nReason: %s", errMsg);
	}
}

int entryCount(sqlite3 * db, char *tName)
{
	int rc;
	sqlite3_stmt *stmt;
	char sql[64];
	sprintf(sql, "select count(*) from '%s';", tName);
	rc = sqlite3_prepare_v2(db,  sql, -1, &stmt, NULL);
	if (rc)
	{
		fprintf(stderr, "Could not count databse\n");
		return 0;
	}

	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		int r = sqlite3_column_int(stmt, 0);
		//printf("%d\n", r);
		sqlite3_finalize(stmt);
		return r;
	}
}

int tableCount(sqlite3 *db)
{
	int rc;
	sqlite3_stmt *stmt;
	const char *sql = "select count(*) from sqlite_master where type='table' and name like '%back_%';";
	rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	if (rc)
	{
		fprintf(stderr, "Could not count tables\n");
		exit(1);
	}
	if ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		int r = sqlite3_column_int(stmt, 0) - 1;
		sqlite3_finalize(stmt);
		return r;
	}
	else
	{
		fprintf(stderr, "unable to count table\n");
		exit(1);
	}
}

void toClipBoard(sqlite3 *db, int id, int attrib)
{
	/* attribs: 0 = password, 1 = login, 2 = url */
	char sql[128],
		data[64],
		command[128],
		*attr;
	sqlite3_stmt *stmt;
	int rc;
	
	switch(attrib)
	{
		case 0:
			attr = "password";
			break;
		case 1:
			attr = "login";
			break;
		case 2:
			attr = "url";
			break;
	}
	//printf("before constructing sql\n");
	sprintf(sql, "select %s from entries where id=%d;", attr, id);
	rc = sqlite3_prepare_v2(db , sql, -1, &stmt, NULL);
	if (rc)
	{
		fprintf(stderr, "Unable to prep statement (toClipBoard)\n");
		exit(1);
	}
	//printf("post prepare \n");
	if ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		strcpy(data, sqlite3_column_text(stmt, 0));
	}

	sprintf(command, "echo %s | xclip -selection clipboard", data);
	system(command);
	sqlite3_finalize(stmt);

}

void copyTable(sqlite3 *db, char *from, char *to)
{
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	char sql[256],
		tName[32],
		*errMsg = 0;
	int rc;
	sqlite3_stmt *stmt;
	if (to == NULL)
	{
		sprintf(tName, "back_%d-%d-%d_%d:%d:%d", tm.tm_year + 1900, 
								tm.tm_mon + 1, 
								tm.tm_mday, 
								tm.tm_hour, 
								tm.tm_min, 
								tm.tm_sec);
	}
	else
		strcpy(tName, to);
	sprintf(sql, "create table '%s' ("\
							"id integer primary key autoincrement,"\
							"name text,"\
							"url text,"\
							"login text,"\
							"password text);", tName);
	rc = sqlite3_exec(db, sql,NULL, NULL, &errMsg);
	if (rc)
	{
		fprintf(stderr, "Unable to create backup databse\nError Msg: %s\nSQL: %s\n", errMsg, sql);
		exit(1);
	}
	memset(sql, '\0', sizeof(sql));
	sprintf(sql, "select * from '%s';", from);
	rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	if (rc)
	{
		fprintf(stderr, "Could not prepare statement (backUpEntries)\nSQL: %s\n", sql);
		exit(1);
	}
	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		add(db, sqlite3_column_text(stmt, 1), 
				sqlite3_column_text(stmt, 2),
				sqlite3_column_text(stmt, 3),
				sqlite3_column_text(stmt, 4),
				tName);
	}
	sqlite3_finalize(stmt);

}

void renameTable(sqlite3 *db, char *old, char *new)
{
	int rc;
	char sql[64],
		*errMsg = 0;
	sprintf(sql, "alter table %s rename to %s;", old, new);

	rc = sqlite3_exec(db, sql, NULL, NULL, &errMsg);
	if (rc)
	{
		fprintf(stderr, "Could not rename table\nErrmsg: %s\n SQL: %s\n", errMsg, sql);
	}
}

void removeTable(sqlite3 *db, char *tName)
{
	int rc;
	char *errMsg = 0,
		sql[64];
	sprintf(sql, "drop table %s;", tName);
	rc = sqlite3_exec(db, sql, NULL, NULL, &errMsg);
	if (rc)
	{
		fprintf(stderr, "Unable to remove table: %s\nError msg: %s\nSQL: %s\n", tName, errMsg, sql);
	}
}

void tables(sqlite3 *db, int count, char arr[count][32])
{
	int rc, i = 0;
	sqlite3_stmt *stmt;
	char tName[32];
	rc = sqlite3_prepare_v2(db, "select * from sqlite_master where type='table' and name like '%back_%';", -1, &stmt, NULL);
	if (rc)
	{
		fprintf(stderr, "Unable to prep statement (tables)\nSQL: .tables\n");
		exit(1);
	}
	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		memset(tName, '\0', 32);
		strcpy(tName, sqlite3_column_text(stmt, 1));
		if (strcmp(tName, "entries") != 0)
		{
			strcpy(arr[i], tName);
			i++;
		}
	}

	sqlite3_finalize(stmt);
}
// void search(sqlite3 *db, int size, char arr[size][512])
// {

// }
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sqlite3.h"

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

void toString(sqlite3 *db, int size, char arr[size][512], int ids[size], int *noShow, int order)
{
	char* sql = NULL,
		 *output = NULL;
	if (order != 0)
	{
		switch(order)
		{
			case 1:
				sql = "select * from entries order by name;";
				break;
			case 2:
				sql = "select * from entries order by url;";
				break;
			case 3:
				sql = "select * from entries order by login;";
				break;
			case -1:
				sql = "select * from entries order by name DESC;";
				break;
			case -2:
				sql = "select * from entries order by url DESC;";
				break;
			case -3:
				sql = "select * from entries order by login DESC;";
				break;
		}
	}
	else
	{
		sql = "select * from entries;";
	}
	sqlite3_stmt *stmt;
	int rc;

	//sqlite3_reset(stmt);

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
		output = (char*)malloc(sizeof(char)*512);
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
	}

	free(output);
}

void add(sqlite3 * db, char *name, char *url, char *login, char *password)
{
	char *errMsg = 0;
	char *sql = (char*)malloc(sizeof(char) * (strlen(name) + strlen(login) + strlen(password) + strlen(url)) + 128);
	sprintf(sql, "insert into entries (id, name, url, login, password) values (NULL, \"%s\", \"%s\",  \"%s\", \"%s\");", name, url, login, password);
	if (sqlite3_exec(db, sql, NULL, NULL, &errMsg) == SQLITE_OK)
	{
		printf("Appended to databse successfully\n");
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

int tableSize(sqlite3 * db)
{
	int rc;
	sqlite3_stmt *stmt;
	const char *sql = "select count(*) from entries;";
	rc = sqlite3_prepare_v2(db,  sql, -1, &stmt, NULL);
	if (rc)
	{
		fprintf(stderr, "Could not count databse\n");
		return 0;
	}

	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		return sqlite3_column_int(stmt, 0);
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


}

// void search(sqlite3 *db, int size, char arr[size][512])
// {

// }
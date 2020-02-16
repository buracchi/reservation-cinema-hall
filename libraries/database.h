#pragma once

#ifdef __unix__
#include <stdio.h>
#include <stdint.h>
#include <pthread.h>

/*
 *	query language:
 *	
 *				ADD [SECTION]
 *				ADD [KEY] FROM [SECTION]
 *				GET [KEY] FROM [SECTION]
 *				SET [KEY] FROM [SECTION] AS [VALUE]
 * */

#define DBMSG_SUCC "OPERATION SUCCEDED"
#define DBMSG_FAIL "OPERATION FAILED"
#define DBMSG_ERR "DATABASE FAILURE"

typedef void* database_t;

/*	Initiazlize database from file return 1 and set properly errno on error	*/
extern database_t database_init(const char *filename);
/*	Close database return EOF and set properly errno on error */
extern int database_close(const database_t handle);
/*	Execute a query return 1 and set properly errno on error */
extern int database_execute(const database_t handle, const char *query, char **result);
#endif
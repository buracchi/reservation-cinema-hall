/*	Simple NoSQL Database	*/

#include "database.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>
#include <errno.h>

#include "asprintf.h"
#include "storage.h"

struct cinema_info {
	int rows;
	int columns;
};

struct database {
	storage_t storage;
	struct cinema_info cinema_info;
};

//static int parse_query(struct query* parsed_query, const char* query);
static int parse_query(const char* query, char*** parsed);
static int procedure_populate(const database_t handle, char** result);
static int procedure_setup(const database_t handle, char** result);
static int procedure_clean(const database_t handle, char** result);
static int procedure_get(const database_t handle, const char** query, char** result);
static int procedure_set(const database_t handle, const char** query, char** result);
static int procedure_map(const database_t handle, const char** query, char** result);
static int procedure_book(const database_t handle, const char** query, char** result);
static int procedure_unbook(const database_t handle, const char** query, char** result);

database_t database_init(const char* filename) {
	struct database* database;
	if ((database = malloc(sizeof(struct database))) == NULL) {
		return NULL;
	}
	if ((database->storage = storage_init(filename)) == NULL) {
		free(database);
		return NULL;
	}
	database->cinema_info.columns = 0;
	database->cinema_info.rows = 0;
	return database;
}

/*	Close database return EOF and set properly errno on error */

int database_close(const database_t handle) {
	struct database* database = (struct database*)handle;
	storage_close(database->storage);
	free(database);
	return 0;
}

/*	Execute a query return 1 and set properly errno on error */

int database_execute(const database_t handle, const char* query, char** result) {
	struct database* database = (struct database*)handle;
	/*	Parse query	*/
	int ret;
	int n_param;
	char** parsed_query;
	if ((n_param = parse_query(query, &parsed_query)) == -1) {
		return 1;
	}
	else if (n_param == 1 && !strcmp(parsed_query[0], "POPULATE")) {
		ret = procedure_populate(database, result);
	}
	else if (n_param == 1 && !strcmp(parsed_query[0], "SETUP")) {
		ret = procedure_setup(database, result);
	}
	else if (n_param == 1 && !strcmp(parsed_query[0], "CLEAN")) {
		ret = procedure_clean(database, result);
	}
	else if (n_param == 2 && !strcmp(parsed_query[0], "GET")) {
		ret = procedure_get(database, &(parsed_query[1]), result);
	}
	else if (n_param == 3 && !strcmp(parsed_query[0], "SET")) {
		ret = procedure_set(database, &(parsed_query[1]), result);
	}
	else if (n_param == 2 && !strcmp(parsed_query[0], "MAP")) {
		ret = procedure_map(database, &(parsed_query[1]), result);
	}
	else if (n_param > 2 && !strcmp(parsed_query[0], "BOOK")) {
		ret = procedure_book(database, &(parsed_query[1]), result);
	}
	else if (n_param > 2 && !strcmp(parsed_query[0], "DELETE")) {
		ret = procedure_unbook(database, &(parsed_query[1]), result);
	}
	else {
		*result = strdup(MSG_FAIL);
		ret = 0;
	}
	free(*parsed_query);
	free(parsed_query);
	return ret;
}

/* return 0 on success, 1 on failure */

/*	this should be in storage probably	*/
/*

struct query {
	char key[MAXLEN + 1];
	char value[MAXLEN + 1];
};

int parse_query(struct query* parsed_query, const char* query) {
	int ret = 0;
	int ntoken = 0;
	char* buff;
	char* saveptr;
	char** token;
	memset(parsed_query->key, 0, MAXLEN + 1);
	memset(parsed_query->value, 0, MAXLEN + 1);
	if ((buff = strdup(query)) == NULL) {
		return 1;
	}
	if ((token = malloc(sizeof(char*))) == NULL) {
		return 1;
	}
	if ((token[0] = strtok_r(buff, " ", &saveptr)) == NULL) {
		return 1;
	}
	ntoken++;
	do {
		ntoken++;
		if ((token = realloc(token, sizeof(char*) * (size_t)ntoken)) == NULL) {
			return 1;
		}
		token[ntoken - 1] = strtok_r(NULL, " ", &saveptr);
	} while (token[ntoken - 1] != NULL);
	ntoken--;
	switch (ntoken) {
	case 1:
		if (strlen(token[0]) > MAXLEN) {
			ret = 1;
			break;
		}
		strncpy(parsed_query->key, token[0], MAXLEN + 1);
		break;
	case 2:
		if (strlen(token[0]) > MAXLEN || strlen(token[1]) > MAXLEN) {
			ret = 1;
			break;
		}
		strncpy(parsed_query->key, token[0], MAXLEN + 1);
		strncpy(parsed_query->value, token[1], MAXLEN + 1);
		break;
	default:
		ret = 1;
	}
	free(buff);
	free(token);
	if (ret) {
		return 1;
	}
	return 0;
}
*/
/*	return in parsed parameter a NULL terminated vector of parsed string */

static int parse_query(const char* query, char*** parsed) {
	int ntoken;
	char* buffer = NULL;
	char* saveptr = NULL;
	char** token = NULL;

	ntoken = 0;
	if ((buffer = strdup(query)) == NULL) {
		return -1;
	}
	if ((token = malloc(sizeof(char*))) == NULL) {
		return -1;
	}
	token[0] = strtok_r(buffer, " ", &saveptr);
	ntoken++;
	do {
		ntoken++;
		if ((token = realloc(token, sizeof(char*) * (size_t)ntoken)) == NULL) {
			return -1;
		}
		token[ntoken - 1] = strtok_r(NULL, " ", &saveptr);
	} while (token[ntoken - 1] != NULL);
	ntoken--;
	*parsed = token;
	return ntoken;
}

static int procedure_populate(const database_t handle, char** result) {
	struct database* database = (struct database*)handle;
	char* msg_init[] = {
	"SET IP 127.0.0.1",
	"SET PORT 55555",
	"SET PID 0",
	"SET TIMESTAMP 0",
	"SET ROWS 1",
	"SET COLUMNS 1",
	"SET FILM Titolo",
	"SET SHOWTIME 00:00",
	"SET ID_COUNTER 0",
	"SET 0 0",
	NULL
	};
	for (int i = 0; msg_init[i]; i++) {
		if (database_execute(database, msg_init[i], result)) {
			return 1;
		}
		free(*result);
	}
	*result = strdup(MSG_SUCC);
	return 0;
}

static int procedure_setup(const database_t handle, char** result) {
	struct database* database = (struct database*)handle;
	int clean = 0;

	database_execute(database, "GET ROWS", result);
	database->cinema_info.rows = atoi(*result);
	free(*result);
	database_execute(database, "GET COLUMNS", result);
	database->cinema_info.columns = atoi(*result);
	free(*result);
	for (int i = 0; i < database->cinema_info.rows * database->cinema_info.columns; i++) {
		char* query;
		if (asprintf(&query, "GET %d", i) == -1) {
			return 1;
		}
		if (database_execute(database, query, result) == 1) {
			return 1;
		}
		if (!strncmp(*result, MSG_FAIL, strlen(MSG_FAIL))) {
			clean = 1;
			free(query);
			free(*result);
			if (asprintf(&query, "SET %d 0", i) == -1) {
				return 1;
			}
			if (database_execute(database, query, result) == 1) {
				return 1;
			}
		}
		free(query);
		free(*result);
	}
	if (clean) {
		if (procedure_clean(database, result)) {
			return 1;
		}
		free(*result);
	}
	*result = strdup(MSG_SUCC);
	return 0;
}

static int procedure_clean(const database_t handle, char** result) {
	struct database* database = (struct database*)handle;
	for (int i = 0; i < database->cinema_info.rows * database->cinema_info.columns; i++) {
		char* tmp_query;
		char* buffer;
		if (asprintf(&tmp_query, "%d 0", i) == -1) {
			return 1;
		}
		if (procedure_set(database, tmp_query, &buffer)) {
			return 1;
		}
		free(tmp_query);
		free(buffer);
	}
	if (procedure_set(database, "ID_COUNTER 0", result)) {
		return 1;
	}
	return 0;
}

static int procedure_get(const database_t handle, const char** query, char** result) {
	struct database* database = (struct database*)handle;
	if (storage_lock_shared(database->storage, query[0])) {
		return 1;
	}
	if (storage_load(database->storage, query[0], result)) {
		return 1;
	}
	if (storage_unlock(database->storage, query[0])) {
		return 1;
	}
	return 0;
}

static int procedure_set(const database_t handle, const char** query, char** result) {
	struct database* database = (struct database*)handle;
	if (storage_lock_exclusive(database->storage, query[0])) {
		return 1;
	}
	if (storage_store(database->storage, query[0], query[1], result)) {
		return 1;
	}
	if (storage_unlock(database->storage, query[0])) {
		return 1;
	}
	return 0;
}

static int procedure_map(const database_t handle, const char** query, char** result) {
	struct database* database = (struct database*)handle;
	int id = 0;
	char* tmp_query = NULL;
	char* buffer = NULL;
	char* ptr = NULL;
	char* map = NULL;
	id = !strlen(query) ? -1 : atoi(query);
	if (!(database->cinema_info.rows && database->cinema_info.columns)) {
		asprintf(result, "");
		return 0;
	}
	for (int i = 0; i < database->cinema_info.rows * database->cinema_info.columns; i++) {
		if (asprintf(&tmp_query, "%d", i) == -1) {
			return 1;
		}
		if (procedure_get(database, tmp_query, &buffer)) {
			return 1;
		}
		free(tmp_query);
		ptr = map;
		if (atoi(buffer)) {
			if (atoi(buffer) == id) {
				free(buffer);
				asprintf(&buffer, "1");
			}
			else {
				free(buffer);
				asprintf(&buffer, "2");
			}
		}
		if (asprintf(&map, "%s %s", map, buffer) == -1) {
			free(buffer);
			free(ptr);
			return 1;
		}
		free(buffer);
		free(ptr);
	}
	*result = map;
	return 0;
}

static int procedure_book(const database_t handle, const char** query, char** result) {
	struct database* database = (struct database*)handle;
	int id;
	int ret;
	int ntoken;
	char* buffer = NULL;
	char* saveptr = NULL;
	char** token = NULL;
	char** rquery = NULL;
	char** wquery = NULL;

	ntoken = 0;
	if ((buffer = strdup(query)) == NULL) {
		return 1;
	}
	if ((token = malloc(sizeof(char*))) == NULL) {
		return 1;
	}
	token[0] = strtok_r(buffer, " ", &saveptr);
	ntoken++;
	do {
		ntoken++;
		if ((token = realloc(token, sizeof(char*) * (size_t)ntoken)) == NULL) {
			return 1;
		}
		token[ntoken - 1] = strtok_r(NULL, " ", &saveptr);
	} while (token[ntoken - 1] != NULL);
	ntoken--;
	if (!strcmp(token[0], "0")) {
		char* id_query;
		char* result;
		if (database_execute(database, "GET ID_COUNTER", &result) == 1) {
			return -1;
		}
		id = atoi(result);
		id++;
		free(result);
		if (asprintf(&id_query, "SET ID_COUNTER %d", id) == -1) {
			return -1;
		}
		if (database_execute(database, id_query, &result) == 1) {
			free(id_query);
			return -1;
		}
		free(id_query);
		free(result);
		if (id == -1) {
			return 1;
		}
	}
	else {
		id = atoi(token[0]);
	}
	ntoken--;
	/*	free memory on error	*/
	if ((rquery = malloc(sizeof(char*) * (size_t)(ntoken))) == NULL) {
		return 1;
	}
	if ((wquery = malloc(sizeof(char*) * (size_t)(ntoken))) == NULL) {
		return 1;
	}
	for (int i = 0; i < ntoken; i++) {
		if (asprintf(&(rquery[i]), "GET %s", token[i + 1]) == -1) {
			return 1;
		}
		if (asprintf(&(wquery[i]), "SET %s %d", token[i + 1], id) == -1) {
			return 1;
		}
	}
	free(buffer);
	free(token);
	ret = 0;
	for (int i = 0; i < ntoken; i++) {
		if (database_execute(database, rquery[i], result) == 1) {
			for (int j = i; j < ntoken; free(rquery[++j]));
			for (int j = 0; j < ntoken; free(wquery[j++]));
			free(rquery);
			free(wquery);
			return 1;
		}
		if (strcmp(*result, "0")) {
			ret = 1;
		}
		free(rquery[i]);
		free(*result);
		if (ret) {
			for (int j = i; j < ntoken; free(rquery[++j]));
			for (int j = 0; j < ntoken; free(wquery[j++]));
			free(rquery);
			free(wquery);
			return 1;
		}
	}
	free(rquery);
	for (int i = 0; i < ntoken; i++) {
		if (database_execute(database, wquery[i], result) == 1) {
			for (int j = i; j < ntoken; free(wquery[++j]));
			free(wquery);
			return 1;
		}
		free(wquery[i]);
		free(*result);
	}
	free(wquery);
	if (asprintf(result, "%d", id) == -1) {
		return 1;
	}
	return 0;
	/*
	//	2PL implementation
	char* id;
	int ntoken;
	char** token;
	char** ordered_request;
	int* tmp;	//tmp variable to order request
	int abort = 0;
	//	get token
	if ((ordered_request = malloc(sizeof(char*) * ntoken)) == NULL) {
		return 1;
	}
	if ((tmp = malloc(sizeof(int) * database->cinema_info.rows * database->cinema_info.columns)) == NULL) {
		return 1;
	}
	memset(tmp, 0, database->cinema_info.rows * database->cinema_info.columns * sizeof(int));
	for (int i = 0; i < ntoken; i++) {
		tmp[atoi(token[i])] = 1;
	}
	for (int i = 0, j = 0; i < database->cinema_info.rows * database->cinema_info.columns; i++) {
		if (tmp[i]) {
			tmp[j] = i;
			j++;
		}
	}
	for (int i = 0; i < ntoken; i++) {
		asprintf(&(ordered_request[i]), "%d", tmp[i]);
	}
	for (int i = 0; i < ntoken; i++) {
		if (storage_lock_exclusive(database->storage, ordered_request[i])) {
			return 1;
		}
	}
	char* seat_id;
	for (int i = 0; i < ntoken; i++) {
		if (storage_load(database->storage, ordered_request[i], &seat_id)) {
			return 1;
		}
		if (atoi(seat_id)) {
			free(seat_id);
			abort = 1;
			break;
		}
		free(seat_id);
	}
	//get valid id
	if (!abort) {
		char* buffer;
		for (int i = 0; i < ntoken; i++) {
			if (storage_store(database->storage, ordered_request[i], id, &buffer)) {
				return 1;
			}
			free(buffer);
		}
		//set result as id
	}
	else {
		//set result as failed
	}
	for (int i = 0; i < ntoken; i++) {
		if (storage_unlock(database->storage, ordered_request[i])) {
			return 1;
		}
	}
	for (int i = 0; i < ntoken; i++) {
		free(ordered_request[i]);
	}
	free(ordered_request);
	*/
}

static int procedure_unbook(const database_t handle, const char** query, char** result) {
	struct database* database = (struct database*)handle;
	int ret;
	int ntoken;
	char* id;
	char* buffer = NULL;
	char* saveptr = NULL;
	char** token = NULL;
	char** rquery = NULL;
	char** wquery = NULL;

	ntoken = 0;
	if ((buffer = strdup(query)) == NULL) {
		return 1;
	}
	if ((token = malloc(sizeof(char*))) == NULL) {
		return 1;
	}
	token[0] = strtok_r(buffer, " ", &saveptr);
	ntoken++;
	do {
		ntoken++;
		if ((token = realloc(token, sizeof(char*) * (size_t)ntoken)) == NULL) {
			return 1;
		}
		token[ntoken - 1] = strtok_r(NULL, " ", &saveptr);
	} while (token[ntoken - 1] != NULL);
	ntoken--;
	id = strdup(*token);
	ntoken--;
	/*	free memory on error	*/
	if ((rquery = malloc(sizeof(char*) * (size_t)(ntoken))) == NULL) {
		return 1;
	}
	if ((wquery = malloc(sizeof(char*) * (size_t)(ntoken))) == NULL) {
		return 1;
	}
	for (int i = 0; i < ntoken; i++) {
		if (asprintf(&(rquery[i]), "GET %s", token[i + 1]) == -1) {
			return 1;
		}
		if (asprintf(&(wquery[i]), "SET %s 0", token[i + 1]) == -1) {
			return 1;
		}
	}
	free(buffer);
	free(token);
	ret = 0;
	for (int i = 0; i < ntoken; i++) {
		if (database_execute(database, rquery[i], result) == 1) {
			for (int j = i; j < ntoken; free(rquery[++j]));
			for (int j = 0; j < ntoken; free(wquery[j++]));
			free(rquery);
			free(wquery);
			return 1;
		}
		if (strcmp(*result, id)) {
			ret = 1;
		}
		free(rquery[i]);
		free(*result);
		if (ret) {
			for (int j = i; j < ntoken; free(rquery[++j]));
			for (int j = 0; j < ntoken; free(wquery[j++]));
			free(rquery);
			free(wquery);
			free(id);
			return 1;
		}
	}
	free(rquery);
	free(id);
	for (int i = 0; i < ntoken; i++) {
		if (database_execute(database, wquery[i], result) == 1) {
			for (int j = i; j < ntoken; free(wquery[++j]));
			free(wquery);
			return 1;
		}
		free(wquery[i]);
		free(*result);
	}
	free(wquery);
	if (asprintf(result, MSG_SUCC) == -1) {
		return 1;
	}
	return 0;
}

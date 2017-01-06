/**
 * gcc s1.c -lsqlite3 -o s1
 */

#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>

static int callback(void *NotUsed, int argc, char **argv, char **azColName)
{
    int i;

    for (i = 0; i < argc; i++) {
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");

    return 0;
}

int main(int argc, char *argv[])
{
    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;
    char *sql;
    const char *data = "Callback function called";

    printf("sqlite3 info:\n");
    printf("version number: %d\n", sqlite3_libversion_number());
    printf("version: %s\n", sqlite3_libversion());
    printf("source id: %s\n", sqlite3_sourceid());
    printf("threadsafe: %d\n", sqlite3_threadsafe());

    rc = sqlite3_open("s1.db", &db);

    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));

        return 1;
    }
    fprintf(stderr, "Opened database successfully\n");

    /* Create SQL statement */
    sql = "CREATE TABLE COMPANY("
        "ID INT PRIMARY KEY     NOT NULL,"
        "NAME           TEXT    NOT NULL,"
        "AGE            INT     NOT NULL," "ADDRESS        CHAR(50)," "SALARY         REAL );";

    /* Execute SQL statement */
    rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } else {
        fprintf(stdout, "Table created successfully\n");
    }

    /* Create SQL statement */
    sql = "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY) "
        "VALUES (1, 'Paul', 32, 'California', 20000.00 ); "
        "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY) "
        "VALUES (2, 'Allen', 25, 'Texas', 15000.00 ); "
        "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY)"
        "VALUES (3, 'Teddy', 23, 'Norway', 20000.00 );"
        "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY)" "VALUES (4, 'Mark', 25, 'Rich-Mond ', 65000.00 );";

    /* Execute SQL statement */
    rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } else {
        fprintf(stdout, "Records created successfully\n");
    }

    /* Create SQL statement */
    sql = "SELECT * from COMPANY";

    /* Execute SQL statement */
    rc = sqlite3_exec(db, sql, callback, (void *)data, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } else {
        fprintf(stdout, "Operation done successfully\n");
    }

    /* Create merged SQL statement */
    sql = "DELETE from COMPANY where ID=2; " "SELECT * from COMPANY";

    /* Execute SQL statement */
    rc = sqlite3_exec(db, sql, callback, (void *)data, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } else {
        fprintf(stdout, "Operation done successfully\n");
    }

    sqlite3_close(db);

    return 0;
}

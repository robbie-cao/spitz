/**
 * gcc s1.c -lsqlite3 -o s1
 *
 * samples from http://zetcode.com/db/sqlitec/
 */

#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>

int main(void) {

    sqlite3 *db;
    sqlite3_stmt *res;
    char *err_msg = 0;
    char *sql;
    int step;
    int rc = 0;

    printf("sqlite3 v%s\n", sqlite3_libversion());

    rc = sqlite3_open("s2.db", &db);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);

        return 1;
    }

    // statements create a Cars table and fill it with data.
    sql = "DROP TABLE IF EXISTS Cars;"
        "CREATE TABLE Cars(Id INT, Name TEXT, Price INT);"
        "INSERT INTO Cars VALUES(1, 'Audi', 52642);"
        "INSERT INTO Cars VALUES(2, 'Mercedes', 57127);"
        "INSERT INTO Cars VALUES(3, 'Skoda', 9000);"
        "INSERT INTO Cars VALUES(4, 'Volvo', 29000);"
        "INSERT INTO Cars VALUES(5, 'Bentley', 350000);"
        "INSERT INTO Cars VALUES(6, 'Citroen', 21000);"
        "INSERT INTO Cars VALUES(7, 'Hummer', 41400);"
        "INSERT INTO Cars VALUES(8, 'Volkswagen', 21600);";

    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);

    if (rc != SQLITE_OK ) {

        fprintf(stderr, "SQL error: %s\n", err_msg);

        sqlite3_free(err_msg);
        sqlite3_close(db);

        return 1;
    }

    // In SQLite, INTEGER PRIMARY KEY column is auto-incremented.
    // There is also an AUTOINCREMENT keyword.
    // When applied in INTEGER PRIMARY KEY AUTOINCREMENT a slightly
    // different algorithm for Id creation is used.
    //
    // When using auto-incremented columns, we need to explicitly state
    // the column names except for the auto-incremented column, which is omitted.
    sql = "CREATE TABLE Friends(Id INTEGER PRIMARY KEY, Name TEXT);"
        "INSERT INTO Friends(Name) VALUES ('Tom');"
        "INSERT INTO Friends(Name) VALUES ('Rebecca');"
        "INSERT INTO Friends(Name) VALUES ('Jim');"
        "INSERT INTO Friends(Name) VALUES ('Roger');"
        "INSERT INTO Friends(Name) VALUES ('Robert');";

    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);

    if (rc != SQLITE_OK ) {
        fprintf(stderr, "Failed to create table\n");
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
    } else {
        fprintf(stdout, "Table Friends created successfully\n");
    }

    // Returns the row Id of the most recent successfull insert into the table
    int last_id = sqlite3_last_insert_rowid(db);
    printf("The last Id of the inserted row is %d\n", last_id);


    // Question mark (?) is used as a placeholder which is later replaced with an actual value
    sql = "SELECT Id, Name FROM Cars WHERE Id = ?";

    // compiles the SQL query
    rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);

    if (rc == SQLITE_OK) {
        // Binds an integer value to the prepared statement.
        // The placeholder is replaced with integer value 3
        //
        // The second parameter is the index of the SQL parameter to be set
        // The third parameter is the value to bind to the parameter
        sqlite3_bind_int(res, 1, 3);
    } else {
        fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
    }

    step = sqlite3_step(res);

    if (step == SQLITE_ROW) {

        printf("%s: ", sqlite3_column_text(res, 0));
        printf("%s\n", sqlite3_column_text(res, 1));

    }

    sqlite3_finalize(res);


    // Named placeholders are prefixed with the colon (:) character or the at-sign (@) character
    sql = "SELECT Id, Name FROM Cars WHERE Id = @id";

    rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);

    if (rc == SQLITE_OK) {
        // returns the index of an SQL parameter given its name
        int idx = sqlite3_bind_parameter_index(res, "@id");
        int value = 4;
        sqlite3_bind_int(res, idx, value);

    } else {

        fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
    }

    step = sqlite3_step(res);

    if (step == SQLITE_ROW) {

        printf("%s: ", sqlite3_column_text(res, 0));
        printf("%s\n", sqlite3_column_text(res, 1));

    }

    sqlite3_finalize(res);

    sqlite3_close(db);

    return 0;
}

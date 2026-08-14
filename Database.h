#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include <string>
#include <iostream>

class Database {
public:
    static bool executeSQL(sqlite3* db, const std::string& sql);
    static bool DBCheck(sqlite3* db);
    static std::string getTextOrEmpty(sqlite3_stmt* stmt, int col);
};

#endif
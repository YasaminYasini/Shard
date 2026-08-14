# include "Database.h"
# include <iostream>
# include <sqlite3.h>
# include <string>
# include <memory>
# include <vector>


// SQLite helper function
bool Database::executeSQL(sqlite3* db, const std::string& sql) 
{
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL Error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}


bool Database::DBCheck(sqlite3* db)
{
    if (!db) 
    {
    std::cerr << "Database not open." << std::endl;
    return false;
    }
    return true;
}

std::string Database::getTextOrEmpty(sqlite3_stmt* stmt, int col) 
{
    const unsigned char* text = sqlite3_column_text(stmt, col);
    return (text) ? reinterpret_cast<const char*>(text) : "";
}
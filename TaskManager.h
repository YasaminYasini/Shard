# include <iostream>
# include <sqlite3.h>
# include <string>
# include <memory>
# include "TaskConf.h"


struct Task {
    int id;
    int parentId;
    int level;
    std::string status;
    int priority;
    int progress;
    std::string info;
};

class TaskManager 
{
private:
    sqlite3* db;

    bool executeSQL(const std::string& sql) 
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

public:
    TaskManager(const std::string& dbPath)
    {
        int rc = sqlite3_open(dbPath.c_str(), &db);
        if (rc != SQLITE_OK)
        {
            std::cerr <<"Cannot open database: "<< sqlite3_errmsg(db) << std::endl;
            db = nullptr;
            return;
        }

        executeSQL("PRAGMA foreign_keys = ON;");

        const char* createTableSQL = R"(
        CREATE TABLE Task (
            TaskID    INTEGER PRIMARY KEY CHECK( TaskID >= 1 ),
            ParentID  INTEGER,
            Level     INTEGER DEFAULT 1 CHECK( Level >= 1 AND Level <= 3 ),
            TaskInfo  TEXT NOT NULL,
            Status    TEXT DEFAULT 'pending' CHECK( Status IN ('pending','active','done') ),
            Priority  INTEGER DEFAULT 3 CHECK( Priority >= 1 AND Priority <= 5 ),
            Progress  INTEGER DEFAULT 0 CHECK( Progress >= 0 AND Progress <= 100 ),
            FOREIGN KEY (ParentID) REFERENCES Task(TaskID) ON DELETE CASCADE
        );
        )";

        executeSQL(createTableSQL);


        executeSQL("CREATE INDEX IF NOT EXISTS idx_tasks_parentID ON Task(ParentID);");
    }

    TaskManager()
    {
        if (db)
        {
            sqlite3_close(db);
        }
    }

    int getTaskLevel(int TaskID)
    {
        if (!db) return -1;
        
        const char* searchDB = "SELECT level FROM Task WHERE TaskID = ?;";
        sqlite3_stmt* stmt;

        int rc = sqlite3_prepare_v2(db, searchDB, -1, &stmt, nullptr);
        if (rc !=SQLITE_OK)
        {
            return -1;
        }
        sqlite3_bind_int(stmt, 1, TaskID);
        rc = sqlite3_step(stmt);

        int level = -1;
        if (rc == SQLITE_ROW)
        {
            level = sqlite3_column_int(stmt, 0);
        } 

        sqlite3_finalize(stmt);
        return level;
    }

    bool addTask(
        const std::string& taskInfo,
        int priority = 3,
        const std::string& status = "pending",
        int progress = 0,
        int parentID = -1
    )
    {
        if (!db) return -1;

        int parent_level;

        if (parentID != -1)
        {
            parent_level = getTaskLevel(parentID);
            if (parent_level==-1)
            {
                std::cerr << "Supertask not found. " << std::endl;
                return false;
            }

            if (parent_level >= TaskLimits::MAX_LEVEL)
            {
                std::cerr << "Max depth reached. " << std::endl;
                return false;
            }
        }

        sqlite3_stmt* stmt;
        const char* sql = R"(
        INSERT INTO Task (ParentID, Level, TaskInfo, Status, Priority, Progress)
        VALUES (?, ?, ?, ?, ?, ?);
        )";

        int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

        if (rc != SQLITE_OK)
        {
            std::cerr << "Failed to prepare statement. " << sqlite3_errmsg(db) << std::endl;
            return false;
        }

        int newlevel = (parentID == -1) ? 1 : parent_level + 1;

        if (parentID == -1) sqlite3_bind_null(stmt, 1); sqlite3_bind_int(stmt, 1, parentID);
        sqlite3_bind_int(stmt, 2, newlevel);
        sqlite3_bind_text(stmt, 3, taskInfo.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, status.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 5, priority);
        sqlite3_bind_int(stmt, 6, progress);

        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        if (rc!=SQLITE_DONE)
        {
            std::cerr << "Failed to insert task. " << sqlite3_errmsg(db) << std::endl;
            return false;
        }

        std::cout << "Inserted successfully" << std::endl;
        return true;
    }

    std::vector<Task> SearchTask(
        const std::string& keyword = "",
        const std:: string& status = "",
        int priority = -1,
        int progress = -1,
        int parentID = -1,
        int level = -1
    )
    {   
        sqlite3_stmt* stmt;
        std::string sql = "SELECT * FROM Task WHERE 1=1";
        
        if (!keyword.empty()){
            sql += "AND TaskInfo LIKE '%' || ? || '%'";
        }
        if (priority != -1) {
            sql += " AND Priority = ?";
        }
        if (!status.empty()) {
            sql += " AND Status = ?";
        }
        if (parentID != -1) {
            sql += " AND ParentID = ?";
        }
        if (level != -1) {
            sql += " AND Level = ?";
        }

        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);

        if (rc!=SQLITE_OK)
        {
            std::cerr << "Unable to find any value. " << sqlite3_errmsg(db) << std::endl;
            return;
        }

        if (!keyword.empty()) sqlite3_bind_text(stmt, 1, keyword.c_str(), -1, SQLITE_TRANSIENT);
        if (priority != -1) sqlite3_bind_int(stmt, 2, priority);
        if (!status.empty()) sqlite3_bind_text(stmt, 3, status.c_str(), -1, SQLITE_TRANSIENT);
        if (parentID != -1) sqlite3_bind_int(stmt, 4, parentID);
        if (level != -1) sqlite3_bind_int(stmt, 5, level);

        
    }
};
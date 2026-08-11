# include "TaskManager.h"
# include "TaskConf.h"
# include <iostream>
# include <sqlite3.h>
# include <string>
# include <memory>
# include <vector>



// SQLite helper function
bool TaskManager::executeSQL(const std::string& sql) 
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


bool TaskManager::DBCheck()
{
    if (!db) {
    std::cerr << "Database not open." << std::endl;
    return false;
}
return true;
}


// constructor
TaskManager::TaskManager(const std::string& dbPath)
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
    CREATE TABLE IF NOT EXISTS Task (
        TaskID    INTEGER PRIMARY KEY CHECK( TaskID >= 1 ),
        ParentID  INTEGER,
        Level     INTEGER DEFAULT 1 CHECK( Level >= 1 AND Level <= 3 ),
        TaskInfo  TEXT NOT NULL,
        Status    TEXT DEFAULT 'pending' CHECK( Status IN ('pending','active','done') ),
        Priority  INTEGER DEFAULT 3 CHECK( Priority >= 1 AND Priority <= 5 ),
        Progress  INTEGER DEFAULT 0 CHECK( Progress >= 0 AND Progress <= 100 ),
        CreatedDate TEXT DEFAULT (date('now')),
        Deadline  TEXT,
        Category  TEXT  DEFAULT 'General',
        FOREIGN KEY (ParentID) REFERENCES Task(TaskID) ON DELETE CASCADE
    );
    )";

    executeSQL(createTableSQL);
    executeSQL("CREATE INDEX IF NOT EXISTS idx_tasks_parentID ON Task(ParentID);");
}


// destructor
TaskManager::~TaskManager()
{
    if (db)
    {
        sqlite3_close(db);
    }
}


// task existence verifier. also returns level.
int TaskManager::getTaskLevel(int TaskID)
{
    if (!DBCheck()) return -1;
    
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


bool TaskManager::addTask(
    const std::string& taskInfo,
    int priority,
    const std::string& status,
    int progress,
    int parentID,
    const std::string& category,
    const std::string& deadline,
    const std::string& creation
)
{
    if (!DBCheck()) return false;
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

    std::string sql = "INSERT INTO Task (ParentID, Level, TaskInfo, Status, Priority, Progress, Category";
    sql += (!deadline.empty()) ? ", Deadline" : "";
    sql += (!creation.empty()) ? ", CreatedDate": "";
    sql += ") VALUES (?, ?, ?, ?, ?, ?, ?";
    sql += (!deadline.empty()) ? ", ?" : "";
    sql += (!creation.empty()) ? ", ?": "";
    sql += ");";

    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK)
    {
        std::cerr << "Failed to prepare statement. " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    int newlevel = (parentID == -1) ? 1 : parent_level + 1;
    int binder = 1;

    if (parentID == -1) 
    {
        sqlite3_bind_null(stmt, binder++);
    }else
    {
        sqlite3_bind_int(stmt, binder++, parentID);
    }

    sqlite3_bind_int(stmt, binder++, newlevel);
    sqlite3_bind_text(stmt, binder++, taskInfo.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, binder++, status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, binder++, priority);
    sqlite3_bind_int(stmt, binder++, progress);
    sqlite3_bind_text(stmt, binder++, category.c_str(), -1, SQLITE_TRANSIENT);
    if (!deadline.empty()) sqlite3_bind_text(stmt, binder++, deadline.c_str(), -1, SQLITE_TRANSIENT);
    if (!creation.empty()) sqlite3_bind_text(stmt, binder++, creation.c_str(), -1, SQLITE_TRANSIENT);

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

std::string TaskManager::getTextOrEmpty(sqlite3_stmt* stmt, int col) {
    const unsigned char* text = sqlite3_column_text(stmt, col);
    return (text) ? reinterpret_cast<const char*>(text) : "";
}

std::vector<Task> TaskManager::searchTask(
    const std::string& keyword,
    const std:: string& status,
    int priority,
    int progress,
    int parentID,
    int level,
    const std::string& deadline,
    const std::string& creation,
    const std::string& category
)
{   
    std::vector<Task> results;
    if (!DBCheck()) return results;
    
    sqlite3_stmt* stmt;
    std::string sql = "SELECT * FROM Task WHERE 1=1";
    
    if (!keyword.empty()) sql += " AND TaskInfo LIKE '%' || ? || '%'";
    if (priority != -1) sql += " AND Priority = ?";
    if (progress!= -1) sql += " AND Progress = ?";
    if (!status.empty()) sql += " AND Status = ?";
    if (parentID != -1) sql += " AND ParentID = ?";
    if (level != -1) sql += " AND Level = ?";
    if (!deadline.empty()) sql += " AND Deadline = ?";
    if (!creation.empty()) sql += " AND CreatedDate = ?";        
    if (!category.empty()) sql += " AND Category = ?";        
    sql += ";";

    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);

    if (rc!=SQLITE_OK)
    {
        std::cerr << "Unable to find any value. " << sqlite3_errmsg(db) << std::endl;
        return results;
    }

    int binder =1;
    if (!keyword.empty()) sqlite3_bind_text(stmt, binder++, keyword.c_str(), -1, SQLITE_TRANSIENT);
    if (priority != -1) sqlite3_bind_int(stmt, binder++, priority);
    if (progress != -1) sqlite3_bind_int(stmt, binder++, progress);
    if (!status.empty()) sqlite3_bind_text(stmt, binder++, status.c_str(), -1, SQLITE_TRANSIENT);
    if (parentID != -1) sqlite3_bind_int(stmt, binder++, parentID);
    if (level != -1) sqlite3_bind_int(stmt, binder++, level);
    if (!deadline.empty()) sqlite3_bind_text(stmt, binder++, deadline.c_str(), -1, SQLITE_TRANSIENT);
    if (!creation.empty()) sqlite3_bind_text(stmt, binder++, creation.c_str(), -1, SQLITE_TRANSIENT);        
    if (!category.empty()) sqlite3_bind_text(stmt, binder++, category.c_str(), -1, SQLITE_TRANSIENT); 

    while (sqlite3_step(stmt) == SQLITE_ROW) {
            Task task;
            task.id = sqlite3_column_int(stmt, 0);
            task.parentId = sqlite3_column_int(stmt, 1);
            task.level = sqlite3_column_int(stmt, 2);
            task.info = getTextOrEmpty(stmt, 3);
            task.status = getTextOrEmpty(stmt, 4);
            task.priority = sqlite3_column_int(stmt, 5);
            task.progress = sqlite3_column_int(stmt, 6);
            task.createdDate =getTextOrEmpty(stmt, 7);
            task.deadline =getTextOrEmpty(stmt, 8);
            task.category =getTextOrEmpty(stmt, 9);
            
            
            results.push_back(task);
        }

    sqlite3_finalize(stmt);
    return results;
}


bool TaskManager::deleteTask(int taskID)
{
    if (!DBCheck()) return false;

    if (getTaskLevel(taskID) == -1) 
    {
        std::cerr << "TaskID "<< taskID << "not found. "<< std::endl;
        return false;
    }

    sqlite3_stmt* stmt;

    const char* sql = "DELETE FROM Task WHERE TaskID = ?";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK )
    {
        std::cerr << "Failed to prepare delete: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, taskID);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE)
    {
        std::cerr << "Failed to delete task: " << sqlite3_errmsg(db) << std::endl;
        return false ;
    }
    
    int changes = sqlite3_changes(db);
    if (changes == 0) 
    {
        std::cerr << "Task ID " << taskID << " not found." << std::endl;
        return false;
    }
    return true; 
    
}


bool TaskManager::updateTask(
    int taskID,
    const std::string& taskInfo,
    const std::string& status,
    const std::string& creation,
    const std::string& deadline,
    const std::string& category,
    int priority,
    int progress
) 
{
    if (!DBCheck()) return false;
    if (getTaskLevel(taskID) == -1) 
    {
        std::cerr << "Task ID " << taskID << " not found." << std::endl;
        return false;
    }
    std::string sql = "UPDATE Task SET ";
    std::vector<std::string> updates;

    // Only add fields that are provided (non-default values)
    if (!taskInfo.empty()) updates.push_back("TaskInfo = ?");
    if (!status.empty()) updates.push_back("Status = ?");
    if (!creation.empty()) updates.push_back("CreatedDate = ?");
    if (!deadline.empty()) updates.push_back("Deadline = ?");
    if (!category.empty()) updates.push_back("Category = ?");
    if (priority != -1) updates.push_back("Priority = ?");
    if (progress != -1) updates.push_back("Progress = ?");

    if (updates.empty()) 
    {
        std::cerr << "No fields to update." << std::endl;
        return false;
    }

    for (size_t i = 0; i < updates.size(); ++i) 
    {
        if (i > 0) sql += ", ";
        sql += updates[i];
    }

    sql += " WHERE TaskID = ?;";
    sqlite3_stmt* stmt;

    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) 
    {
        std::cerr << "Failed to prepare update: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    int bindIndex = 1;
    if (!taskInfo.empty()) sqlite3_bind_text(stmt, bindIndex++, taskInfo.c_str(), -1, SQLITE_TRANSIENT);
    if (!status.empty()) sqlite3_bind_text(stmt, bindIndex++, status.c_str(), -1, SQLITE_TRANSIENT);
    if (!creation.empty()) sqlite3_bind_text(stmt, bindIndex++, creation.c_str(), -1, SQLITE_TRANSIENT);
    if (!deadline.empty()) sqlite3_bind_text(stmt, bindIndex++, deadline.c_str(), -1, SQLITE_TRANSIENT);
    if (!category.empty()) sqlite3_bind_text(stmt, bindIndex++, category.c_str(), -1, SQLITE_TRANSIENT);
    if (priority != -1) sqlite3_bind_int(stmt, bindIndex++, priority);
    if (progress != -1) sqlite3_bind_int(stmt, bindIndex++, progress);


    sqlite3_bind_int(stmt, bindIndex, taskID);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) 
    {
        std::cerr << "Failed to update task: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    int changes = sqlite3_changes(db);

    if (changes == 0) 
    {
        std::cerr << "Task ID " << taskID << " not found or no changes made." << std::endl;
        return false;
    }

    std::cout << "Task " << taskID << " updated successfully." << std::endl;
    return true;
}

std::vector<Task> TaskManager::returnAllTasks()
{
    return searchTask();
}

std::vector<Task> TaskManager::getSubTasks(int TaskID)
{
   return searchTask("","",-1,-1, TaskID, -1,"","","");
}

Task TaskManager::retrieveTask(int taskID) {
    Task result;

    if (!DBCheck()) return result;

    const char* sql = "SELECT * FROM Task WHERE TaskID = ?;";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return result;
    }

    sqlite3_bind_int(stmt, 1, taskID);
    rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW) {
        result.id = sqlite3_column_int(stmt, 0);
        result.parentId = sqlite3_column_int(stmt, 1);
        result.level = sqlite3_column_int(stmt, 2);
        result.info = getTextOrEmpty(stmt, 3);
        result.status = getTextOrEmpty(stmt, 4);
        result.priority = sqlite3_column_int(stmt, 5);
        result.progress = sqlite3_column_int(stmt, 6);
        result.category = getTextOrEmpty(stmt, 7);
        result.createdDate = getTextOrEmpty(stmt, 8);
        result.deadline = getTextOrEmpty(stmt, 9);
    }

    sqlite3_finalize(stmt);
    return result;  // If not found, result.id will be 0
}
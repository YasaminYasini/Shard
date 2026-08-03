#ifndef TASKMANAGER_H
#define TASKMANAGER_H

#include <sqlite3.h>
#include <string>
#include <vector>

struct Task 
{
    int id;
    int parentId;
    int level;
    std::string info;
};

namespace TaskLimits 
{
    inline constexpr int MAX_LEVEL=3;
}

class TaskManager 
{
private:
    sqlite3* db;
    bool executeSQL(const std::string& sql);

public:
    TaskManager(const std::string& dbPath);
    ~TaskManager();

    bool addTask(const std::string& taskInfo, int priority = 3, const std::string& status = "pending", int progress = 0, int parentID = -1);
    std::vector<Task> getSubTasks(int parentID);
    std::vector<Task> searchTask(const std::string& keyword = "");
    bool deleteTask(int taskID);
    bool updateTask(int taskID, const std::string& taskInfo = "", const std::string& status = "", const std::string& creation = "", const std::string& deadline = "", const std::string& category = "", int priority = -1, int progress = -1  )
    // ... other declarations ...
};

#endif  
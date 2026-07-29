#ifndef TASKMANAGER_H
#define TASKMANAGER_H

#include <sqlite3.h>
#include <string>
#include <vector>

struct Task {
    int id;
    int parentId;
    int level;
    std::string info;
};

namespace TaskLimits {
    inline constexpr int MAX_LEVEL=3;
}

class TaskManager {
private:
    sqlite3* db;
    bool executeSQL(const std::string& sql);

public:
    TaskManager(const std::string& dbPath);
    ~TaskManager();

    bool addTask(const std::string& info, int priority = 3);
    std::vector<Task> getSubTasks(int parentID);
    std::vector<Task> searchTasks(const std::string& keyword = "");
    bool deleteTask(int taskID);
    // ... other declarations ...
};

#endif
#ifndef TASKMANAGER_H
#define TASKMANAGER_H

#include <sqlite3.h>
#include <string>
#include <vector>

struct Task {
    int id;
    int parentId;
    int level;
    std::string createdDate;
    std::string deadline;
    std::string category;
    std::string status;
    int priority;
    int progress;
    std::string info;

    // Default constructor (sets everything to safe default values)
    Task()
        : id(0),
          parentId(-1),
          level(0),
          priority(3),
          progress(0) {}
};

class TaskManager 
{
private:
    sqlite3* db;
    bool executeSQL(const std::string& sql);

    bool DBCheck();
public:
    TaskManager(const std::string& dbPath);

    ~TaskManager();

    int getTaskLevel(int TaskID);
    
    bool addTask(
        const std::string& taskInfo,
        int priority = 3,
        const std::string& status = "pending",
        int progress = 0, int parentID = -1,
        const std::string& category = "General",
        const std::string& deadline = "",
        const std::string& creation = ""
    );

    std::vector<Task> getSubTasks(int parentID);

    std::vector<Task> searchTask(
        const std::string& keyword = "", 
        const std:: string& status = "", 
        int priority = -1, int progress = -1, 
        int parentID = -1, int level = -1, 
        const std::string& deadline = "", 
        const std::string& creation = "", 
        const std::string& category = ""
    );
    
    bool deleteTask(int taskID);

    bool updateTask(int taskID, 
        const std::string& taskInfo = "", 
        const std::string& status = "", 
        const std::string& creation = "", 
        const std::string& deadline = "", 
        const std::string& category = "", 
        int priority = -1, int progress = -1 
    );

};

#endif  
#include <iostream>
#include <string> 
#include <vector>
#include "TaskConf.h"
#include <algorithm>

enum stats {notStarted, inProgress, done};

class Task {

private:
    int taskID;
    std::string name;
    int priority;
    stats status;
    int progress;
    std::vector<Task> subtasks;
    int level;

    static int generateID()
    {
        static int nextID = 1;
        return nextID++;
    }

    // only addSubTask can access this due to max depth in tree.
    Task(const std::string& taskName, int taskLevel = 1, stats taskStatus = notStarted, int prior = 1, int percentage = 0)
        : taskID(generateID()), name(taskName), level(taskLevel), status(taskStatus), priority(prior), progress(percentage) 
    {
        setTaskPercentage(percentage);
        setTaskPriority(prior);
        setTaskStatus(status);
            
    }
 public:
    
    //constructor
    Task(const std::string& taskName, stats taskStatus = notStarted, int prior = 1, int percentage = 0)
        :  taskID(generateID()), name(taskName), status(taskStatus), priority(prior), progress(percentage)
    {
        level = 1;
        setTaskPercentage(percentage);
        setTaskPriority(prior);
        setTaskStatus(status);
            
    }

    // Mutators


    void setTaskStatus(stats taskStatus)
    {
        status = taskStatus;
    }

    void setTaskPriority(int taskPriority)
    {
        if (taskPriority < 1) taskPriority = 1;
        if (taskPriority > 5) taskPriority = 5;
        priority = taskPriority;
    }

    void setTaskPercentage(int taskPercentage)
    {
        if (taskPercentage < 0) taskPercentage = 0;
        if (taskPercentage > 100) taskPercentage = 100;
        progress = taskPercentage;
    }

    // Getters
    std::string name() const { return name; }
    int priority() const { return priority; }
    stats status() const { return status; }
    int progress() const { return progress; }
    int level() const { return level; }
    const std::vector<Task>& subtasks() const { return subtasks; }
    int id() const { return taskID; }
    const Task& subtaskSelector(size_t index) const { return subtasks.at(index); }

    // functionalities
    bool addSubTask(const std::string& subTaskName)
    {
        if (level < TaskLimits::MAX_LEVEL) {
            subtasks.emplace_back(subTaskName, level+1);
            return true;
        }
        return false;
    }

    bool deleteSubTask(size_t index)
    {
        if (index < subtasks.size())
        {
            subtasks.erase(subtasks.begin()+ index);
            return true;
        }
        return false;
    }

    // use the subtask as fast as you store it.
    Task &subtaskSelector (size_t index)
    {
        return subtasks.at(index);
    }

};


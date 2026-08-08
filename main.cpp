#include "TaskManager.h"
#include "TaskConf.h"
#include <iostream>
#include <vector>
#include <string>

int main()
{
    std::cout << "Welcome home, Master. proceeding..." << std::endl;

    // Creating DB manager
    TaskManager tm("Tasks.db");
    std::cout << "Database opened successfully." << std::endl;

    // Add a task
    bool success = tm.addTask("learn C++", 5, "active", 10);
    if (success) {
        std::cout << "Task created successfully." << std::endl;
    } else {
        std::cout << "Failed to create task." << std::endl;
        return 1;
    }

    // Search for the task
    std::cout << "Proceeding to search tasks..." << std::endl;
    std::vector<Task> tasks = tm.searchTask("C++");

    if (tasks.empty()) {
        std::cout << "No tasks found." << std::endl;
        return 1;
    }

    // Display all found tasks
    for (const Task& task : tasks) {
        std::cout << "ID: " << task.id
                  << "   Info: " << task.info
                  << "   Status: " << task.status << std::endl;
    }

    // Use the actual task ID from the search result
    int taskId = tasks[0].id;

    // Update the task
    success = tm.updateTask(taskId, "learn SQLite");
    if (success) {
        std::cout << "Task modified successfully." << std::endl;
    } else {
        std::cout << "Failed to modify task." << std::endl;
    }

    // Delete the task
    success = tm.deleteTask(taskId);
    if (success) {
        std::cout << "Task removed successfully." << std::endl;
    } else {
        std::cout << "Failed to remove task." << std::endl;
    }

    return 0;
}
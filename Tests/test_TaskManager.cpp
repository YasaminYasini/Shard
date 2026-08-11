#include "../TaskManager.h"
#include <iostream>
#include <cassert>
#include <string>
#include <unistd.h>


std::string getTestDBPath()
{
    return "test_addTask_" + std::to_string(getpid()) + ".db";
}


void cleanup(const std::string& dbPath)
{
    remove(dbPath.c_str());
}


void printPass(const std::string& msg)
{
    std::cout << "[PASS]" << msg << std::endl;
}


void printFail(const std::string& msg)
{
    std::cout << "[FAIL]" << msg << std::endl;
}


bool testAddTask(const std::string& dbPath)
{
    //setup
    std::cout << "\n==== Testing addTask() ====" << std::endl;
    TaskManager tm(dbPath);

    //to avoid overlap with previous runs
    remove(dbPath.c_str());
    TaskManager tmFresh(dbPath);

    bool allPassed = true;

/* Case I   : adding a root task */
    std::cout << "\n Case I: root task" << std::endl;
    
    //verifying the success of the transaction
    bool result = tmFresh.addTask("Learn C++", 5, "active", 50);
    (result)? printPass("addTask() returned True"): printFail("addTask() returned False");

    auto tasks = tmFresh.searchTask("Learn C++");
    //verifying the existance of the task in the tasks table
    if (tasks.size() == 1) 
    {
        printPass("Task found in database");
    } else 
    {
        printFail("Task not found in database (size: " + std::to_string(tasks.size()) + ")");
        allPassed = false;
    }

    // verifying the correctness of the values
    if (!tasks.empty()) 
    {
        const Task& task = tasks[0];
        if (task.info == "Learn C++") 
        {
            printPass("Task info is correct");
        } else 
        {
            printFail("Task info is '" + task.info + "' (expected 'Learn C++')");
            allPassed = false;
        }

        if (task.priority == 5) 
        {
            printPass("Priority is correct");
        } else 
        {
            printFail("Priority is " + std::to_string(task.priority) + " (expected 5)");
            allPassed = false;
        }

        if (task.status == "active") 
        {
            printPass("Status is correct");
        } else 
        {
            printFail("Status is '" + task.status + "' (expected 'active')");
            allPassed = false;
        }

        if (task.progress == 50) 
        {
            printPass("Progress is correct");
        } else 
        {
            printFail("Progress is " + std::to_string(task.progress) + " (expected 50)");
            allPassed = false;
        }

        if (task.parentId == 0) 
        {
            printPass("Parent ID is -1 (root task)");
        } else 
        {
            printFail("Parent ID is " + std::to_string(task.parentId) + " (expected -1)");
            allPassed = false;
        }

        if (task.level == 1)
        {
            printPass("Level is 1");
        } else
        {
            printFail("Level is " + std::to_string(task.level) + " (expected 1)");
            allPassed = false;
        }
    }

    int taskId = tasks.empty()? -1 : tasks[0].id;

/* Case II  : adding a subtask */
    std::cout << "\n Case II: subtask of depth 2" << std::endl;
    if (taskId != -1)
    {
        result = tmFresh.addTask("Understand pointers", 4, "pending", 0, taskId);
        if (result)
        {
            printPass("addTask() for subtask returned True");
        } else
        {
            printFail("addTask() for subtask returned False");
            allPassed = false;
        }

        auto children = tmFresh.getSubTasks(taskId);
        if (children.size() == 1) 
        {
            printPass("Subtask found under parent");
        } else 
        {
            printFail("Subtask not found under parent (size: " + std::to_string(children.size()) + ")");
            allPassed = false;
        }

        if (!children.empty())
        {
            const Task& child = children[0];
            if (child.parentId == taskId)
            {
                printPass("subtask has the right parent ID");
            } else
            {
                printFail("subtask parent ID is: "+std::to_string(child.parentId)+ " (expected: "+std::to_string(taskId)+")");
                allPassed = false;
            }

            if(child.level == 2)
            {
                printPass("subtask level is 2");
            } else
            {
                printFail("subtask level " + std::to_string(child.level) +" (expected: 2)");
                allPassed = false;
            }
        }
    } else
    {
        printFail("Skipping subtask due to an issue in the previous test.");
        allPassed = false;
    }

/* Case III : adding a task with max depth*/
    std::cout << "\nCase III: subtask with max depth (3) " << std::endl;
    
    if (taskId != -1)
    {
        auto children = tmFresh.getSubTasks(taskId);
        if (!children.empty())
        {
            int childId = children[0].id;

            result = tmFresh.addTask("dynamic pointer", 3, "pending", 0, childId);
            if (result)
            {
                printPass("addTask() for level 3 subtask returned true");
            } else
            {
                printFail("addTask() for level 3 subtask returned false");
                allPassed = false;
            }

            auto grandchildren = tmFresh.getSubTasks(childId);
            if (!grandchildren.empty())
            {
                const Task& grandchild = grandchildren[0];
                if (grandchild.level == 3)
                {
                    printPass("Grandchild level is 3");
                } else
                {
                    printFail("grandchild level "+ std::to_string(grandchild.level) +" (expected: 3)");
                    allPassed = false;
                }
            } else
            {
                printFail("Grandchild not found");
                allPassed = false;
            }

/* Case IV  : adding a task beyond max depth*/
            std::cout << "\nCase IV: adding a task beyond max depth." << std::endl;
            int grandchildID = grandchildren.empty() ? -1 : grandchildren[0].id;
            if (grandchildID != -1)
            {
                result = tmFresh.addTask("why are we even here?", 3, "pending", 0, grandchildID);
                if (!result)
                {
                    printPass("addTask() correctly rejected depth beyond 3");
                } else
                {
                    printFail("addTask() allowed level 4 (should have been rejected)");
                    allPassed = false;
                }
            }
        }
    }

/* Case V   : adding task with invalid parent*/
    std::cout << "\nCase V: adding a task with an invalid parent" << std::endl;

    result = tmFresh.addTask("learn about references", 4, "pending", 0, 9999);
    if (!result)
    {
        printPass("addTask() correctly rejected invalid parent ID");
    } else
    {
        printFail("addTask() accepted invalid parent ID (should have returned false)");
        allPassed = false;
    }

/* Case VI  : adding a task with default parameters*/
    std::cout << "\nCase VI: Add a task with default parameters" << std::endl;

    result = tmFresh.addTask("Default task");
    if (result) 
    {
        printPass("addTask() with default parameters returned true");
    } else 
    {
        printFail("addTask() with default parameters returned false");
        allPassed = false;
    }

    // Verify default values
    auto defaultTasks = tmFresh.searchTask("Default task");
    if (!defaultTasks.empty()) 
    {
        const Task& task = defaultTasks[0];
        if (task.priority == 3) 
        {
            printPass("Default priority is 3");
        } else 
        {
            printFail("Default priority is " + std::to_string(task.priority) + " (expected 3)");
            allPassed = false;
        }

        if (task.status == "pending") 
        {
            printPass("Default status is 'pending'");
        } else 
        {
            printFail("Default status is '" + task.status + "' (expected 'pending')");
            allPassed = false;
        }

        if (task.progress == 0) 
        {
            printPass("Default progress is 0");
        } else 
        {
            printFail("Default progress is " + std::to_string(task.progress) + " (expected 0)");
            allPassed = false;
        }

        if (task.parentId == 0) 
        {
            printPass("Default parent ID is -1 (root task)");
        } else 
        {
            printFail("Default parent ID is " + std::to_string(task.parentId) + " (expected -1)");
            allPassed = false;
        }
    }

    // cleanup
    cleanup(dbPath);
    std::cout << "\n Cleanup: Test database removed." << std::endl;

    // results
    if (allPassed) {
        std::cout << "\n✅ All addTask() scenarios passed!" << std::endl;
    } else {
        std::cout << "\n❌ Some addTask() scenarios failed." << std::endl;
    }


    return allPassed;
}


bool testDeleteTask(const std::string& dbPath)
{
    //setup
    std::cout << "\n==== Testing deleteTask() ====" << std::endl;
    TaskManager tm(dbPath);

    //to avoid overlap with previous runs
    remove(dbPath.c_str());
    TaskManager tmFresh(dbPath);

    bool allPassed = true;

/* Case I   : removing a parent task*/
    std::cout << "\nCase I: removing a root task" << std::endl;

    bool parent = tmFresh.addTask("Learn C++");
    if (parent)
    {
        int parentId = -1;
        auto parentResults = tmFresh.searchTask("Learn C++");
        if (!parentResults.empty()) 
        {
            parentId = parentResults[0].id;
        } else 
        {
            printFail("Parent not found after adding");
            allPassed = false;
        }
        bool result = tmFresh.deleteTask(parentId);
        if (result)
        {
            printPass("deleteTask() for deleting a root task returned true");
        } else
        {
            printFail("deleteTask() for deleting a root task returned false");
            allPassed = false;
        }

        auto parent_exists = tmFresh.searchTask("Learn C++");
        if (parent_exists.empty() || parent_exists[0].id != parentId)
        {
            printPass("deleteTask() successfully removed the root task");
        } else
        {
            printFail("deleteTask() failed to remove the root task");
            allPassed= false;
        }
    } else
    {
        printFail("parent not created");
        allPassed = false;
    }
/* Case II  : removing a parent task with subtasks of level 2 and 3*/
    std::cout << "\nCase II: removing a parent task with children and grandchildren" <<std::endl;
    bool success = tmFresh.addTask("Learn C++");
    if (!success) 
    {
        printFail("failed to add parent task");
        allPassed = false;
    }

    auto parents = tmFresh.searchTask("Learn C++");
    if (parents.empty()) 
    {
        printFail("parent task not found");
        allPassed = false;
    }
    int parentId = parents[0].id;

    success = tmFresh.addTask("Learn referencing", 4, "pending", 0, parentId);
    if (!success) 
    {
        printFail("failed to add child task");
        allPassed = false;
    }

    auto children = tmFresh.getSubTasks(parentId);
    if (children.empty()) 
    {
        printFail("child task not found");
        allPassed = false;
    }
    int childId = children[0].id;

    success = tmFresh.addTask("Master pointers", 3, "pending", 0, childId);
    if (!success) 
    {
        printFail("failed to add grandchild task");
        allPassed = false;
    }

    auto grandchildren = tmFresh.getSubTasks(childId);
    if (grandchildren.empty()) 
    {
        printFail("grandchild task not found");
        allPassed = false;
    }
    int grandchildId = grandchildren[0].id;

    success = tmFresh.deleteTask(parentId);
    if (!success) 
    {
        printFail("deleteTask returned false for cascade delete");
        allPassed = false;
    }

    auto allTasks = tmFresh.searchTask();
    if (allTasks.empty()) 
    {
        printPass("All tasks deleted (cascade worked)");
    } else 
    {
        printFail("Cascade delete left " + std::to_string(allTasks.size()) + " tasks behind");
        allPassed = false;
    }

/* Case III : removing a nonexistent task*/
    std::cout << "\nCase III: removing an invalid task" << std::endl;
    
    allTasks = tmFresh.searchTask();
    bool result = tmFresh.deleteTask(9999);
    if (!result)
    {
        printPass("deleteTask() for deleting invalid task returned the correct value false");
    } else
    {
        printFail("deleteTask() for deleting invalid task returned true (expected false)");
        allPassed = false;
    }

    auto modifiedTasks = tmFresh.searchTask();
    if (allTasks == modifiedTasks)
    {
        printPass("deleteTask() did not modify the Task table during invalid task removal");
    } else
    {
        printFail("deleteTask() modified the Task table during invalid task removal");
    }

/* Case IV  : removing a subtask directly*/
    std::cout << "\nCase IV: removing a child directly" << std::endl;

    success = tmFresh.addTask("Learn C++");
    if (!success) 
    {
        printFail("failed to add parent task");
        allPassed = false;
    }

    parents = tmFresh.searchTask("Learn C++");
    if (parents.empty()) 
    {
        printFail("parent task not found");
        allPassed = false;
    }
    parentId = parents[0].id;

    success = tmFresh.addTask("Learn referencing", 4, "pending", 0, parentId);
    if (!success) 
    {
        printFail("failed to add child task");
        allPassed = false;
    }

    children = tmFresh.getSubTasks(parentId);
    if (children.empty()) 
    {
        printFail("child task not found");
        allPassed = false;
    }
    childId = children[0].id;

    result = tmFresh.deleteTask(childId);

    if (result)
    {
        printPass("deleteTask() for removing child task returned true");
    } else
    {
        printFail("deleteTask() for removing child task returned false");
        allPassed = false;
    }

    auto parentCheck = tmFresh.searchTask("Learn C++");
    if (!parentCheck.empty())
    {
        auto children_modified = tmFresh.getSubTasks(parentId);
        if (children != children_modified)
        {
            printPass("deleteTask() successfully removed child task (parent remains intact)");
        } else
        {
            printFail("deleteTask() failed to remove child task");
            allPassed = false;
        }
    } else
    {
        printFail("deleteTask() affected parent task while removing child task");
        allPassed = false;
    }

    // results
    if (allPassed) {
        std::cout << "\n✅ All deleteTask() scenarios passed!" << std::endl;
    } else {
        std::cout << "\n❌ Some deleteTask() scenarios failed." << std::endl;
    }


    return allPassed;

}


bool testUpdateTask(const std::string& dbPath) {
    std::cout << "\n==== Testing updateTask() ====" << std::endl;

    // Clean up any existing test database
    remove(dbPath.c_str());
    TaskManager tmFresh(dbPath);
    bool allPassed = true;
    bool success = false;

/* Case I   : update a single field (title)*/
    std::cout << "\nCase I: Update one field only (Title was selected here)" << std::endl;

    success = tmFresh.addTask("Original Title", 5, "active", 50);
    if (!success) 
    {
        printFail("Failed to add task");
        allPassed = false;
    }

    auto tasks = tmFresh.searchTask("Original Title");
    if (tasks.empty()) 
    {
        printFail("Task not found");
        allPassed = false;
    } else 
    {
        int taskId = tasks[0].id;

        success = tmFresh.updateTask(taskId, "Updated Title");
        if (success) 
        {
            printPass("updateTask() returned true");
        } else 
        {
            printFail("updateTask() returned false");
            allPassed = false;
        }

        auto updated = tmFresh.searchTask("Updated Title");
        if (updated.empty()) 
        {
            printFail("Updated task not found");
            allPassed = false;
        } else 
        {
            const Task& task = updated[0];
            if (task.info == "Updated Title") 
            {
                printPass("Title updated correctly");
            } else 
            {
                printFail("Title is '" + task.info + "' (expected 'Updated Title')");
                allPassed = false;
            }

            // Verify other fields unchanged
            if (task.priority == 5) 
            {
                printPass("Priority unchanged (5)");
            } else 
            {
                printFail("Priority changed to " + std::to_string(task.priority));
                allPassed = false;
            }

            if (task.status == "active") 
            {
                printPass("Status unchanged ('active')");
            } else 
            {
                printFail("Status changed to '" + task.status + "'");
                allPassed = false;
            }

            if (task.progress == 50) 
            {
                printPass("Progress unchanged (50)");
            } else 
            {
                printFail("Progress changed to " + std::to_string(task.progress));
                allPassed = false;
            }
        }
    }

/* Case II  : update multiple fields*/
    std::cout << "\nCase II: Update multiple fields" << std::endl;

    success = tmFresh.addTask("Task A", 3, "pending", 20);
    if (!success) 
    {
        printFail("Failed to add task");
        allPassed = false;
    }

    auto taskA = tmFresh.searchTask("Task A");
    if (taskA.empty()) 
    {
        printFail("Task not found");
        allPassed = false;
    } else 
    {
        int taskId = taskA[0].id;

        success = tmFresh.updateTask(taskId, "Task B", "done", "", "", "", 5, 100);
        if (success) 
        {
            printPass("updateTask() returned true for multiple fields");
        } else 
        {
            printFail("updateTask() returned false for multiple fields");
            allPassed = false;
        }

        auto updated = tmFresh.searchTask("Task B");
        if (updated.empty()) 
        {
            printFail("Updated task not found");
            allPassed = false;
        } else 
        {
            const Task& task = updated[0];
            if (task.info == "Task B") 
            {
                printPass("Title updated correctly");
            } else 
            {
                printFail("Title is '" + task.info + "' (expected 'Task B')");
                allPassed = false;
            }

            if (task.priority == 5) 
            {
                printPass("Priority updated to 5");
            } else 
            {
                printFail("Priority is " + std::to_string(task.priority) + " (expected 5)");
                allPassed = false;
            }

            if (task.status == "done") 
            {
                printPass("Status updated to 'done'");
            } else 
            {
                printFail("Status is '" + task.status + "' (expected 'done')");
                allPassed = false;
            }

            if (task.progress == 100) 
            {
                printPass("Progress updated to 100");
            } else 
            {
                printFail("Progress is " + std::to_string(task.progress) + " (expected 100)");
                allPassed = false;
            }
        }
    }

/* Case III : update with no changes (should return false)*/
    std::cout << "\nCase III: No-op update (no fields changed)" << std::endl;

    success = tmFresh.addTask("No-op Task", 3, "pending", 0);
    if (!success) 
    {
        printFail("Failed to add task");
        allPassed = false;
    }

    auto noopTasks = tmFresh.searchTask("No-op Task");
    if (noopTasks.empty()) 
    {
        printFail("Task not found");
        allPassed = false;
    } else 
    {
        int taskId = noopTasks[0].id;

        // Call update with no changes
        success = tmFresh.updateTask(taskId);
        if (!success) 
        {
            printPass("updateTask() correctly returned false for no changes");
        } else 
        {
            printFail("updateTask() returned true for no changes (should be false)");
            allPassed = false;
        }

        // Verify the task is still the same
        auto stillThere = tmFresh.searchTask("No-op Task");
        if (stillThere.empty()) 
        {
            printFail("Task was deleted or modified");
            allPassed = false;
        } else 
        {
            const Task& task = stillThere[0];
            if (task.priority == 3 && task.status == "pending" && task.progress == 0) 
            {
                printPass("Task unchanged (correct)");
            } else 
            {
                printFail("Task was modified despite no changes");
                allPassed = false;
            }
        }
    }
/* Case IV  : update with an invalid task ID*/
    std::cout << "\nCase IV: Invalid task ID" << std::endl;

    success = tmFresh.updateTask(9999, "This should fail");
    if (!success) 
    {
        printPass("updateTask() correctly returned false for invalid ID");
    } else 
    {
        printFail("updateTask() returned true for invalid ID (should be false)");
        allPassed = false;
    }
/* Case V   : update with empty string for title (should not change)*/
    std::cout << "\nCase V: Empty title (should be ignored)" << std::endl;

    success = tmFresh.addTask("Preserve Me", 4, "active", 60);
    if (!success) 
    {
        printFail("Failed to add task");
        allPassed = false;
    }

    auto preserveTasks = tmFresh.searchTask("Preserve Me");
    if (preserveTasks.empty()) 
    {
        printFail("Task not found");
        allPassed = false;
    } else 
    {
        int taskId = preserveTasks[0].id;

        // Update with empty title (should be ignored)
        success = tmFresh.updateTask(taskId, "", "done", "", "", "", 5, 80);
        if (success) 
        {
            printPass("updateTask() returned true");
        } else 
        {
            printFail("updateTask() returned false");
            allPassed = false;
        }

        auto updated = tmFresh.searchTask("Preserve Me");
        if (updated.empty()) 
        {
            printFail("Task not found after update");
            allPassed = false;
        } else 
        {
            const Task& task = updated[0];
            if (task.info == "Preserve Me") 
            {
                printPass("Title unchanged (empty string ignored)");
            } else 
            {
                printFail("Title changed to '" + task.info + "' (should be 'Preserve Me')");
                allPassed = false;
            }

            if (task.status == "done") 
            {
                printPass("Status updated to 'done'");
            } else 
            {
                printFail("Status not updated");
                allPassed = false;
            }

            if (task.priority == 5) 
            {
                printPass("Priority updated to 5");
            } else 
            {
                printFail("Priority not updated");
                allPassed = false;
            }
        }
    }
/* Case VI  : update deadline and category*/
    std::cout << "\nCase VI: Update deadline and category" << std::endl;

    success = tmFresh.addTask("Date Task", 3, "pending", 0);
    if (!success) 
    {
        printFail("Failed to add task");
        allPassed = false;
    }

    auto dateTasks = tmFresh.searchTask("Date Task");
    if (dateTasks.empty()) 
    {
        printFail("Task not found");
        allPassed = false;
    } else 
    {
        int taskId = dateTasks[0].id;

        success = tmFresh.updateTask(taskId, "", "", "", "2025-12-31", "Work");
        if (success) 
        {
            printPass("updateTask() returned true for deadline/category");
        } else 
        {
            printFail("updateTask() returned false for deadline/category");
            allPassed = false;
        }

        auto updated = tmFresh.searchTask("Date Task");
        if (updated.empty()) 
        {
            printFail("Task not found after update");
            allPassed = false;
        } else 
        {
            const Task& task = updated[0];
            if (task.deadline == "2025-12-31") 
            {
                printPass("Deadline updated correctly");
            } else 
            {
                printFail("Deadline is '" + task.deadline + "' (expected '2025-12-31')");
                allPassed = false;
            }

            if (task.category == "Work") 
            {
                printPass("Category updated correctly");
            } else 
            {
                printFail("Category is '" + task.category + "' (expected 'Work')");
                allPassed = false;
            }
        }
    }


    // cleanup
    cleanup(dbPath);

    if (allPassed) {
        std::cout << "\n✅ All updateTask() scenarios passed!" << std::endl;
    } else {
        std::cout << "\n❌ Some updateTask() scenarios failed." << std::endl;
    }

    return allPassed;
}


bool testSearchTask(const std::string& dbPath) {
    std::cout << "\n==== Testing searchTask() ====" << std::endl;

    // Clean up any existing test database
    remove(dbPath.c_str());
    TaskManager tmFresh(dbPath);
    bool allPassed = true;
    bool success = false;

    // setup: Add a variety of tasks for search testing
    std::cout << "\nSetup: Adding test tasks..." << std::endl;

    // adding root tasks and returning root IDs
    tmFresh.addTask("Learn C++", 5, "active", 50);
    tmFresh.addTask("Learn SQLite", 4, "active", 30);
    tmFresh.addTask("Build Qt UI", 3, "pending", 0);
    tmFresh.addTask("Write documentation", 2, "done", 100);

    auto roots = tmFresh.searchTask("Learn C++");
    if (roots.empty()) {
        printFail("Setup failed: 'Learn C++' not found");
        allPassed = false;
    }
    int rootId = roots.empty() ? -1 : roots[0].id;

    // adding subtasks (level 2)
    if (rootId != -1) 
    {
        tmFresh.addTask("Understand pointers", 4, "pending", 0, rootId);
        tmFresh.addTask("Master algorithms", 5, "active", 60, rootId);

        auto children = tmFresh.getSubTasks(rootId);
        if (children.size() >= 2) 
        {
            int childId = children[0].id;
            tmFresh.addTask("Advanced pointers", 3, "pending", 0, childId);
        }
    }

    std::cout << "  Setup complete." << std::endl;

/* Case I   : keyword search (exact match)*/
    std::cout << "\nCase I: Keyword search (exact match)" << std::endl;

    auto results = tmFresh.searchTask("Learn C++");
    if (results.size() == 1) 
    {
        printPass("Found exactly 1 task for 'Learn C++'");
        if (results[0].info == "Learn C++") 
        {
            printPass("Correct task found");
        } else 
        {
            printFail("Wrong task found");
            allPassed = false;
        }
    } else 
    {
        printFail("Expected 1 task, found " + std::to_string(results.size()));
        allPassed = false;
    }

/* Case II  : keyword search (partial match)*/
    std::cout << "\nCase II: Keyword search (partial match)" << std::endl;

    results = tmFresh.searchTask("Learn");
    if (results.size() == 2) 
    {
        printPass("Found exactly 2 tasks containing 'Learn'");
    } else 
    {
        printFail("Expected 2 tasks, found " + std::to_string(results.size()));
        allPassed = false;
    }
/* Case III : priority filter*/
    std::cout << "\nCase III: Priority filter" << std::endl;

    results = tmFresh.searchTask("", "", 5);
    if (results.size() == 2) 
    {
        printPass("Found exactly 2 tasks with priority 5");
    } else 
    {
        printFail("Expected 2 tasks, found " + std::to_string(results.size()));
        allPassed = false;
    }

/* Case IV  : status filter*/
    std::cout << "\n  Scenario 4: Status filter" << std::endl;

    results = tmFresh.searchTask("", "active");
    if (results.size() == 3) {
        printPass("Found exactly 3 active tasks");
    } else {
        printFail("Expected 3 tasks, found " + std::to_string(results.size()));
        allPassed = false;
    }

/* Case V: parent ID filter*/
    std::cout << "\nCase V: Parent ID filter" << std::endl;

    if (rootId != -1) {
        results = tmFresh.searchTask("", "", -1, -1, rootId);
        // We added 2 subtasks to root
        if (results.size() == 2) {
            printPass("Found exactly 2 subtasks of root");
        } else {
            printFail("Expected 2 subtasks, found " + std::to_string(results.size()));
            allPassed = false;
        }
    }

/* Case VI  : level filter*/
    std::cout << "\nCase VI: Level filter" << std::endl;

    results = tmFresh.searchTask("", "", -1, -1, -1, 1);
    // We added 4 root tasks
    if (results.size() == 4) {
        printPass("Found exactly 4 level 1 tasks");
    } else {
        printFail("Expected 4 tasks, found " + std::to_string(results.size()));
        allPassed = false;
    }

/* Case VII : combined filters (keyword + status)*/
    std::cout << "\nCase VII: Combined filters (keyword + status)" << std::endl;

    results = tmFresh.searchTask("Learn", "active");
    // "Learn C++" (active) and "Learn SQLite" (active) = 2
    if (results.size() == 2) {
        printPass("Combined filter found 2 tasks");
    } else {
        printFail("Expected 2 tasks, found " + std::to_string(results.size()));
        allPassed = false;
    }

/* Case VIII: empty results*/
    std::cout << "\nCase VIII: Empty results" << std::endl;

    results = tmFresh.searchTask("nonexistent_keyword");
    if (results.empty()) {
        printPass("Empty results returned correctly");
    } else {
        printFail("Expected 0 tasks, found " + std::to_string(results.size()));
        allPassed = false;
    }

/* Case IX  : no filters (get all tasks)*/
    std::cout << "\nCase IX: No filters (get all tasks)" << std::endl;

    results = tmFresh.searchTask();
    // We added 4 roots + 2 subtasks + 1 grandchild = 7 total
    if (results.size() == 7) {
        printPass("No filters returned all 7 tasks");
    } else {
        printFail("Expected 7 tasks, found " + std::to_string(results.size()));
        allPassed = false;
    }

    //cleanup
    cleanup(dbPath);

    if (allPassed) {
        std::cout << "\n✅ All searchTask() scenarios passed!" << std::endl;
    } else {
        std::cout << "\n❌ Some searchTask() scenarios failed." << std::endl;
    }

    return allPassed;
}


bool testRetrieveTask(const std::string& dbPath) {
    std::cout << "\n==== Testing retrieveTask() ====" << std::endl;

    remove(dbPath.c_str());
    TaskManager tmFresh(dbPath);
    bool allPassed = true;

/* Case I: fetch by valid ID*/
    std::cout << "\nCase I: Fetch by valid ID" << std::endl;

    bool success = tmFresh.addTask("Unique Task", 5, "active", 75);
    if (!success) 
    {
        printFail("Failed to add task for valid ID test");
        return false;
    }

    // Search to get the ID
    auto results = tmFresh.searchTask("Unique Task");
    if (results.empty()) 
    {
        printFail("Setup failed: task not found");
        return false;
    }
    int taskId = results[0].id;

    // Fetch by ID
    Task task = tmFresh.retrieveTask(taskId);
    if (task.id == taskId) {
        printPass("retrieveTask() returned correct ID");
    } else {
        printFail("retrieveTask() returned ID " + std::to_string(task.id) + " (expected " + std::to_string(taskId) + ")");
        allPassed = false;
    }

    if (task.info == "Unique Task") {
        printPass("Task info matches");
    } else {
        printFail("Task info is '" + task.info + "' (expected 'Unique Task')");
        allPassed = false;
    }

    if (task.priority == 5) {
        printPass("Priority matches");
    } else {
        printFail("Priority mismatch");
        allPassed = false;
    }

/* Case II  : fetch by invalid ID*/
    std::cout << "\nCase II: Fetch by invalid ID" << std::endl;

    Task invalidTask = tmFresh.retrieveTask(99999);
    if (invalidTask.id == 0) {
        printPass("retrieveTask() returned default Task for invalid ID (id=0)");
    } else {
        printFail("retrieveTask() returned ID " + std::to_string(invalidTask.id) + " for invalid ID (expected 0)");
        allPassed = false;
    }

    //cleanup
    cleanup(dbPath);

    if (allPassed) {
        std::cout << "\n✅ All retrieveTask() scenarios passed!" << std::endl;
    } else {
        std::cout << "\n❌ Some retrieveTask() scenarios failed." << std::endl;
    }

    return allPassed;
}






int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "      TaskManager Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;

    std::string dbPath = getTestDBPath();

    bool addPassed = testAddTask(dbPath);
    bool deletePassed = testDeleteTask(dbPath);
    bool updatePassed = testUpdateTask(dbPath);
    bool searchPassed = testSearchTask(dbPath);
    bool retrieveTaskPassed = testRetrieveTask(dbPath);

    std::cout << "\n========================================" << std::endl;
    std::cout << "            FINAL RESULTS" << std::endl;
    std::cout << "========================================" << std::endl;

    if (addPassed) {
        std::cout << "✅ addTask()    : PASSED" << std::endl;
    } else {
        std::cout << "❌ addTask()    : FAILED" << std::endl;
    }

    if (deletePassed) {
        std::cout << "✅ deleteTask() : PASSED" << std::endl;
    } else {
        std::cout << "❌ deleteTask() : FAILED" << std::endl;
    }

    if (updatePassed) {
        std::cout << "✅ updateTask() : PASSED" << std::endl;
    } else {
        std::cout << "❌ updateTask() : FAILED" << std::endl;
    }

    if (searchPassed) {
        std::cout << "✅ searchTask() : PASSED" << std::endl;
    } else {
        std::cout << "❌ searchTask() : FAILED" << std::endl;
    }
    
    if (retrieveTaskPassed) {
        std::cout << "✅ retrieveTask() : PASSED" << std::endl;
    } else {
        std::cout << "❌ retrieveTask() : FAILED" << std::endl;
    }

    std::cout << "========================================" << std::endl;

    return (addPassed && deletePassed && updatePassed) ? 0 : 1;
}
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
    std::cout << "testing addTask()" << std::endl;
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

    auto tasks = tmFresh.searchTask("Learn C++")
    if (tasks.size()==1)

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

        if (task.parentId == -1) 
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
            allPassed = False;
        }

        if (!children.empty())
        {
            const Task& child = children[0];
            if (children.parentId = taskId)
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
                printFail("subtask level "+ std::to_string+" (expected: 2)");
                allPassed = false;
            }
        }
    } else
    {
        printFail("Skipping subtask due to an issue in the previous test.");
        allPassed = false;
    }

/* Case III : adding a task with max depth*/
    std::cout << "\n Case III: subtask with max depth (3) " << std::endl;
    
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
            std::cout << "case IV: adding a task beyond max depth." << std::endl;
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
    st::cout << "\n Case V: adding a task with an invalid parent" << std::endl;

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
    std::cout << "\n  Case VI: Add a task with default parameters" << std::endl;

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

        if (task.parentId == -1) 
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
    if (allPassed)
    {
        std::cout << "All tests PASSED for addTask() function!" << std::endl;
    } else
    {
        std::cout << "Some test failed for addTask() function." << std::endl;
    }

    return allPassed;
}


int main()
{

}

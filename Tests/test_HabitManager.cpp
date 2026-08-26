#include "../Database.h"
#include "../HabitManager.h"
#include <gtest/gtest.h>
#include <sqlite3.h>

class HabitManagerTest : public ::testing::Test
{
protected:

    sqlite3* db = nullptr;
    HabitManager* manager = nullptr;
    std::string dbPath;

    static int testCounter;

    void SetUp() override
    {
        dbPath = "file:HabitManagerTest_" +
                 std::to_string(testCounter++) +
                 "?mode=memory&cache=shared";

        int rc = sqlite3_open_v2(
            dbPath.c_str(),
            &db,
            SQLITE_OPEN_READWRITE |
            SQLITE_OPEN_CREATE |
            SQLITE_OPEN_URI,
            nullptr
        );

        ASSERT_EQ(rc, SQLITE_OK);

        manager = new HabitManager(dbPath);
    }

    void TearDown() override
    {
        delete manager;
        manager = nullptr;

        sqlite3_close(db);
        db = nullptr;
    }
};


int HabitManagerTest::testCounter = 0;


// ===== Add Habit =====
TEST_F(HabitManagerTest, ValidCustomValues) 
{
    bool result = manager->addHabit("Exercise", "Health", 5);
    EXPECT_TRUE(result);

    sqlite3_stmt* stmt;
    const char* sql = "SELECT Priority, Category FROM Habit WHERE Name = 'Exercise';";
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    
    ASSERT_EQ(sqlite3_step(stmt), SQLITE_ROW);
    EXPECT_EQ(sqlite3_column_int(stmt, 0), 5);
    EXPECT_STREQ((const char*)sqlite3_column_text(stmt, 1), "Health");
    
    sqlite3_finalize(stmt);
}

TEST_F(HabitManagerTest, DefaultValues) 
{
    bool result = manager->addHabit("Read Book"); // Defaults: "General", 3
    EXPECT_TRUE(result);

    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, "SELECT Category, Priority FROM Habit WHERE Name='Read Book';", -1, &stmt, nullptr);
    ASSERT_EQ(sqlite3_step(stmt), SQLITE_ROW);
    EXPECT_STREQ((const char*)sqlite3_column_text(stmt, 0), "General");
    EXPECT_EQ(sqlite3_column_int(stmt, 1), 3);
    sqlite3_finalize(stmt);
}

TEST_F(HabitManagerTest, InvalidPriorityLow) 
{
    bool result = manager->addHabit("Meditate", "Wellness", 0);
    EXPECT_TRUE(result);
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, "SELECT Priority FROM Habit WHERE Name='Meditate';", -1, &stmt, nullptr);
    ASSERT_EQ(sqlite3_step(stmt), SQLITE_ROW);
    EXPECT_EQ(sqlite3_column_int(stmt, 0), 1);
    sqlite3_finalize(stmt);
}

TEST_F(HabitManagerTest, EmptyName) 
{
    bool result = manager->addHabit("", "General", 3);
    EXPECT_FALSE(result); 
}


// ===== Get Habit =====
TEST_F(HabitManagerTest, GetExistingHabit) 
{
    bool inserted = manager->addHabit("Morning Run", "Health", 5);
    ASSERT_TRUE(inserted);

    Habit habit = manager->getHabit(1);

    EXPECT_EQ(habit.id, 1);
    EXPECT_EQ(habit.name, "Morning Run");
    EXPECT_EQ(habit.category, "Health");
    EXPECT_EQ(habit.priority, 5);
    EXPECT_FALSE(habit.createdDate.empty());
    EXPECT_EQ(habit.active, 1);              
    EXPECT_EQ(habit.streak, 0);
    EXPECT_EQ(habit.bestStreak, 0);
    EXPECT_EQ(habit.totalDone, 0);
    EXPECT_DOUBLE_EQ(habit.strength, 0);
}

TEST_F(HabitManagerTest, GetDefaultValues) 
{
    bool inserted = manager->addHabit("Read Book"); // Defaults to "General", 3
    ASSERT_TRUE(inserted);

    Habit habit = manager->getHabit(1);

    EXPECT_EQ(habit.name, "Read Book");
    EXPECT_EQ(habit.category, "General");
    EXPECT_EQ(habit.priority, 3);
}

TEST_F(HabitManagerTest, GetNonExistentHabit) 
{
    Habit habit = manager->getHabit(999);

    EXPECT_EQ(habit.id, 0);
    EXPECT_TRUE(habit.name.empty());
    EXPECT_TRUE(habit.category.empty());
    EXPECT_EQ(habit.priority, 0);
}

TEST_F(HabitManagerTest, GetWithZeroId) {
    Habit habit = manager->getHabit(0);
    EXPECT_EQ(habit.id, 0);
    EXPECT_TRUE(habit.name.empty());
}

TEST_F(HabitManagerTest, GetWithNegativeId) {
    Habit habit = manager->getHabit(-42);
    EXPECT_EQ(habit.id, 0);
    EXPECT_TRUE(habit.name.empty());
}

TEST_F(HabitManagerTest, GetSecondHabit) {
    manager->addHabit("First Habit");
    manager->addHabit("Second Habit");

    Habit first = manager->getHabit(1);
    Habit second = manager->getHabit(2);

    EXPECT_EQ(first.name, "First Habit");
    EXPECT_EQ(second.name, "Second Habit");
    EXPECT_EQ(first.id, 1);
    EXPECT_EQ(second.id, 2);
}


// ===== Delete Habit =====
TEST_F(HabitManagerTest, DeleteExistingHabit)
{
    bool inserted = manager->addHabit("Exercise", "Health", 5);
    ASSERT_TRUE(inserted);

    Habit habit = manager->getHabit(1);
    ASSERT_EQ(habit.id, 1);
    EXPECT_EQ(habit.name, "Exercise");

    bool result = manager->deleteHabit(habit.id);
    EXPECT_TRUE(result);

    // Verify the habit is actually gone.
    Habit deletedHabit = manager->getHabit(1);
    EXPECT_EQ(deletedHabit.id, 0);
    EXPECT_TRUE(deletedHabit.name.empty());
}


TEST_F(HabitManagerTest, DeleteNonExistentHabit)
{
    bool result = manager->deleteHabit(999);

    EXPECT_FALSE(result);
}


TEST_F(HabitManagerTest, DeleteWithZeroID)
{
    bool result = manager->deleteHabit(0);

    EXPECT_FALSE(result);
}


TEST_F(HabitManagerTest, DeleteWithNegativeID)
{
    bool result = manager->deleteHabit(-5);

    EXPECT_FALSE(result);
}


TEST_F(HabitManagerTest, DeleteTwice)
{
    bool inserted = manager->addHabit("Read", "Learning", 3);
    ASSERT_TRUE(inserted);

    Habit habit = manager->getHabit(1);
    ASSERT_EQ(habit.id, 1);

    bool firstDelete = manager->deleteHabit(habit.id);
    EXPECT_TRUE(firstDelete);

    bool secondDelete = manager->deleteHabit(habit.id);
    EXPECT_FALSE(secondDelete);
}


TEST_F(HabitManagerTest, DeleteSpecificHabitAmongMany)
{
    bool firstInserted = manager->addHabit("HabitA", "Work", 1);
    bool secondInserted = manager->addHabit("HabitB", "Work", 2);

    ASSERT_TRUE(firstInserted);
    ASSERT_TRUE(secondInserted);

    Habit first = manager->getHabit(1);
    Habit second = manager->getHabit(2);

    ASSERT_EQ(first.id, 1);
    ASSERT_EQ(second.id, 2);

    bool result = manager->deleteHabit(first.id);
    EXPECT_TRUE(result);

    // First habit should be gone.
    Habit deleted = manager->getHabit(1);
    EXPECT_EQ(deleted.id, 0);
    EXPECT_TRUE(deleted.name.empty());

    // Second habit should still exist.
    Habit remaining = manager->getHabit(2);
    EXPECT_EQ(remaining.id, 2);
    EXPECT_EQ(remaining.name, "HabitB");
}


// ===== Update Habit =====
TEST_F(HabitManagerTest, UpdateExistingHabitOneField)
{
    // Modifying name
    bool insert = manager->addHabit("HabitA", "Work", 1);
    ASSERT_TRUE(insert);

    Habit habit = manager->getHabit(1);
    ASSERT_EQ(habit.id, 1);
    
    bool result = manager->updateHabit(1, "HabitB");
    ASSERT_TRUE(result);

    Habit modified = manager->getHabit(1);

    EXPECT_EQ(habit.id, modified.id);
    EXPECT_EQ(modified.name, "HabitB");
    EXPECT_EQ(habit.priority, modified.priority);
    EXPECT_EQ(habit.category, modified.category);
}

TEST_F(HabitManagerTest, UpdateExistingHabitSeveralFields)
{
    // Modifying name and category
    bool insert = manager->addHabit("HabitA", "Work", 1);
    ASSERT_TRUE(insert);

    Habit habit = manager->getHabit(1);
    ASSERT_EQ(habit.id, 1);
    
    bool result = manager->updateHabit(1, "HabitB", "Health");
    ASSERT_TRUE(result);
    Habit modified = manager->getHabit(1);

    EXPECT_EQ(habit.id, modified.id);
    EXPECT_EQ(modified.name, "HabitB");
    EXPECT_EQ(habit.priority, modified.priority);
    EXPECT_EQ(modified.category, "Health");
}

TEST_F(HabitManagerTest, UpdateNonExistentHabit)
{
    bool result = manager->updateHabit(999, "HabitB");
    EXPECT_FALSE(result);
}

TEST_F(HabitManagerTest, UpdateZeroHabit)
{
    bool result = manager->updateHabit(0, "HabitB");
    EXPECT_FALSE(result);
}

TEST_F(HabitManagerTest, UpdateNegativeHabit)
{
    bool result = manager->updateHabit(-10, "HabitB");
    EXPECT_FALSE(result);
}

TEST_F(HabitManagerTest, UpdateToInvalidTrue)
{
    bool insert = manager->addHabit("HabitA", "Work", 1);
    ASSERT_TRUE(insert);

    Habit habit = manager->getHabit(1);
    ASSERT_EQ(habit.id, 1);

    bool result = manager->updateHabit(1, "HabitB", "", 10);
    ASSERT_TRUE(result);

    Habit modified = manager->getHabit(1);

    EXPECT_EQ(habit.id, modified.id);
    EXPECT_EQ(modified.name, "HabitB");
    EXPECT_EQ(habit.priority, modified.priority);
    EXPECT_EQ(habit.category, modified.category);
}

TEST_F(HabitManagerTest, UpdateToInvalidFalse)
{
    bool insert = manager->addHabit("HabitA", "Work", 1);
    ASSERT_TRUE(insert);
    Habit habit = manager->getHabit(1);

    bool result = manager->updateHabit(1, "", "", 10);
    EXPECT_FALSE(result);

    Habit modified = manager->getHabit(1);

    EXPECT_EQ(habit.id, modified.id);
    EXPECT_EQ(modified.name, habit.name);
    EXPECT_EQ(modified.category, habit.category);
    EXPECT_EQ(modified.priority, habit.priority);
}


// ===== Habit Exists ======
TEST_F(HabitManagerTest, HabitExistsTrue)
{
    bool insert = manager->addHabit("Habit");
    ASSERT_TRUE(insert);
    
    EXPECT_TRUE(manager->habitExists(1));  
}

TEST_F(HabitManagerTest, HabitExistsFalse)
{
    EXPECT_FALSE(manager->habitExists(999));
}

TEST_F(HabitManagerTest, InvalidHabitExists)
{
    EXPECT_FALSE(manager->habitExists(0));
    EXPECT_FALSE(manager->habitExists(-1));
}

TEST_F(HabitManagerTest, HabitExistsAfterDeletion)
{
    manager->addHabit("Habit");
    ASSERT_TRUE(manager->habitExists(1));
    
    manager->deleteHabit(1);
    EXPECT_FALSE(manager->habitExists(1));
}


// ===== Get All Habits =====
TEST_F(HabitManagerTest, GetAllHabitsMultipleEntries)
{
    bool insert1 = manager->addHabit("HabitA", "Health", 5);
    bool insert2 = manager->addHabit("HabitB", "Work", 2);
    ASSERT_TRUE(insert1 && insert2);

    auto habits = manager->getAllHabits();
    ASSERT_EQ(habits.size(), 2);  

    EXPECT_EQ(habits[0].name, "HabitA");
    EXPECT_EQ(habits[0].category, "Health");
    EXPECT_EQ(habits[0].priority, 5);
    EXPECT_EQ(habits[1].name, "HabitB");
    EXPECT_EQ(habits[1].category, "Work");
    EXPECT_EQ(habits[1].priority, 2);
}

TEST_F(HabitManagerTest, GetAllHabitsEmpty)
{
    auto habits = manager->getAllHabits();
    ASSERT_EQ(habits.size(), 0);
}

TEST_F(HabitManagerTest, GetAllHabitsAfterDelete)
{
    bool insert1 = manager->addHabit("HabitA", "Health", 5);
    bool insert2 = manager->addHabit("HabitB", "Work", 2);
    bool delete1 = manager->deleteHabit(1);

    ASSERT_TRUE(insert1 && insert2 && delete1);

    auto habits = manager->getAllHabits();
    ASSERT_EQ(habits.size(), 1);  

    EXPECT_EQ(habits[0].id, 2);
    EXPECT_EQ(habits[0].name, "HabitB");
    EXPECT_EQ(habits[0].category, "Work");
    EXPECT_EQ(habits[0].priority, 2);
}


// ===== Archive / Unarchive Habit =====
TEST_F(HabitManagerTest, ArchiveExistingHabit)
{
    bool inserted = manager->addHabit("HabitToArchive", "Health", 3);
    ASSERT_TRUE(inserted);

    bool archived = manager->archiveHabit(1, true);
    EXPECT_TRUE(archived);

    Habit habit = manager->getHabit(1);
    EXPECT_EQ(habit.id, 1);
    EXPECT_EQ(habit.name, "HabitToArchive");
    EXPECT_EQ(habit.active, 0); 

    auto active = manager->getActiveHabits();
    EXPECT_TRUE(active.empty());
}

TEST_F(HabitManagerTest, UnarchiveArchivedHabit)
{
    manager->addHabit("ArchivedHabit");
    bool archived = manager->archiveHabit(1, true);
    ASSERT_TRUE(archived);

    Habit habit = manager->getHabit(1);
    ASSERT_EQ(habit.active, 0);

    bool unarchived = manager->archiveHabit(1, false);
    EXPECT_TRUE(unarchived);

    habit = manager->getHabit(1);
    EXPECT_EQ(habit.active, 1);

    auto active = manager->getActiveHabits();
    ASSERT_EQ(active.size(), 1);
    EXPECT_EQ(active[0].id, 1);
    EXPECT_EQ(active[0].name, "ArchivedHabit");
}

TEST_F(HabitManagerTest, ArchiveNonExistentHabit)
{
    bool archived = manager->archiveHabit(999, true);
    EXPECT_FALSE(archived);

    bool unarchived = manager->archiveHabit(999, false);
    EXPECT_FALSE(unarchived);

    auto all = manager->getAllHabits();
    EXPECT_TRUE(all.empty());
}

TEST_F(HabitManagerTest, ArchiveInvalidId)
{
    EXPECT_FALSE(manager->archiveHabit(0, true));
    EXPECT_FALSE(manager->archiveHabit(-5, true));
    EXPECT_FALSE(manager->archiveHabit(0, false));
    EXPECT_FALSE(manager->archiveHabit(-5, false));
}

TEST_F(HabitManagerTest, ArchiveAlreadyArchivedHabit)
{
    manager->addHabit("AlreadyArchived");
    manager->archiveHabit(1, true);

    bool archivedAgain = manager->archiveHabit(1, true);
    EXPECT_TRUE(archivedAgain); 

    Habit habit = manager->getHabit(1);
    EXPECT_EQ(habit.active, 0);
}

TEST_F(HabitManagerTest, UnarchiveAlreadyActiveHabit)
{
    manager->addHabit("AlreadyActive");

    bool unarchived = manager->archiveHabit(1, false);
    EXPECT_TRUE(unarchived);

    Habit habit = manager->getHabit(1);
    EXPECT_EQ(habit.active, 1);
}


// ===== Get Active / Archived Habits =====

TEST_F(HabitManagerTest, GetActiveHabitsDefaultReturnsOnlyActive)
{
    manager->addHabit("ActiveHabit", "Health", 3);
    manager->addHabit("ArchivedHabit", "Work", 2);
    
    bool archived = manager->archiveHabit(2, true);
    ASSERT_TRUE(archived);

    auto active = manager->getActiveHabits();
    
    ASSERT_EQ(active.size(), 1);
    EXPECT_EQ(active[0].id, 1);
    EXPECT_EQ(active[0].name, "ActiveHabit");
    EXPECT_EQ(active[0].active, 1);
}

TEST_F(HabitManagerTest, GetActiveHabitsWithFalseReturnsOnlyArchived)
{
    manager->addHabit("ActiveHabit", "Health", 3);
    manager->addHabit("ArchivedHabit", "Work", 2);
    
    manager->archiveHabit(2, true);

    auto archived = manager->getActiveHabits(false);
    
    ASSERT_EQ(archived.size(), 1);
    EXPECT_EQ(archived[0].id, 2);
    EXPECT_EQ(archived[0].name, "ArchivedHabit");
    EXPECT_EQ(archived[0].active, 0);
}

TEST_F(HabitManagerTest, GetActiveHabitsUnarchive)
{
    manager->addHabit("HabitA");
    manager->addHabit("HabitB");
    
    manager->archiveHabit(1, true);
    
    auto active = manager->getActiveHabits();
    ASSERT_EQ(active.size(), 1);
    EXPECT_EQ(active[0].id, 2);
    
    manager->archiveHabit(1, false);
    
    active = manager->getActiveHabits();
    ASSERT_EQ(active.size(), 2);

    EXPECT_EQ(active[0].id, 1);
    EXPECT_EQ(active[0].name, "HabitA");
    EXPECT_EQ(active[1].id, 2);
    EXPECT_EQ(active[1].name, "HabitB");
}

TEST_F(HabitManagerTest, GetActiveHabitsEmpty)
{
    auto active = manager->getActiveHabits();
    ASSERT_TRUE(active.empty());
    EXPECT_EQ(active.size(), 0);
    
    auto archived = manager->getActiveHabits(false);
    ASSERT_TRUE(archived.empty());
    EXPECT_EQ(archived.size(), 0);
}

TEST_F(HabitManagerTest, GetActiveHabitsAllArchived)
{
    manager->addHabit("Temp1");
    manager->addHabit("Temp2");
    
    manager->archiveHabit(1, true);
    manager->archiveHabit(2, true);
    
    auto active = manager->getActiveHabits();
    EXPECT_TRUE(active.empty());
    
    auto archived = manager->getActiveHabits(false);
    ASSERT_EQ(archived.size(), 2);
    EXPECT_EQ(archived[0].id, 1);
    EXPECT_EQ(archived[1].id, 2);
}

//TODO: test streak functions
// ===== Test getIntColumn =====
TEST_F(HabitManagerTest, GetIntColumnExistingHabit)
{
    manager->addHabit("TestHabit", "Health", 4);
    
    EXPECT_EQ(manager->getIntColumn(1, "Priority"), 4);
    EXPECT_EQ(manager->getIntColumn(1, "Streak"), 0);
    EXPECT_EQ(manager->getIntColumn(1, "BestStreak"), 0);
    EXPECT_EQ(manager->getIntColumn(1, "TotalDone"), 0);
    EXPECT_EQ(manager->getIntColumn(1, "Active"), 1);
}

TEST_F(HabitManagerTest, GetIntColumnNonExistentHabit)
{
    EXPECT_EQ(manager->getIntColumn(999, "Priority"), -1);
    EXPECT_EQ(manager->getIntColumn(999, "Streak"), -1);
}

TEST_F(HabitManagerTest, GetIntColumnInvalidColumn)
{
    manager->addHabit("TestHabit");
    EXPECT_EQ(manager->getIntColumn(1, "InvalidColumn"), -1);
}

TEST_F(HabitManagerTest, GetIntColumnInvalidId)
{
    EXPECT_EQ(manager->getIntColumn(0, "Priority"), -1);
    EXPECT_EQ(manager->getIntColumn(-5, "Priority"), -1);
}

TEST_F(HabitManagerTest, GetIntColumnAfterUpdate)
{
    manager->addHabit("TestHabit", "Health", 3);
    
    EXPECT_EQ(manager->getIntColumn(1, "Priority"), 3);
    EXPECT_EQ(manager->getIntColumn(1, "Streak"), 0);
    EXPECT_EQ(manager->getIntColumn(1, "BestStreak"), 0);
    EXPECT_EQ(manager->getIntColumn(1, "TotalDone"), 0);
    
    manager->updateHabit(1, "", "", 5);
    EXPECT_EQ(manager->getIntColumn(1, "Priority"), 5);
    
    manager->updateHabit(1, "NewName", "", -1);
    EXPECT_EQ(manager->getIntColumn(1, "Priority"), 5);
}


// ===== Habit Completion =====
TEST_F(HabitManagerTest, completeExistingHabitInsert)
{
    manager->addHabit("TestHabit", "Health", 3);
    
    bool completed = manager->completeHabit(1);
    EXPECT_TRUE(completed);
    
    EXPECT_EQ(manager->getIntColumn(1, "Streak"), 1);
    EXPECT_EQ(manager->getIntColumn(1, "BestStreak"), 1);
    EXPECT_EQ(manager->getIntColumn(1, "TotalDone"), 1);
}

TEST_F(HabitManagerTest, completeExistingHabitUpdate)
{
    manager->addHabit("TestHabit");
    
    manager->completeHabit(1);
    bool completed = manager->completeHabit(1);
    EXPECT_TRUE(completed);
    
    EXPECT_EQ(manager->getIntColumn(1, "Streak"), 1);
    EXPECT_EQ(manager->getIntColumn(1, "BestStreak"), 1);
    EXPECT_EQ(manager->getIntColumn(1, "TotalDone"), 2);
}

TEST_F(HabitManagerTest, completeNonExistentHabit)
{
    bool completed = manager->completeHabit(999);
    EXPECT_FALSE(completed);
}

TEST_F(HabitManagerTest, completeInvalidHabit)
{
    EXPECT_FALSE(manager->completeHabit(0));
    EXPECT_FALSE(manager->completeHabit(-5));
}

TEST_F(HabitManagerTest, completeHabitConsecutiveDays)
{
    manager->addHabit("TestHabit");

    manager->completeHabit(1, "2026-01-01");
    EXPECT_EQ(manager->getIntColumn(1, "Streak"), 1);
    EXPECT_EQ(manager->getIntColumn(1, "BestStreak"), 1);
    EXPECT_EQ(manager->getIntColumn(1, "TotalDone"), 1);

    manager->completeHabit(1, "2026-01-02");
    EXPECT_EQ(manager->getIntColumn(1, "Streak"), 2);
    EXPECT_EQ(manager->getIntColumn(1, "BestStreak"), 2);
    EXPECT_EQ(manager->getIntColumn(1, "TotalDone"), 2);

    manager->completeHabit(1, "2026-01-03");
    manager->completeHabit(1, "2026-01-03"); 
    EXPECT_EQ(manager->getIntColumn(1, "Streak"), 3);
    EXPECT_EQ(manager->getIntColumn(1, "BestStreak"), 3);
    EXPECT_EQ(manager->getIntColumn(1, "TotalDone"), 4); 
}

TEST_F(HabitManagerTest, completeHabitAfterGapResetsStreak)
{
    manager->addHabit("TestHabit");

    manager->completeHabit(1, "2026-01-01");
    manager->completeHabit(1, "2026-01-02");
    EXPECT_EQ(manager->getIntColumn(1, "Streak"), 2);

    manager->completeHabit(1, "2026-01-04");
    EXPECT_EQ(manager->getIntColumn(1, "Streak"), 1); 
    EXPECT_EQ(manager->getIntColumn(1, "BestStreak"), 2);
}

TEST_F(HabitManagerTest, completeArchivedHabit)
{
    manager->addHabit("ArchivedHabit");
    manager->archiveHabit(1, true);
    
    bool completed = manager->completeHabit(1);
    EXPECT_FALSE(completed);
    
    EXPECT_EQ(manager->getIntColumn(1, "TotalDone"), 0);
    EXPECT_EQ(manager->getIntColumn(1, "Active"), 0);
}

TEST_F(HabitManagerTest, unarchiveThenComplete)
{
    manager->addHabit("Habit");
    manager->archiveHabit(1, true);
    
    manager->archiveHabit(1, false);
    EXPECT_EQ(manager->getIntColumn(1, "Active"), 1);
    
    bool completed = manager->completeHabit(1);
    EXPECT_TRUE(completed);
    EXPECT_EQ(manager->getIntColumn(1, "TotalDone"), 1);
    EXPECT_EQ(manager->getIntColumn(1, "Streak"), 1);
}
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
    EXPECT_DOUBLE_EQ(habit.strength, 0.5);
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

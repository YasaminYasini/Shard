#include <gtest/gtest.h>
#include "HabitManager.h" // Your class
#include <sqlite3.h>


class HabitManagerTest : public ::testing::Test 
{
protected:
    sqlite3* db = nullptr;
    HabitManager* manager = nullptr;


    void SetUp() override 
    {
        sqlite3_open(":memory:", &db);
        manager = new HabitManager(db); 
    }


    void TearDown() override 
    {
        delete manager;
        sqlite3_close(db);
    }
};

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

TEST_F(HabitManagerTest, InvalidPriorityLow) {
    bool result = manager->addHabit("Meditate", "Wellness", 0);
    EXPECT_FALSE(result); 
}

TEST_F(HabitManagerTest, InvalidPriorityHigh) {
    bool result = manager->addHabit("Meditate", "Wellness", 6);
    EXPECT_FALSE(result);
}

TEST_F(HabitManagerTest, DefaultValues) {
    bool result = manager->addHabit("Read Book");
    EXPECT_TRUE(result);

    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, "SELECT Category, Priority FROM Habit WHERE Name='Read Book';", -1, &stmt, nullptr);
    ASSERT_EQ(sqlite3_step(stmt), SQLITE_ROW);
    EXPECT_STREQ((const char*)sqlite3_column_text(stmt, 0), "General");
    EXPECT_EQ(sqlite3_column_int(stmt, 1), 3);
    sqlite3_finalize(stmt);
}

TEST_F(HabitManagerTest, EmptyName) {
    bool result = manager->addHabit("", "General", 3);
    EXPECT_FALSE(result);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
# include "HabitManager.h"
# include "Database.h"
# include <iostream>
# include <sqlite3.h>
# include <string>
# include <memory>
# include <vector>

HabitManager::HabitManager(const std::string& dbPath)
{
    int rc = sqlite3_open(dbPath.c_str(), &db);
    if (rc != SQLITE_OK)
    {
        std::cerr <<"Cannot open database: "<< sqlite3_errmsg(db) << std::endl;
        db = nullptr;
        return;
    }

    Database::executeSQL(db, "PRAGMA foreign_keys = ON;");

    const char* createHabitSQL = 
    R"(
    CREATE TABLE IF NOT EXISTS Habit (
    HabitID     INTEGER PRIMARY KEY AUTOINCREMENT,
    Name        TEXT NOT NULL,
    Category    TEXT DEFAULT 'General',
    Priority    INTEGER DEFAULT 3 CHECK( Priority >= 1 AND Priority <= 5 ),
    CreatedDate TEXT DEFAULT (date('now')),
    Active      INTEGER DEFAULT 1,
    LastDone    TEXT,
    Streak      INTEGER DEFAULT 0,
    BestStreak  INTEGER DEFAULT 0,
    TotalDone   INTEGER DEFAULT 0,
    Strength    REAL DEFAULT 0.5             
    );
    )";

    const char* createHabitHistorySQL = 
    R"(
    CREATE TABLE IF NOT EXISTS HabitHistory (
    HistoryID   INTEGER PRIMARY KEY,
    HabitID     INTEGER,
    DoneDate    TEXT DEFAULT (date('now')),
    Count       INTEGER DEFAULT 1,            -- For habits that can be done multiple times/day
    FOREIGN KEY (HabitID) REFERENCES Habit(HabitID) ON DELETE CASCADE,
    UNIQUE(HabitID, DoneDate)
    );
    )";


    const char* streakTrigger = 
    R"(
    CREATE TRIGGER IF NOT EXISTS update_habit_on_completion
    AFTER INSERT OR UPDATE ON HabitHistory
    FOR EACH ROW
    BEGIN
        UPDATE Habit
        SET 
            LastDone = date('now'),
            Streak = CASE
                WHEN date('now') = date(LastDone, '+1 day') THEN Streak + 1
                WHEN date('now') = date(LastDone) THEN Streak
                ELSE 1
            END,
            TotalDone = (SELECT SUM(Count) FROM HabitHistory WHERE HabitID = NEW.HabitID)
            Strength = CASE
                WHEN date('now') = date(LastDone, '+1 day') THEN MIN(Strength + 0.1, 1.0)
                WHEN date('now') = date(LastDone) THEN Strength
                ELSE MAX(Strength - 0.05, 0.0)
            END
        WHERE HabitID = NEW.HabitID;

        UPDATE Habit
        SET BestStreak = MAX(Streak, BestStreak)
        WHERE HabitID = NEW.HabitID;
    END;
    )";

    Database::executeSQL(db, createHabitSQL);
    Database::executeSQL(db, createHabitHistorySQL);
    Database::executeSQL(db, streakTrigger);
}


HabitManager::~HabitManager()
{
    if (db)
    {
        sqlite3_close(db);
    }
}

bool HabitManager::addHabit(
    const std::string& name,
    const std::string& category,
    int priority
)
{
    if (!Database::DBCheck(db)) return false;

    sqlite3_stmt* stmt;
    const char* sql = "INSERT INTO Habit (Name, Category, Priority, CreatedDate) VALUES (?, ?, ?, date('now'));";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    if (rc!= SQLITE_OK)
    {
        std::cerr << "Failed to prepare statement. " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    if (priority < 1) priority = 1;
    if (priority > 5) priority = 5; 
    if (name.empty())
    {
        std::cout << "Failed to create habit: missing argument <Name>" << std::endl;
        return false;
    }

    int binder = 1;
    sqlite3_bind_text(stmt, binder++, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, binder++, category.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, binder++ , priority);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if(rc != SQLITE_DONE)
    {
        std::cerr << "Failed to create habit." << sqlite3_errmsg(db) << std::endl;
        return false;
    }
    std::cout << "Inserted successfully" << std::endl;
    return true;
}


bool HabitManager::deleteHabit(int habitID)
{
    if (!Database::DBCheck(db)) return false;

    if (!habitExists(habitID))
    {
        std::cerr << "HabitID " << habitID << "not found. " << std::endl;
        return false;
    }

    sqlite3_stmt* stmt;
    const char* sql = "DELETE FROM Habit WHERE HabitID = ?;";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK)
    {
        std::cerr << "Failed to prepare delete: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_int(stmt, 1, habitID);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE)
    {
        std::cerr << "Failed to delete Habit: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    int changes = sqlite3_changes(db);
    if (changes == 0)
    {
        std::cerr << "Habit ID " << habitID << "not found." << std::endl;
        return false;
    }
    return true;
}


bool HabitManager::updateHabit(
    int habitID, 
    const std::string& name, 
    const std::string& category,
    int priority
    )
{
    if (!Database::DBCheck(db)) return false;
    if (!habitExists(habitID)) return false;
    
    sqlite3_stmt* stmt;
    std::string sql = "UPDATE Habit SET ";
    std::vector<std::string> updates;

    if (!name.empty()) updates.push_back("Name = ?");
    if (!category.empty()) updates.push_back("Category = ?");
    if (priority != -1 && priority < 5 && priority > 1) updates.push_back("Priority = ?");

    if (updates.empty())
    {
        std::cerr << "No fields to update." << std::endl;
        return false;
    }

    for (size_t i = 0; i < updates.size(); ++i)
    {
        if (i>0) sql += ", ";
        sql += updates[i];
    }

    sql += " WHERE HabitID = ?";

    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        std::cerr << "Failed to prepare update: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    int bindIndex = 1;
    if (!name.empty()) sqlite3_bind_text(stmt, bindIndex++, name.c_str(), -1, SQLITE_TRANSIENT);
    if (!category.empty()) sqlite3_bind_text(stmt, bindIndex++, category.c_str(), -1, SQLITE_TRANSIENT);
    if (priority != -1 && priority < 5 && priority >1) sqlite3_bind_int(stmt, bindIndex++, priority);

    sqlite3_bind_int(stmt, bindIndex, habitID);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) 
    {
        std::cerr << "Failed to update habit: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    int changes = sqlite3_changes(db);

    if (changes == 0) 
    {
        std::cerr << "Habit ID " << habitID << " not found or no changes made." << std::endl;
        return false;
    }

    std::cout << "Habit " << habitID << " updated successfully." << std::endl;
    return true;
}


Habit HabitManager::getHabit(int habitID)
{
    Habit habit;

    if (!Database::DBCheck(db)) return habit;

    sqlite3_stmt* stmt;
    const char* sql = "SELECT * FROM Habit WHERE HabitID = ?;";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK) 
    {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return habit;
    }

    sqlite3_bind_int(stmt, 1, habitID);

    rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW) 
    {
        habit.id = sqlite3_column_int(stmt, 0);
        habit.name = Database::getTextOrEmpty(stmt, 1);
        habit.category = Database::getTextOrEmpty(stmt, 2);
        habit.priority = sqlite3_column_int(stmt, 3);
        habit.createdDate = Database::getTextOrEmpty(stmt, 4);
        habit.active = sqlite3_column_int(stmt, 5);
        habit.lastDone = Database::getTextOrEmpty(stmt, 6);
        habit.streak = sqlite3_column_int(stmt, 7);
        habit.bestStreak = sqlite3_column_int(stmt, 8);
        habit.totalDone = sqlite3_column_int(stmt, 9);
        habit.strength = sqlite3_column_double(stmt, 10);
    } else {
        std::cerr << "Habit ID " << habitID << " not found." << std::endl;
    }

    sqlite3_finalize(stmt);
    return habit;
}


bool HabitManager::habitExists(int habitID)
{
    if (!Database::DBCheck(db)) return false;

    sqlite3_stmt* stmt;
    const char* sql = "SELECT 1 FROM Habit WHERE HabitID = ?;";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) 
    {
        return false;
    }

    sqlite3_bind_int(stmt, 1, habitID);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_ROW);
}


std::vector<Habit> HabitManager::getAllHabits()
{
    std::vector<Habit> habits;

    if (!Database::DBCheck(db)) return habits;

    sqlite3_stmt* stmt;
    const char* sql = "SELECT * FROM Habit;";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK) 
    {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return habits;
    }

    rc = sqlite3_step(stmt);

    while (rc == SQLITE_ROW) 
    {
        Habit habit;
        habit.id = sqlite3_column_int(stmt, 0);
        habit.name = Database::getTextOrEmpty(stmt, 1);
        habit.category = Database::getTextOrEmpty(stmt, 2);
        habit.priority = sqlite3_column_int(stmt, 3);
        habit.createdDate = Database::getTextOrEmpty(stmt, 4);
        habit.active = sqlite3_column_int(stmt, 5);
        habit.lastDone = Database::getTextOrEmpty(stmt, 6);
        habit.streak = sqlite3_column_int(stmt, 7);
        habit.bestStreak = sqlite3_column_int(stmt, 8);
        habit.totalDone = sqlite3_column_int(stmt, 9);
        habit.strength = sqlite3_column_double(stmt, 10);

        habits.push_back(habit);
    }

    sqlite3_finalize(stmt);
    return habits;
}


std::vector<Habit> HabitManager::getActiveHabits(bool active)
{
    std::vector<Habit> habits;

    if (!Database::DBCheck(db)) return habits;

    sqlite3_stmt* stmt;
    const char* sql;

    if (active)
    {
        sql = "SELECT * FROM Habit WHERE Active = 1;";
    } else
    {
        sql = "SELECT * FROM Habit WHERE Active = 0;";
    }

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return habits;
    }

    rc = sqlite3_step(stmt);

    while (rc == SQLITE_ROW) 
    {
        Habit habit;
        habit.id = sqlite3_column_int(stmt, 0);
        habit.name = Database::getTextOrEmpty(stmt, 1);
        habit.category = Database::getTextOrEmpty(stmt, 2);
        habit.priority = sqlite3_column_int(stmt, 3);
        habit.createdDate = Database::getTextOrEmpty(stmt, 4);
        habit.active = sqlite3_column_int(stmt, 5);
        habit.lastDone = Database::getTextOrEmpty(stmt, 6);
        habit.streak = sqlite3_column_int(stmt, 7);
        habit.bestStreak = sqlite3_column_int(stmt, 8);
        habit.totalDone = sqlite3_column_int(stmt, 9);
        habit.strength = sqlite3_column_double(stmt, 10);

        habits.push_back(habit);
    }

    sqlite3_finalize(stmt);
    return habits;
}

bool HabitManager::archiveHabit(int habitID, bool archive)
{
    if (!Database::DBCheck(db)) return false;
    if (!habitExists(habitID)) return false;

    sqlite3_stmt* stmt;
    const char* sql;

    if (archive)
    {
        sql = "UPDATE Habit SET Active = 0 WHERE HabitID = ?;";
    }else
    {
        sql = "UPDATE Habit SET Active = 1 WHERE HabitID = ?;";
    }

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        std::cerr << "Failed to prepare archive: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_int(stmt, 1, habitID);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) 
    {
        std::cerr << "Failed: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    int changes = sqlite3_changes(db);

    if (changes == 0) 
    {
        std::cerr << "Habit ID " << habitID << " not found or no changes made." << std::endl;
        return false;
    }

    if (archive) 
    {
        std::cout << "Habit " << habitID << " archived successfully." << std::endl;
    } else
    {
        std::cout << "Habit " << habitID << " retrieved successfully from archive." << std::endl;        
    }

    return true;
}

// ===== Streak Functions =====
int HabitManager::getIntColumn(int habitID, const std::string& columnName)
{
    if (!Database::DBCheck(db)) return -1;
    if (!habitExists(habitID)) return -1;

    sqlite3_stmt* stmt;
    std::string sql = "SELECT " + columnName + " FROM Habit WHERE HabitID = ?;";

    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) 
    {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return -1;
    }

    sqlite3_bind_int(stmt, 1, habitID);
    rc = sqlite3_step(stmt);

    if (rc != SQLITE_ROW) 
    {
        std::cerr << "Failed to retrieve " << columnName << " for habit " << habitID << std::endl;
        sqlite3_finalize(stmt);
        return -1;
    }

    int result = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return result;
}

int HabitManager::getCurrentStreak(int habitID) 
{
    return getIntColumn(habitID, "Streak");
}

int HabitManager::getBestStreak(int habitID) 
{
    return getIntColumn(habitID, "BestStreak");
}

int HabitManager::getTotalCompletions(int habitID) 
{
    return getIntColumn(habitID, "TotalDone");
}

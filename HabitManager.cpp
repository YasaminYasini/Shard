# include "HabitManager.h"
# include "Database.h"
# include <iostream>
# include <sqlite3.h>
# include <string>
# include <memory>
# include <vector>
# include <ctime>
# include <cstdio>
# include <unordered_map>


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
    Strength    REAL DEFAULT 0);
    )";

    const char* createHabitHistorySQL = 
    R"(
    CREATE TABLE IF NOT EXISTS HabitHistory (
    HistoryID   INTEGER PRIMARY KEY,
    HabitID     INTEGER,
    DoneDate    TEXT DEFAULT (date('now')),
    Count       INTEGER DEFAULT 1,
    FOREIGN KEY (HabitID) REFERENCES Habit(HabitID) ON DELETE CASCADE,
    UNIQUE(HabitID, DoneDate));
    )";


    const char* insertTrigger =
    R"(
    CREATE TRIGGER IF NOT EXISTS update_habit_after_insert
    AFTER INSERT ON HabitHistory
    FOR EACH ROW
    BEGIN

        UPDATE Habit
        SET
            LastDone = (
                SELECT MAX(DoneDate)
                FROM HabitHistory
                WHERE HabitID = NEW.HabitID
            ),

            TotalDone = (
                SELECT COALESCE(SUM(Count), 0)
                FROM HabitHistory
                WHERE HabitID = NEW.HabitID
            ),

            Streak = (
                SELECT COALESCE(MAX(group_count), 0)
                FROM (
                    SELECT
                        COUNT(*) AS group_count,
                        MAX(DoneDate) AS group_last_date
                    FROM (
                        SELECT
                            DoneDate,
                            date(
                                DoneDate,
                                '-' || ROW_NUMBER() OVER (
                                    ORDER BY DoneDate
                                ) || ' days'
                            ) AS streak_group
                        FROM HabitHistory
                        WHERE HabitID = NEW.HabitID
                    )
                    GROUP BY streak_group
                )
                WHERE group_last_date = (
                    SELECT MAX(DoneDate)
                    FROM HabitHistory
                    WHERE HabitID = NEW.HabitID
                )
            ),

            BestStreak = (
                SELECT COALESCE(MAX(group_count), 0)
                FROM (
                    SELECT COUNT(*) AS group_count
                    FROM (
                        SELECT
                            DoneDate,
                            date(
                                DoneDate,
                                '-' || ROW_NUMBER() OVER (
                                    ORDER BY DoneDate
                                ) || ' days'
                            ) AS streak_group
                        FROM HabitHistory
                        WHERE HabitID = NEW.HabitID
                    )
                    GROUP BY streak_group
                )
            )

        WHERE HabitID = NEW.HabitID;

    END;
    )";

    const char* updateTrigger =
    R"(
    CREATE TRIGGER IF NOT EXISTS update_habit_after_update
    AFTER UPDATE ON HabitHistory
    FOR EACH ROW
    BEGIN
        UPDATE Habit
        SET
            LastDone = (
                SELECT MAX(DoneDate)
                FROM HabitHistory
                WHERE HabitID = OLD.HabitID
            ),

            TotalDone = (
                SELECT COALESCE(SUM(Count), 0)
                FROM HabitHistory
                WHERE HabitID = OLD.HabitID
            ),

            Streak = (
                SELECT COALESCE(MAX(group_count), 0)
                FROM (
                    SELECT
                        COUNT(*) AS group_count,
                        MAX(DoneDate) AS group_last_date
                    FROM (
                        SELECT
                            DoneDate,
                            date(
                                DoneDate,
                                '-' || ROW_NUMBER() OVER (
                                    ORDER BY DoneDate
                                ) || ' days'
                            ) AS streak_group
                        FROM HabitHistory
                        WHERE HabitID = OLD.HabitID
                    )
                    GROUP BY streak_group
                )
                WHERE group_last_date = (
                    SELECT MAX(DoneDate)
                    FROM HabitHistory
                    WHERE HabitID = OLD.HabitID
                )
            ),

            BestStreak = (
                SELECT COALESCE(MAX(group_count), 0)
                FROM (
                    SELECT COUNT(*) AS group_count
                    FROM (
                        SELECT
                            DoneDate,
                            date(
                                DoneDate,
                                '-' || ROW_NUMBER() OVER (
                                    ORDER BY DoneDate
                                ) || ' days'
                            ) AS streak_group
                        FROM HabitHistory
                        WHERE HabitID = OLD.HabitID
                    )
                    GROUP BY streak_group
                )
            )

        WHERE HabitID = OLD.HabitID;

        UPDATE Habit
        SET
            LastDone = (
                SELECT MAX(DoneDate)
                FROM HabitHistory
                WHERE HabitID = NEW.HabitID
            ),

            TotalDone = (
                SELECT COALESCE(SUM(Count), 0)
                FROM HabitHistory
                WHERE HabitID = NEW.HabitID
            ),

            Streak = (
                SELECT COALESCE(MAX(group_count), 0)
                FROM (
                    SELECT
                        COUNT(*) AS group_count,
                        MAX(DoneDate) AS group_last_date
                    FROM (
                        SELECT
                            DoneDate,
                            date(
                                DoneDate,
                                '-' || ROW_NUMBER() OVER (
                                    ORDER BY DoneDate
                                ) || ' days'
                            ) AS streak_group
                        FROM HabitHistory
                        WHERE HabitID = NEW.HabitID
                    )
                    GROUP BY streak_group
                )
                WHERE group_last_date = (
                    SELECT MAX(DoneDate)
                    FROM HabitHistory
                    WHERE HabitID = NEW.HabitID
                )
            ),

            BestStreak = (
                SELECT COALESCE(MAX(group_count), 0)
                FROM (
                    SELECT COUNT(*) AS group_count
                    FROM (
                        SELECT
                            DoneDate,
                            date(
                                DoneDate,
                                '-' || ROW_NUMBER() OVER (
                                    ORDER BY DoneDate
                                ) || ' days'
                            ) AS streak_group
                        FROM HabitHistory
                        WHERE HabitID = NEW.HabitID
                    )
                    GROUP BY streak_group
                )
            )

        WHERE HabitID = NEW.HabitID;

    END;
    )";

    const char* deleteTrigger =
    R"(
    CREATE TRIGGER IF NOT EXISTS update_habit_after_delete
    AFTER DELETE ON HabitHistory
    FOR EACH ROW
    BEGIN

        UPDATE Habit
        SET
            LastDone = (
                SELECT MAX(DoneDate)
                FROM HabitHistory
                WHERE HabitID = OLD.HabitID
            ),

            TotalDone = (
                SELECT COALESCE(SUM(Count), 0)
                FROM HabitHistory
                WHERE HabitID = OLD.HabitID
            ),

            Streak = (
                SELECT COALESCE(MAX(group_count), 0)
                FROM (
                    SELECT
                        COUNT(*) AS group_count,
                        MAX(DoneDate) AS group_last_date
                    FROM (
                        SELECT
                            DoneDate,
                            date(
                                DoneDate,
                                '-' || ROW_NUMBER() OVER (
                                    ORDER BY DoneDate
                                ) || ' days'
                            ) AS streak_group
                        FROM HabitHistory
                        WHERE HabitID = OLD.HabitID
                    )
                    GROUP BY streak_group
                )
                WHERE group_last_date = (
                    SELECT MAX(DoneDate)
                    FROM HabitHistory
                    WHERE HabitID = OLD.HabitID
                )
            ),

            BestStreak = (
                SELECT COALESCE(MAX(group_count), 0)
                FROM (
                    SELECT COUNT(*) AS group_count
                    FROM (
                        SELECT
                            DoneDate,
                            date(
                                DoneDate,
                                '-' || ROW_NUMBER() OVER (
                                    ORDER BY DoneDate
                                ) || ' days'
                            ) AS streak_group
                        FROM HabitHistory
                        WHERE HabitID = OLD.HabitID
                    )
                    GROUP BY streak_group
                )
            )

        WHERE HabitID = OLD.HabitID;

    END;
    )";


    Database::executeSQL(db, createHabitSQL);
    Database::executeSQL(db, createHabitHistorySQL);
    Database::executeSQL(db, insertTrigger);
    Database::executeSQL(db, updateTrigger);
    Database::executeSQL(db, deleteTrigger);
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
    if (priority != -1 && priority <= 5 && priority >= 1) updates.push_back("Priority = ?");

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
    if (priority != -1 && priority <= 5 && priority >= 1) sqlite3_bind_int(stmt, bindIndex++, priority);

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

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) 
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

    

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) 
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



// ===== HabitHistory Table Functions =====
bool HabitManager::completeHabit(int habitID,  const std::string& completionDate)
{
    if (!Database::DBCheck(db)) return false;
    if (!habitExists(habitID)) return false;

    if (getIntColumn(habitID, "Active") == 0) 
    {
        std::cerr << "Habit ID " << habitID << " is archived. Unarchive first to complete it." << std::endl;
        return false;
    }

    std::string date = completionDate.empty() ? "date('now')" : "'" + completionDate + "'";

    sqlite3_stmt* stmt;
    std::string sql = "INSERT INTO HabitHistory (HabitID, DoneDate, Count) "
                      "VALUES (?, " + date + ", 1) "
                      "ON CONFLICT(HabitID, DoneDate) DO UPDATE SET Count = Count + 1;";

    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) 
    {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_int(stmt, 1, habitID);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE)
    {
        std::cerr << "Failed to log completion for habit " << habitID << ": " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    return true;
}


std::vector<HabitHistoryEntry> HabitManager::getHabitHistory(int habitID,  int days)
{
    std::vector<HabitHistoryEntry> history;

    if (!Database::DBCheck(db)) return history;
    if (!habitExists(habitID)) return history;
    if (days < 0) days = 0;
    if (days > 3650) days = 3650;

    sqlite3_stmt* stmt;
    const char* sql = R"(
        SELECT HistoryID, HabitID, DoneDate, Count
        FROM HabitHistory
        WHERE HabitID = ?
          AND DoneDate >= date('now', '-' || ? || ' days')
          AND DoneDate <= date('now')
        ORDER BY DoneDate DESC;
    )";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return history;
    }

    sqlite3_bind_int(stmt, 1, habitID);
    sqlite3_bind_int(stmt, 2, days);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        HabitHistoryEntry entry;
        entry.historyId = sqlite3_column_int(stmt, 0);
        entry.habitId   = sqlite3_column_int(stmt, 1);
        entry.doneDate  = Database::getTextOrEmpty(stmt, 2);
        entry.count     = sqlite3_column_int(stmt, 3);
        history.push_back(entry);
    }

    sqlite3_finalize(stmt);
    return history;
}


std::vector<HabitHistoryEntry> HabitManager::getHabitHistoryByDateRange(
    int habitID, 
    const std::string& startDate, 
    const std::string& endDate)
{
    std::vector<HabitHistoryEntry> history;

    if (!Database::DBCheck(db)) return history;
    if (!habitExists(habitID)) return history;

    if (startDate.empty() || endDate.empty()) {
        std::cerr << "Start date and end date cannot be empty." << std::endl;
        return history;
    }

    sqlite3_stmt* stmt;
    const char* sql = R"(
        SELECT HistoryID, HabitID, DoneDate, Count
        FROM HabitHistory
        WHERE HabitID = ?
          AND DoneDate >= ?
          AND DoneDate <= ?
        ORDER BY DoneDate DESC;
    )";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return history;
    }

    sqlite3_bind_int(stmt, 1, habitID);
    sqlite3_bind_text(stmt, 2, startDate.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, endDate.c_str(), -1, SQLITE_STATIC);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        HabitHistoryEntry entry;
        entry.historyId = sqlite3_column_int(stmt, 0);
        entry.habitId   = sqlite3_column_int(stmt, 1);
        entry.doneDate  = Database::getTextOrEmpty(stmt, 2);
        entry.count     = sqlite3_column_int(stmt, 3);
        history.push_back(entry);
    }

    sqlite3_finalize(stmt);
    return history;
}

float HabitManager::getSuccessRate(int habitID, int days)
{
    if (!Database::DBCheck(db)) return 0;
    if (!habitExists(habitID)) return 0;
    if (days <= 0) return 0;

    sqlite3_stmt* stmt;

    const char* sql =
        "SELECT COUNT(DISTINCT DoneDate) FROM HabitHistory "
        "WHERE HabitID = ? "
        "AND DoneDate >= date('now', '-' || ? || ' days');";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK)
    {
        std::cerr << "Failed to prepare statement: "
                  << sqlite3_errmsg(db) << std::endl;
        return 0;
    }

    sqlite3_bind_int(stmt, 1, habitID);
    sqlite3_bind_int(stmt, 2, days);

    float completedDays = 0;

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        completedDays = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);

    return completedDays * 100 / days;
}


HabitCalendarData HabitManager::getHabitCalendar(int habitID, 
                                                  const std::string& startDate, 
                                                  int days)
{
    HabitCalendarData data;
    if (!Database::DBCheck(db)) return data;
    if (!habitExists(habitID)) return data;
    if (days <= 0) return data;
    if (days > 3650) days = 3650;

    // Determining the end date (inclusive) = startDate + (days - 1)
    sqlite3_stmt* stmt;
    const char* rangeSql = "SELECT date(?), date(?, '+' || ? || ' days', '-1 day');";
    int rc = sqlite3_prepare_v2(db, rangeSql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare range query: " << sqlite3_errmsg(db) << std::endl;
        return data;
    }
    sqlite3_bind_text(stmt, 1, startDate.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, startDate.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, days);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        data.startDate = Database::getTextOrEmpty(stmt, 0);
        data.endDate   = Database::getTextOrEmpty(stmt, 1);
    } else {
        sqlite3_finalize(stmt);
        return data;
    }
    sqlite3_finalize(stmt);

    // Query actual completion data within that range
    const char* dataSql = "SELECT DoneDate, SUM(Count) FROM HabitHistory "
                          "WHERE HabitID = ? AND DoneDate >= ? AND DoneDate <= ? "
                          "GROUP BY DoneDate ORDER BY DoneDate ASC;";

    rc = sqlite3_prepare_v2(db, dataSql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) 
    {
        std::cerr << "Failed to prepare data query: " << sqlite3_errmsg(db) << std::endl;
        return data;
    }
    sqlite3_bind_int(stmt, 1, habitID);
    sqlite3_bind_text(stmt, 2, data.startDate.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, data.endDate.c_str(), -1, SQLITE_STATIC);

    std::unordered_map<std::string, int> countMap;

    while (sqlite3_step(stmt) == SQLITE_ROW) 
    {
        std::string date = Database::getTextOrEmpty(stmt, 0);
        int count = sqlite3_column_int(stmt, 1);
        countMap[date] = count;
    }
    sqlite3_finalize(stmt);

    // Building the full vector with zero‑filled gaps
    auto parseDate = [](const std::string& s) -> std::tm 
    {
        std::tm tm = {};
        std::sscanf(s.c_str(), "%d-%d-%d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday);
        tm.tm_year -= 1900;
        tm.tm_mon  -= 1;
        return tm;
    };
    auto formatDate = [](const std::tm& tm) -> std::string 
    {
        char buf[11];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d", &tm);
        return std::string(buf);
    };
    auto addDays = [&](const std::string& date, int offset) -> std::string 
    {
        std::tm tm = parseDate(date);
        tm.tm_mday += offset;
        std::mktime(&tm);
        return formatDate(tm);
    };

    std::string current = data.startDate;
    while (current <= data.endDate) 
    {
        CalendarEntry entry;
        entry.date = current;
        auto it = countMap.find(current);
        entry.count = (it != countMap.end()) ? it->second : 0;
        data.entries.push_back(entry);
        current = addDays(current, 1);
    }

    return data;
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


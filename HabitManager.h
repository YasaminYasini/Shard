#ifndef HABITMANAGER_H
#define HABITMANAGER_H

#include "Database.h"
#include <string>
#include <vector>
#include <map>

struct Habit 
{
    int id;
    std::string name;
    std::string category;
    int priority;
    std::string createdDate;
    int active;

    std::string lastDone;
    int streak;
    int bestStreak;
    int totalDone;
    double strength;

    Habit()
        : id(0),
          priority(0),
          active(1),
          streak(0),
          bestStreak(0),
          totalDone(0),
          strength(0) {}

    bool operator==(const Habit& other) const 
    {
        return id == other.id &&
               name == other.name &&
               category == other.category &&
               priority == other.priority &&
               createdDate == other.createdDate &&
               active == other.active &&
               lastDone == other.lastDone &&
               streak == other.streak &&
               bestStreak == other.bestStreak &&
               totalDone == other.totalDone &&
               strength == other.strength;
    }

    bool operator!=(const Habit& other) const 
    {
        return !(*this == other);
    }
};

struct HabitHistoryEntry 
{
    int historyId;
    int habitId;
    std::string doneDate;
    int count;

    HabitHistoryEntry()
        : habitId(0),
          count(1) {}

    bool operator==(const HabitHistoryEntry& other) const {
        return habitId == other.habitId &&
               doneDate == other.doneDate &&
               count == other.count;
    }

    bool operator!=(const HabitHistoryEntry& other) const {
        return !(*this == other);
    }
};


struct CalendarEntry {
    std::string date;
    int count;
};


struct HabitCalendarData 
{
    int habitId;
    std::string habitName;
    std::string startDate;
    std::string endDate;
    std::vector<CalendarEntry> entries;
    int maxCount;
    int totalCompletions;
    int streak;
    int bestStreak;

    HabitCalendarData()
        : habitId(0),
          maxCount(0),
          totalCompletions(0),
          streak(0),
          bestStreak(0) {}
};

/**
 * @class HabitManager
 * @brief Manages habit tracking with SQLite persistence.
 * 
 * Provides CRUD operations, streak tracking (via triggers), and history retrieval.
 * All date‑based logic uses SQLite's date functions for consistency.
 */
class HabitManager 
{
private:
    sqlite3* db;

    bool executeSQL(const std::string& sql);
    bool DBCheck();
    std::string getTextOrEmpty(sqlite3_stmt* stmt, int col);

public:
    HabitManager(const std::string& dbPath);
    ~HabitManager();

    // ===== Habit Table Functions =====
    /*
    The CRUD section of the habit manager. 
    items such as date of creation or streak related values are not included here to encapsulate the values
    and any modifications related to the user are implemented in more succinct functions to avoid
    overcomplications (such as archiveHabit).

    functions such as getHabit and getAllHabits are general getters, though streak related values can 
    also be accessed using the streak related functions below in the "Streak Functions" section
    */
    bool addHabit(const std::string& name, const std::string& category = "General", int priority = 3);
    bool deleteHabit(int habitID);
    /**
     * @brief Update an existing habit.
     * @note Unlike addHabit, this does NOT clamp priority. Invalid priorities are rejected.
     * @return false if priority is invalid or no fields changed.
     */
    bool updateHabit(int habitID, const std::string& name = "", const std::string& category = "", int priority=-1);
    Habit getHabit(int habitID);
    bool habitExists(int habitID);
    std::vector<Habit> getAllHabits();
    /**
    * @brief Toggles a habit's active status.
    * 
    * @param archive true = archive (set Active=0), false = unarchive (set Active=1).
    */
    std::vector<Habit> getActiveHabits(bool active = true);
    bool archiveHabit(int habitID, bool archive = true);


    // ===== HabitHistory Table Functions =====
    bool completeHabit(int habitID,  const std::string& completionDate = "date(now)");
    /**
    * @brief Retrieves habit history by days (back from today).
    */
    std::vector<HabitHistoryEntry> getHabitHistory(int habitID, int days = 365);
    /**
    * @brief Retrieves habit history for an explicit date range (inclusive).
    */
    std::vector<HabitHistoryEntry> getHabitHistoryByDateRange(int habitID, const std::string& startDate, const std::string& endDate="now");
    float getSuccessRate(int habitID, int days = 30);
    /**
    * @brief Get calendar heatmap data for a habit over a time window.
    * 
    * @param habitID ID of the habit.
    * @param startDate Start of the window (inclusive), default "now".
    * @param days Number of days to include (default 365).
    * @return HabitCalendarData with zero‑filled gaps and counts per day.
    */
    HabitCalendarData getHabitCalendar(int habitID, const std::string& startDate = "now", int days = 365);
    
    
    // ===== Streak Functions =====
    /**
    * @brief Get an integer column value from the Habit table.
    * @return -1 if habit not found or column invalid.
    */
    int getIntColumn(int habitID, const std::string& columnName);
    int getCurrentStreak(int habitID);
    int getBestStreak(int habitID);
    int getTotalCompletions(int habitID);
};

#endif
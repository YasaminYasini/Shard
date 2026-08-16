#ifndef HABITMANAGER_H
#define HABITMANAGER_H

#include "Database.h"
#include <string>
#include <vector>
#include <map>

struct Habit {
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
          priority(3),
          active(1),
          streak(0),
          bestStreak(0),
          totalDone(0),
          strength(0.5) {}

    bool operator==(const Habit& other) const {
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

    bool operator!=(const Habit& other) const {
        return !(*this == other);
    }
};

struct HabitHistoryEntry {
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

struct HabitCalendarData {
    int habitId;
    std::string habitName;
    std::map<std::string, int> dailyCounts;
    std::vector<std::string> dates;
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

class HabitManager {
private:
    Database& db;

    bool executeSQL(const std::string& sql);
    bool DBCheck();
    std::string getTextOrEmpty(sqlite3_stmt* stmt, int col);

public:
    HabitManager(Database& database);
    ~HabitManager();

    // ===== Habit Table Functions =====
    bool addHabit(const std::string& name, const std::string& category = "General", int priority = 3);
    bool deleteHabit(int habitID);
    bool updateHabit(int habitID, const std::string& name = "", const std::string& category = "", int priority);
    Habit getHabit(int habitID);
    bool habitExists(int habitID);
    std::vector<Habit> getAllHabits();
    std::vector<Habit> getActiveHabits(bool active = true);
    bool archiveHabit(int habitID, bool archive = true);



    // ===== HabitHistory Table Functions =====
    bool completeHabit(int habitID);
    std::vector<HabitHistoryEntry> getHabitHistory(int habitID, int days = 365);
    int getSuccessRate(int habitID, int days = 30);
    HabitCalendarData getHabitCalendar(int habitID);

    // ===== Streak Functions =====
    int getCurrentStreak(int habitID);
    int getBestStreak(int habitID);
    int getTotalCompletions(int habitID);
};

#endif
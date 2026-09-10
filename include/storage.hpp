#pragma once
#include "models.hpp"
#include <string>
#include <vector>
#include <sqlite3.h>

class TrafficDatabase {
public:
    TrafficDatabase(const std::string& db_path);
    ~TrafficDatabase();

    bool insertRecord(const TrafficRecord& record);
    bool insertRecords(const std::vector<TrafficRecord>& records);
    
    std::vector<TrafficRecord> getRecordsForSegment(const std::string& segment_id, int64_t start_time, int64_t end_time);
    std::vector<std::string> getAllSegments();

private:
    void initSchema();
    sqlite3* m_db;
};

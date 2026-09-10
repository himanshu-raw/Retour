#include "storage.hpp"
#include <iostream>
#include <stdexcept>

TrafficDatabase::TrafficDatabase(const std::string& db_path) : m_db(nullptr) {
    if (sqlite3_open(db_path.c_str(), &m_db) != SQLITE_OK) {
        throw std::runtime_error("Failed to open SQLite database: " + std::string(sqlite3_errmsg(m_db)));
    }
    initSchema();
}

TrafficDatabase::~TrafficDatabase() {
    if (m_db) {
        sqlite3_close(m_db);
    }
}

void TrafficDatabase::initSchema() {
    const char* sql = 
        "CREATE TABLE IF NOT EXISTS traffic_records ("
        "segment_id TEXT, "
        "timestamp INTEGER, "
        "speed REAL, "
        "volume INTEGER);"
        "CREATE INDEX IF NOT EXISTS idx_segment_time ON traffic_records(segment_id, timestamp);";
    
    char* err_msg = nullptr;
    if (sqlite3_exec(m_db, sql, 0, 0, &err_msg) != SQLITE_OK) {
        std::cerr << "SQL error: " << err_msg << std::endl;
        sqlite3_free(err_msg);
    }
}

bool TrafficDatabase::insertRecord(const TrafficRecord& record) {
    const char* sql = "INSERT INTO traffic_records (segment_id, timestamp, speed, volume) VALUES (?, ?, ?, ?)";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, record.segment_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, record.timestamp);
    sqlite3_bind_double(stmt, 3, record.speed);
    sqlite3_bind_int(stmt, 4, record.volume);

    bool result = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return result;
}

bool TrafficDatabase::insertRecords(const std::vector<TrafficRecord>& records) {
    sqlite3_exec(m_db, "BEGIN TRANSACTION", nullptr, nullptr, nullptr);
    
    const char* sql = "INSERT INTO traffic_records (segment_id, timestamp, speed, volume) VALUES (?, ?, ?, ?)";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_exec(m_db, "ROLLBACK", nullptr, nullptr, nullptr);
        return false;
    }

    for (const auto& rec : records) {
        sqlite3_bind_text(stmt, 1, rec.segment_id.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 2, rec.timestamp);
        sqlite3_bind_double(stmt, 3, rec.speed);
        sqlite3_bind_int(stmt, 4, rec.volume);
        
        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }
    
    sqlite3_finalize(stmt);
    sqlite3_exec(m_db, "COMMIT", nullptr, nullptr, nullptr);
    return true;
}

std::vector<TrafficRecord> TrafficDatabase::getRecordsForSegment(const std::string& segment_id, int64_t start_time, int64_t end_time) {
    std::vector<TrafficRecord> results;
    const char* sql = "SELECT timestamp, speed, volume FROM traffic_records WHERE segment_id = ? AND timestamp >= ? AND timestamp <= ? ORDER BY timestamp ASC";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, segment_id.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 2, start_time);
        sqlite3_bind_int64(stmt, 3, end_time);
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            TrafficRecord rec;
            rec.segment_id = segment_id;
            rec.timestamp = sqlite3_column_int64(stmt, 0);
            rec.speed = sqlite3_column_double(stmt, 1);
            rec.volume = sqlite3_column_int(stmt, 2);
            results.push_back(rec);
        }
        sqlite3_finalize(stmt);
    }
    return results;
}

std::vector<std::string> TrafficDatabase::getAllSegments() {
    std::vector<std::string> results;
    const char* sql = "SELECT DISTINCT segment_id FROM traffic_records";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::string seg_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            results.push_back(seg_id);
        }
        sqlite3_finalize(stmt);
    }
    return results;
}

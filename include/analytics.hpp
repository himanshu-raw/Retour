#pragma once
#include "models.hpp"
#include <string>
#include <vector>
#include <unordered_map>

struct SegmentMetrics {
    std::string segment_id;
    double current_avg_speed;
    double baseline_speed;
    double current_delay; // delay compared to baseline
    bool is_hotspot;
    bool is_anomaly;
    double z_score;
    double lat = 0.0;
    double lon = 0.0;
};

class AnalyticsEngine {
public:
    AnalyticsEngine();
    ~AnalyticsEngine() = default;

    // Process new batch of records
    void processRecords(const std::vector<TrafficRecord>& records);

    // Get current metrics for a segment
    SegmentMetrics getMetricsForSegment(const std::string& segment_id) const;

    // Get all congested segments ranked by severity
    std::vector<SegmentMetrics> getHotspots() const;

private:
    void updateSegmentMetrics(const std::string& segment_id, const std::vector<double>& recent_speeds);

    // In-memory state for rolling calculations
    std::unordered_map<std::string, std::vector<double>> m_recent_speeds; // window of speeds
    std::unordered_map<std::string, SegmentMetrics> m_current_metrics;
    std::unordered_map<std::string, std::pair<double, double>> m_coordinates; // lat, lon
    
    
    const size_t WINDOW_SIZE = 10; // Number of records for rolling average
    const double FREE_FLOW_SPEED = 65.0; // km/h or mph
    const double HOTSPOT_THRESHOLD_RATIO = 0.5; // less than 50% of free flow speed
    const double ANOMALY_ZSCORE_THRESHOLD = -2.0; // 2 std deviations below mean
};

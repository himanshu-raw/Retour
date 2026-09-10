#include "analytics.hpp"
#include <numeric>
#include <cmath>
#include <algorithm>
#include <iostream>

AnalyticsEngine::AnalyticsEngine() {}

void AnalyticsEngine::processRecords(const std::vector<TrafficRecord>& records) {
    // Group records by segment
    std::unordered_map<std::string, std::vector<double>> incoming_by_segment;
    for (const auto& rec : records) {
        incoming_by_segment[rec.segment_id].push_back(rec.speed);
        
        // Keep track of latest coordinates
        if (rec.lat != 0.0 && rec.lon != 0.0) {
            m_coordinates[rec.segment_id] = {rec.lat, rec.lon};
        }
    }

    for (const auto& [seg_id, speeds] : incoming_by_segment) {
        // Add to window
        auto& window = m_recent_speeds[seg_id];
        for (double s : speeds) {
            window.push_back(s);
            if (window.size() > WINDOW_SIZE) {
                window.erase(window.begin());
            }
        }
        
        updateSegmentMetrics(seg_id, window);
    }
}

void AnalyticsEngine::updateSegmentMetrics(const std::string& segment_id, const std::vector<double>& recent_speeds) {
    if (recent_speeds.empty()) return;

    // Calculate mean
    double sum = std::accumulate(recent_speeds.begin(), recent_speeds.end(), 0.0);
    double mean = sum / recent_speeds.size();

    // Calculate std deviation
    double sq_sum = std::inner_product(recent_speeds.begin(), recent_speeds.end(), recent_speeds.begin(), 0.0);
    double stdev = std::sqrt(sq_sum / recent_speeds.size() - mean * mean);

    double latest_speed = recent_speeds.back();
    
    // Z-Score for the latest speed
    double z_score = 0;
    if (stdev > 0.001) { // avoid division by zero
        // if latest speed is much lower than historical mean
        z_score = (latest_speed - mean) / stdev;
    }

    bool is_hotspot = (latest_speed < (FREE_FLOW_SPEED * HOTSPOT_THRESHOLD_RATIO));
    bool is_anomaly = (z_score < ANOMALY_ZSCORE_THRESHOLD);
    
    double current_delay = FREE_FLOW_SPEED - latest_speed;
    if (current_delay < 0) current_delay = 0;

    SegmentMetrics metrics;
    metrics.segment_id = segment_id;
    metrics.current_avg_speed = mean;
    metrics.baseline_speed = FREE_FLOW_SPEED;
    metrics.current_delay = current_delay;
    metrics.is_hotspot = is_hotspot;
    metrics.is_anomaly = is_anomaly;
    metrics.z_score = z_score;
    
    // Attach coordinates if available
    auto coord_it = m_coordinates.find(segment_id);
    if (coord_it != m_coordinates.end()) {
        metrics.lat = coord_it->second.first;
        metrics.lon = coord_it->second.second;
    }

    m_current_metrics[segment_id] = metrics;
}

SegmentMetrics AnalyticsEngine::getMetricsForSegment(const std::string& segment_id) const {
    auto it = m_current_metrics.find(segment_id);
    if (it != m_current_metrics.end()) {
        return it->second;
    }
    // Return empty if not found
    SegmentMetrics empty{};
    empty.segment_id = segment_id;
    return empty;
}

std::vector<SegmentMetrics> AnalyticsEngine::getHotspots() const {
    std::vector<SegmentMetrics> hotspots;
    for (const auto& [seg_id, metrics] : m_current_metrics) {
        if (metrics.is_hotspot) {
            hotspots.push_back(metrics);
        }
    }
    
    // Sort by severity (highest delay first)
    std::sort(hotspots.begin(), hotspots.end(), [](const SegmentMetrics& a, const SegmentMetrics& b) {
        return a.current_delay > b.current_delay;
    });
    
    return hotspots;
}

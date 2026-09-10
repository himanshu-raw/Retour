#include "forecasting.hpp"
#include <algorithm>

ForecastingModule::ForecastingModule() {}

void ForecastingModule::updateModel(const std::string& segment_id, double actual_speed) {
    auto it = m_smoothed_values.find(segment_id);
    if (it == m_smoothed_values.end()) {
        // Initialize with first observation
        m_smoothed_values[segment_id] = actual_speed;
        m_observation_counts[segment_id] = 1;
    } else {
        // Exponential smoothing update
        double s_prev = it->second;
        double s_new = ALPHA * actual_speed + (1.0 - ALPHA) * s_prev;
        m_smoothed_values[segment_id] = s_new;
        m_observation_counts[segment_id]++;
    }
}

ForecastResult ForecastingModule::predictNext(const std::string& segment_id) const {
    ForecastResult result;
    result.segment_id = segment_id;
    
    auto it = m_smoothed_values.find(segment_id);
    if (it != m_smoothed_values.end()) {
        result.predicted_speed = it->second;
        
        // Simple confidence metric based on observation count (maxes out at 10)
        int counts = m_observation_counts.at(segment_id);
        result.confidence = std::min(1.0, counts / 10.0);
    } else {
        result.predicted_speed = 65.0; // Default to free flow
        result.confidence = 0.0;
    }
    
    return result;
}

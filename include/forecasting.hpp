#pragma once
#include <string>
#include <vector>
#include <unordered_map>

struct ForecastResult {
    std::string segment_id;
    double predicted_speed;
    double confidence; // 0.0 to 1.0
};

class ForecastingModule {
public:
    ForecastingModule();
    ~ForecastingModule() = default;

    // Update internal state with new actual speed
    void updateModel(const std::string& segment_id, double actual_speed);

    // Predict next period's speed using exponential smoothing
    ForecastResult predictNext(const std::string& segment_id) const;

private:
    // Simple exponential smoothing: S_t = alpha * Y_t + (1 - alpha) * S_{t-1}
    const double ALPHA = 0.3; 
    std::unordered_map<std::string, double> m_smoothed_values;
    std::unordered_map<std::string, int> m_observation_counts;
};

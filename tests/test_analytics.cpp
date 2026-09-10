#include <gtest/gtest.h>
#include "analytics.hpp"
#include "forecasting.hpp"
#include "models.hpp"
#include <vector>

TEST(AnalyticsTest, HotspotDetection) {
    AnalyticsEngine engine;
    
    std::vector<TrafficRecord> records;
    for (int i = 0; i < 10; ++i) {
        records.push_back({"SEG_1", i, 20.0, 100}); // Very slow (20 < 65 * 0.5)
        records.push_back({"SEG_2", i, 60.0, 50});  // Normal speed
    }
    
    engine.processRecords(records);
    
    auto hotspots = engine.getHotspots();
    ASSERT_EQ(hotspots.size(), 1);
    EXPECT_EQ(hotspots[0].segment_id, "SEG_1");
    EXPECT_TRUE(hotspots[0].is_hotspot);
    
    auto metrics2 = engine.getMetricsForSegment("SEG_2");
    EXPECT_FALSE(metrics2.is_hotspot);
}

TEST(ForecastingTest, ExponentialSmoothing) {
    ForecastingModule forecast;
    
    forecast.updateModel("SEG_1", 60.0);
    auto res1 = forecast.predictNext("SEG_1");
    EXPECT_DOUBLE_EQ(res1.predicted_speed, 60.0);
    
    forecast.updateModel("SEG_1", 40.0);
    auto res2 = forecast.predictNext("SEG_1");
    // alpha = 0.3
    // S_new = 0.3 * 40.0 + 0.7 * 60.0 = 12.0 + 42.0 = 54.0
    EXPECT_DOUBLE_EQ(res2.predicted_speed, 54.0);
}

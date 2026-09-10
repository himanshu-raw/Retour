#pragma once
#include <string>

struct TrafficRecord {
    std::string segment_id;
    int64_t timestamp;
    double speed;
    int volume;
    double lat = 0.0;
    double lon = 0.0;
};

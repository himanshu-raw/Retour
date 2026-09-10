#pragma once
#include <string>

struct TrafficRecord {
    std::string segment_id;
    int64_t timestamp;
    double speed;
    int volume;
};

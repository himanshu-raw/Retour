#include "ingestion.hpp"
#include <nlohmann/json.hpp>
#include <chrono>
#include <random>
#include <iostream>

using json = nlohmann::json;

DataIngestion::DataIngestion(std::shared_ptr<ConcurrentQueue<TrafficRecord>> queue)
    : m_queue(queue), m_running(false), m_poll_interval_ms(1000) {}

DataIngestion::~DataIngestion() {
    stop();
}

void DataIngestion::start(int poll_interval_ms) {
    if (m_running) return;
    m_poll_interval_ms = poll_interval_ms;
    m_running = true;
    m_worker = std::thread(&DataIngestion::pollLoop, this);
}

void DataIngestion::stop() {
    if (m_running) {
        m_running = false;
        if (m_worker.joinable()) {
            m_worker.join();
        }
    }
}

void DataIngestion::pollLoop() {
    while (m_running) {
        generateSyntheticData();
        std::this_thread::sleep_for(std::chrono::milliseconds(m_poll_interval_ms));
    }
}

void DataIngestion::generateSyntheticData() {
    // We simulate polling an API and parsing JSON using nlohmann/json
    // In a real app, this would be an HTTP GET request to a traffic API
    
    // Create some synthetic JSON response
    json j = json::array();
    
    // Setup RNG
    static std::mt19937 gen(std::random_device{}());
    static std::uniform_real_distribution<> speed_dist(10.0, 70.0);
    static std::uniform_int_distribution<> vol_dist(5, 50);

    auto now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    // 3 segments for example
    for(int i = 1; i <= 3; ++i) {
        std::string seg_id = "SEG_" + std::to_string(i);
        json record = {
            {"segment_id", seg_id},
            {"timestamp", now},
            {"speed", speed_dist(gen)},
            {"volume", vol_dist(gen)}
        };
        j.push_back(record);
    }

    // Parse the JSON array and push to queue
    for (const auto& item : j) {
        try {
            TrafficRecord rec;
            rec.segment_id = item["segment_id"].get<std::string>();
            rec.timestamp = item["timestamp"].get<int64_t>();
            rec.speed = item["speed"].get<double>();
            rec.volume = item["volume"].get<int>();
            
            m_queue->push(rec);
        } catch (const json::exception& e) {
            std::cerr << "JSON Parsing error: " << e.what() << std::endl;
        }
    }
}

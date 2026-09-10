#pragma once
#include "models.hpp"
#include "concurrent_queue.hpp"
#include <thread>
#include <atomic>
#include <memory>
#include <string>

class DataIngestion {
public:
    // Requires HERE API key and a bounding box (e.g. "41.9773,-87.6234;41.9773,-87.6234" depending on HERE API format)
    DataIngestion(std::shared_ptr<ConcurrentQueue<TrafficRecord>> queue, 
                  const std::string& api_key, 
                  const std::string& bbox);
    ~DataIngestion();

    void start(int poll_interval_ms);
    void stop();

private:
    void pollLoop();
    void fetchLiveTrafficData();

    std::shared_ptr<ConcurrentQueue<TrafficRecord>> m_queue;
    std::string m_api_key;
    std::string m_bbox;
    
    std::thread m_worker;
    std::atomic<bool> m_running;
    int m_poll_interval_ms;
};

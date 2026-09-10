#pragma once
#include "models.hpp"
#include "concurrent_queue.hpp"
#include <thread>
#include <atomic>
#include <memory>

class DataIngestion {
public:
    DataIngestion(std::shared_ptr<ConcurrentQueue<TrafficRecord>> queue);
    ~DataIngestion();

    void start(int poll_interval_ms);
    void stop();

private:
    void pollLoop();
    void generateSyntheticData();

    std::shared_ptr<ConcurrentQueue<TrafficRecord>> m_queue;
    std::thread m_worker;
    std::atomic<bool> m_running;
    int m_poll_interval_ms;
};

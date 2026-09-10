#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>

template <typename T>
class ConcurrentQueue {
public:
    void push(const T& item) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_queue.push(item);
        lock.unlock();
        m_cond.notify_one();
    }

    void push(T&& item) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_queue.push(std::move(item));
        lock.unlock();
        m_cond.notify_one();
    }

    std::optional<T> pop() {
        std::unique_lock<std::mutex> lock(m_mutex);
        while (m_queue.empty() && !m_done) {
            m_cond.wait(lock);
        }
        if (m_queue.empty()) {
            return std::nullopt;
        }
        T val = std::move(m_queue.front());
        m_queue.pop();
        return val;
    }

    void finish() {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_done = true;
        lock.unlock();
        m_cond.notify_all();
    }

    bool empty() {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

private:
    std::queue<T> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cond;
    bool m_done = false;
};

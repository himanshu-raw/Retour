#pragma once
#include "analytics.hpp"
#include "storage.hpp"
#include "forecasting.hpp"
#include <httplib.h>
#include <thread>
#include <memory>

class ApiServer {
public:
    ApiServer(int port, 
              std::shared_ptr<AnalyticsEngine> analytics, 
              std::shared_ptr<TrafficDatabase> db,
              std::shared_ptr<ForecastingModule> forecasting);
    ~ApiServer();

    void start();
    void stop();

private:
    void setupRoutes();

    int m_port;
    std::shared_ptr<AnalyticsEngine> m_analytics;
    std::shared_ptr<TrafficDatabase> m_db;
    std::shared_ptr<ForecastingModule> m_forecasting;
    
    httplib::Server m_server;
    std::thread m_server_thread;
};

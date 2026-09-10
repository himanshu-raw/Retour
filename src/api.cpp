#include "api.hpp"
#include <nlohmann/json.hpp>
#include <iostream>

using json = nlohmann::json;

ApiServer::ApiServer(int port, 
                     std::shared_ptr<AnalyticsEngine> analytics, 
                     std::shared_ptr<TrafficDatabase> db,
                     std::shared_ptr<ForecastingModule> forecasting)
    : m_port(port), m_analytics(analytics), m_db(db), m_forecasting(forecasting) {
    setupRoutes();
}

ApiServer::~ApiServer() {
    stop();
}

void ApiServer::setupRoutes() {
    m_server.set_mount_point("/", "./public");

    m_server.Get("/hotspots", [this](const httplib::Request&, httplib::Response& res) {
        auto hotspots = m_analytics->getHotspots();
        json j = json::array();
        for (const auto& hs : hotspots) {
            j.push_back({
                {"segment_id", hs.segment_id},
                {"current_avg_speed", hs.current_avg_speed},
                {"current_delay", hs.current_delay},
                {"is_anomaly", hs.is_anomaly},
                {"z_score", hs.z_score},
                {"lat", hs.lat},
                {"lon", hs.lon}
            });
        }
        res.set_content(j.dump(), "application/json");
    });

    m_server.Get(R"(/segment/(.*)/history)", [this](const httplib::Request& req, httplib::Response& res) {
        std::string segment_id = req.matches[1];
        
        // Let's get history for the last 1 hour (3600 seconds)
        auto now = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        auto records = m_db->getRecordsForSegment(segment_id, now - 3600, now);
        
        json j = json::array();
        for (const auto& r : records) {
            j.push_back({
                {"timestamp", r.timestamp},
                {"speed", r.speed},
                {"volume", r.volume}
            });
        }
        res.set_content(j.dump(), "application/json");
    });

    m_server.Get(R"(/forecast/(.*))", [this](const httplib::Request& req, httplib::Response& res) {
        std::string segment_id = req.matches[1];
        auto forecast = m_forecasting->predictNext(segment_id);
        
        json j = {
            {"segment_id", forecast.segment_id},
            {"predicted_speed", forecast.predicted_speed},
            {"confidence", forecast.confidence}
        };
        res.set_content(j.dump(), "application/json");
    });
}

void ApiServer::start() {
    m_server_thread = std::thread([this]() {
        std::cout << "Starting API Server on port " << m_port << std::endl;
        m_server.listen("0.0.0.0", m_port);
    });
}

void ApiServer::stop() {
    m_server.stop();
    if (m_server_thread.joinable()) {
        m_server_thread.join();
    }
}

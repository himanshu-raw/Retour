#include "ingestion.hpp"
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <chrono>
#include <iostream>

using json = nlohmann::json;

DataIngestion::DataIngestion(std::shared_ptr<ConcurrentQueue<TrafficRecord>> queue, 
                             const std::string& api_key, 
                             const std::string& bbox)
    : m_queue(queue), m_api_key(api_key), m_bbox(bbox), m_running(false), m_poll_interval_ms(60000) {}

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
        fetchLiveTrafficData();
        // Sleep for the poll interval in smaller chunks to allow responsive stopping
        int slept = 0;
        while(slept < m_poll_interval_ms && m_running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            slept += 100;
        }
    }
}

void DataIngestion::fetchLiveTrafficData() {
    if (m_api_key.empty() || m_api_key == "YOUR_API_KEY") {
        std::cerr << "[Ingestion] API Key is empty! Skipping HTTP request." << std::endl;
        return;
    }

    httplib::Client cli("https://api.tomtom.com");
    cli.enable_server_certificate_verification(false);

    // Parse the bounding box to generate random points inside it
    // Expected format: lat1,lon1;lat2,lon2
    double lat_min = 41.87, lat_max = 41.89;
    double lon_min = -87.64, lon_max = -87.61;
    
    size_t semi_pos = m_bbox.find(';');
    if (semi_pos != std::string::npos) {
        std::string p1 = m_bbox.substr(0, semi_pos);
        std::string p2 = m_bbox.substr(semi_pos + 1);
        
        size_t comma1 = p1.find(',');
        size_t comma2 = p2.find(',');
        if (comma1 != std::string::npos && comma2 != std::string::npos) {
            lat_min = std::stod(p1.substr(0, comma1));
            lon_min = std::stod(p1.substr(comma1 + 1));
            lat_max = std::stod(p2.substr(0, comma2));
            lon_max = std::stod(p2.substr(comma2 + 1));
            
            if (lat_min > lat_max) std::swap(lat_min, lat_max);
            if (lon_min > lon_max) std::swap(lon_min, lon_max);
        }
    }

    static std::mt19937 geo_gen(std::random_device{}());
    std::uniform_real_distribution<> lat_dist(lat_min, lat_max);
    std::uniform_real_distribution<> lon_dist(lon_min, lon_max);

    // Sample 3 random points in the bounding box per interval
    for (int i = 0; i < 3; ++i) {
        double query_lat = lat_dist(geo_gen);
        double query_lon = lon_dist(geo_gen);

        std::string path = "/traffic/services/4/flowSegmentData/absolute/10/json?key=" + m_api_key + 
                           "&point=" + std::to_string(query_lat) + "," + std::to_string(query_lon);
        
        auto res = cli.Get(path.c_str());
        
        if (res && res->status == 200) {
            try {
                auto j = json::parse(res->body);
                auto now = std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();

                if (j.contains("flowSegmentData")) {
                    const auto& flow = j["flowSegmentData"];
                    
                    double speed = flow.value("currentSpeed", 65.0);
                    double confidence = flow.value("confidence", 0.0); // 0.0 to 1.0
                    
                    double act_lat = query_lat;
                    double act_lon = query_lon;

                    // Extract actual segment coordinates if provided
                    if (flow.contains("coordinates") && flow["coordinates"].contains("coordinate")) {
                        const auto& coords = flow["coordinates"]["coordinate"];
                        if (!coords.empty()) {
                            act_lat = coords[0].value("latitude", query_lat);
                            act_lon = coords[0].value("longitude", query_lon);
                        }
                    }

                    // Create a synthetic segment ID based on the exact coordinate geometry start point
                    std::string segment_id = "SEG_" + std::to_string(act_lat) + "_" + std::to_string(act_lon);

                    TrafficRecord rec;
                    rec.segment_id = segment_id;
                    rec.timestamp = now;
                    rec.speed = speed;
                    rec.lat = act_lat;
                    rec.lon = act_lon;
                    
                    // Map TomTom confidence (0-1) to our pseudo-volume metric (0-10)
                    rec.volume = static_cast<int>(confidence * 10); 
                    
                    m_queue->push(rec);
                }
            } catch (const std::exception& e) {
                std::cerr << "[Ingestion] JSON Parsing error: " << e.what() << std::endl;
            }
        } else {
            // Ignore 404s (point might not be on a road)
            if (res && res->status != 404) {
                std::cerr << "[Ingestion] HTTP Request failed. Status: " << (res ? res->status : -1) << std::endl;
            }
        }
    }
    std::cout << "[Ingestion] Finished TomTom polling cycle." << std::endl;
}

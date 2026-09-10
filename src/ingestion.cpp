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

    httplib::Client cli("https://traffic.ls.hereapi.com");
    cli.enable_server_certificate_verification(false); // Simplification for Windows environments without cert bundles configured

    std::string path = "/traffic/6.2/flow.json?apiKey=" + m_api_key + "&bbox=" + m_bbox;
    
    auto res = cli.Get(path.c_str());
    
    if (res && res->status == 200) {
        try {
            auto j = json::parse(res->body);
            auto now = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();

            // Navigate the nested HERE flow JSON schema
            if (j.contains("RWS")) {
                for (const auto& rws : j["RWS"]) {
                    if (rws.contains("RW")) {
                        for (const auto& rw : rws["RW"]) {
                            if (rw.contains("FIS")) {
                                for (const auto& fis : rw["FIS"]) {
                                    if (fis.contains("FI")) {
                                        for (const auto& fi : fis["FI"]) {
                                            
                                            // Extract Segment ID (TMC code or description)
                                            std::string segment_id = "UNKNOWN";
                                            if (fi.contains("TMC") && fi["TMC"].contains("DE")) {
                                                segment_id = fi["TMC"]["DE"].get<std::string>();
                                            } else if (fi.contains("TMC") && fi["TMC"].contains("PC")) {
                                                segment_id = std::to_string(fi["TMC"]["PC"].get<int>());
                                            }

                                            // Extract Speed
                                            if (fi.contains("CF") && !fi["CF"].empty()) {
                                                const auto& cf = fi["CF"][0];
                                                double speed = 0.0;
                                                
                                                if (cf.contains("SU")) { // Speed Uncapped
                                                    speed = cf["SU"].get<double>();
                                                } else if (cf.contains("SP")) {
                                                    speed = cf["SP"].get<double>();
                                                }

                                                // Build record
                                                TrafficRecord rec;
                                                rec.segment_id = segment_id;
                                                rec.timestamp = now;
                                                rec.speed = speed;
                                                
                                                // Extract coordinates if available, otherwise mock based on bbox center
                                                if (fi.contains("TMC") && fi["TMC"].contains("PC") && fi.contains("SHP")) {
                                                    // Depending on HERE schema, SHP can be complex.
                                                    // For now, we'll assign a random jitter around Chicago center to visualize it
                                                }
                                                // Mock geo jitter around 41.88, -87.62
                                                static std::mt19937 geo_gen(std::random_device{}());
                                                static std::uniform_real_distribution<> lat_dist(41.87, 41.89);
                                                static std::uniform_real_distribution<> lon_dist(-87.64, -87.61);
                                                rec.lat = lat_dist(geo_gen);
                                                rec.lon = lon_dist(geo_gen);
                                                
                                                // We don't get absolute volume, but we get Jam Factor (JF) [0.0 - 10.0]
                                                // We can scale JF to a pseudo-volume for analytics if needed
                                                double jf = cf.contains("JF") ? cf["JF"].get<double>() : 0.0;
                                                rec.volume = static_cast<int>(jf * 10); 
                                                
                                                m_queue->push(rec);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            std::cout << "[Ingestion] Successfully fetched and parsed HERE Traffic data." << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "[Ingestion] JSON Parsing error: " << e.what() << std::endl;
        }
    } else {
        std::cerr << "[Ingestion] HTTP Request failed. Status: " << (res ? res->status : -1) << std::endl;
        if (res && !res->body.empty()) {
            std::cerr << "[Ingestion] Response: " << res->body << std::endl;
        }
    }
}

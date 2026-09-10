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

    // Instead of random sampling, we hardcode 5 major high-traffic highways in India
    // to GUARANTEE data for the live dashboard!
    struct Coord { double lat; double lon; };
    std::vector<Coord> target_points = {
        {28.59, 77.22}, // Delhi (Outer Ring Road)
        {19.08, 72.85}, // Mumbai (Western Express Highway)
        {12.95, 77.70}, // Bangalore (Outer Ring Road)
        {17.43, 78.34}, // Hyderabad (Outer Ring Road)
        {13.04, 80.25}  // Chennai (Anna Salai)
    };

    // Sample these 5 known high-traffic points per interval
    for (const auto& point : target_points) {
        double query_lat = point.lat;
        double query_lon = point.lon;

        std::string url = "https://api.tomtom.com/traffic/services/4/flowSegmentData/absolute/10/json?key=" + m_api_key + 
                           "&point=" + std::to_string(query_lat) + "," + std::to_string(query_lon);
        
        std::string curl_cmd = "curl -s \"" + url + "\"";
        std::string response_body;
        
#ifdef _WIN32
        FILE* pipe = _popen(curl_cmd.c_str(), "r");
#else
        FILE* pipe = popen(curl_cmd.c_str(), "r");
#endif

        if (pipe) {
            char buffer[256];
            while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                response_body += buffer;
            }
#ifdef _WIN32
            _pclose(pipe);
#else
            pclose(pipe);
#endif

            if (!response_body.empty() && response_body.find("\"flowSegmentData\"") != std::string::npos) {
                try {
                    auto j = json::parse(response_body);
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
        } // closes if (!response_body.empty())
        } // closes if (pipe)
    } // closes for loop
    std::cout << "[Ingestion] Finished TomTom polling cycle." << std::endl;
}

#include "ingestion.hpp"
#include "storage.hpp"
#include "analytics.hpp"
#include "forecasting.hpp"
#include "api.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <cstdlib>

int main() {
    std::cout << "Starting Retour Traffic Analytics System..." << std::endl;

    // Check for HERE API Key in environment
    const char* env_api_key = std::getenv("HERE_API_KEY");
    std::string api_key = env_api_key ? std::string(env_api_key) : "YOUR_API_KEY";
    
    // Default bounding box for Chicago (as an example)
    const char* env_bbox = std::getenv("HERE_BBOX");
    std::string bbox = env_bbox ? std::string(env_bbox) : "41.87,-87.64;41.89,-87.61";

    if (api_key == "YOUR_API_KEY") {
        std::cerr << "[WARNING] HERE_API_KEY environment variable not set!" << std::endl;
        std::cerr << "Live traffic ingestion will skip HTTP requests. Please run with: " << std::endl;
        std::cerr << "  $env:HERE_API_KEY=\"<your_key>\" ; .\\retour_app.exe" << std::endl;
    } else {
        std::cout << "[INFO] Using HERE Traffic API with bounding box: " << bbox << std::endl;
    }

    // Initialize Components
    auto queue = std::make_shared<ConcurrentQueue<TrafficRecord>>();
    auto db = std::make_shared<TrafficDatabase>("traffic.db");
    auto analytics = std::make_shared<AnalyticsEngine>();
    auto forecasting = std::make_shared<ForecastingModule>();

    // Start API Server on port 8080
    ApiServer api(8080, analytics, db, forecasting);
    api.start();

    // Start Data Ingestion (polling every 60 seconds for live API limits)
    DataIngestion ingestion(queue, api_key, bbox);
    ingestion.start(60000);

    // Main processing loop
    std::cout << "Processing loop started." << std::endl;
    while (true) {
        std::vector<TrafficRecord> batch;
        
        // Drain the queue
        while (!queue->empty()) {
            auto opt_rec = queue->pop();
            if (opt_rec) {
                batch.push_back(*opt_rec);
            }
        }

        if (!batch.empty()) {
            // 1. Store in DB
            db->insertRecords(batch);

            // 2. Analytics Engine
            analytics->processRecords(batch);

            // 3. Forecasting Update (using the latest speed from each segment in the batch)
            // Simplified: just update with every record
            for (const auto& rec : batch) {
                forecasting->updateModel(rec.segment_id, rec.speed);
            }
            
            std::cout << "Processed batch of " << batch.size() << " records." << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    return 0;
}

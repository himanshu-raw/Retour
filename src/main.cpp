#include "ingestion.hpp"
#include "storage.hpp"
#include "analytics.hpp"
#include "forecasting.hpp"
#include "api.hpp"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    std::cout << "Starting Retour Traffic Analytics System..." << std::endl;

    // Initialize Components
    auto queue = std::make_shared<ConcurrentQueue<TrafficRecord>>();
    auto db = std::make_shared<TrafficDatabase>("traffic.db");
    auto analytics = std::make_shared<AnalyticsEngine>();
    auto forecasting = std::make_shared<ForecastingModule>();

    // Start API Server on port 8080
    ApiServer api(8080, analytics, db, forecasting);
    api.start();

    // Start Data Ingestion (polling every 2 seconds)
    DataIngestion ingestion(queue);
    ingestion.start(2000);

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

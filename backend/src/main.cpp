#include <cstdlib>
#include <iostream>
#include "aegis/api.hpp"

// Usage: aegis_server [db_path] [database_dir] [port]
int main(int argc, char** argv) {
    const std::string dbPath = argc > 1 ? argv[1] : "../database/aegis.db";
    const std::string dbDir  = argc > 2 ? argv[2] : "../database";
    const int port           = argc > 3 ? std::atoi(argv[3]) : 8080;

    try {
        aegis::Database db(dbPath);
        db.execScript(aegis::readFile(dbDir + "/schema.sql"));
        if (db.scalarInt("SELECT COUNT(*) FROM disasters") == 0) {
            db.execScript(aegis::readFile(dbDir + "/seed.sql"));
            std::cout << "Database seeded with sample scenario.\n";
        }

        aegis::App app;
        aegis::registerDashboardRoutes(app, db);
        aegis::registerMapRoutes(app, db);
        aegis::registerRescueRoutes(app, db);
        aegis::registerResourceRoutes(app, db);
        aegis::registerShelterRoutes(app, db);
        aegis::registerLabRoutes(app, db);
        // Browsers send OPTIONS before cross-origin JSON requests.
        CROW_CATCHALL_ROUTE(app)([](const crow::request& req) {
            if (req.method == crow::HTTPMethod::Options) return crow::response(204);
            return aegis::jsonError(404, "Route not found: " + req.url);
        });

        std::cout << "AEGIS backend listening on http://localhost:" << port << "\n";
        app.port(port).multithreaded().run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << "\n";
        return 1;
    }
    return 0;
}

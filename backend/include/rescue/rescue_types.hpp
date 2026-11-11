#pragma once
#include <string>
#include <vector>

namespace aegis {

enum class SeverityLevel {
    Low = 1,
    Medium = 2,
    High = 3,
    Critical = 4
};

inline std::string severityToString(int level) {
    switch (level) {
        case 4: return "Critical";
        case 3: return "High";
        case 2: return "Medium";
        case 1: default: return "Low";
    }
}

inline int stringToSeverity(const std::string& s) {
    if (s == "Critical" || s == "critical" || s == "5" || s == "4") return 4;
    if (s == "High" || s == "high" || s == "3") return 3;
    if (s == "Medium" || s == "medium" || s == "2") return 2;
    return 1;
}

struct EmergencyRequest {
    int id = 0;
    int disaster_id = 1;
    std::string victim_name;
    std::string emergency_type; // Fire, Flood, Medical, Accident, Building Collapse, Other
    std::string severity;       // Critical, High, Medium, Low
    int severity_level = 1;     // 1 (Low) to 4 (Critical)
    int affected_people = 1;
    int location_id = 0;
    std::string location_name;
    std::string description;
    std::string required_assistance;
    double priority_score = 0.0;
    std::string status = "pending"; // pending, assigned, in_progress, completed
    int assigned_team_id = 0;
    std::string assigned_team_name;
    std::string created_at;
    double waiting_minutes = 0.0;
};

struct RescueTeam {
    int id = 0;
    std::string name;
    std::string team_type; // medical, fire, flood, general
    int current_location_id = 0;
    std::string current_location_name;
    std::string availability = "available"; // available, deployed, offline
    int capacity = 10;
    int current_assignment_id = 0;
};

struct RescueAssignment {
    int id = 0;
    int request_id = 0;
    int team_id = 0;
    std::string team_name;
    std::string assigned_at;
    std::string status = "active";
    double route_distance = 0.0;
    std::vector<int> path_node_ids;
    std::vector<std::string> path_node_names;
};

struct PriorityBreakdown {
    double severity_component = 0.0;
    double people_component = 0.0;
    double waiting_component = 0.0;
    double type_bonus = 0.0;
    double total_score = 0.0;
};

} // namespace aegis

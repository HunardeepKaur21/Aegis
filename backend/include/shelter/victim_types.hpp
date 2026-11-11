#pragma once
#include <string>
#include <vector>

namespace aegis {

struct VictimRecord {
    int id = 0;
    std::string name;
    int age = 0;
    std::string gender = "O";
    std::string health_status = "stable";
    int accompanying_people = 0;
    std::string emergency_category = "Medium"; // Critical, High, Medium, Low
    int current_location_id = 0;
    std::string location_name;
    bool medical_assistance = false;
    int assigned_shelter_id = 0;
    std::string assigned_shelter_name;
    std::string status = "awaiting_shelter"; // awaiting_shelter, sheltered, evacuated
    std::string created_at;
};

struct ShelterInfo {
    int id = 0;
    std::string name;
    int location_id = 0;
    std::string location_name;
    int total_capacity = 0;
    int current_occupancy = 0;
    int available_capacity = 0;
    bool has_medical = false;
    std::string facilities;
    std::string status = "Available"; // Available, Limited Capacity, Full
    double occupancy_pct = 0.0;
};

struct ShelterRecommendation {
    ShelterInfo shelter;
    double distance_km = 0.0;
    std::vector<int> path_node_ids;
    std::vector<std::string> path_node_names;
    double recommendation_score = 0.0;
    bool is_reachable = false;
    std::string reason;
};

struct RecommendationResponse {
    VictimRecord victim;
    bool found = false;
    ShelterRecommendation primary_recommendation;
    std::vector<ShelterRecommendation> alternatives;
    std::string message;
};

} // namespace aegis

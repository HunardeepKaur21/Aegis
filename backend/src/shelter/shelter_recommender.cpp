#include "shelter/shelter_recommender.hpp"
#include <algorithm>
#include <iomanip>
#include <sstream>

namespace aegis {

RecommendationResponse ShelterRecommender::recommendShelter(
    const VictimRecord& victim,
    const std::vector<ShelterInfo>& shelters,
    const Graph& roadGraph,
    const std::map<int, std::string>& locationNames)
{
    RecommendationResponse resp;
    resp.victim = victim;

    int people_needed = 1 + victim.accompanying_people;
    std::vector<ShelterRecommendation> candidates;

    for (const auto& sh : shelters) {
        ShelterRecommendation rec;
        rec.shelter = sh;

        // Check capacity
        int available = sh.total_capacity - sh.current_occupancy;
        if (available < people_needed || sh.status == "Full") {
            rec.is_reachable = false;
            rec.reason = "Shelter full or lacks capacity for " + std::to_string(people_needed) + " people";
            continue;
        }

        // Check if victim location and shelter location are valid nodes in graph
        if (!roadGraph.hasNode(victim.current_location_id) || !roadGraph.hasNode(sh.location_id)) {
            // Same location edge case
            if (victim.current_location_id == sh.location_id && victim.current_location_id > 0) {
                rec.is_reachable = true;
                rec.distance_km = 0.0;
                rec.path_node_ids = {sh.location_id};
                rec.path_node_names = {locationNames.count(sh.location_id) ? locationNames.at(sh.location_id) : "Location " + std::to_string(sh.location_id)};
            } else {
                rec.is_reachable = false;
                rec.reason = "Location not connected to road network";
                continue;
            }
        } else {
            // Run Dijkstra
            PathResult pr = roadGraph.dijkstra(victim.current_location_id, sh.location_id);
            if (!pr.found) {
                rec.is_reachable = false;
                rec.reason = "Unreachable due to road blockages or disconnection";
                continue;
            }

            rec.is_reachable = true;
            rec.distance_km = pr.distance;
            rec.path_node_ids = pr.path;
            for (int nodeId : pr.path) {
                auto it = locationNames.find(nodeId);
                rec.path_node_names.push_back(it != locationNames.end() ? it->second : "Loc #" + std::to_string(nodeId));
            }
        }

        // Calculate score: lower is better
        // Score = distance_km * 1.0 + (occupancy_ratio) * 4.0 - (medical_bonus ? 5.0 : 0.0)
        double occupancy_ratio = sh.total_capacity > 0 ? (static_cast<double>(sh.current_occupancy) / sh.total_capacity) : 1.0;
        double score = rec.distance_km + (occupancy_ratio * 4.0);

        if (victim.medical_assistance) {
            if (sh.has_medical) {
                score -= 6.0; // Strong bonus for medical facilities when required
            } else {
                score += 8.0; // Penalty if victim needs medical help but shelter lacks it
            }
        }

        rec.recommendation_score = score;

        std::ostringstream ss;
        ss << std::fixed << std::setprecision(1) << rec.distance_km << " km away, " 
           << (sh.total_capacity - sh.current_occupancy) << " spots available";
        if (victim.medical_assistance && sh.has_medical) {
            ss << " (Equipped with Medical Bay)";
        }
        rec.reason = ss.str();

        candidates.push_back(std::move(rec));
    }

    if (candidates.empty()) {
        resp.found = false;
        resp.message = "No available shelters found with sufficient capacity or open road routes.";
        return resp;
    }

    // Sort candidates by score ascending (best first)
    std::sort(candidates.begin(), candidates.end(), [](const ShelterRecommendation& a, const ShelterRecommendation& b) {
        if (std::abs(a.recommendation_score - b.recommendation_score) > 1e-4) {
            return a.recommendation_score < b.recommendation_score;
        }
        // Tie breaker: lower distance, then higher remaining capacity
        if (std::abs(a.distance_km - b.distance_km) > 1e-4) {
            return a.distance_km < b.distance_km;
        }
        return (a.shelter.total_capacity - a.shelter.current_occupancy) > (b.shelter.total_capacity - b.shelter.current_occupancy);
    });

    resp.found = true;
    resp.primary_recommendation = candidates[0];
    for (size_t i = 1; i < candidates.size(); ++i) {
        resp.alternatives.push_back(candidates[i]);
    }

    std::ostringstream msg;
    msg << "Recommended '" << resp.primary_recommendation.shelter.name << "' as optimal destination ("
        << std::fixed << std::setprecision(1) << resp.primary_recommendation.distance_km 
        << " km, " << (resp.primary_recommendation.shelter.total_capacity - resp.primary_recommendation.shelter.current_occupancy) 
        << " available spots).";
    resp.message = msg.str();

    return resp;
}

} // namespace aegis

#pragma once
#include <map>
#include <string>
#include <vector>
#include "graphs/graph.hpp"
#include "shelter/victim_types.hpp"

namespace aegis {

class ShelterRecommender {
public:
    static RecommendationResponse recommendShelter(
        const VictimRecord& victim,
        const std::vector<ShelterInfo>& shelters,
        const Graph& roadGraph,
        const std::map<int, std::string>& locationNames);
};

} // namespace aegis

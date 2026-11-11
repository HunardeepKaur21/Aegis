#pragma once
#include <string>
#include <vector>
#include "lab/lab_types.hpp"
#include "lab/lab_sorting.hpp"
#include "lab/lab_searching.hpp"
#include "lab/lab_dp.hpp"
#include "lab/lab_structures.hpp"

namespace aegis {

class LabEngine {
public:
    static std::vector<AlgorithmMeta> getAlgorithmCatalog();

    static LabRunResult execute(
        const std::string& category,
        const std::string& algorithm,
        const std::vector<int>& input_data,
        const std::string& operation = "",
        int param_val = 0,
        const std::string& str_param1 = "",
        const std::string& str_param2 = "");

    static LabCompareResult compare(
        const std::string& algo_a,
        const std::string& algo_b,
        const std::vector<int>& input_data);
};

} // namespace aegis

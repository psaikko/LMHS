#pragma once

#include <vector>
#include <unordered_map>

#include "Defines.h"

namespace NonoptHS {

void common(std::vector<int>& out_hs,
            const std::vector<std::vector<int>>& new_cores,
            const std::vector<std::vector<int>>&,
            const std::unordered_map<int, weight_t>&,
            const std::unordered_map<int, unsigned>& coreClauseCounts);

void greedy(std::vector<int>& out_hs,
            const std::vector<std::vector<int>>& new_cores,
            const std::vector<std::vector<int>>&,
            const std::unordered_map<int, weight_t>&,
            const std::unordered_map<int, unsigned>& coreClauseCounts);

NonOptHSFunc frac(double fracSize);

void disjoint(std::vector<int>& out_hs,
              const std::vector<std::vector<int>>& new_cores,
              const std::vector<std::vector<int>>&,
              const std::unordered_map<int, weight_t>&,
              const std::unordered_map<int, unsigned>& coreClauseCounts);
}
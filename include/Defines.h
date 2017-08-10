#pragma once

#include <functional>
#include <vector>
#include <unordered_map>

#include "Weights.h"

typedef std::function<void(
    std::vector<int>& out_hs, const std::vector<std::vector<int>>& new_cores,
    const std::vector<std::vector<int>>& cores,
    const std::unordered_map<int, weight_t>& weights,
    const std::unordered_map<int, unsigned>& coreClauseCounts)> NonOptHSFunc;

enum MinimizeAlgorithm {rerefute, constructive, binary, destructive, cardinality};
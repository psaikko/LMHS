#ifndef NONOPT_HS
#define NONOPT_HS

#include <vector>
#include <unordered_map>
#include <functional>

namespace NonoptHS {

typedef std::function<void(
    std::vector<int>& out_hs, const std::vector<std::vector<int>>& new_cores,
    const std::vector<std::vector<int>>& cores,
    const std::unordered_map<int, double>& weights,
    const std::unordered_map<int, unsigned>& coreClauseCounts)> funcType;

void common(std::vector<int>& out_hs,
            const std::vector<std::vector<int>>& new_cores,
            const std::vector<std::vector<int>>&,
            const std::unordered_map<int, double>&,
            const std::unordered_map<int, unsigned>& coreClauseCounts);

void greedy(std::vector<int>& out_hs,
            const std::vector<std::vector<int>>& new_cores,
            const std::vector<std::vector<int>>&,
            const std::unordered_map<int, double>&,
            const std::unordered_map<int, unsigned>& coreClauseCounts);

funcType frac(double fracSize);

void disjoint(std::vector<int>& out_hs,
              const std::vector<std::vector<int>>& new_cores,
              const std::vector<std::vector<int>>&,
              const std::unordered_map<int, double>&,
              const std::unordered_map<int, unsigned>& coreClauseCounts);
}

#endif
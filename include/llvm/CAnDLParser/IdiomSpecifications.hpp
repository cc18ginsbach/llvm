#ifndef _CONSTRAINTCOMPOSED_HPP_
#define _CONSTRAINTCOMPOSED_HPP_
#include "llvm/Support/Timer.h"
#include "Solution.hpp"
#include <climits>
#include <string>
#include <vector>

namespace llvm {
class Function;
class Value;
}

// This file containts the constraint specifications that are used for idiom detection.
//using IdiomSpecification = std::vector<std::pair<std::string,std::unique_ptr<SolverAtom>>>;
using IdiomSpecification = std::vector<Solution>;
IdiomSpecification DetectDistributive(llvm::Function& function, unsigned max_solutions = UINT_MAX);
IdiomSpecification DetectHoistSelect (llvm::Function& function, unsigned max_solutions = UINT_MAX);
IdiomSpecification DetectSCoP        (llvm::Function& function, unsigned max_solutions = UINT_MAX);
IdiomSpecification DetectExperiment  (llvm::Function& function, unsigned max_solutions = UINT_MAX);

#endif

#ifndef _SOLUTION_HPP_
#define _SOLUTION_HPP_
#include <unordered_map>
#include <climits>
#include <memory>
#include <vector>
#include <string>
#include <map>

class FunctionWrap;
class SolverAtom;

namespace llvm {
class ModuleSlotTracker;
class Function;
class Value;
}

class Solution
{
public:
    operator llvm::Value* () const;
    operator std::map<std::string,llvm::Value*> () const;

    Solution operator[](std::string str) const;
    Solution operator[](unsigned idx) const;

    Solution get() const { return *this; }

    template<typename T1, typename ... Tn>
    Solution get(T1 t1, Tn ... tn) const { return operator[](t1).get(tn...); }

    Solution prune() const;
    std::string print_json(llvm::ModuleSlotTracker&) const;

    static std::vector<Solution> Find(std::vector<std::pair<std::string,std::unique_ptr<SolverAtom>>> atoms,
                                      llvm::Function& function, unsigned max_solutions = UINT_MAX);

private:
    Solution() : single_value(nullptr) {}
    Solution(std::vector<std::string> labels, std::vector<llvm::Value*> values);

    void set_precomputed_strings(std::shared_ptr<std::unordered_map<llvm::Value*,std::string>>);

    llvm::Value*                                                  single_value;
    std::vector<Solution>                                         vector_value;
    std::vector<std::pair<std::string,Solution>>                  map_value;
    std::shared_ptr<std::unordered_map<llvm::Value*,std::string>> instr_strings;
};

#endif

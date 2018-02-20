#include "llvm/CAnDLPasses/CustomPasses.hpp"
#include "llvm/CAnDLParser/IdiomSpecifications.hpp"
#include "llvm/CAnDLPasses/Transforms.hpp"
#include "llvm/IR/ModuleSlotTracker.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include <string>
#include <vector>
#include <map>

using namespace llvm;

class ResearchPreprocessor : public ModulePass
{
public:
    static char ID;

    ResearchPreprocessor() : ModulePass(ID),
                         constraint_specs{{"HOISTSELECT",  DetectHoistSelect,  &transform_hoistselect_pattern},
                                          {"DISTRIBUTIVE", DetectDistributive, &transform_distributive}} { }

    bool runOnModule(Module& module) override;

private:
    std::vector<std::tuple<std::string,IdiomSpecification(*)(llvm::Function&,unsigned),
                                       void(*)(Function&,std::map<std::string,Value*>)>> constraint_specs;
};

bool ResearchPreprocessor::runOnModule(Module& module)
{
    ModuleSlotTracker slot_tracker(&module);

    std::vector<Value*> removed_instructions;

    for(Function& function : module.getFunctionList())
    {
        if(!function.isDeclaration())
        {
            for(const auto& spec : constraint_specs)
            {
                auto solutions = std::get<1>(spec)(function, UINT_MAX);

                if(!solutions.empty())
                {
                    for(auto& solution : solutions)
                    {
                        std::map<std::string,llvm::Value*> solution_map = solution;

                        auto find_it = solution_map.find("value");
                        if(find_it == solution_map.end())
                            find_it = solution_map.find("select");


                        bool do_continue = false;
                        for(auto& entry: solution_map)
                        {
                            auto find_it2 = std::find(removed_instructions.begin(),
                                                      removed_instructions.end(), entry.second);

                            if(find_it2 != removed_instructions.end())
                            {
                                do_continue = true;
                                break;
                            }
                        }

                        if(do_continue)
                            continue;

                        (*std::get<2>(spec))(function, solution);

                        if(find_it != solution_map.end())
                        {
                            removed_instructions.push_back(find_it->second);
                        }
                    }
                }
            }
        }
    }

    return false;
}

char ResearchPreprocessor::ID = 0;

static RegisterPass<ResearchPreprocessor> X("research-preprocessor", "Research preprocessor", false, false);

ModulePass *llvm::createResearchPreprocessorPass() {
  return new ResearchPreprocessor();
}

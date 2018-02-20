#include "llvm/CAnDLPasses/CustomPasses.hpp"
#include "llvm/CAnDLParser/IdiomSpecifications.hpp"
#include "llvm/CAnDLPasses/Transforms.hpp"
#include "llvm/IR/ModuleSlotTracker.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <map>

using namespace llvm;

class ResearchReplacer : public ModulePass
{
public:
    static char ID;

    ResearchReplacer() : ModulePass(ID),
                         simple_constraint_specs{{"Experiment", DetectExperiment}},
                         loop_constraint_specs{{"SCoP", DetectSCoP}} { }

    bool runOnModule(Module& module) override;

private:
    std::vector<std::pair<std::string,IdiomSpecification(*)(Function&,unsigned)>> simple_constraint_specs;
    std::vector<std::pair<std::string,IdiomSpecification(*)(Function&,unsigned)>> loop_constraint_specs;
};

bool ResearchReplacer::runOnModule(Module& module)
{
    ModuleSlotTracker slot_tracker(&module);

    std::stringstream sstr;
    sstr<<"replace-report-"<<(std::string)module.getName()<<".json";
    std::ofstream ofs(sstr.str().c_str());
    ofs<<"{\n  \"filename\": \""<<(std::string)module.getName()<<"\",\n  \"loops\": [";

    char first_hit1 = true;
    for(Function& function : module.getFunctionList())
    {
        if(!function.isDeclaration())
        {
            for(const auto& spec : loop_constraint_specs)
            {
                std::vector<Solution> solutions = spec.second(function,100);

                for(const auto& solution : solutions)
                {
                    Instruction* precursor = dyn_cast<Instruction>((Value*)solution.get("precursor"));

                    unsigned line_begin = UINT_MAX;
                    if(precursor && precursor->getDebugLoc())
                        line_begin = precursor->getDebugLoc().getLine();

                    ofs<<(first_hit1?"\n":",\n");
                    ofs<<"    {\n      \"function\": \""<<(std::string)function.getName()<<"\",\n";
                    if(line_begin != UINT_MAX)
                        ofs<<"      \"line\": "<<line_begin<<",\n";
                    else
                        ofs<<"      \"line\": \"???\",\n";
                    ofs<<"      \"idioms\": [\n";
                    ofs<<"\n    {\n      \"type\": \""<<spec.first<<"\",\n";
                    ofs<<"      \"solution\":\n        ";
                    for(char c : solution.prune().print_json(slot_tracker))
                    {
                        ofs.put(c);
                        if(c == '\n') ofs<<"        ";
                    }
                    ofs<<"\n    }";
                    ofs<<"]";
                    ofs<<"\n    }";
                    first_hit1 = false;
                }
            }
        }
    }

    ofs<<"],\n  \"transformations\": [";

    char first_hit = true;
    for(Function& function : module.getFunctionList())
    {
        if(!function.isDeclaration())
        {
            for(const auto& spec : simple_constraint_specs)
            {
                auto solutions = spec.second(function, UINT_MAX);

                if(!solutions.empty())
                {
                    for(auto& solution : solutions)
                    {
                        ofs<<(first_hit?"\n":",\n");
                        ofs<<"    {\n      \"function\": \""<<(std::string)function.getName()<<"\",\n";
                        ofs<<"      \"type\": \""<<spec.first<<"\",\n";
                        ofs<<"      \"solution\":\n        ";
                        for(char c : solution.prune().print_json(slot_tracker))
                        {
                            ofs.put(c);
                            if(c == '\n') ofs<<"        ";
                        }
                        ofs<<"\n    }";
                        first_hit = false;
                    }
                }
            }
        }
    }

    ofs<<"]\n}\n";

    return false;
}

char ResearchReplacer::ID = 0;

static RegisterPass<ResearchReplacer> X("research-replacer", "Research replacer", false, false);

ModulePass *llvm::createResearchReplacerPass() {
  return new ResearchReplacer();
}

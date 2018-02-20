#include "llvm/CAnDLPasses/CustomPasses.hpp"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"

using namespace llvm;

class ResearchMergePointercalcs : public FunctionPass
{
public:
    static char ID;

    ResearchMergePointercalcs() : FunctionPass(ID) {}

    bool runOnFunction(Function& function) override;
};

bool ResearchMergePointercalcs::runOnFunction(Function& function)
{
    // Remove Premature GEP instructions
    std::vector<std::pair<Instruction*,Instruction*>> replacements;

    for(BasicBlock& block : function.getBasicBlockList())
    {
        for(Instruction& instruction : block.getInstList())
        {
            if(auto gep_inst = dyn_cast<GetElementPtrInst>(&instruction))
            {
                std::vector<Value*> reverse_index_vector;
                for(unsigned i = 1; i < gep_inst->getNumOperands(); i++)
                {
                    reverse_index_vector.push_back(gep_inst->getOperand(gep_inst->getNumOperands()-i));
                }

                while(auto gep_origin = dyn_cast<GetElementPtrInst>(gep_inst->getOperand(0)))
                {
                    auto first_arg  = gep_origin->getOperand(gep_origin->getNumOperands()-1);
                    auto second_arg = reverse_index_vector.back();

                    auto first_const_arg  = dyn_cast<ConstantInt>(first_arg);
                    auto second_const_arg = dyn_cast<ConstantInt>(second_arg);

                    Value* summed_index = nullptr;

                    if(second_const_arg && second_const_arg->getSExtValue() == 0)
                    {
                        summed_index = gep_origin->getOperand(gep_origin->getNumOperands()-1);
                    }
                    else if(first_const_arg && first_const_arg->getSExtValue() == 0)
                    {
                        summed_index = reverse_index_vector.back();
                    }
                    else
                    {
                        summed_index = BinaryOperator::Create(Instruction::Add,
                                                              gep_origin->getOperand(gep_origin->getNumOperands()-1),
                                                              reverse_index_vector.back(), "", &instruction);
                    }

                    reverse_index_vector.back() = summed_index;

                    for(unsigned i = 2; i < gep_origin->getNumOperands(); i++)
                    {
                        reverse_index_vector.push_back(gep_origin->getOperand(gep_origin->getNumOperands()-i));
                    }

                    gep_inst = gep_origin;
                }

                if(gep_inst != &instruction)
                {
                    std::reverse(reverse_index_vector.begin(), reverse_index_vector.end());

                    auto* new_gep_instr = GetElementPtrInst::Create(gep_inst->getSourceElementType(),
                                                                    gep_inst->getOperand(0),
                                                                    reverse_index_vector);

                    replacements.emplace_back(&instruction, new_gep_instr);
                }
            }
        }
    }

    for(auto entry : replacements)
    {
        ReplaceInstWithInst(entry.first, entry.second);
    }

    return false;
}

char ResearchMergePointercalcs::ID = 0;

static RegisterPass<ResearchMergePointercalcs> X("research-merge-pointercalcs", "Research merge pointer calculations", false, false);

FunctionPass *llvm::createResearchMergePointercalcsPass() {
  return new ResearchMergePointercalcs();
}

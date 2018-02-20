#include "llvm/CAnDLPasses/CustomPasses.hpp"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"

using namespace llvm;

class ResearchFixOrToAdd : public BasicBlockPass
{
public:
    static char ID;

    ResearchFixOrToAdd() : BasicBlockPass(ID) {}

    bool runOnBasicBlock(BasicBlock& block) override;
};

bool ResearchFixOrToAdd::runOnBasicBlock(BasicBlock& block)
{
    std::vector<BasicBlock::iterator> worklist;
    for(Instruction& instruction : block.getInstList())
    {
        Instruction* mul_inst;
        ConstantInt* or_const_arg;
        ConstantInt* mul_const_arg;

        if(instruction.getOpcode() == Instruction::Or &&
           (mul_inst = dyn_cast<Instruction>(instruction.getOperand(0))) &&
           mul_inst->getOpcode() == Instruction::Mul &&
           (or_const_arg = dyn_cast<ConstantInt>(instruction.getOperand(1))) &&
           (mul_const_arg = dyn_cast<ConstantInt>(mul_inst->getOperand(1))))
        {
            auto mul_const = mul_const_arg->getZExtValue();
            auto or_const  = or_const_arg->getZExtValue();

            while(mul_const % 2 == 0)
            {
                mul_const /= 2;
                or_const  /= 2;
            }

            if(!or_const)
                worklist.emplace_back(&instruction);
        }
    }

    for(auto& entry : worklist)
    {
        IRBuilder<> builder(&block, entry);
        ReplaceInstWithValue(block.getInstList(), entry,
                             builder.CreateAdd(entry->getOperand(0), entry->getOperand(1)));
    }

    return !worklist.empty();
}

char ResearchFixOrToAdd::ID = 0;

static RegisterPass<ResearchFixOrToAdd> X("research-fix-or-to-add", "Research fix or to add", false, false);

BasicBlockPass *llvm::createResearchFixOrToAddPass() {
  return new ResearchFixOrToAdd();
}

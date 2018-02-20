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

class ResearchFixShlToMul : public BasicBlockPass
{
public:
    static char ID;

    ResearchFixShlToMul() : BasicBlockPass(ID) {}

    bool runOnBasicBlock(BasicBlock& module) override;
};

bool ResearchFixShlToMul::runOnBasicBlock(BasicBlock& block)
{
    std::vector<std::pair<BasicBlock::iterator,ConstantInt*>> worklist;
    for(Instruction& instruction : block.getInstList())
    {
        ConstantInt* constant;

        if(instruction.getOpcode() == Instruction::Shl &&
           (constant = dyn_cast<ConstantInt>(instruction.getOperand(1))))
        {
            worklist.emplace_back(&instruction, ConstantInt::get(constant->getType(), 1<<constant->getZExtValue()));
        }
    }

    for(auto& entry : worklist)
    {
        IRBuilder<> builder(&block, entry.first);
        ReplaceInstWithValue(block.getInstList(), entry.first,
                             builder.CreateMul(entry.first->getOperand(0), entry.second));
    }

    return !worklist.empty();
}

char ResearchFixShlToMul::ID = 0;

static RegisterPass<ResearchFixShlToMul> X("research-fix-shl-to-mul", "Research fix shl to mul", false, false);

BasicBlockPass *llvm::createResearchFixShlToMulPass() {
  return new ResearchFixShlToMul();
}

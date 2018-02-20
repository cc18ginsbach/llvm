#include "llvm/CAnDLPasses/Transforms.hpp"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Value.h"
#include <sstream>
#include <string>
#include <vector>

using namespace llvm;

void transform_hoistselect_pattern(Function& function, std::map<std::string,Value*> solution)
{
    SelectInst*        select_inst  = dyn_cast<SelectInst>       (solution["select"]);
    GetElementPtrInst* gep_inst1    = dyn_cast<GetElementPtrInst>(solution["input1"]);
    GetElementPtrInst* gep_inst2    = dyn_cast<GetElementPtrInst>(solution["input2"]);
    Value*             base_pointer = solution["base"];

    std::vector<Value*> index_list;

    for(unsigned i = 1; i < gep_inst1->getNumOperands(); i++)
    {
        index_list.push_back(SelectInst::Create(select_inst->getCondition(), gep_inst1->getOperand(i),
                                                gep_inst2->getOperand(i), "", select_inst));
    }

    GetElementPtrInst* new_gep = GetElementPtrInst::Create(gep_inst1->getSourceElementType(),
                                                           base_pointer, index_list, "", select_inst);

    for(BasicBlock& basic_block : function.getBasicBlockList())
    {
        for(Instruction& instruction : basic_block.getInstList())
        {
            for(unsigned i = 0; i < instruction.getNumOperands(); i++)
                if(instruction.getOperand(i) == select_inst)
                    instruction.setOperand(i, new_gep);
        }
    }

    select_inst->eraseFromParent();
}

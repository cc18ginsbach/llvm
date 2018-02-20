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

class ResearchPointerarithmetic : public BasicBlockPass
{
public:
    static char ID;

    ResearchPointerarithmetic() : BasicBlockPass(ID) {}

    bool runOnBasicBlock(BasicBlock& module) override;
};

Value* smart_add(IRBuilder<>& builder, Value* a, ConstantInt* b)
{
    if(b->isZero()) return a;

    BinaryOperator* binop;
    BinaryOperator* binop2;
    ConstantInt*    c;
    ConstantInt*    d;

    if((binop = dyn_cast<BinaryOperator>(a)) &&
       binop->getOpcode() == Instruction::Add &&
       (c = dyn_cast<ConstantInt>(binop->getOperand(1))))
    {
        return builder.CreateAdd(binop->getOperand(0),
                                 builder.CreateAdd(c, b));
    }
    if((binop = dyn_cast<BinaryOperator>(a)) &&
       binop->getOpcode() == Instruction::Mul &&
       (c = dyn_cast<ConstantInt>(binop->getOperand(1))) &&
       (binop2 = dyn_cast<BinaryOperator>(binop->getOperand(0))) &&
       binop2->getOpcode() == Instruction::Add &&
       (d = dyn_cast<ConstantInt>(binop2->getOperand(1))) &&
       b->getZExtValue() % c->getZExtValue() == 0)
    {
        return builder.CreateMul(builder.CreateAdd(binop2->getOperand(0),
                                                   builder.CreateAdd(d, builder.CreateUDiv(b, c))), c);
    }
    return builder.CreateAdd(a, b);
}

Value* smart_add(IRBuilder<>& builder, Value* a, Value* b)
{
    ConstantInt* c;
    if((c = dyn_cast<ConstantInt>(a))) return smart_add(builder, b, c);
    if((c = dyn_cast<ConstantInt>(b))) return smart_add(builder, a, c);

    
    BinaryOperator* binop1;
    BinaryOperator* binop2;
    ConstantInt*    fac1;
    ConstantInt*    fac2;

    if((binop1 = dyn_cast<BinaryOperator>(a)) &&
       binop1->getOpcode() == Instruction::Mul &&
       (fac1 = dyn_cast<ConstantInt>(binop1->getOperand(1))) &&
       (binop2 = dyn_cast<BinaryOperator>(a)) &&
       binop2->getOpcode() == Instruction::Mul &&
       (fac2 = dyn_cast<ConstantInt>(binop2->getOperand(1))))
    {
        if(fac1->getZExtValue() % fac2->getZExtValue() == 0)
        {
            return builder.CreateMul(builder.CreateAdd(builder.CreateMul(binop1->getOperand(0),
                                                                         builder.CreateUDiv(fac1, fac2)),
                                                       binop2->getOperand(0)), fac2);
        }
        if(fac2->getZExtValue() % fac1->getZExtValue() == 0)
        {
            return builder.CreateMul(builder.CreateAdd(binop1->getOperand(0),
                                                       builder.CreateMul(binop2->getOperand(0),
                                                                         builder.CreateUDiv(fac2, fac1))), fac1);
        }        
    }

    return builder.CreateAdd(a, b);
}


Value* smart_mul(IRBuilder<>& builder, Value* a, ConstantInt* b)
{
    if(b->isOne()) return a;

    BinaryOperator* binop;
    BinaryOperator* binop2;
    ConstantInt*    c;
    ConstantInt*    d;

    if((binop = dyn_cast<BinaryOperator>(a)) &&
       binop->getOpcode() == Instruction::Mul &&
       (c = dyn_cast<ConstantInt>(binop->getOperand(1))))
    {
        return builder.CreateMul(binop->getOperand(0),
                                 builder.CreateMul(c, b));
    }
    if((binop = dyn_cast<BinaryOperator>(a)) &&
       binop->getOpcode() == Instruction::Add &&
       (c = dyn_cast<ConstantInt>(binop->getOperand(1))) &&
       (binop2 = dyn_cast<BinaryOperator>(binop->getOperand(0))) &&
       binop2->getOpcode() == Instruction::Mul &&
       (d = dyn_cast<ConstantInt>(binop2->getOperand(1))))
    {
        return builder.CreateAdd(builder.CreateMul(binop2->getOperand(0),
                                                   builder.CreateMul(d, b)),
                                 builder.CreateMul(c, b));
    }
    return builder.CreateMul(a, b);
}

Value* smart_mul(IRBuilder<>& builder, Value* a, Value* b)
{
    ConstantInt* c;
    if((c = dyn_cast<ConstantInt>(a))) return smart_mul(builder, b, c);
    if((c = dyn_cast<ConstantInt>(b))) return smart_mul(builder, a, c);
    return builder.CreateMul(a, b);
}

bool ResearchPointerarithmetic::runOnBasicBlock(BasicBlock& block)
{
    DataLayout data_layout(block.getParent()->getParent());

    std::vector<std::pair<BasicBlock::iterator,PointerType*>> worklist;
    for(Instruction& instruction : block.getInstList())
    {
        GetElementPtrInst* gepinst;
        PointerType*       pointertype;

        if((gepinst = dyn_cast<GetElementPtrInst>(&instruction)) &&
           (pointertype = dyn_cast<PointerType>(gepinst->getPointerOperandType())) &&
           !pointertype->getElementType()->isIntegerTy(8))
        {
            
            worklist.emplace_back(&instruction, pointertype);
        }
    }

    for(auto& entry : worklist)
    {
        IRBuilder<> builder(&block, entry.first);

        Value* index = ConstantInt::get(entry.first->getOperand(1)->getType(), 0);
        Type*  type  = entry.second;

        std::vector<Value*> indizes;
        std::vector<size_t> factors;

        for(unsigned i = 1; i < entry.first->getNumOperands(); i++)
        {
            Value*          operand = entry.first->getOperand(i);

            PointerType*    pointer_type;
            SequentialType* seq_type;
            StructType*     struct_type;
            ConstantInt*    struct_member;

            if((pointer_type = dyn_cast<PointerType>(type)))
            {
                index = smart_add(builder,
                            index,
                            smart_mul(builder,
                                operand,
                                ConstantInt::get(
                                    operand->getType(),
                                    data_layout.getTypeAllocSize(pointer_type->getElementType()))));
                type = pointer_type->getElementType();
            }
            else if((seq_type = dyn_cast<SequentialType>(type)))
            {
                index = smart_add(builder,
                            index,
                            smart_mul(builder,
                                operand,
                                ConstantInt::get(
                                    operand->getType(),
                                    data_layout.getTypeAllocSize(seq_type->getElementType()))));
                type = seq_type->getElementType();
            }
            else if((struct_type = dyn_cast<StructType>(type)) &&
                    (struct_member = dyn_cast<ConstantInt>(operand)))
            {
                unsigned member_nr = struct_member->getZExtValue();

                auto offset = data_layout.getStructLayout(struct_type)->getElementOffset(member_nr);

                index = smart_add(builder,
                            index,
                            ConstantInt::get(
                                operand->getType(),
                                offset));
                type = struct_type->getElementType(member_nr);
            }
        }

        ReplaceInstWithValue(block.getInstList(), entry.first,
            builder.CreateBitCast(
                builder.CreateGEP(
                    builder.CreateBitCast(
                        entry.first->getOperand(0),
                        Type::getInt8PtrTy(entry.first->getContext(),
                                           entry.second->getAddressSpace())),
                    index),
                entry.first->getType()));
    }

    return !worklist.empty();
}

char ResearchPointerarithmetic::ID = 0;

static RegisterPass<ResearchPointerarithmetic> X1("research-pointerarithmetic", "Research pointerarithmetic", false, false);


BasicBlockPass *llvm::createResearchPointerarithmeticPass() {
  return new ResearchPointerarithmetic();
}

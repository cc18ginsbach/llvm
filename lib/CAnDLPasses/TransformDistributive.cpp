#include "llvm/CAnDLPasses/Transforms.hpp"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Value.h"
#include <sstream>
#include <string>
#include <vector>

using namespace llvm;

void transform(std::map<std::string,Value*> solution, Function* abs)
{
    llvm::ReplaceInstWithInst(llvm::dyn_cast<Instruction>(solution["sqrt_call"]),
                              llvm::CallInst::Create(abs, {solution["a"]}));
}

void transform_distributive(Function& function, std::map<std::string,Value*> solution)
{
    Instruction* sum   = dyn_cast<Instruction>(solution["value"]);
    Value* first_prod  = nullptr;
    Value* second_prod = nullptr;

    std::vector<Value*> prod1_factors;
    std::vector<Value*> prod2_factors;
    std::vector<Value*> sum1_factors;
    std::vector<Value*> sum2_factors;
    std::vector<Value*> sum1_results;
    std::vector<Value*> sum2_results;

    for(unsigned i = 0; i < 16 ; i++)
    {
        std::stringstream index_sstr;
        index_sstr<<"["<<i<<"]";

        auto find_prod1_it   = solution.find("product1.factors"+index_sstr.str());
        auto find_prod2_it   = solution.find("product2.factors"+index_sstr.str());
        auto find_sum1_it    = solution.find("sum1.factors"+index_sstr.str());
        auto find_sum2_it    = solution.find("sum2.factors"+index_sstr.str());
        auto find_sumres1_it = solution.find("sum1.results"+index_sstr.str());
        auto find_sumres2_it = solution.find("sum2.results"+index_sstr.str());

        if(find_prod1_it != solution.end() && find_prod1_it->second != nullptr)
        {
            prod1_factors.push_back(find_prod1_it->second);
        }
        if(find_prod2_it != solution.end() && find_prod2_it->second != nullptr)
        {
            prod2_factors.push_back(find_prod2_it->second);
        }
        if(find_sum1_it != solution.end() && find_sum1_it->second != nullptr)
        {
            sum1_factors.push_back(find_sum1_it->second);
        }
        if(find_sum2_it != solution.end() && find_sum2_it->second != nullptr)
        {
            sum2_factors.push_back(find_sum2_it->second);
        }
        if(find_sumres1_it != solution.end() && find_sumres1_it->second != nullptr)
        {
            sum1_results.push_back(find_sumres1_it->second);
        }
        if(find_sumres2_it != solution.end() && find_sumres2_it->second != nullptr)
        {
            sum2_results.push_back(find_sumres2_it->second);
        }
    }

    std::reverse(prod1_factors.begin(), prod1_factors.end());
    std::reverse(prod2_factors.begin(), prod2_factors.end());
    std::reverse(sum1_factors.begin(), sum1_factors.end());
    std::reverse(sum2_factors.begin(), sum2_factors.end());
    std::reverse(sum1_results.begin(), sum1_results.end());
    std::reverse(sum2_results.begin(), sum2_results.end());

    if(prod1_factors.size() > 0 && prod2_factors.size() > 0 && sum1_factors.size() > 0 && sum2_factors.size() > 0 &&
       sum1_factors.size() == sum1_results.size() && sum2_factors.size() == sum2_results.size())
    {
        if(prod1_factors.size() == 1) first_prod = ConstantInt::get(prod1_factors.front()->getType(), 1);
        else                          first_prod = prod1_factors[1];

        if(prod2_factors.size() == 1) second_prod = ConstantInt::get(prod2_factors.front()->getType(), 1);
        else                          second_prod = prod2_factors[1];

        for(unsigned j = 2; j <= prod1_factors.size()-1; j++)
        {
            first_prod = BinaryOperator::Create(Instruction::Mul, prod1_factors[j], first_prod, "", sum);
        }

        for(unsigned j = 2; j <= prod2_factors.size()-1; j++)
        {
            second_prod = BinaryOperator::Create(Instruction::Mul, prod2_factors[j], second_prod, "", sum);
        }

        Value* new_sum  = BinaryOperator::Create(Instruction::Add, first_prod, second_prod,      "", sum);
        Value* new_prod = BinaryOperator::Create(Instruction::Mul, new_sum,    prod1_factors[0], "", sum);

        Value* final_value = new_prod;

        Value* first_sum = nullptr;
        if(sum1_factors.size() > 1)
        {
            first_sum = sum1_factors[1];

            for(unsigned j = 2; j <= sum1_factors.size()-1; j++)
            {
                if(auto binary_op = dyn_cast<BinaryOperator>(sum1_results[j]))
                {
                    first_sum = BinaryOperator::Create(binary_op->getOpcode(), sum1_factors[j], first_sum, "",  sum);
                }
            } 
        }

        Value* second_sum = nullptr;
        if(sum2_factors.size() > 1)
        {
            second_sum = sum2_factors[1];

            for(unsigned j = 2; j <= sum2_factors.size()-1; j++)
            {
                if(auto binary_op = dyn_cast<BinaryOperator>(sum2_results[j]))
                {
                    second_sum = BinaryOperator::Create(binary_op->getOpcode(), sum2_factors[j], second_sum, "",  sum);
                }
            } 
        }

        if(first_sum && second_sum)
        {
            llvm::Value* new_product = BinaryOperator::Create(Instruction::Add, first_sum, second_sum, "", sum);
            final_value = BinaryOperator::Create(Instruction::Add, new_product, final_value, "", sum);
        }
        else if(first_sum)
        {
            final_value = BinaryOperator::Create(Instruction::Add, first_sum, final_value, "", sum);
        }
        else if(second_sum)
        {
            final_value = BinaryOperator::Create(Instruction::Add, second_sum, final_value, "", sum);
        }

        for(BasicBlock& basic_block : function.getBasicBlockList())
        {
            for(Instruction& instruction : basic_block.getInstList())
            {
                for(unsigned i = 0; i < instruction.getNumOperands(); i++)
                    if(instruction.getOperand(i) == sum)
                        instruction.setOperand(i, final_value);
            }
        }

        sum->eraseFromParent();
    }
}

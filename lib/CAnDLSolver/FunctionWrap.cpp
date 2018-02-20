#include "llvm/CAnDLSolver/FunctionWrap.hpp"
#include "llvm/IR/SymbolTableListTraits.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Value.h"
#include <unordered_map>
#include <sstream>
#include <vector>

namespace {
std::vector<std::vector<unsigned>> reverse_graph(const std::vector<std::vector<unsigned>>& graph)
{
    std::vector<std::vector<unsigned>> result(graph.size());

    for(unsigned i = 0; i < graph.size(); i++)
        for(unsigned j = 0; j < graph[i].size(); j++)
            if(graph[i][j] < result.size())
                result[graph[i][j]].push_back(i);

    return result;
}

std::vector<std::vector<std::vector<unsigned>>> split_graph(const std::vector<std::vector<unsigned>>& graph)
{
    std::vector<std::vector<std::vector<unsigned>>> result;

    for(unsigned i = 0; i < graph.size(); i++)
    {
        while(result.size() < graph[i].size())
            result.emplace_back(graph.size());

        for(unsigned j = 0; j < graph[i].size(); j++)
            result[j][i].push_back(graph[i][j]);
    }

    return result;
}

std::vector<std::vector<std::vector<unsigned>>> split_reverse_graph(const std::vector<std::vector<unsigned>>& graph)
{
    std::vector<std::vector<std::vector<unsigned>>> result;

    for(unsigned i = 0; i < graph.size(); i++)
    {
        while(result.size() < graph[i].size())
            result.emplace_back(graph.size());

        for(unsigned j = 0; j < graph[i].size(); j++)
            result[j][graph[i][j]].push_back(i);
    }

    return result;
}

std::vector<std::vector<unsigned>> graph_union(const std::vector<std::vector<unsigned>>& graph1,
                                               const std::vector<std::vector<unsigned>>& graph2)
{
    std::vector<std::vector<unsigned>> result(graph1.size());

    for(unsigned i = 0; i < graph1.size() && i < graph2.size(); i++)
    {
        result[i].reserve(graph1[i].size() + graph2[i].size());
        result[i].insert(result[i].end(), graph1[i].begin(), graph1[i].end());
        result[i].insert(result[i].end(), graph2[i].begin(), graph2[i].end());
    }

    return result;
}

std::vector<std::vector<unsigned>> sort_graph(std::vector<std::vector<unsigned>> graph)
{
    for(unsigned i = 0; i < graph.size(); i++)
    {
        std::sort(graph[i].begin(), graph[i].end());
        graph[i].erase(std::unique(graph[i].begin(), graph[i].end()), graph[i].end());
    }

    return graph;
}

std::vector<std::vector<unsigned>> construct_cdg(std::unordered_map<llvm::Instruction*,unsigned>& instr_hash,
                                                 std::unordered_map<llvm::Value*,      unsigned>& value_hash)
{
    std::vector<std::vector<unsigned>> cdg(value_hash.size() + instr_hash.size());

    for(auto instruction : instr_hash)
    {
        if(llvm::TerminatorInst* terminator_instr = llvm::dyn_cast<llvm::TerminatorInst>(instruction.first))
        {
            for(unsigned i = 0; i < terminator_instr->getNumOperands(); i++)
            {
                if(llvm::BasicBlock* target_block = llvm::dyn_cast<llvm::BasicBlock>(terminator_instr->getOperand(i)))
                {
                    for(auto& target_instruction : *target_block)
                    {
                         cdg[value_hash.size() + instruction.second].push_back(
                             value_hash.size() + instr_hash[&target_instruction]);
                    }
                }
            }
        }
    }

    return cdg;
}

std::vector<std::vector<unsigned>> construct_cfg(std::unordered_map<llvm::Instruction*,unsigned>& instr_hash,
                                                 std::unordered_map<llvm::Value*,      unsigned>& value_hash)
{
    std::vector<std::vector<unsigned>> cfg(value_hash.size() + instr_hash.size());

    for(auto instruction : instr_hash)
    {
        if(llvm::TerminatorInst* term_inst = llvm::dyn_cast<llvm::TerminatorInst>(instruction.first))
        {
            for(auto& operand : term_inst->operands())
            {
                if(llvm::BasicBlock* block_cast = llvm::dyn_cast<llvm::BasicBlock>(&operand))
                {
                    cfg[value_hash.size() + instruction.second].push_back(
                        value_hash.size() + instr_hash[&block_cast->front()]);
                }
            }
        }
        else
        {
            cfg[value_hash.size() + instruction.second].push_back(
                value_hash.size() + instruction.second + 1);
        }
    }

    return cfg;
}

std::vector<std::vector<unsigned>> construct_rdfg(std::unordered_map<llvm::Instruction*,unsigned>& instr_hash,
                                                  std::unordered_map<llvm::Value*,      unsigned>& value_hash)
{
    std::vector<std::vector<unsigned>> rdfg(value_hash.size() + instr_hash.size());

    for(auto instruction : instr_hash)
    {
        if(auto call_inst = llvm::dyn_cast<llvm::CallInst>(instruction.first))
        {
            llvm::Instruction* instr = llvm::dyn_cast<llvm::Instruction>(call_inst->getCalledValue());
            auto               it1   = value_hash.find(call_inst->getCalledValue());
            auto               it2   = instr_hash.find(instr);

            if(it1 != value_hash.end())
            {
                rdfg[value_hash.size() + instruction.second].push_back(it1->second);
            }
            if(it2 != instr_hash.end())
            {
                rdfg[value_hash.size() + instruction.second].push_back(value_hash.size() + it2->second);
            }

            for(auto& operand : instruction.first->operands())
            {
                if(operand.get() != call_inst->getCalledValue())
                {
                    llvm::Instruction* instr = llvm::dyn_cast<llvm::Instruction>(&operand);
                    auto               it1   = value_hash.find(operand.get());
                    auto               it2   = instr_hash.find(instr);

                    if(it1 != value_hash.end())
                    {
                        rdfg[value_hash.size() + instruction.second].push_back(it1->second);
                    }
                    if(it2 != instr_hash.end())
                    {
                        rdfg[value_hash.size() + instruction.second].push_back(value_hash.size() + it2->second);
                    }
                }
            }
        }
        else if(instruction.first->getOpcode() != llvm::Instruction::BitCast &&
                instruction.first->getOpcode() != llvm::Instruction::ZExt &&
                instruction.first->getOpcode() != llvm::Instruction::SExt)
        {
            for(auto& operand : instruction.first->operands())
            {
                auto chase = operand.get();

                llvm::Instruction* chase_inst;
                while((chase_inst = llvm::dyn_cast<llvm::Instruction>(chase)) &&
                      (chase_inst->getOpcode() == llvm::Instruction::BitCast ||
                       chase_inst->getOpcode() == llvm::Instruction::ZExt ||
                       chase_inst->getOpcode() == llvm::Instruction::SExt))
                {
                    chase = chase_inst->getOperand(0);
                }

                llvm::Instruction* instr = llvm::dyn_cast<llvm::Instruction>(chase);
                auto               it1   = value_hash.find(chase);
                auto               it2   = instr_hash.find(instr);

                if(it1 != value_hash.end())
                {
                    rdfg[value_hash.size() + instruction.second].push_back(it1->second);
                }
                if(it2 != instr_hash.end())
                {
                    rdfg[value_hash.size() + instruction.second].push_back(value_hash.size() + it2->second);
                }
            }
        }
    }

    return rdfg;
}
}

double FunctionWrap::stopped_time = 0.0;

FunctionWrap::FunctionWrap(llvm::Function& llvm_function)
{
    std::unordered_map<llvm::Instruction*,unsigned> instr_hash;
    std::unordered_map<llvm::Value*,unsigned>       value_hash;

    std::vector<llvm::Value*> instructions;
    for(auto& block : llvm_function.getBasicBlockList())
    {
        for(auto& instruction : block)
        {
            instr_hash[&instruction] = instructions.size();
            instructions.push_back(&instruction);

            for(auto& operand : instruction.operands())
            {
                if(llvm::isa<llvm::Constant>(operand.get()) || llvm::isa<llvm::Argument>(operand.get()))
                {
                    if(value_hash.find(operand.get()) == value_hash.end())
                    {
                        value_hash[operand.get()] = size();
                        push_back(operand.get());
                    }
                }
            }
        }
    }

    insert(end(), instructions.begin(), instructions.end());

    cdg   = sort_graph(construct_cdg(instr_hash, value_hash));
    rcdg  = reverse_graph(cdg);
    cfg   = construct_cfg(instr_hash, value_hash);
    ocfg  = split_graph(cfg);
    rcfg  = reverse_graph(cfg);
    rocfg = split_reverse_graph(cfg);
    rdfg  = construct_rdfg(instr_hash, value_hash);
    rodfg = split_graph(rdfg);
    dfg   = reverse_graph(rdfg);
    odfg  = split_reverse_graph(rdfg);
    pdg   = graph_union(cdg, dfg);
    rpdg  = reverse_graph(pdg);
    cfg   = sort_graph(cfg);
    rdfg  = sort_graph(rdfg);
    pdg   = sort_graph(pdg);
}

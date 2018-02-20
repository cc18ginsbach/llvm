#include "llvm/CAnDLSolver/BackendClasses.hpp"
#include "llvm/IR/Instructions.h"

BackendIncomingValue::BackendIncomingValue(const FunctionWrap& w)
                    : wrap(w), values(nullptr,nullptr,nullptr) { }

template<unsigned idx>
SkipResult BackendIncomingValue::skip_invalid(unsigned& c) const
{
    if(auto value = c < wrap.size() ? wrap[c] : nullptr)
    {
        auto in_value  = std::get<0>(values);
        auto term_inst = std::get<1>(values);
        auto phi_node  = std::get<2>(values);

        if((idx == 0 && (in_value  = llvm::dyn_cast<llvm::Value>         (value))) ||
           (idx == 1 && (term_inst = llvm::dyn_cast<llvm::TerminatorInst>(value))) ||
           (idx == 2 && (phi_node  = llvm::dyn_cast<llvm::PHINode>       (value))))
        {
            if(!phi_node || (!term_inst && !in_value))
                return SkipResult::PASS; 

            for(auto in : llvm::zip(phi_node->blocks(), phi_node->incoming_values()))
            {
                if((!in_value  || in_value == std::get<1>(in)) &&
                   (!term_inst || term_inst->getParent() == std::get<0>(in)))
                {
                    return SkipResult::PASS;
                }
            }
        }
    }

    if(c + 1 >= wrap.size())
        return SkipResult::FAIL;

    c = c + 1;
    return SkipResult::CHANGE;
}


template<>
void BackendIncomingValue::fixate<0>(unsigned c)
{
    std::get<0>(values) = llvm::dyn_cast<llvm::Value>(wrap[c]);
}

template<>
void BackendIncomingValue::fixate<1>(unsigned c)
{
    std::get<1>(values) = llvm::dyn_cast<llvm::TerminatorInst>(wrap[c]);
}

template<>
void BackendIncomingValue::fixate<2>(unsigned c)
{
    std::get<2>(values) = llvm::dyn_cast<llvm::PHINode>(wrap[c]);
}

template SkipResult BackendIncomingValue::skip_invalid<0>(unsigned&) const;
template SkipResult BackendIncomingValue::skip_invalid<1>(unsigned&) const;
template SkipResult BackendIncomingValue::skip_invalid<2>(unsigned&) const;

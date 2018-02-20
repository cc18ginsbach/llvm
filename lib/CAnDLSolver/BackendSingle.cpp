#include "llvm/CAnDLSolver/BackendClasses.hpp"

BackendSingle::BackendSingle(std::vector<SolverAtom::Value> h) : hits(std::move(h)), hit_start(hits.begin()) { }

SkipResult BackendSingle::skip_invalid(SolverAtom::Value& c) const
{
    for(auto ptr = hit_start; ptr != hits.end(); ptr++)
    {
        if(*ptr > c)
        {
            c = *ptr;
            return SkipResult::CHANGEPASS;
        }

        if(*ptr == c)
            return SkipResult::PASS;
    }

    return SkipResult::FAIL;
}

void BackendSingle::begin() { hit_start = hits.begin(); }

void BackendSingle::fixate(SolverAtom::Value c)
{
    for(auto ptr = hit_start; ptr != hits.end(); ptr++)
    {
        if(*ptr >= c)
        {
            hit_start = ptr;
            return;
        }
    }
}

void BackendSingle::resume() { }

#include "llvm/CAnDLSolver/BackendClasses.hpp"

BackendEdge::BackendEdge(const Graph& gf, const Graph& gb)
           : graphs{{gf,gb}}, amount_completed(0), src_ptr(nullptr), dst_ptr(nullptr) { }

template<unsigned idx>
SkipResult BackendEdge::skip_invalid(unsigned& c) const
{
    if(amount_completed == 0)
    {
        if(c < std::get<idx>(graphs).get().size())
            return SkipResult::PASS;

        return SkipResult::FAIL;
    }

    for(auto ptr = dst_ptr; ptr != src_ptr->end(); ptr++)
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

template<unsigned idx>
void BackendEdge::begin() { if(amount_completed == 1) dst_ptr = src_ptr->begin(); }

template<unsigned idx>
void BackendEdge::fixate(unsigned c)
{
    if(++amount_completed == 1) src_ptr = std::get<idx>(graphs).get().begin() + c;
    else
    {
        for(auto ptr = dst_ptr; ptr != src_ptr->end(); ptr++)
        {
            if(*ptr >= c)
            {
                dst_ptr = ptr;
                return;
            }
        }
    }
}

template<unsigned idx>
void BackendEdge::resume() { amount_completed--; }

template SkipResult BackendEdge::skip_invalid<0>(unsigned&) const;
template SkipResult BackendEdge::skip_invalid<1>(unsigned&) const;
template void BackendEdge::begin<0>();
template void BackendEdge::begin<1>();
template void BackendEdge::fixate<0>(unsigned);
template void BackendEdge::fixate<1>(unsigned);
template void BackendEdge::resume<0>();
template void BackendEdge::resume<1>();

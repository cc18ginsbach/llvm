#include "llvm/CAnDLSolver/BackendClasses.hpp"

BackendSameSets::BackendSameSets(unsigned N) : total_values(N), number_filled{{0, 0}},
                                  filled_values{{std::vector<unsigned>(N),
                                                 std::vector<unsigned>(N)}} { }

template<unsigned idx1>
SkipResult BackendSameSets::skip_invalid(unsigned idx2, unsigned& c) const
{
    if(std::get<1-idx1>(number_filled) == total_values)
    {
        auto find_it = std::get<1-idx1>(difference).lower_bound(c);

        if(find_it == std::get<1-idx1>(difference).end())
            return SkipResult::FAIL;

        if(find_it->first == c)
            return SkipResult::PASS;

        c = find_it->first;
        return SkipResult::CHANGEPASS;
    }
    else return SkipResult::PASS;
}

template<unsigned idx1>
void BackendSameSets::begin(unsigned idx2) { }

template<unsigned idx1>
void BackendSameSets::fixate(unsigned idx2, unsigned c)
{
    ++std::get<idx1>(number_filled);
    std::get<idx1>(filled_values)[idx2] = c;

    auto find_it1 = std::get<1-idx1>(difference).find(c);

    if(find_it1 != std::get<1-idx1>(difference).end())
    {
        if(--find_it1->second == 0)
        {
            std::get<1-idx1>(difference).erase(find_it1);
        }
        return;
    }

    auto find_it2 = std::get<idx1>(difference).find(c);

    if(find_it2 != std::get<idx1>(difference).end())
    {
        ++find_it2->second;
        return;
    }

    std::get<idx1>(difference).insert({c, 1});
}
template<unsigned idx1>
void BackendSameSets::resume(unsigned idx2)
{
    --std::get<idx1>(number_filled);
    unsigned c = std::get<idx1>(filled_values)[idx2];

    auto find_it1 = std::get<idx1>(difference).find(c);

    if(find_it1 != std::get<idx1>(difference).end())
    {
        if(--find_it1->second == 0)
        {
            std::get<idx1>(difference).erase(find_it1);
        }
        return;
    }

    auto find_it2 = std::get<1-idx1>(difference).find(c);

    if(find_it2 != std::get<1-idx1>(difference).end())
    {
        ++find_it2->second;
        return;
    }

    std::get<1-idx1>(difference).insert({c, 1});
}

template SkipResult BackendSameSets::skip_invalid<0>(unsigned,unsigned&) const;
template SkipResult BackendSameSets::skip_invalid<1>(unsigned,unsigned&) const;
template void BackendSameSets::begin<0>(unsigned);
template void BackendSameSets::begin<1>(unsigned);
template void BackendSameSets::fixate<0>(unsigned,unsigned);
template void BackendSameSets::fixate<1>(unsigned,unsigned);
template void BackendSameSets::resume<0>(unsigned);
template void BackendSameSets::resume<1>(unsigned);

#ifndef _BACKEND_DIRECT_CLASSES_HPP_
#define _BACKEND_DIRECT_CLASSES_HPP_
#include "BackendSelectors.hpp"

template<typename ... SolverAtomN>
class BackendAnd;

template<typename SolverAtom1, typename SolverAtom2>
class BackendAnd<SolverAtom1,SolverAtom2> : public SolverAtom
{
public:
    BackendAnd() = default;
    BackendAnd(SolverAtom1 s1, SolverAtom2 s2);

    SkipResult skip_invalid(SolverAtom::Value& c) const final;

    void begin () final;
    void fixate(SolverAtom::Value c) final;
    void resume() final;

private:
    SolverAtom1 constraint_head;
    SolverAtom2 constraint_tail;
};

template<typename SolverAtom1, typename SolverAtom2>
BackendAnd<SolverAtom1,SolverAtom2>::BackendAnd(SolverAtom1 s1, SolverAtom2 s2)
                                   : constraint_head(std::move(s1)), constraint_tail(std::move(s2)) { }

template<typename SolverAtom1, typename SolverAtom2>
SkipResult BackendAnd<SolverAtom1,SolverAtom2>::skip_invalid(SolverAtom::Value& c) const
{
    SkipResult first_result = constraint_head.skip_invalid(c);

    if(first_result == SkipResult::FAIL)
        return SkipResult::FAIL;

    if(first_result == SkipResult::CHANGE)
        return SkipResult::CHANGE;

    SkipResult second_result = constraint_tail.skip_invalid(c);

    if(second_result == SkipResult::FAIL)
        return SkipResult::FAIL;

    if(second_result != SkipResult::PASS)
        return SkipResult::CHANGE;

    return first_result;
}

template<typename SolverAtom1, typename SolverAtom2>
void BackendAnd<SolverAtom1,SolverAtom2>::begin()
{
    constraint_head.begin();
    constraint_tail.begin();
}

template<typename SolverAtom1, typename SolverAtom2>
void BackendAnd<SolverAtom1,SolverAtom2>::fixate(SolverAtom::Value c)
{
    constraint_head.fixate(c);
    constraint_tail.fixate(c);
}

template<typename SolverAtom1, typename SolverAtom2>
void BackendAnd<SolverAtom1,SolverAtom2>::resume()
{
    constraint_head.resume();
    constraint_tail.resume();
}

template<typename SolverAtom1, typename SolverAtom2>
class BackendOr : public SolverAtom
{
public:
    BackendOr() = default;
    BackendOr(SolverAtom1 s1, SolverAtom2 s2, std::shared_ptr<std::pair<unsigned,unsigned>> shared);

    SkipResult skip_invalid(SolverAtom::Value& c) const final;

    void begin () final;
    void fixate(SolverAtom::Value c) final;
    void resume() final;

private:
    SolverAtom1                                   constraint_head;
    SolverAtom2                                   constraint_tail;
    std::shared_ptr<std::pair<unsigned,unsigned>> disabled_since_member;
};


template<typename SolverAtom1, typename SolverAtom2>
BackendOr<SolverAtom1,SolverAtom2>::BackendOr(SolverAtom1 s1, SolverAtom2 s2, std::shared_ptr<std::pair<unsigned,unsigned>> shared)
                                   : constraint_head(std::move(s1)), constraint_tail(std::move(s2)),
                                     disabled_since_member(std::move(shared)) { }

template<typename SolverAtom1, typename SolverAtom2>
SkipResult BackendOr<SolverAtom1,SolverAtom2>::skip_invalid(SolverAtom::Value& c) const
{
    SkipResult   result_so_far = SkipResult::FAIL;
    SolverAtom::Value c_so_far = c;

    if(disabled_since_member->first == 0)
    {
        SolverAtom::Value c_copy = c;
        SkipResult new_result = constraint_head.skip_invalid(c_copy);

        if(new_result == SkipResult::PASS)
        {
            return SkipResult::PASS;
        }

        if(new_result != SkipResult::FAIL && (result_so_far == SkipResult::FAIL || c_copy < c_so_far))
        {
            c_so_far = c_copy;
            result_so_far = new_result;
        }
    }

    if(disabled_since_member->second == 0)
    {
        SolverAtom::Value c_copy = c;
        SkipResult new_result = constraint_tail.skip_invalid(c_copy);

        if(new_result == SkipResult::PASS)
        {
            return SkipResult::PASS;
        }

        if(new_result != SkipResult::FAIL && (result_so_far == SkipResult::FAIL || c_copy < c_so_far))
        {
            c_so_far = c_copy;
            result_so_far = new_result;
        }
    }

    c = c_so_far;
    return result_so_far;
}

template<typename SolverAtom1, typename SolverAtom2>
void BackendOr<SolverAtom1,SolverAtom2>::begin()
{
    if(disabled_since_member->first  == 0) constraint_head.begin();
    if(disabled_since_member->second == 0) constraint_tail.begin();
}

template<typename SolverAtom1, typename SolverAtom2>
void BackendOr<SolverAtom1,SolverAtom2>::fixate(SolverAtom::Value c)
{
    if(disabled_since_member->first > 0) disabled_since_member->first++;
    else
    {
        SolverAtom::Value temp = c;

        if(constraint_head.skip_invalid(temp) != SkipResult::PASS)
        {
            disabled_since_member->first = 1;
        }
        else
        {
            constraint_head.fixate(c);
        }
    }

    if(disabled_since_member->second > 0) disabled_since_member->second ++;
    else
    {
        SolverAtom::Value temp = c;

        if(constraint_tail.skip_invalid(temp) != SkipResult::PASS)
        {
            disabled_since_member->second = 1;
        }
        else
        {
            constraint_tail.fixate(c);
        }
    }
}

template<typename SolverAtom1, typename SolverAtom2>
void BackendOr<SolverAtom1,SolverAtom2>::resume()
{
    if(disabled_since_member->first > 0) disabled_since_member->first--;
    else                                 constraint_head.resume();

    if(disabled_since_member->second > 0) disabled_since_member->second--;
    else                                 constraint_tail.resume();
}


#endif

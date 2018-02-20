#include "llvm/CAnDLSolver/BackendClasses.hpp"

BackendCollect::BackendCollect(std::array<unsigned,2>, std::vector<std::unique_ptr<SolverAtom>> nloc,
                                                       std::vector<std::unique_ptr<SolverAtom>> loc)
               : nonlocals(std::move(nloc)), locals(std::move(loc)),
                 filled_nonlocals(0), filled_locals(locals.size(), 0)
{
    if(nonlocals.empty())
    {
        Solver solver(std::move(locals));

        while(true)
        {
            auto solution = solver.next_solution();

            if(solution.empty()) break;

            solutions.insert(solutions.end(), solution.begin(), solution.end());
        }

        locals = solver.swap_specials();
    }
}

template<unsigned idx1>
SkipResult BackendCollect::skip_invalid(unsigned idx2, SolverAtom::Value &c) const
{
    if(idx1 == 0)
    {
        return nonlocals[idx2]->skip_invalid(c);
    }
    else if(idx1 == 1)
    {
        unsigned flat_solution_idx = filled_locals[idx2%locals.size()] * locals.size() + (idx2%locals.size());

        if(flat_solution_idx < solutions.size())
        {
            if(c == solutions[flat_solution_idx])
            {
                return SkipResult::PASS;
            }
            else if(c < solutions[flat_solution_idx])
            {
                c = solutions[flat_solution_idx];
                return SkipResult::CHANGEPASS;
            }
            else return SkipResult::FAIL;
        }
        else if(c == UINT_MAX - 1)
        {
            return SkipResult::PASS;
        }
        else if(c < UINT_MAX - 1)
        {
            c = UINT_MAX - 1;
            return SkipResult::CHANGEPASS;
        }
        else return SkipResult::FAIL;
    }
}

template<unsigned idx1>
void BackendCollect::begin(unsigned idx2)
{
    if(idx1 == 0)
    {
        nonlocals[idx2]->begin();
    }
}

template<unsigned idx1>
void BackendCollect::fixate(unsigned idx2, SolverAtom::Value c)
{
    if(idx1 == 0)
    {
        nonlocals[idx2]->fixate(c);

        if(++filled_nonlocals == nonlocals.size())
        {
            Solver solver(std::move(locals));

            while(true)
            {
                auto solution = solver.next_solution();

                if(solution.empty()) break;

                solutions.insert(solutions.end(), solution.begin(), solution.end());
            }

            locals = solver.swap_specials();
        }
    }
    else if(idx1 == 1)
    {
        filled_locals[idx2%locals.size()]++;
    }
}

template<unsigned idx1>
void BackendCollect::resume(unsigned idx2)
{
    if(idx1 == 0)
    {
        if(filled_nonlocals-- == nonlocals.size())
        {
            solutions.clear();
        }

        nonlocals[idx2]->resume();
    }
    else if(idx1 == 1)
    {
        filled_locals[idx2%locals.size()]--;
    }
}

template SkipResult BackendCollect::skip_invalid<0>(unsigned,SolverAtom::Value&) const;
template SkipResult BackendCollect::skip_invalid<1>(unsigned,SolverAtom::Value&) const;
template       void BackendCollect::begin<0>(unsigned);
template       void BackendCollect::begin<1>(unsigned);
template       void BackendCollect::fixate<0>(unsigned,SolverAtom::Value);
template       void BackendCollect::fixate<1>(unsigned,SolverAtom::Value);
template       void BackendCollect::resume<0>(unsigned);
template       void BackendCollect::resume<1>(unsigned);

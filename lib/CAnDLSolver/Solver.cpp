#include "llvm/CAnDLSolver/Solver.hpp"
#include <vector>
#include <cstdio>
#include <iostream>

unsigned Solver::max_iterations = 0;

Solver::Solver(std::vector<std::unique_ptr<SolverAtom>> s) : iterator(UINT_MAX)
{
    swap_specials(std::move(s));
}

Solver::~Solver()
{
    swap_specials();
}

std::vector<std::unique_ptr<SolverAtom>> Solver::swap_specials(std::vector<std::unique_ptr<SolverAtom>> specials)
{
    if(iterator != UINT_MAX)
    {
        while(--iterator != UINT_MAX)
        {
            specializations[iterator]->resume();
        }
    }

    if(specials.empty())
    {
        iterator = UINT_MAX;
    }
    else
    {
        iterator = 0;
        specials[iterator]->begin();
    }

    solution.assign(specials.size(), SolverAtom::Value());

    std::swap(specializations, specials);
    return specials;
}

std::vector<SolverAtom::Value> Solver::next_solution(unsigned max_steps)
{
    max_iterations = UINT_MAX;
    if(iterator == UINT_MAX)
        return {};

    while(max_iterations > 0)
    {
        max_iterations --;

        SkipResult result = specializations[iterator]->skip_invalid(solution[iterator]);

        if(result == SkipResult::CHANGE)
            continue;

        if(result == SkipResult::FAIL)
        {
            if(--iterator == UINT_MAX)
                return {};

            specializations[iterator]->resume();
            solution[iterator]++;
            continue;
        }

        specializations[iterator]->fixate(solution[iterator]);

        if(iterator + 1 == specializations.size())
        {
            auto solution_copy = solution;
            specializations[iterator]->resume();
            solution[iterator]++;
            return solution_copy;
        }

        specializations[++iterator]->begin();
        solution[iterator] = SolverAtom::Value();
    }

    return {};
}

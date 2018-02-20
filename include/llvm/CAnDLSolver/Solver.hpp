#ifndef _CONSTRAINT_INTERFACE_HPP_
#define _CONSTRAINT_INTERFACE_HPP_
#include <climits>
#include <memory>
#include <vector>

enum class SkipResult
{
    FAIL       = 0,
    PASS       = 1,
    CHANGE     = 2,
    CHANGEPASS = 3
};

/* This class implements the representation of constraints that the solver expects to perform its backtracking
   algorithm.
   Essentially the solver guarantees to call the begin, resume, fixate, cancel members whenever it moves between
   variables and they allow the instances of this class to "prepare" for being needed.
   The "skip_invalid" member function is the crucial one and increments the argument that is passed to it until
   it does not contradict any constraint anymore.
   the return value depends on whether this was successfull. */
class SolverAtom
{
public:
    using Value = unsigned;

    virtual ~SolverAtom() { }

    virtual SkipResult skip_invalid(Value& c) const = 0;

    virtual void begin() = 0;
    virtual void fixate(Value c) = 0;
    virtual void resume() = 0;
};

/* The class performs the backtracking algorithm. */
class Solver
{
friend class Solution;
public:
    Solver(std::vector<std::unique_ptr<SolverAtom>> specializations);
    ~Solver();

    std::vector<SolverAtom::Value> next_solution(unsigned max_steps = UINT_MAX);

    std::vector<std::unique_ptr<SolverAtom>> swap_specials(std::vector<std::unique_ptr<SolverAtom>> specials = {});

private:
    unsigned                                 iterator;
    std::vector<SolverAtom::Value>           solution;
    std::vector<std::unique_ptr<SolverAtom>> specializations;

    static unsigned max_iterations;
};

#endif

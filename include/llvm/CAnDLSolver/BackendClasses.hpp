#ifndef _BACKEND_CLASSES_HPP_
#define _BACKEND_CLASSES_HPP_
#include "llvm/IR/Instructions.h"
#include "Solver.hpp"
#include "FunctionWrap.hpp"
#include "GraphEngine.hpp"
#include <vector>
#include <memory>
#include <map>

/* This class implements he logical dicjunction in the constraint description system.
   The constructor takes an arbitrary amount of constraints and the resulting constraints enforces that all of them are
   satisfied at once. */
class BackendCollect
{
public:
    BackendCollect(std::array<unsigned,2> size, std::vector<std::unique_ptr<SolverAtom>> nloc,
                                                std::vector<std::unique_ptr<SolverAtom>> loc);

    template<unsigned idx1> SkipResult skip_invalid(unsigned idx2, SolverAtom::Value &c) const;

    template<unsigned idx1> void begin (unsigned idx2);
    template<unsigned idx1> void fixate(unsigned idx2, SolverAtom::Value c);
    template<unsigned idx1> void resume(unsigned idx2);

private:
    std::vector<std::unique_ptr<SolverAtom>> nonlocals;
    std::vector<std::unique_ptr<SolverAtom>> locals;
    unsigned                                 filled_nonlocals;
    std::vector<unsigned>                    filled_locals;
    std::vector<SolverAtom::Value>           solutions;
};

class BackendSingle : public SolverAtom
{
public:
    BackendSingle() = default;
    BackendSingle(std::vector<SolverAtom::Value> hits);

    SkipResult skip_invalid(SolverAtom::Value& c) const final;

    void begin () final;
    void fixate(SolverAtom::Value c) final;
    void resume() final;

private:
    std::vector<SolverAtom::Value>                 hits;
    std::vector<SolverAtom::Value>::const_iterator hit_start;
};

class BackendEdge
{
public:
    using Graph = std::vector<std::vector<unsigned>>;
    BackendEdge(const Graph& gf, const Graph& gb);

    template<unsigned idx> SkipResult skip_invalid(unsigned& c) const;

    template<unsigned idx> void begin();
    template<unsigned idx> void fixate(unsigned c);
    template<unsigned idx> void resume();

private:
    std::array<std::reference_wrapper<const Graph>,2> graphs;
    unsigned                                          amount_completed;
    Graph::const_iterator                             src_ptr;
    std::vector<unsigned>::const_iterator             dst_ptr;
};

template<bool lt, bool eq, bool gt>
class BackendOrdering
{
public:
    BackendOrdering();

    template<unsigned idx> SkipResult skip_invalid(SolverAtom::Value& c) const;

    template<unsigned idx> void begin ()                    { }
    template<unsigned idx> void fixate(SolverAtom::Value c) { if(amount_completed++ == 0) other_value = c; }
    template<unsigned idx> void resume()                    { amount_completed--; }

private:
    unsigned amount_completed;
    unsigned other_value;
};

class BackendIncomingValue
{
public:
    BackendIncomingValue(const FunctionWrap& w);

    template<unsigned idx> SkipResult skip_invalid(unsigned& c) const;

    template<unsigned idx> void begin () { }
    template<unsigned idx> void fixate(unsigned c);
    template<unsigned idx> void resume() { std::get<idx>(values) = nullptr; }

private:
    const FunctionWrap&                                           wrap;
    std::tuple<llvm::Value*,llvm::TerminatorInst*,llvm::PHINode*> values;
};

template<bool reverse,bool allow_unstrict>
class BackendDominate
{
public:
    BackendDominate(std::array<unsigned,3> size, const std::vector<std::vector<unsigned>>& graph_forw);

    template<unsigned idx1> SkipResult skip_invalid(unsigned idx2, unsigned& c) const;

    template<unsigned idx1> void begin (unsigned) { }

    template<unsigned idx1> void fixate(unsigned idx2, unsigned c)
    {
        if(c != UINT_MAX-1)
        {
            std::get<idx1>(value_masks)[idx2] = true;
            filled_values[idx1][used_values[idx1]++] = c;
        }

        std::get<idx1>(remaining_values)--;
    }
    template<unsigned idx1> void resume(unsigned idx2)
    {
        if(std::get<idx1>(value_masks)[idx2])
        {
            std::get<idx1>(value_masks)[idx2] = false;
            used_values[idx1]--;
        }

        std::get<idx1>(remaining_values)++;
    }
    
private:
    mutable GraphEngine                 graph_engine;
    std::vector<std::vector<unsigned>>  graph_forw;
    std::array<unsigned,3>              used_values;
    std::array<unsigned,3>              remaining_values;
    std::array<std::vector<unsigned>,3> filled_values;
    std::array<std::vector<bool>,3>     value_masks;
};

class BackendSameSets
{
public:
    BackendSameSets(unsigned N);

    template<unsigned idx1> SkipResult skip_invalid(unsigned idx2, unsigned& c) const;

    template<unsigned idx1> void begin (unsigned idx2);
    template<unsigned idx1> void fixate(unsigned idx2, unsigned c);
    template<unsigned idx1> void resume(unsigned idx2);

private:
    unsigned                                  total_values;
    std::array<unsigned,2>                    number_filled;
    std::array<std::vector<unsigned>,2>       filled_values;
    std::array<std::map<unsigned,unsigned>,2> difference;
};

#endif

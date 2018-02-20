#include "llvm/CAnDLSolver/BackendClasses.hpp"

template<bool reverse,bool allow_unstrict>
BackendDominate<reverse,allow_unstrict>::BackendDominate(std::array<unsigned,3> size,
                                                         const std::vector<std::vector<unsigned>>& gf)
               : graph_engine(gf.size()), graph_forw(gf), used_values{{0,0,0}}, remaining_values(size),
                 filled_values{{std::vector<unsigned>(size[0]),
                                std::vector<unsigned>(size[1]),
                                std::vector<unsigned>(size[2])}},
                 value_masks{{std::vector<bool>(size[0],false),
                              std::vector<bool>(size[1],false),
                              std::vector<bool>(size[2],false)}} { }

template<bool reverse,bool allow_unstrict>
template<unsigned idx1>
SkipResult BackendDominate<reverse,allow_unstrict>::skip_invalid(unsigned idx2, SolverAtom::Value& c) const
{
    if(c >= graph_forw.size() && c != UINT_MAX-1)
    {
        if(c < UINT_MAX-1)
        {
            c = UINT_MAX-1;
            return SkipResult::CHANGE;
        }
        else return SkipResult::FAIL;
    }
    else 
    {
        if(remaining_values[(idx1+1)%3] + remaining_values[(idx1+2)%3] > 0 || (idx1 != 1 && remaining_values[idx1] > 1))
        {
            return SkipResult::PASS;
        }
        else
        {
            bool domination_holds = false;

            graph_engine.initialize();

            if(allow_unstrict)
            {
                graph_engine.set_destinations(filled_values[2].begin(), filled_values[2].begin() + used_values[2]);

                if(c != UINT_MAX-1 && idx1 == 2) graph_engine.set_destinations(&c, &c+1);

                graph_engine.set_dominators(filled_values[1].begin(), filled_values[1].begin() + used_values[1]);

                if(c != UINT_MAX-1 && idx1 == 1)  graph_engine.set_dominators(&c, &c+1);
            }
            else
            {
                graph_engine.set_dominators(filled_values[1].begin(), filled_values[1].begin() + used_values[1]);

                if(c != UINT_MAX-1 && idx1 == 1) graph_engine.set_dominators(&c, &c+1);

                graph_engine.set_destinations(filled_values[2].begin(), filled_values[2].begin() + used_values[2]);

                if(c != UINT_MAX-1 && idx1 == 2) graph_engine.set_destinations(&c, &c+1);
            }


            if(graph_engine.set_origins(filled_values[0].begin(), filled_values[0].begin() + used_values[0]) &&
               (c == UINT_MAX-1 || idx1 != 0 || graph_engine.set_origins(&c, &c+1)))
            {
                domination_holds = graph_engine.fill(graph_forw);
            }

            if(remaining_values[1] == (idx1==1) && ((domination_holds && reverse) || (!domination_holds && !reverse)))
            {
                if(c >= UINT_MAX-1)
                {
                    return SkipResult::FAIL;
                }
                else if(c + 1 < graph_forw.size())
                {
                    c = c + 1;
                    return SkipResult::CHANGE;
                }
                else
                {
                   c = UINT_MAX-1;
                   return SkipResult::CHANGE;
                }
            }

            return SkipResult::PASS;
        }
    }
}

template      class BackendDominate<false,false>;
template SkipResult BackendDominate<false,false>::skip_invalid<0>(unsigned,SolverAtom::Value&) const;
template SkipResult BackendDominate<false,false>::skip_invalid<1>(unsigned,SolverAtom::Value&) const;
template SkipResult BackendDominate<false,false>::skip_invalid<2>(unsigned,SolverAtom::Value&) const;
template      class BackendDominate<false,true>;
template SkipResult BackendDominate<false,true>::skip_invalid<0>(unsigned,SolverAtom::Value&) const;
template SkipResult BackendDominate<false,true>::skip_invalid<1>(unsigned,SolverAtom::Value&) const;
template SkipResult BackendDominate<false,true>::skip_invalid<2>(unsigned,SolverAtom::Value&) const;
template      class BackendDominate<true,false>;
template SkipResult BackendDominate<true,false>::skip_invalid<0>(unsigned,SolverAtom::Value&) const;
template SkipResult BackendDominate<true,false>::skip_invalid<1>(unsigned,SolverAtom::Value&) const;
template SkipResult BackendDominate<true,false>::skip_invalid<2>(unsigned,SolverAtom::Value&) const;
template      class BackendDominate<true,true>;
template SkipResult BackendDominate<true,true>::skip_invalid<0>(unsigned,SolverAtom::Value&) const;
template SkipResult BackendDominate<true,true>::skip_invalid<1>(unsigned,SolverAtom::Value&) const;
template SkipResult BackendDominate<true,true>::skip_invalid<2>(unsigned,SolverAtom::Value&) const;

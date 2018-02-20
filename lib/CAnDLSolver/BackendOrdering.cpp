#include "llvm/CAnDLSolver/BackendClasses.hpp"

template<bool lt, bool eq, bool gt>
BackendOrdering<lt,eq,gt>::BackendOrdering() : amount_completed(0) { }

template<bool lt, bool eq, bool gt>
template<unsigned idx>
SkipResult BackendOrdering<lt,eq,gt>::skip_invalid(SolverAtom::Value& c) const
{
    if(!amount_completed)
        return SkipResult::PASS;

    if(c <= other_value)
    {
        if(c < other_value)
        {
            if((lt && idx == 0) || (gt && idx == 1))
                return SkipResult::PASS;

            if(eq)
            {
                c = other_value;
                return SkipResult::CHANGEPASS;
            }
        }

        if(((gt && idx == 0) || (lt && idx == 1)) && other_value < UINT_MAX)
        {
            c = other_value + 1;
            return SkipResult::CHANGEPASS;
        }

        if(c == other_value && eq)
            return SkipResult::PASS;
    }

    else if((gt && idx == 0) || (lt && idx == 1))
        return SkipResult::PASS;

    return SkipResult::FAIL;
}

template      class BackendOrdering<false,false,false>;
template SkipResult BackendOrdering<false,false,false>::skip_invalid<0>(SolverAtom::Value&) const;
template SkipResult BackendOrdering<false,false,false>::skip_invalid<1>(SolverAtom::Value&) const;
template      class BackendOrdering<false,false,true>;
template SkipResult BackendOrdering<false,false,true>::skip_invalid<0>(SolverAtom::Value&) const;
template SkipResult BackendOrdering<false,false,true>::skip_invalid<1>(SolverAtom::Value&) const;
template      class BackendOrdering<false,true,false>;
template SkipResult BackendOrdering<false,true,false>::skip_invalid<0>(SolverAtom::Value&) const;
template SkipResult BackendOrdering<false,true,false>::skip_invalid<1>(SolverAtom::Value&) const;
template      class BackendOrdering<false,true,true>;
template SkipResult BackendOrdering<false,true,true>::skip_invalid<0>(SolverAtom::Value&) const;
template SkipResult BackendOrdering<false,true,true>::skip_invalid<1>(SolverAtom::Value&) const;
template      class BackendOrdering<true,false,false>;
template SkipResult BackendOrdering<true,false,false>::skip_invalid<0>(SolverAtom::Value&) const;
template SkipResult BackendOrdering<true,false,false>::skip_invalid<1>(SolverAtom::Value&) const;
template      class BackendOrdering<true,false,true>;
template SkipResult BackendOrdering<true,false,true>::skip_invalid<0>(SolverAtom::Value&) const;
template SkipResult BackendOrdering<true,false,true>::skip_invalid<1>(SolverAtom::Value&) const;
template      class BackendOrdering<true,true,false>;
template SkipResult BackendOrdering<true,true,false>::skip_invalid<0>(SolverAtom::Value&) const;
template SkipResult BackendOrdering<true,true,false>::skip_invalid<1>(SolverAtom::Value&) const;
template      class BackendOrdering<true,true,true>;
template SkipResult BackendOrdering<true,true,true>::skip_invalid<0>(SolverAtom::Value&) const;
template SkipResult BackendOrdering<true,true,true>::skip_invalid<1>(SolverAtom::Value&) const;

#ifndef _FUNCTION_WRAP_HH_
#define _FUNCTION_WRAP_HH_
#include <vector>

namespace llvm {
class Function;
class Value;
}

/* This class is used as a wrapper around llvm::Function. */
class FunctionWrap : public std::vector<llvm::Value*>
{
public:
    FunctionWrap(llvm::Function& llvm_function);

    std::vector<std::vector<unsigned>>              cfg;
    std::vector<std::vector<unsigned>>              rcfg;
    std::vector<std::vector<std::vector<unsigned>>> ocfg;
    std::vector<std::vector<std::vector<unsigned>>> rocfg;
    std::vector<std::vector<unsigned>>              dfg;
    std::vector<std::vector<unsigned>>              rdfg;
    std::vector<std::vector<std::vector<unsigned>>> odfg;
    std::vector<std::vector<std::vector<unsigned>>> rodfg;
    std::vector<std::vector<unsigned>>              cdg;
    std::vector<std::vector<unsigned>>              rcdg;
    std::vector<std::vector<unsigned>>              pdg;
    std::vector<std::vector<unsigned>>              rpdg;

    static double stopped_time;
};

#endif

#ifndef _TRANSFORMS_HPP_
#define _TRANSFORMS_HPP_
#include <unordered_map>
#include <string>
#include <vector>
#include <map>

namespace llvm {
class Instruction;
class BasicBlock;
class Function;
class Value;
}

void transform_distributive(llvm::Function& function, std::map<std::string,llvm::Value*> solution);

void transform_hoistselect_pattern(llvm::Function& wrap, std::map<std::string,llvm::Value*> solution);



#endif

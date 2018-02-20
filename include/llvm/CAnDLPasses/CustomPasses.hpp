#ifndef _CUSTOM_PASSES_HPP_
#define _CUSTOM_PASSES_HPP_
#include "llvm/IR/PassManager.h"

namespace llvm {

class FunctionPass;
class ModulePass;

/* FlattenPass transforms all array accesses to be flat accesses to bare pointers.
   PreprocessorPass implements some additional peephole optimisations.
   ReplacerPass is the central part of this project, it uses the SMT based constraint solver to detect computational
   idioms in LLVM IR code. */
FunctionPass*   createRemovePHIPass();
FunctionPass*   createResearchMergePointercalcsPass();
FunctionPass*   createResearchFlattenPointercalcsPass();
FunctionPass*   createResearchSplitPointercalcsPass();
ModulePass*     createResearchPreprocessorPass();
ModulePass*     createResearchReplacerPass();
BasicBlockPass* createResearchFixOrToAddPass();
BasicBlockPass* createResearchFixShlToMulPass();
BasicBlockPass* createResearchEliminatePointerCasts1Pass();
BasicBlockPass* createResearchEliminatePointerCasts2Pass();
BasicBlockPass* createResearchEliminatePointerCasts3Pass();
BasicBlockPass* createResearchEliminatePointerCasts4Pass();
BasicBlockPass* createResearchPointerarithmeticPass();

class ResearchPreprocessorPass : public PassInfoMixin<ResearchPreprocessorPass> {
public:
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
    return PreservedAnalyses::none();
  }
};

class ResearchReplacerPass : public PassInfoMixin<ResearchReplacerPass> {
public:
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
    return PreservedAnalyses::none();
  }
};

}

#endif

add_llvm_library(LLVMCAnDLPasses

  CustomPassReplace.cpp
  CustomPassPreprocess.cpp
  CustomPassFixOrToAdd.cpp
  CustomPassFixShlToMul.cpp
  CustomPassMergePointercalcs.cpp
  CustomPassPointerarithmetic.cpp

  TransformDistributive.cpp
  TransformHoistSelection.cpp

  DEPENDS
  intrinsics_gen
)


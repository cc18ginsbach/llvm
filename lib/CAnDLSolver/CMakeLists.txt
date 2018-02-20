add_llvm_library(LLVMCAnDLSolver

  Solver.cpp
  Solution.cpp
  FunctionWrap.cpp

  BackendEdge.cpp
  BackendSingle.cpp
  BackendCollect.cpp
  BackendDominate.cpp
  BackendOrdering.cpp
  BackendSameSets.cpp
  BackendIncomingValue.cpp
  BackendSpecializations.cpp

  DEPENDS
  intrinsics_gen
)


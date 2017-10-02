set( LLVM_LINK_COMPONENTS
  ${LLVM_TARGETS_TO_BUILD}
  Option
  Support
  )

#set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
#set( SOURCES
#	steengaardPA.cpp
#	steengaardPA.h
#	macros.h
#)

add_clang_executable(racer
  callGraphAnalysis.h	
  globalVarHandler.h
  globalVarHandler.cpp
  macros.h	
  racer.cpp
  raceAnalysis.h
  raceAnalysis.cpp
  steengaardPA.h
  steengaardPA.cpp
  steengaardPAVisitor.h
  steengaardPAVisitor.cpp
  symtab.h
  symtab.cpp
  symTabBuilder.h
  symTabBuilder.cpp  
  CGFrontendAction.h
  libExt/CompilerInstanceCtu.h
  libExt/CallGraphCtu.h
  libExt/CallGraphCtu.cpp
  )

#add_clang_executable(cgwpa
#	wpCallGraphBuilder.cpp
#)

target_link_libraries(racer
  clangAST
  clangBasic
  clangDriver
  clangFrontend
  clangRewriteFrontend
  clangStaticAnalyzerFrontend
  clangTooling
  )


#add_custom_target(ast)
#foreach(Sourcefile ${SourceFiles})
#add_custom_command(TARGET ast PRE_BUILD
#       COMMAND clang -emit-ast ${SourceFile} -o /repo/emasabu/spa-test/${SourceFiles}.ast)

install(TARGETS racer
  RUNTIME DESTINATION bin)

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
  racer.cpp
  steengaardPA.cpp
  steengaardPA.h
  steengaardPAVisitor.h
  steengaardPAVisitor.cpp
  macros.h
  symTabBuilder.h
  symtab.cpp
  symtab.h
  RaceAnalysis.h
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

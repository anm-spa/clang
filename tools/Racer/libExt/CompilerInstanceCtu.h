/****************************************************************/
/*          All rights reserved (it will be changed)            */
/*          masud.abunaser@mdh.se                               */
/****************************************************************/


//===-- CompilerInstanceCtu.h - Clang Compiler Instance ------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_FRONTEND_COMPILERINSTANCECTU_H_
#define LLVM_CLANG_FRONTEND_COMPILERINSTANCECTU_H_

#include "clang/Frontend/CompilerInstance.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/CompilerInvocation.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/PCHContainerOperations.h"
#include "clang/Frontend/Utils.h"
#include "clang/Lex/HeaderSearchOptions.h"
#include "clang/Lex/HeaderSearch.h"
#include "clang/Lex/ModuleLoader.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/IntrusiveRefCntPtr.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/raw_ostream.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/Version.h"
#include <cassert>
#include <list>
#include <memory>
#include <string>
#include <utility>
#include "../CGFrontendAction.h"

#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/MemoryBufferCache.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/Version.h"
#include "clang/Config/config.h"
#include "clang/Frontend/ChainedDiagnosticConsumer.h"
#include "clang/Frontend/FrontendDiagnostic.h"
#include "clang/Frontend/LogDiagnosticPrinter.h"
#include "clang/Frontend/SerializedDiagnosticPrinter.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "clang/Frontend/VerifyDiagnosticConsumer.h"
#include "clang/Frontend/ASTUnit.h"

using namespace clang;

class CGFrontendAction;

namespace llvm {
   class raw_fd_ostream;
   class Timer;
   class TimerGroup;
 }


namespace clang {
class ASTContext;
class ASTReader;
class CodeCompleteConsumer;
class DiagnosticsEngine;
class DiagnosticConsumer;
class ExternalASTSource;
class FileEntry;
class FileManager;
class FrontendAction;
class MemoryBufferCache;
class Module;
class Preprocessor;
class Sema;
class SourceManager;
class TargetInfo;


class CompilerInstanceCtu : public CompilerInstance {
  
  FrontendAction *frontend;
  bool isCurrentFileAST;
  bool shouldEraseOutputFile;
public:
  CompilerInstanceCtu(std::shared_ptr<PCHContainerOperations> PCHContainerOps =
			       std::make_shared<PCHContainerOperations>(), MemoryBufferCache *SharedPCMCache = nullptr): CompilerInstance(PCHContainerOps, SharedPCMCache)
  {}
~CompilerInstanceCtu(){
}

  void setFrontendAction(FrontendAction *Val)
  {
    frontend=Val;
    isCurrentFileAST=false;
    shouldEraseOutputFile=false;
  }  

  FrontendAction *getFrontendAction()
  {
    return frontend;
  }

  void setFrontendFileStatus()
  {
    isCurrentFileAST=frontend->isCurrentFileAST();
    shouldEraseOutputFile=getDiagnostics().hasErrorOccurred();
  }
  bool ExecuteActionCtu(FrontendAction &Act)
  {
    assert(hasDiagnostics() && "Diagnostics engine is not initialized!");
    assert(!getFrontendOpts().ShowHelp && "Client must handle '-help'!");
    assert(!getFrontendOpts().ShowVersion && "Client must handle '-version'!");

    // FIXME: Take this as an argument, once all the APIs we used have moved to
    // taking it as an input instead of hard-coding llvm::errs.
    raw_ostream &OS = llvm::errs();

    // Create the target instance.
    setTarget(TargetInfo::CreateTargetInfo(getDiagnostics(),
                                         getInvocation().TargetOpts));
    if (!hasTarget())
      return false;
    
    // Create TargetInfo for the other side of CUDA compilation.
    if (getLangOpts().CUDA && !getFrontendOpts().AuxTriple.empty()) {
      auto TO = std::make_shared<TargetOptions>();
      TO->Triple = getFrontendOpts().AuxTriple;
      TO->HostTriple = getTarget().getTriple().str();
      setAuxTarget(TargetInfo::CreateTargetInfo(getDiagnostics(), TO));
    }
    
  // Inform the target of the language options.
  //
  // FIXME: We shouldn't need to do this, the target should be immutable once
  // created. This complexity should be lifted elsewhere.
  getTarget().adjust(getLangOpts());

  // Adjust target options based on codegen options.
  getTarget().adjustTargetOptions(getCodeGenOpts(), getTargetOpts());

  // rewriter project will change target built-in bool type from its default. 
  if (getFrontendOpts().ProgramAction == frontend::RewriteObjC)
    getTarget().noSignedCharForObjCBool();

  // Validate/process some options.
  if (getHeaderSearchOpts().Verbose)
    OS << "clang -cc1 version "<< CLANG_VERSION_STRING
       << " based upon " /*<< BACKEND_PACKAGE_STRING*/
       << " default target " << llvm::sys::getDefaultTargetTriple() << "\n";

  if (getFrontendOpts().ShowTimers)
    createFrontendTimer();

  if (getFrontendOpts().ShowStats || !getFrontendOpts().StatsFile.empty())
    llvm::EnableStatistics(false);

  for (const FrontendInputFile &FIF : getFrontendOpts().Inputs) {
    // Reset the ID tables if we are reusing the SourceManager and parsing
    // regular files.
    if (hasSourceManager() && !Act.isModelParsingAction())
      getSourceManager().clearIDTables();
    //std::unique_ptr<clang::CGFrontendAction> cf=&Act;

    
    // CGFrontendAction should be parametric to compilerinstance
    CGFrontendAction *cf=static_cast<CGFrontendAction *>(&Act);
    if(cf)
    if (cf->BeginSourceFile(*this, FIF)) {
      setFrontendFileStatus();
      cf->Execute();   
      cf->EndSourceFileCtu();
    }
  }

  // Notify the diagnostic client that all files were processed.
  getDiagnostics().getClient()->finish();

  if (getDiagnosticOpts().ShowCarets) {
    // We can have multiple diagnostics sharing one diagnostic client.
    // Get the total number of warnings/errors from the client.
    unsigned NumWarnings = getDiagnostics().getClient()->getNumWarnings();
    unsigned NumErrors = getDiagnostics().getClient()->getNumErrors();

    if (NumWarnings)
      OS << NumWarnings << " warning" << (NumWarnings == 1 ? "" : "s");
    if (NumWarnings && NumErrors)
      OS << " and ";
    if (NumErrors)
      OS << NumErrors << " error" << (NumErrors == 1 ? "" : "s");
    if (NumWarnings || NumErrors)
      OS << " generated.\n";
  }

  if (getFrontendOpts().ShowStats) {
    if (hasFileManager()) {
      getFileManager().PrintStats();
      OS << '\n';
    }
    llvm::PrintStatistics(OS);
  }
  StringRef StatsFile = getFrontendOpts().StatsFile;
  if (!StatsFile.empty()) {
    std::error_code EC;
    auto StatS = llvm::make_unique<llvm::raw_fd_ostream>(StatsFile, EC,
                                                         llvm::sys::fs::F_Text);
    if (EC) {
      getDiagnostics().Report(diag::warn_fe_unable_to_open_stats_file)
          << StatsFile << EC.message();
    } else {
      llvm::PrintStatisticsJSON(*StatS);
    }
  }
  return !getDiagnostics().getClient()->getNumErrors();
}


void EndCompilerActionOnSourceFile() {
  // Inform the diagnostic client we are done with this source file.
  getDiagnosticClient().EndSourceFile();
 
  // Inform the preprocessor we are done.
  if (hasPreprocessor())
    getPreprocessor().EndSourceFile();
 
  // Sema references the ast consumer, so reset sema first.
  // FIXME: There is more per-file stuff we could just drop here?
  bool DisableFree = getFrontendOpts().DisableFree;
  if (DisableFree) {
    resetAndLeakSema();
    resetAndLeakASTContext();
    BuryPointer(takeASTConsumer().get());
  } else {
    setSema(nullptr);
    setASTContext(nullptr);
    setASTConsumer(nullptr);
  }
  FrontendAction *frontend=getFrontendAction();
  if (getFrontendOpts().ShowStats) {
    llvm::errs() << "\nSTATISTICS FOR '" << frontend->getCurrentFile() << "':\n";
    getPreprocessor().PrintStats();
    getPreprocessor().getIdentifierTable().PrintStats();
    getPreprocessor().getHeaderSearchInfo().PrintStats();
    getSourceManager().PrintStats();
    llvm::errs() << "\n";
  }

  // Cleanup the output streams, and erase the output files if instructed by the
  // FrontendAction.
  if(isCurrentFileAST) 
    {
    if (DisableFree) {
      resetAndLeakPreprocessor();
      resetAndLeakSourceManager();
      resetAndLeakFileManager();
    } else {
      setPreprocessor(nullptr);
      setSourceManager(nullptr);
      setFileManager(nullptr);
    }
  }
  clearOutputFiles(shouldEraseOutputFile);
  getLangOpts().setCompilingModule(LangOptions::CMK_None);
}


  
};
}
#endif

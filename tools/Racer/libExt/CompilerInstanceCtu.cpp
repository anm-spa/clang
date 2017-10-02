/****************************************************************/
/*          All rights reserved (it will be changed)            */
/*          masud.abunaser@mdh.se                               */
/****************************************************************/

#include "CompilerInstanceCtu.h"

bool CompilerInstanceCtu::ExecuteActionCtu(clang::FrontendAction &Act) {
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
    OS << "clang -cc1 version " CLANG_VERSION_STRING
       << " based upon " << BACKEND_PACKAGE_STRING
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
    if (Act.BeginSourceFile(*this, FIF)) {
      Act.Execute();
      Act.EndSourceFileCtu();
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


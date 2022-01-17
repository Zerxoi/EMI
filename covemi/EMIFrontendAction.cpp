#include <filesystem>
#include <fstream>

#include "clang/Frontend/CompilerInstance.h"

#include "EMIFrontendAction.h"
#include "EMIASTConsumer.h"

EMIFrontendAction::EMIFrontendAction(std::string Extension, int MethodOption, const std::string &OutputOption) : Extension(Extension), MethodOption(MethodOption), OutputOption(OutputOption) {}

void EMIFrontendAction::EndSourceFileAction()
{
  clang::SourceManager &SM = TheRewriter.getSourceMgr();

  // Write EMI buffer to local file
  std::error_code err;
  std::filesystem::path filePath = FileName;
  std::filesystem::path emiPath;

  std::filesystem::path outputpath = OutputOption;
  if (outputpath.empty() || !outputpath.has_root_directory())
  {
    emiPath /= filePath.parent_path();
  }
  if (!outputpath.empty())
  {
    emiPath /= outputpath;
  }
  emiPath /= filePath.stem();
  emiPath += Extension;
  emiPath += filePath.extension();

  std::filesystem::create_directories(emiPath.parent_path());
  std::string path = emiPath.string();
  llvm::raw_fd_ostream ofs(path, err);
  TheRewriter.getEditBuffer(SM.getMainFileID()).write(ofs);
  ofs.close();

  llvm::outs() << "EMI file location: " << path << "\n";
}

GCovFrontendAction::GCovFrontendAction(int MethodOption, const std::string &OutputOption) : EMIFrontendAction(".gcov", MethodOption, OutputOption) {}

std::unique_ptr<clang::ASTConsumer> GCovFrontendAction::CreateASTConsumer(clang::CompilerInstance &CI, llvm::StringRef file)
{
  TheRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
  FileName = file.str();
  return std::make_unique<GCovConsumer>(TheRewriter, CI.getASTContext(), file, MethodOption);
}

LLVMCovFrontendAction::LLVMCovFrontendAction(int MethodOption, const std::string &OutputOption) : EMIFrontendAction(".llvm-cov", MethodOption, OutputOption) {}

std::unique_ptr<clang::ASTConsumer> LLVMCovFrontendAction::CreateASTConsumer(clang::CompilerInstance &CI, llvm::StringRef file)
{
  TheRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
  FileName = file.str();
  return std::make_unique<LLVMCovConsumer>(TheRewriter, CI.getASTContext(), file, MethodOption);
}
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

#include "EMIFrontendAction.h"

static llvm::cl::OptionCategory ClangToolCategory("Clang Tool Options");

int main(int argc, const char *argv[])
{
    clang::tooling::CommonOptionsParser op(argc, argv, ClangToolCategory);
    clang::tooling::ClangTool Tool(op.getCompilations(), op.getSourcePathList());

    // ClangTool::run accepts a FrontendActionFactory, which is then used to
    // create new objects implementing the FrontendAction interface.
    Tool.run(clang::tooling::newFrontendActionFactory<GCovFrontendAction>().get());
    Tool.run(clang::tooling::newFrontendActionFactory<LLVMCovFrontendAction>().get());
}
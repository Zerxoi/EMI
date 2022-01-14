#include "fstream"

#include "DiffFrontendAction.h"
#include "DiffASTConsumer.h"

DiffFrontendAction::DiffFrontendAction(const std::vector<int> &gcovLines, const std::vector<int> &llvmcovLines, const std::filesystem::path &DirPath, const std::vector<DiffParser *> *DiffParserVector)
    : gcovLines(gcovLines), llvmcovLines(llvmcovLines), DirPath(DirPath), DiffParserVector(DiffParserVector){};

std::unique_ptr<clang::ASTConsumer> DiffFrontendAction::CreateASTConsumer(clang::CompilerInstance &Compiler, llvm::StringRef InFile)
{
    const std::vector<int> *lines;
    if (InFile.contains(".gcov"))
    {
        CoverageToolId = 0;
        lines = &gcovLines;
    }
    else if (InFile.contains(".llvm-cov"))
    {
        CoverageToolId = 1;
        lines = &llvmcovLines;
    }
    else
    {
        throw std::runtime_error("Unable to tell if the file was generated by gcov or llvm-cov");
    }
    return std::make_unique<DiffASTConsumer>(&Compiler.getASTContext(), *lines, CoverageToolId, InFile, DiffParserVector, DiffReasonVector);
}

void DiffFrontendAction::EndSourceFileAction()
{
    std::filesystem::create_directories(DirPath);
    std::filesystem::path path = DirPath;
    std::string coverageTool;
    if (CoverageToolId == 0)
    {
        path /= "gcov.map";
        coverageTool = "gcov";
    }
    else if (CoverageToolId == 1)
    {
        path /= "llvm-cov.map";
        coverageTool = "llvm-cov";
    }
    else
    {
        throw std::runtime_error("Cannot handle CoverageToolId with value " + CoverageToolId);
    }

    std::ofstream ofs(path);

    for (const auto &reason : DiffReasonVector)
    {
        if (reason->getCoverageToolId() == CoverageToolId)
        {
            ofs << reason->getDescription() << "@" << reason->getFileType() << ":" << reason->getLineNum() << "\n";
        }
    }
    ofs.close();
    llvm::outs() << coverageTool << " diff reason location: " << path << "\n";
}
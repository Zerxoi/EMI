#include "fstream"

#include "DiffFrontendAction.h"
#include "DiffASTConsumer.h"
#include "Const.h"
#include "Util.h"

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
    const std::vector<int> *lines;
    if (CoverageToolId == 0)
    {
        path /= "gcov.map";
        coverageTool = "gcov";
        lines = &gcovLines;
    }
    else if (CoverageToolId == 1)
    {
        path /= "llvm-cov.map";
        coverageTool = "llvm-cov";
        lines = &llvmcovLines;
    }
    else
    {
        throw std::runtime_error("Cannot handle CoverageToolId with value " + CoverageToolId);
    }

    std::ofstream ofs(path);

    int index = 0;
    for (; index < DiffReasonVector.size(); index++)
    {
        const DiffReason *reason = DiffReasonVector.at(index);
        if (reason->getParser() != nullptr)
        {
            auto parser = reason->getParser();
            ofs << util::idToString(parser->getCoverageToolId()) << ":" << parser->getDescription() << "#" << parser->getCount() << "@" << reason->getLineNum() << "\n";
        }
        else
        {
            ofs << reason->getDescription() << "@" << reason->getLineNum() << "\n";
        }
    }
    while (index < lines->size())
    {
        ofs << reason::description::terminated << "@" << lines->at(index++) << "\n";
    }

    ofs << "=================================== Diff Report ===================================\n";
    int total = 0;
    for (const auto &parser : *DiffParserVector)
    {
        if (parser->getFileTypeId() == CoverageToolId)
        {
            int count = parser->getCount();
            total += count;
            ofs << "#" << util::idToString(parser->getCoverageToolId()) << "@" << parser->getDescription() << ":" << count << "\n";
        }
    }
    ofs << "\n";
    ofs << "Total:" << total << "\n";

    ofs.close();
    llvm::outs() << coverageTool << " diff reason location: " << path << "\n";
}
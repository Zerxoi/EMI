#include "clang/AST/Expr.h"

#include "Util.h"
#include "DiffParser.h"

DiffParser::DiffParser(int CoverageToolId, int FileTypeId, std::string Description) : CoverageToolId(CoverageToolId), FileTypeId(FileTypeId), Description(Description) {}

int DiffParser::getFileTypeId()
{
    return FileTypeId;
}

std::string DiffParser::getFileType()
{
    return util::idToString(FileTypeId);
}

int DiffParser::getCoverageToolId()
{
    return CoverageToolId;
}

std::string DiffParser::getCoverageTool()
{
    return util::idToString(CoverageToolId);
}

const std::string &DiffParser::getDescription()
{
    return Description;
}

int DiffParser::getCount()
{
    return Count;
}

UnmarkedLabelDiffParser::UnmarkedLabelDiffParser() : UnmarkedLabelStmt(nullptr), DiffParser(0, 1, "Unmarked Label"){};

bool UnmarkedLabelDiffParser::parse(const clang::Stmt *s, clang::ASTContext *Context)
{
    if (clang::isa<clang::LabelStmt>(s))
    {
        if (!util::isAncestorRelation(s, UnmarkedLabelStmt))
        {
            UnmarkedLabelStmt = s;
            Count++;
        }
        return true;
    }
    if (util::isAncestorRelation(s, UnmarkedLabelStmt))
    {
        return true;
    }
    return false;
}

ConstArrayInitializationDiffParser::ConstArrayInitializationDiffParser() : DiffParser(0, 0, "Const Array Initialization"){};

bool ConstArrayInitializationDiffParser::parse(const clang::Stmt *s, clang::ASTContext *Context)
{
    if (auto declStmt = clang::dyn_cast<clang::DeclStmt>(s))
    {
        for (auto decl : declStmt->decls())
        {
            if (auto *varDecl = clang::dyn_cast<clang::VarDecl>(decl))
            {
                if (varDecl->hasInit() && clang::isa<clang::ConstantArrayType>(varDecl->getType()) && varDecl->getType().isConstQualified())
                {
                    Count++;
                    return true;
                }
            }
        }
    }
    return false;
}

IfOptimizeDiffParser::IfOptimizeDiffParser() : IfOptimizeStmt(nullptr), DiffParser(0, 0, "If Optimize") {}

bool IfOptimizeDiffParser::isEvaluatable(const clang::Expr *expr, const clang::ASTContext &Context)
{
    if (auto binOpr = clang::dyn_cast<clang::BinaryOperator>(expr))
    {
        if (binOpr->isCommaOp())
        {
            return isEvaluatable(binOpr->getRHS(), Context);
        }
    }
    if (auto parenOpr = clang::dyn_cast<clang::ParenExpr>(expr))
    {
        return isEvaluatable(parenOpr->getSubExpr(), Context);
    }
    return expr->isEvaluatable(Context);
}

bool IfOptimizeDiffParser::parse(const clang::Stmt *s, clang::ASTContext *Context)
{
    if (auto ifStmt = clang::dyn_cast<clang::IfStmt>(s))
    {
        if (isEvaluatable(ifStmt->getCond(), *Context))
        {
            if (!util::isAncestorRelation(s, IfOptimizeStmt))
            {
                IfOptimizeStmt = s;
                Count++;
            }
            return true;
        }
    }
    if (util::isAncestorRelation(s, IfOptimizeStmt))
    {
        return true;
    }
    return false;
}

JumpBlockDiffParser::JumpBlockDiffParser() : PreBlockJumpStmt(nullptr), DiffParser(1, 1, "Jump Block") {}

bool JumpBlockDiffParser::isBlockJumpStmt(const clang::Stmt *s)
{
    if (s != nullptr && (clang::isa<clang::ReturnStmt>(s) || clang::isa<clang::BreakStmt>(s) || clang::isa<clang::ContinueStmt>(s)))
    {
        if (s != PreBlockJumpStmt)
        {
            Count++;
        }
        return true;
    }
    return false;
}

bool JumpBlockDiffParser::containsBlockJumpStmt(const clang::Stmt *s)
{
    if (auto ifStmt = clang::dyn_cast<clang::IfStmt>(s))
    {
        auto thenStmt = ifStmt->getThen();
        auto elseStmt = ifStmt->getElse();
        if (isBlockJumpStmt(thenStmt) || isBlockJumpStmt(elseStmt))
        {
            return true;
        }
        else if (auto thenCompoudStmt = clang::dyn_cast<clang::CompoundStmt>(thenStmt))
        {
            for (auto stmt : thenCompoudStmt->children())
            {
                if (containsBlockJumpStmt(stmt))
                {
                    return true;
                }
            }
        }
        else if (elseStmt != nullptr)
        {
            if (auto elseCompoudStmt = clang::dyn_cast<clang::CompoundStmt>(elseStmt))
                for (auto stmt : elseCompoudStmt->children())
                {
                    if (containsBlockJumpStmt(stmt))
                    {
                        return true;
                    }
                }
        }
    }
    else if (auto forStmt = clang::dyn_cast<clang::ForStmt>(s))
    {
        auto bodyStmt = forStmt->getBody();
        if (isBlockJumpStmt(bodyStmt))
        {
            return true;
        }
        else if (auto bodyCompoundStmt = clang::dyn_cast<clang::CompoundStmt>(bodyStmt))
        {
            for (auto stmt : bodyCompoundStmt->children())
            {
                if (containsBlockJumpStmt(stmt))
                {
                    return true;
                }
            }
        }
    }
    return isBlockJumpStmt(s);
}

bool JumpBlockDiffParser::parse(const clang::Stmt *s, clang::ASTContext *Context)
{
    auto preStmt = util::getSiblingStmt(s, -1, Context);
    if (containsBlockJumpStmt(preStmt))
    {
        PreBlockJumpStmt = s;
        return true;
    }
    else
        return false;
}

const std::vector<DiffParser *> *createDiffParserVector()
{
    std::vector<DiffParser *> *vector = new std::vector<DiffParser *>;
    vector->push_back(new IfOptimizeDiffParser);
    vector->push_back(new UnmarkedLabelDiffParser);
    vector->push_back(new ConstArrayInitializationDiffParser);
    vector->push_back(new JumpBlockDiffParser);

    return vector;
}
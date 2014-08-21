#pragma once

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/CompilationDatabase.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/ArgumentsAdjusters.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Lex/Lexer.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/ArrayRef.h"

#include <vector>
#include <map>
#include <string>


class Parser {
public:
    
    class Options {
    public:
        
        Options& outputDirectory( const std::string& path ){ mOutputDirectory = path; return *this; }
        Options& inputDirectory( const std::string& path ){ mInputDirectory = path; return *this; }
        
        Options& inputFileList( const std::vector<std::string>& list ){ mInputFileList = list; return *this; }
        Options& excludeFileList( const std::vector<std::string>& list ){ mExcludeFileList = list; return *this; }
        Options& excludeDirectoryList( const std::vector<std::string>& list ){ mExcludeDirectoryList = list; return *this; }
        Options& compilerFlags( const std::vector<std::string>& flags ){ mCompilerFlags = flags; return *this; }
        
        std::string getOutputDirectory(){ return mOutputDirectory; }
        std::string getInputDirectory(){ return mInputDirectory; }
        
        const std::vector<std::string>& getInputFileList(){ return mInputFileList; }
        const std::vector<std::string>& getExcludeFileList(){ return mExcludeFileList; }
        const std::vector<std::string>& getExcludeDirectoryList(){ return mExcludeDirectoryList; }
        const std::vector<std::string>& getCompilerFlags(){ return mCompilerFlags; }
    protected:
        std::string                 mOutputDirectory;
        std::string                 mInputDirectory;
        
        std::vector<std::string>    mInputFileList;
        std::vector<std::string>    mExcludeFileList;
        std::vector<std::string>    mExcludeDirectoryList;
        
        std::vector<std::string>    mCompilerFlags;
    };
    
    Parser( Options options = Options() );
    
protected:
    
    // visitor class
    class Visitor : public clang::RecursiveASTVisitor<Visitor> {
    public:
        //! constructor
        Visitor( clang::ASTContext* context ) : mContext(context) {}
        
        //! visits exceptions
        bool VisitCXXThrowExpr(clang::CXXThrowExpr *declaration);
        //! visits enumerators
        bool VisitEnumDecl(clang::EnumDecl *declaration);
        //! visits namespace aliases
        bool VisitNamespaceAliasDecl(clang::NamespaceAliasDecl *declaration);
        //! visits namespaces
        bool VisitNamespaceDecl(clang::NamespaceDecl *declaration);
        //! visits typedefs
        bool VisitTypedefDecl(clang::TypedefDecl *declaration);
        //! visits c++ classes
        bool VisitCXXRecordDecl(clang::CXXRecordDecl *declaration);
        //! visits functions
        bool VisitFunctionDecl(clang::FunctionDecl *declaration);
        
        //! stops template instantiations visits
        bool shouldVisitTemplateInstantiations() const { return false; }
        //! stops implicit code visits
        bool shouldVisitImplicitCode() const { return false; }
        
    private:
        //! returns the full declaration as a string
        template<typename T>
        std::string declToString(T *d);
        //! returns whether the declaration is in the main file or not
        template<typename T>
        bool isInMainFile( T *declaration );
        
        clang::ASTContext*                                  mContext;
        std::map<std::string,clang::NamespaceAliasDecl*>    mNamespaceAliases;
        clang::CXXRecordDecl*                               mExceptionDecl;
    };
    
    // consumer class
    class Consumer : public clang::ASTConsumer {
    public:
        //! constructor
        Consumer(clang::ASTContext* context) : mVisitor( context ) {}
        
        //! parses each top-level declarations
        bool HandleTopLevelDecl(clang::DeclGroupRef group) override;
        
    private:
        clang::ASTContext*  mContext;
        Visitor             mVisitor;
    };
    
    // action class
    class FrontendAction : public clang::ASTFrontendAction {
    public:
        //! constructor
        FrontendAction() {}
        
        void EndSourceFileAction() override;
        
        //! returns our writter consumer
        clang::ASTConsumer *CreateASTConsumer( clang::CompilerInstance &compiler, clang::StringRef file ) override;
        
    };
    
    Options mOptions;
};


// templates method definitions

//! returns the full declaration as a string
template<typename T>
std::string Parser::Visitor::declToString(T *d) {
    clang::LangOptions langOpts;
    langOpts.CPlusPlus = true;
    const clang::SourceManager& sm = mContext->getSourceManager();
    return clang::Lexer::getSourceText( clang::CharSourceRange::getTokenRange(d->getSourceRange()), sm, langOpts, 0);
}

//! returns whether the declaration is in the main file or not
template<typename T>
bool Parser::Visitor::isInMainFile( T *declaration ){
    return mContext->getSourceManager().isInMainFile( mContext->getFullLoc( declaration->getLocStart() ) );
}


#pragma once

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Mangle.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/CompilationDatabase.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/ArgumentsAdjusters.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Lex/Lexer.h"
#include "clang/Lex/PPCallbacks.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Lex/Preprocessor.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/ArrayRef.h"


#include <vector>
#include <map>
#include <string>
#include <fstream>
#include <sstream>


class Parser {
public:
    
    class Options {
    public:
        
        Options& outputDirectory( const std::string& path ){ mOutputDirectory = path; return *this; }
        Options& inputDirectory( const std::string& path ){ mInputDirectory = path; return *this; }
        Options& license( const std::string& license ){ mLicense = license; return *this; }
        
        Options& inputFileList( const std::vector<std::string>& list ){ mInputFileList = list; return *this; }
        Options& excludeFileList( const std::vector<std::string>& list ){ mExcludeFileList = list; return *this; }
        Options& excludeDirectoryList( const std::vector<std::string>& list ){ mExcludeDirectoryList = list; return *this; }
        Options& compilerFlags( const std::vector<std::string>& flags ){ mCompilerFlags = flags; return *this; }
        Options& unsupportedTypes( const std::vector<std::string>& types ){ mUnsupportedTypes = types; return *this; }
        
        std::string getOutputDirectory() const { return mOutputDirectory; }
        std::string getInputDirectory() const { return mInputDirectory; }
        std::string getLicense() const { return mLicense; }
        
        const std::vector<std::string>& getInputFileList() const { return mInputFileList; }
        const std::vector<std::string>& getExcludeFileList() const { return mExcludeFileList; }
        const std::vector<std::string>& getExcludeDirectoryList() const { return mExcludeDirectoryList; }
        const std::vector<std::string>& getCompilerFlags() const { return mCompilerFlags; }
        const std::vector<std::string>& getUnsupportedTypes() const { return mUnsupportedTypes; }
        
    protected:
        std::string                 mOutputDirectory;
        std::string                 mInputDirectory;
        std::string                 mLicense;
        
        std::vector<std::string>    mInputFileList;
        std::vector<std::string>    mExcludeFileList;
        std::vector<std::string>    mExcludeDirectoryList;
        std::vector<std::string>    mCompilerFlags;
        std::vector<std::string>    mUnsupportedTypes;
    };
    
    Parser( Options options = Options() );
    
protected:
    struct Output {
        Output() : mIsInNamespace(false) {}
        
        std::stringstream   mClassDecl;
        std::stringstream   mClassDef;
        std::stringstream   mClassFieldDecl;
        std::stringstream   mClassFieldDef;
        std::stringstream   mClassMethodDecl;
        std::stringstream   mClassMethodDef;
        
        std::stringstream   mTemplatesDecl;
        std::stringstream   mTemplatesDef;
        
        std::stringstream   mEnumsDecl;
        std::stringstream   mFunctionDef;
        
        std::stringstream   mDeclCalls;
        std::stringstream   mDefCalls;
        
        bool    mIsInNamespace;
        
        std::string mCurrentEnumScope;
        std::string mCurrentFunctionScope;
    };
    
    // visitor class
    class Visitor : public clang::RecursiveASTVisitor<Visitor> {
    public:
        //! constructor
        Visitor( clang::ASTContext* context, Output& output, const Options& options ) : mContext(context), mOutput(output), mOptions(options) {}
        
        //! visits exceptions
        bool VisitCXXThrowExpr(clang::CXXThrowExpr *expr);
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
        
        //! returns the full scope from a DeclContext
        std::string getFullScope( clang::DeclContext* declarationContext, const std::string& currentScope );
        //! returns the full scope from a QualType
        std::string getFullScope( const clang::QualType& type, const std::string& currentScope );
        //! returns the class scope from a DeclContext
        std::string getClassScope( clang::DeclContext* declarationContext, const std::string& currentScope );
        //! returns the class scope from a QualType
        std::string getClassScope( const clang::QualType& type, const std::string& currentScope );
        
        //! returns the name of a Declaration
        std::string getDeclarationName( clang::NamedDecl* declaration );
        //! returns the qualified/scoped name of a Declaration
        std::string getDeclarationQualifiedName( clang::NamedDecl* declaration );
        //! returns the name of a QualType
        std::string getTypeName( const clang::QualType &type );
        //! returns the qualified/scoped name of a QualType
        std::string getTypeQualifiedName( const clang::QualType &type );
        //! returns the list of argument name of a function
        std::string getFunctionArgList( clang::FunctionDecl *function );
        //! returns the qualified/scoped list of argument type name of a function
        std::string getFunctionArgTypeList( clang::FunctionDecl *function );
        //! returns the qualified/scoped list of argument names of a function
        std::string getFunctionArgNameList( clang::FunctionDecl *function );
        //! returns the return type name of a function
        std::string getFunctionReturnType( clang::FunctionDecl *function );
        //! returns the qualified/scoped return type name of a function
        std::string getFunctionQualifiedReturnType( clang::FunctionDecl *function );
        
        //! replaces namespaces by aliases and returns the corrected string
        std::string replaceNamespacesByAliases( const std::string &declaration );
        //! makes each word first char upper case, removes the :: and returns the styled string
        std::string styleScopedName( const std::string &declaration );
        //! returns quoted string
        std::string quote( const std::string &declaration );
        
        
        clang::DeclContext* getTypeDeclContext( const clang::QualType& type );
        
        //! returns wether a string contains unsupported types
        bool isSupported( const std::string& expr );
        
        clang::ASTContext*                                  mContext;
        Output&                                             mOutput;
        const Options&                                      mOptions;
        std::map<std::string,clang::NamespaceAliasDecl*>    mNamespaceAliases;
        clang::CXXRecordDecl*                               mExceptionDecl;
    };
    
    // consumer class
    class Consumer : public clang::ASTConsumer {
    public:
        //! constructor
        Consumer( clang::ASTContext* context, Output& output, const Options& options ) : mVisitor(context, output, options), mOutput(output) {}
        
        //! parses each top-level declarations
        //bool HandleTopLevelDecl(clang::DeclGroupRef group) override;
        
        // this replaces "HandleTopLevelDecl"
        // override this to call our ExampleVisitor on the entire source file
        void HandleTranslationUnit( clang::ASTContext &context );
        
        
    private:
        clang::ASTContext*  mContext;
        Visitor             mVisitor;
        Output&             mOutput;
    };
    
    class PreprocessorParser : public clang::PPCallbacks {
    public:
        PreprocessorParser( clang::ASTContext* context, Output& output ) : mContext(context), mOutput(output) {}
        
        virtual void InclusionDirective( clang::SourceLocation HashLoc,
                                        const clang::Token &IncludeTok,
                                        clang::StringRef FileName,
                                        bool IsAngled,
                                        clang::CharSourceRange FilenameRange,
                                        const clang::FileEntry *File,
                                        clang::StringRef SearchPath,
                                        clang::StringRef RelativePath,
                                        const clang::Module *Imported);
    private:
        clang::ASTContext*  mContext;
        Output&      mOutput;
    };
    
    // action class
    class FrontendAction : public clang::ASTFrontendAction {
    public:
        //! constructor
        FrontendAction( Output& output, const Options& options ) : mOutput(output), mOptions(options) {}
        
        void EndSourceFileAction() override;
        
        
        //! returns our writter consumer
        clang::ASTConsumer *CreateASTConsumer( clang::CompilerInstance &compiler, clang::StringRef file ) override;
        
    private:
        Output&         mOutput;
        const Options&  mOptions;
    };
    
    Options         mOptions;
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


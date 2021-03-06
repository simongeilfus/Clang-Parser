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


typedef std::shared_ptr<class Object>   ObjectRef;
typedef std::shared_ptr<class Function> FunctionRef;
typedef std::shared_ptr<class Enum>     EnumRef;
typedef std::shared_ptr<class Class>    ClassRef;
typedef std::shared_ptr<class Field>    FieldRef;
typedef std::shared_ptr<class Method>   MethodRef;

class Object {
public:
    static FunctionRef  createFunction( const std::string &name );
    static EnumRef      createEnum( const std::string &name );
    static ClassRef     createClass( const std::string &name );
    static FieldRef     createField( const std::string &name );
    static MethodRef    createMethod( const std::string &name );
    
    //! creates and returns an empty named object
    Object( const std::string &name ) : mName( name ) {}
    
    //! sets the name and returns the object
    Object& name( const std::string &name ) { mName = name; return *this; }
    //! sets the unique name and returns the object
    Object& uniqueName( const std::string &name ) { mUniqueName = name; return *this; }
    //! sets whether the object is constant and returns the object
    Object& constant( bool isConstant = true ) { mIsConst = isConstant; return *this; }
    //! sets whether the object is a template
    Object& templated( bool isTemplate = true ) { mIsTemplate = isTemplate; return *this; }
    
    //! returns the object name
    std::string getName() const { return mName; }
    //! returns the object unique name
    std::string getUniqueName() const { return mUniqueName; }
    //! returns the object kind
    std::string getKind() const { return "Object"; }
    
    //! returns whether the object is static
    bool isStatic() const { return false; }
    //! returns whether the object is const
    bool isConst() const { return mIsConst; }
    //! returns whether the object is a template
    bool isTemplate() const { return mIsTemplate; }
    
protected:
    std::string mName;
    std::string mUniqueName;
    bool        mIsConst;
    bool        mIsTemplate;
};

class Function : public Object {
public:
    //! creates and return an empty named function
    Function( const std::string &name ) : Object( name ) {}
    
    //! sets the function return type
    Object& returnType( const std::string &returnType ) { mReturnType = returnType; return *this; }
    
    //! adds a function parameter
    void addParameter( const std::string &type, const std::string &name ){ mParams.push_back( type + " " + name ); mParamsTypes.push_back( type ); }
    
    //! returns the function return type
    std::string getReturnType() const { return mReturnType; }
    //! returns the function parameters
    const std::vector<std::string>& getParams() const { return mParams; }
    //! returns the function parameters as a string separated by 'separator'
    std::string getParamsNames( const std::string &separator = ", " ) { std::string names; for( auto n : mParams ) names += n + separator; return names.substr( 0, names.length() - separator.length() ); }
    //! returns the function parameters types
    const std::vector<std::string>& getParamsTypes() const { return mParamsTypes; }
    //! returns the function parameters types as a string separated by 'separator'
    std::string getParamsTypesNames( const std::string &separator = ", " ) { std::string names; for( auto n : mParamsTypes ) names += n + separator; return names.substr( 0, names.length() - separator.length() ); }
    //! returns the object kind
    std::string getKind() const { return "Function"; }
    
    
protected:
    std::string                 mReturnType;
    std::vector<std::string>    mParams;
    std::vector<std::string>    mParamsTypes;
};

class Enum : public Object {
public:
    //! creates and returns an empty named enum
    Enum( const std::string &name ) : Object( name ) {}
    
    //! adds an enum value
    void addValue( std::string name ) { mValues.push_back( name ); }
    
    //! returns the values as strings
    const std::vector<std::string>& getValues() const { return mValues; }
    //! returns the object kind
    std::string getKind() const { return "Enum"; }
    
protected:
    std::vector<std::string> mValues;
};


class Field : public Object {
public:
    //! creates and returns an empty named field
    Field( const std::string &name ) : Object( name ), mIsStatic( false ) {}
    
    //! sets whether the field is static
    Field& statical( bool isStatic = true ) { mIsStatic = isStatic; return *this; }
    
    //! returns whether the field is static
    bool isStatic() const { return mIsStatic; }
    //! returns the object kind
    std::string getKind() const { return "Field"; }
    
protected:
    bool mIsStatic;
};

class Method : public Function {
public:
    //! creates and returns an empty named method
    Method( const std::string &name ) : Function( name ), mIsStatic( false ) {}
    
    //! sets whether the method is static
    Method& statical( bool isStatic = true ) { mIsStatic = isStatic; return *this; }
    
    //! returns whether the method is static
    bool isStatic() const { return mIsStatic; }
    //! returns the object kind
    std::string getKind() const { return "Method"; }
    
protected:
    bool mIsStatic;
};

class Class : public Object {
public:
    //! creates and returns an empty named class
    Class( const std::string &name ) : Object( name ) {}
    
    //! adds a field to the class
    void addField( const Field &field ) { mFields.emplace_back( field ); }
    //! adds a method to the class
    void addMethod( const Method &method ) { mMethods.emplace_back( method ); }
    
    //! returns the class fields
    const std::vector<Field>& getFields() const { return mFields; }
    //! returns the class methods
    const std::vector<Method>& getMethods() const { return mMethods; }
    //! returns the object kind
    std::string getKind() const { return "Class"; }
    
protected:
    std::vector<Field>  mFields;
    std::vector<Method> mMethods;
};



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
        Options& supportedOperators( const std::map<std::string,std::string>& operators ){ mSupportedOperators = operators; return *this; }
        
        std::string getOutputDirectory() const { return mOutputDirectory; }
        std::string getInputDirectory() const { return mInputDirectory; }
        std::string getLicense() const { return mLicense; }
        
        const std::vector<std::string>& getInputFileList() const { return mInputFileList; }
        const std::vector<std::string>& getExcludeFileList() const { return mExcludeFileList; }
        const std::vector<std::string>& getExcludeDirectoryList() const { return mExcludeDirectoryList; }
        const std::vector<std::string>& getCompilerFlags() const { return mCompilerFlags; }
        const std::vector<std::string>& getUnsupportedTypes() const { return mUnsupportedTypes; }
        const std::map<std::string,std::string>& getSupportedOperators() const { return mSupportedOperators; }
        
    protected:
        std::string                 mOutputDirectory;
        std::string                 mInputDirectory;
        std::string                 mLicense;
        
        std::vector<std::string>    mInputFileList;
        std::vector<std::string>    mExcludeFileList;
        std::vector<std::string>    mExcludeDirectoryList;
        std::vector<std::string>    mCompilerFlags;
        std::vector<std::string>    mUnsupportedTypes;
        
        std::map<std::string,std::string> mSupportedOperators;
    };
    
    Parser( Options options = Options() );
    
protected:
    struct Output {
        Output() : mIsInNamespace(false) {}
        
        std::stringstream   mClassDecl;
        std::stringstream   mClassDef;
        std::stringstream   mClassExtras;
        std::stringstream   mClassFieldDecl;
        std::stringstream   mClassFieldDef;
        std::stringstream   mClassMethodDecl;
        std::stringstream   mClassMethodDef;
        
        std::stringstream   mTemplatesDecl;
        std::stringstream   mTemplatesDef;
        std::stringstream   mTemplatesSpec;
        std::stringstream   mTemplateDeclCalls;
        
        std::stringstream   mEnumsDecl;
        std::stringstream   mEnumsExtras;
        std::stringstream   mFunctionDef;
        
        std::stringstream   mDeclCalls;
        std::stringstream   mDefCalls;
        
        bool    mIsInNamespace;
        
        std::string mCurrentEnumScope;
        std::string mCurrentFunctionScope;
        
        std::vector<std::string> mClassesNames;
        std::vector<std::string> mClassesWithFields;
        std::vector<std::string> mClassesWithMethods;
        
        std::vector<ClassRef>       mClasses;
        std::vector<EnumRef>        mEnums;
        std::vector<FunctionRef>    mFunctions;
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
        std::string getFullScope( clang::DeclContext* declarationContext, const std::string& currentScope = "" );
        //! returns the full scope from a QualType
        std::string getFullScope( const clang::QualType& type, const std::string& currentScope = "" );
        //! returns the class scope from a DeclContext
        std::string getClassScope( clang::DeclContext* declarationContext, const std::string& currentScope = "" );
        //! returns the class scope from a QualType
        std::string getClassScope( const clang::QualType& type, const std::string& currentScope = "" );
        
        //! returns the name of a Declaration
        std::string getDeclarationName( clang::NamedDecl* declaration );
        //! returns the name of a Declaration
        std::string getDeclarationName( const clang::NamedDecl* declaration );
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
        //! returns the unique name of a declaration
        std::string getMangleName( clang::NamedDecl *declaration );
        
        //! replaces namespaces by aliases and returns the corrected string
        std::string replaceNamespacesByAliases( const std::string &declaration );
        //! makes each word first char upper case, removes the :: and returns the styled string
        std::string styleScopedName( const std::string &declaration );
        //! returns quoted string
        std::string quote( const std::string &declaration );
        
        
        clang::DeclContext* getTypeDeclContext( const clang::QualType& type );
        
        //! returns wether a string contains unsupported types
        bool isSupported( const std::string& expr );
        
        std::vector<std::string> visitedRecords;
        
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


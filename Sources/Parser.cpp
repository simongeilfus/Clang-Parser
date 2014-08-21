#include "Parser.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <streambuf>
#include <algorithm>

#include <boost/filesystem.hpp>

using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;
using namespace std;

namespace fs = boost::filesystem;

Parser::Parser( Options options )
: mOptions( options )
{
    const vector<string>& inputs = mOptions.getInputFileList();
    for( int i = 0; i < inputs.size(); i++ ){
        break;
        std::string filename = inputs[i];
        std::ifstream file( ( mOptions.getInputDirectory() + inputs[i] ).c_str() );
        std::string code((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
        
        runToolOnCodeWithArgs( new FrontendAction, code, mOptions.getCompilerFlags(), inputs[i] );
    }
    
    const vector<string>& excludeDirectories = mOptions.getExcludeDirectoryList();
    fs::path main = mOptions.getInputDirectory();
    fs::recursive_directory_iterator dir( main ), end;
    while ( dir != end )
    {
        const fs::path& current = dir->path();
        if( current.filename() == ".DS_Store" ||
           find( excludeDirectories.begin(), excludeDirectories.end(), current.string() ) != excludeDirectories.end() ){
            dir.no_push();
            ++dir;
            continue;
        }
        
        cout << current << endl;
        
        if( !fs::is_directory( current ) && current.extension() == ".h" ){
            std::string filename = current.filename().string();
            std::ifstream file( ( current.string() ).c_str() );
            std::string code((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());
            
            runToolOnCodeWithArgs( new FrontendAction, code, mOptions.getCompilerFlags(), filename );
        }
        
        ++dir;
    }
    
}


//! visits exceptions
bool Parser::Visitor::VisitCXXThrowExpr(clang::CXXThrowExpr *declaration)
{
    return true;
}
//! visits enumerators
bool Parser::Visitor::VisitEnumDecl(clang::EnumDecl *declaration)
{
    if( isInMainFile( declaration ) &&
       ( declaration->getAccess() == AS_public || declaration->getAccess() == AS_none ) ){
        
        cout << "\tEnum:" << declaration->getNameAsString() << endl;
        for( EnumDecl::enumerator_iterator it = declaration->enumerator_begin(), endIt = declaration->enumerator_end(); it != endIt; ++it ){
            EnumConstantDecl* enumDecl = *it;
            cout << "\t\t" << enumDecl->getNameAsString() << endl;
        }
        
    }
    return true;
}
//! visits namespace aliases
bool Parser::Visitor::VisitNamespaceAliasDecl(clang::NamespaceAliasDecl *declaration)
{
    mNamespaceAliases.insert( make_pair( declaration->getNamespace()->getNameAsString(), declaration ) );
    return true;
}
//! visits namespaces
bool Parser::Visitor::VisitNamespaceDecl(clang::NamespaceDecl *declaration)
{
    if( isInMainFile( declaration ) ){
        cout << declaration->getNameAsString();
        
        if( mNamespaceAliases.count( declaration->getNameAsString() ) > 0 ){
            cout << " ( Alias: " << mNamespaceAliases[declaration->getNameAsString()]->getNameAsString() << " )";
        }
        cout << endl;
    }
    return true;
}
//! visits typedefs
bool Parser::Visitor::VisitTypedefDecl(clang::TypedefDecl *declaration)
{
    if( isInMainFile( declaration ) &&
       ( declaration->getAccess() == AS_public || declaration->getAccess() == AS_none ) ){
        
        std::cout << "\tTypedef: " << declaration->getTypeSourceInfo()->getType().getAsString() << " : " << declaration->getNameAsString() << std::endl;
    }
    return true;
}
//! visits c++ classes
bool Parser::Visitor::VisitCXXRecordDecl(clang::CXXRecordDecl *declaration)
{
    if( declaration->getQualifiedNameAsString() == "std::exception" ){
        mExceptionDecl = declaration;
    }
    
    if( isInMainFile( declaration ) &&
       declaration->isCompleteDefinition() &&
       ( declaration->getAccess() == AS_public || declaration->getAccess() == AS_none ) ){
        
        //conversion_iterator conversion_begin()
        //param_iterator param_begin()
        //init_iterator       init_begin()
        
        const SourceManager& sm = mContext->getSourceManager();
        /* cout << "!!! VisitCXXRecordDecl :    " << endl;
         std::cout << declaration->getQualifiedNameAsString() << std::endl;*/
        
        clang::LangOptions langOpts;
        langOpts.CPlusPlus = true;
        clang::PrintingPolicy printingPolicy(langOpts);
        printingPolicy.SuppressTagKeyword = true;
        //printingPolicy.SuppressScope = true;
        printingPolicy.Bool = true;
        
        
        if( mExceptionDecl!=NULL && declaration->isDerivedFrom(mExceptionDecl) ){
            //cout << "Is derived from std::exception" << endl;
            return true;
        }
        
        cout << endl << "\t" << declaration->getQualifiedNameAsString( printingPolicy ) << endl;
        
        
        /*for( CXXRecordDecl::base_class_iterator it = declaration->bases_begin(),
         endIt = declaration->bases_end(); it != endIt; ++it ){
         CXXBaseSpecifier* base = it;
         
         cout << "\t Base Class: " << base->getType().getAsString() << endl;
         }
         
         for( CXXRecordDecl::base_class_iterator it = declaration->vbases_begin(),
         endIt = declaration->vbases_end(); it != endIt; ++it ){
         CXXBaseSpecifier* base = it;
         cout << "\t Virtual Base Class: " << base->getType().getAsString() << endl;
         }*/
        
        bool hasPublicFields = false;
        bool hasPublicConstructors = false;
        bool hasPublicDestructor = false;
        bool hasPublicMethods = false;
        
        if( !declaration->field_empty() ){
            for( CXXRecordDecl::field_iterator it = declaration->field_begin(),
                endIt = declaration->field_end(); it != endIt; ++it ){
                FieldDecl* field = *it;
                if( field->getAccess() == AS_public ){
                    if( !hasPublicFields ){
                        hasPublicFields = true;
                        cout << "\t\t" << "Public Fields: " << endl;
                    }
                    cout << "\t\t\t" << declToString( field ) << endl;
                }
            }
        }
        
        if( hasPublicFields ) cout << endl;
        
        std::vector<Decl*> constructors;
        for( CXXRecordDecl::ctor_iterator it = declaration->ctor_begin(),
            endIt = declaration->ctor_end(); it != endIt; ++it ){
            CXXConstructorDecl* constructor = *it;
            if( constructor->getAccess() == AS_public && !constructor->isImplicit() ){
                if( !hasPublicConstructors ){
                    hasPublicConstructors = true;
                    cout << "\t\t" << "Public Constructors: " << endl;
                }
                
                std::string r = constructor->getResultType().getAsString( printingPolicy );
                std::string name = constructor->getNameAsString();
                
                int numParams = constructor->getNumParams();
                std::string params;
                for( int i = 0; i < numParams; i++ ){
                    ParmVarDecl* p = constructor->getParamDecl( i );
                    Expr* defaultArg = p->getDefaultArg();
                    
                    std::string defaultArgString;
                    if( defaultArg ){
                        defaultArgString = declToString( defaultArg );
                    }
                    
                    params += p->getType().getAsString(printingPolicy) + " " + p->getNameAsString() + ( defaultArgString.empty() ? "" : " = " + defaultArgString ) + ( i < numParams - 1 ? ", " : "" );
                }
                std::cout << "\t\t\t" << r << " " << name << "(" << ( numParams > 0 ? " " + params + " " : "" ) << ")" << std::endl;
                
                
                constructors.push_back( static_cast<Decl*>( constructor ) );
            }
        }
        if( hasPublicConstructors ) cout << endl;
        
        if( declaration->getDestructor() != NULL ){
            CXXDestructorDecl* destructor = declaration->getDestructor();
            if( destructor->getAccess() == AS_public && !destructor->isImplicit() ){
                if( !hasPublicDestructor ){
                    hasPublicDestructor = true;
                    cout << "\t\t" << "Public Destructor: " << endl;
                }
                cout << "\t\t\t" << declToString( destructor ) << endl;
                cout << endl;
            }
        }
        
        for( CXXRecordDecl::method_iterator it = declaration->method_begin(),
            endIt = declaration->method_end(); it != endIt; ++it ){
            CXXMethodDecl* method = *it;
            bool isConstructor = find( constructors.begin(), constructors.end(), static_cast<Decl*>(method) ) != constructors.end();
            if( method->getAccess() == AS_public && !method->isImplicit() && !isConstructor){
                if( !hasPublicMethods ){
                    hasPublicMethods = true;
                    cout << "\t\t" << "Public Methods: " << endl;
                }
                //cout << "\t\t" << method->getType().getAsString() << endl;
                //cout << "\t\t\t" << declToString( method ) << endl;
                
                std::string r = method->getResultType().getAsString( printingPolicy );
                std::string name = method->getNameAsString();
                
                int numParams = method->getNumParams();
                std::string params;
                for( int i = 0; i < numParams; i++ ){
                    ParmVarDecl* p = method->getParamDecl( i );
                    Expr* defaultArg = p->getDefaultArg();
                    
                    std::string defaultArgString;
                    if( defaultArg ){
                        defaultArgString = declToString( defaultArg );
                    }
                    
                    params += p->getType().getAsString(printingPolicy) + " " + p->getNameAsString() + ( defaultArgString.empty() ? "" : " = " + defaultArgString ) + ( i < numParams - 1 ? ", " : "" );
                }
                std::cout << "\t\t\t" << r << " " << name << "(" << ( numParams > 0 ? " " + params + " " : "" ) << ")" << std::endl;
            }
        }
        
        cout << endl;
    }
    return true;
}
//! visits functions
bool Parser::Visitor::VisitFunctionDecl(clang::FunctionDecl *declaration)
{
    if( isInMainFile( declaration ) && declaration->getAccess() == AS_none ){
        DeclContext* context = declaration->getParent();
        
        cout << "\t" << declToString( declaration ) << " " << context->getDeclKindName() << endl;
    }
    return true;
}


//! parses each top-level declarations
bool Parser::Consumer::HandleTopLevelDecl(clang::DeclGroupRef group)
{
    for ( DeclGroupRef::iterator it = group.begin(), end = group.end(); it != end; ++it ) {
        // Traverse the declaration using our AST visitor.
        Decl* declaration = *it;
        //if( location.isValid() && sm.isInMainFile( location ) ){
        if( declaration != NULL ){
            mVisitor.TraverseDecl( declaration );
        }
        //}
    }
    
    return true;
}


void Parser::FrontendAction::EndSourceFileAction()
{
    //SourceManager &SM = TheRewriter.getSourceMgr();
    //llvm::errs() << "** EndSourceFileAction for: "
    //<< SM.getFileEntryForID(SM.getMainFileID())->getName() << "\n";
}

    //! returns our writter consumer
clang::ASTConsumer * Parser::FrontendAction::CreateASTConsumer( clang::CompilerInstance &compiler, clang::StringRef file )
{
    llvm::errs() << "** Creating AST consumer for: " << file << "\n";
    return new Consumer( &compiler.getASTContext() );
}

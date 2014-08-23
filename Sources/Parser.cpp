#include "Parser.h"

#include <iostream>
#include <streambuf>
#include <algorithm>
#include <deque>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string/replace.hpp>

using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;
using namespace std;

namespace fs = boost::filesystem;

Parser::Parser( Options options )
: mOptions( options )
{
    // copy input string list to a vector of paths
    vector<fs::path> inputs;
    for( auto p : mOptions.getInputFileList() ){
        inputs.push_back( fs::path( p ) );
    }
    
    fs::path inputDirectory = mOptions.getInputDirectory();
    
    // if there's no inputFileList, recursively walk the inputDirectory
    if( inputs.empty() ){
        const vector<string>& excludeDirectories = mOptions.getExcludeDirectoryList();
        fs::recursive_directory_iterator dir( inputDirectory ), end;
        while ( dir != end )
        {
            const fs::path& current = dir->path();
            
            // skip .DS_Store and excludedDirectories
            if( current.filename() == ".DS_Store" ||
               find( excludeDirectories.begin(), excludeDirectories.end(), current.string() ) != excludeDirectories.end() ){
                dir.no_push();
            }
            // if we find a head, add it to the inputList
            else if( !fs::is_directory( current ) && current.extension() == ".h" ){
                inputs.push_back( current );
            }
            
            ++dir;
        }
    }
    
    // run clang tool on each header
    for( int i = 0; i < inputs.size(); i++ ){
        fs::path path = inputs[i];
        fs::path name = path.filename();
        std::ifstream file( path.c_str() );
        std::string code((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
        
        std::string currentDirName = path.parent_path().string();
        boost::replace_all( currentDirName, mOptions.getInputDirectory(), "" );
        
        // make sure the output directory exists
        if( !fs::exists( mOptions.getOutputDirectory() + "/" + currentDirName ) ){
            fs::create_directory( mOptions.getOutputDirectory() + "/" + currentDirName );
        }
        
        cout << "Parsing " << ( currentDirName.empty() ? "/" : currentDirName + "/" ) + name.string() << endl;
        
        OutputStreams streams;
        streams.mDefs = ofstream( mOptions.getOutputDirectory() + "/" + ( currentDirName.empty() ? "" : currentDirName + "/" ) + name.string() );
        if( !mOptions.getLicense().empty() ){
            streams.mDefs << mOptions.getLicense() << endl;
            streams.mDefs << endl;
        }
        streams.mDefs << "#include \"" << name.string() << "\"" << endl;
        streams.mDefs << endl;
        streams.mDefs << "// Avoid having to inform include path if header is already include before" << endl;
        streams.mDefs << "#ifndef ANGELSCRIPT_H" << endl;
        streams.mDefs << "\t" << "#include <angelscript.h>" << endl;
        streams.mDefs << "#endif" << endl;
        streams.mDefs << endl;
        streams.mDefs << "#include \"RegistrationHelper.h\"" << endl;
        streams.mDefs << "#include \"" << "cinder/" << currentDirName << ( currentDirName.empty() ? "" : "/" ) << name.string() << "\"" << endl;
        streams.mDefs << endl;
        streams.mDefs << "namespace as {" << endl;
        streams.mDefs << endl;
        
        runToolOnCodeWithArgs( new FrontendAction( streams ), code, mOptions.getCompilerFlags(), name.string() );
        
        streams.mDefs << "}" << endl;
        streams.mDefs.close();
        
        //cout << endl << endl << endl << streams.mDefs.str() << endl << endl << endl;
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
        
        //cout << "\tEnum:" << declaration->getNameAsString() << endl;
        for( EnumDecl::enumerator_iterator it = declaration->enumerator_begin(), endIt = declaration->enumerator_end(); it != endIt; ++it ){
            EnumConstantDecl* enumDecl = *it;
            //cout << "\t\t" << enumDecl->getNameAsString() << endl;
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
        string ns = declaration->getNameAsString();
        if( mNamespaceAliases.count( ns ) > 0 ){
            ns = mNamespaceAliases[ns]->getNameAsString();
        }
    }
    return true;
}
//! visits typedefs
bool Parser::Visitor::VisitTypedefDecl(clang::TypedefDecl *declaration)
{
    if( isInMainFile( declaration ) &&
       ( declaration->getAccess() == AS_public || declaration->getAccess() == AS_none ) ){
        
        //std::cout << "\tTypedef: " << declaration->getTypeSourceInfo()->getType().getAsString() << " : " << declaration->getNameAsString() << std::endl;
    }
    return true;
}


std::string Parser::Visitor::getScope( DeclContext* declarationContext ){
    string scope = "";
    DeclContext* context = declarationContext;
    
    deque<string> contextNames;
    while( context != NULL && !context->isTranslationUnit() ){
        if( context->isNamespace() ){
            NamespaceDecl* ns = static_cast<NamespaceDecl*>( context );
            
            string nsName = ns->getNameAsString();
            if( mNamespaceAliases.count( nsName ) > 0 ){
                nsName = mNamespaceAliases[nsName]->getNameAsString();
            }
            contextNames.push_front( nsName );
        }
        else if( context->isRecord() ){
            CXXRecordDecl* rec = static_cast<CXXRecordDecl*>( context );
            contextNames.push_front( rec->getNameAsString() );
        }
        
        context = context->getParent();
    }
    
    for( int i = 0; i < contextNames.size(); i++ ){
        scope += contextNames[i] + ( i + 1 < contextNames.size() ? "::" : "" );
    }
    
    return scope;
}

bool Parser::Visitor::VisitClassTemplateDecl( clang::ClassTemplateDecl *declaration )
{
    if( isInMainFile( declaration ) &&
       ( declaration->getAccess() == AS_public || declaration->getAccess() == AS_none ) ){
      //  cout << "\t" << "T" << declaration->getQualifiedNameAsString() << endl;
    }
    return true;
}


bool Parser::Visitor::VisitTranslationUnitDecl( TranslationUnitDecl* declaration ){
    
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
       !declaration->isAnonymousStructOrUnion() &&
       !declaration->isEmpty() &&
       !declaration->isHidden() &&
       ( declaration->getAccess() == AS_public || declaration->getAccess() == AS_none ) ){
       // cout << "\t" << "R" << declaration->getQualifiedNameAsString() << endl;
      /*  if( declaration->isPOD() ){
            cout << "\t" << declaration->getQualifiedNameAsString() << " isPOD" << endl;
        }
        if( declaration->isCLike() ){
            cout << "\t" << declaration->getQualifiedNameAsString() << " isCLike" << endl;
        }
        if( declaration->isAbstract() ){
            cout << "\t" << declaration->getQualifiedNameAsString() << " isAbstract" << endl;
        }
        if( declaration->isTemplateDecl() ){
            cout << "\t" << declaration->getQualifiedNameAsString() << " isTemplateDecl" << endl;
        }
        if( declaration->isLocalClass() ){
            cout << "\t" << declaration->getQualifiedNameAsString() << " isLocalClass" << endl;
        }
        if( declaration->isDynamicClass() ){
            cout << "\t" << declaration->getQualifiedNameAsString() << " isDynamicClass" << endl;
        }
        if( declaration->isLocalClass() ){
            cout << "\t" << declaration->getQualifiedNameAsString() << " isLocalClass" << endl;
        }
        if( declaration->isInterface() ){
            cout << "\t" << declaration->getQualifiedNameAsString() << " isInterface" << endl;
        }
        if( declaration->isHidden() ){
            cout << "\t" << declaration->getQualifiedNameAsString() << " isHidden" << endl;
        }
        if( declaration->isCanonicalDecl() ){
            cout << "\t" << declaration->getQualifiedNameAsString() << " isCanonicalDecl" << endl;
        }
        if( declaration->getPreviousDecl() != NULL ){
            cout << "\t" << declaration->getQualifiedNameAsString() << " has PreviousDecl" << endl;
        }
        if( declaration->getMostRecentDecl() != NULL ){
            cout << "\t" << declaration->getQualifiedNameAsString() << " has MostRecentDecl" << endl;
            cout << "\t\t" << declaration->getTypeForDecl()->getTypeClassName() << endl;
        }
        if (ClassTemplateDecl *Var = llvm::dyn_cast<ClassTemplateDecl>(declaration) ) {
            cout << "\t" << declaration->getQualifiedNameAsString() << " can be casted to ClassTemplateDecl" << endl;
        }
        if( declaration->getTypeForDecl()->isInterfaceType() ){
            cout << "\t" << declaration->getQualifiedNameAsString() << " isInterfaceType" << endl;
        }
        if( declaration->getTypeForDecl()->isTemplateTypeParmType() ){
            cout << "\t" << declaration->getQualifiedNameAsString() << " isTemplateTypeParmType" << endl;
        }
        if( declaration->isFirstDecl() ){
            cout << "\t" << declaration->getQualifiedNameAsString() << " isFirstDecl" << endl;
        }
        
        if( isa<InjectedClassNameType>( declaration->getTypeForDecl() ) ){
            cout << "\t" << declaration->getQualifiedNameAsString() << " is a Template!" << endl;
            ClassTemplateDecl *temp = llvm::dyn_cast<ClassTemplateDecl>( declaration->getCanonicalDecl() );
            if( temp != NULL ){
                cout << "SDFSD" << endl;
            }
            
        }*/
        
        ClassTemplateDecl *temp2 = declaration->getDescribedClassTemplate();
        if( temp2 != NULL ){
            cout << "\t" << "!Template: ";
            cout << "\t\t" << declaration->getNameAsString() << "<";
            TemplateParameterList* params = temp2->getTemplateParameters();
            for( TemplateParameterList::iterator it = params->begin(), itEnd = params->end(); it != itEnd; ++it ){
                cout << (*it)->getNameAsString() << ( it + 1 != itEnd ? ", " : "" );
            }
            cout << ">" << endl;
        }
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
        
        
        /*if( mExceptionDecl!=NULL && declaration->isDerivedFrom(mExceptionDecl) ){
            //cout << "Is derived from std::exception" << endl;
            return true;
        }*/
        
        
        if( declaration->getDeclContext()->isNamespace() ){
            NamespaceDecl* ns = static_cast<NamespaceDecl*>( declaration->getDeclContext() );
            //mStreams.mDefs << "context: " << ns->getNameAsString() << endl;
        }
        
        
        // get names
        string qualifiedName        = declaration->getQualifiedNameAsString( printingPolicy );
        string shortQualifiedName   = qualifiedName;
        string qualifiedFuncName    = qualifiedName;
        string name                 = declaration->getNameAsString();
        
        // check if there's namespace names to replace by aliases
        for( auto alias : mNamespaceAliases ){
            if( shortQualifiedName.find( alias.first ) != string::npos ){
                boost::replace_all( shortQualifiedName, alias.first, alias.second->getNameAsString() );
            }
        }
        
        // make first char of each word uppercase
        qualifiedFuncName[0] = std::toupper( qualifiedName[0] );
        for( int i = 1; i < qualifiedFuncName.length(); i++ ){
            if( qualifiedFuncName[i-1] == ':' && qualifiedFuncName[i] != ':' ){
                qualifiedFuncName[i] = std::toupper( qualifiedName[i] );
            }
        }
        boost::replace_all( qualifiedFuncName, "::", "" );
        
        
        
       /* mStreams.mDefs << "!!! name : " << name << endl;
        mStreams.mDefs << "!!! qualifiedName : " << qualifiedName << endl;
        mStreams.mDefs << "!!! shortQualifiedName : " << shortQualifiedName << endl;
        mStreams.mDefs << "!!! qualifiedFuncName : " << qualifiedFuncName << endl;*/
        
        // starting type function
        mStreams.mDefs << "\t" << "//! registers " << qualifiedName << " class" << endl;
        mStreams.mDefs << "\t" << "void register" << qualifiedFuncName << "( asIScriptEngine* engine )" << endl;
        mStreams.mDefs << "\t" << "{" << endl;
        mStreams.mDefs << "\t\t" << "int r;" << endl;
        mStreams.mDefs << endl;
        
        // extract scope
        string scope            = getScope( declaration->getDeclContext() );
        if( !scope.empty() ){
            mStreams.mDefs << "\t\t" << "// set the current namespace " << endl;
            mStreams.mDefs << "\t\t" << "r = engine->SetDefaultNamespace(\"" + scope + "\"); assert( r >= 0 );" << endl;
            mStreams.mDefs << endl;
        }
        
        // register the type
        mStreams.mDefs << "\t\t" << "// register the object type " << endl;
        mStreams.mDefs << "\t\t" << "r = engine->RegisterObjectType( \"" << name << "\", sizeof(" << declaration->getQualifiedNameAsString() << "), asOBJ_VALUE | asOBJ_APP_CLASS_CDAK ); assert( r >= 0 );" << endl;
        
        // close the namespace
        if( !scope.empty() ){
            mStreams.mDefs << endl;
            mStreams.mDefs << "\t\t" << "// set back to empty default namespace " << endl;
            mStreams.mDefs << "\t\t" << "r = engine->SetDefaultNamespace(\"\"); assert( r >= 0 );" << endl;
        }
        
        // closing type function
        mStreams.mDefs << "\t" << "}" << endl;
        mStreams.mDefs << endl;
        
        
        
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
                        
                        // starting fields function
                        mStreams.mDefs << "\t" << "//! registers " << qualifiedName << " fields" << endl;
                        mStreams.mDefs << "\t" << "void register" << qualifiedFuncName << "Fields( asIScriptEngine* engine )" << endl;
                        mStreams.mDefs << "\t" << "{" << endl;
                        mStreams.mDefs << "\t\t" << "int r;" << endl;
                        mStreams.mDefs << endl;
                        
                        // set the namespace
                        if( !scope.empty() ){
                            mStreams.mDefs << "\t\t" << "// set the current namespace " << endl;
                            mStreams.mDefs << "\t\t" << "r = engine->SetDefaultNamespace(\"" + scope + "\"); assert( r >= 0 );" << endl;
                            mStreams.mDefs << endl;
                        }
                    }
                    string fieldName = field->getNameAsString();
                    string fieldType = field->getType().getAsString( printingPolicy );
                    string fieldDecl = fieldType + " " + fieldName;
                    
                    // register the field
                    //mStreams.mDefs << "\t\t" << "// register " << name << " " << fieldDecl << endl;
                    mStreams.mDefs << "\t\t" << "r = engine->RegisterObjectProperty( \"" << name << "\", \"" << fieldDecl << "\", asOFFSET( " << qualifiedName <<  ", " << fieldName << " ) ); assert( r >= 0 );" << endl;
                    //mStreams.mDefs << endl;
                }
            }
        }
        
        if( hasPublicFields ) {
            
            // close the namespace
            if( !scope.empty() ){
                mStreams.mDefs << endl;
                mStreams.mDefs << "\t\t" << "// set back to empty default namespace " << endl;
                mStreams.mDefs << "\t\t" << "r = engine->SetDefaultNamespace(\"\"); assert( r >= 0 );" << endl;
            }
            
            // closing type function
            mStreams.mDefs << "\t" << "}" << endl;
            mStreams.mDefs << endl;
        }
        
        std::vector<Decl*> constructors;
        for( CXXRecordDecl::ctor_iterator it = declaration->ctor_begin(),
            endIt = declaration->ctor_end(); it != endIt; ++it ){
            CXXConstructorDecl* constructor = *it;
            if( constructor->getAccess() == AS_public && !constructor->isImplicit() ){
                if( !hasPublicConstructors ){
                    hasPublicConstructors = true;
                    //cout << "\t\t" << "Public Constructors: " << endl;
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
                //std::cout << "\t\t\t" << r << " " << name << "(" << ( numParams > 0 ? " " + params + " " : "" ) << ")" << std::endl;
                
                
                constructors.push_back( static_cast<Decl*>( constructor ) );
            }
        }
        //if( hasPublicConstructors ) cout << endl;
        
        if( declaration->getDestructor() != NULL ){
            CXXDestructorDecl* destructor = declaration->getDestructor();
            if( destructor->getAccess() == AS_public && !destructor->isImplicit() ){
                if( !hasPublicDestructor ){
                    hasPublicDestructor = true;
                    //cout << "\t\t" << "Public Destructor: " << endl;
                }
                //cout << "\t\t\t" << declToString( destructor ) << endl;
                //cout << endl;
            }
        }
        
        for( CXXRecordDecl::method_iterator it = declaration->method_begin(),
            endIt = declaration->method_end(); it != endIt; ++it ){
            CXXMethodDecl* method = *it;
            bool isConstructor = find( constructors.begin(), constructors.end(), static_cast<Decl*>(method) ) != constructors.end();
            if( method->getAccess() == AS_public && !method->isImplicit() && !isConstructor){
                if( !hasPublicMethods ){
                    hasPublicMethods = true;
                    
                    // starting methods function
                    mStreams.mDefs << "\t" << "//! registers " << qualifiedName << " methods" << endl;
                    mStreams.mDefs << "\t" << "void register" << qualifiedFuncName << "Methods( asIScriptEngine* engine )" << endl;
                    mStreams.mDefs << "\t" << "{" << endl;
                    mStreams.mDefs << "\t\t" << "int r;" << endl;
                    mStreams.mDefs << endl;
                    
                    // set the namespace
                    if( !scope.empty() ){
                        mStreams.mDefs << "\t\t" << "// set the current namespace " << endl;
                        mStreams.mDefs << "\t\t" << "r = engine->SetDefaultNamespace(\"" + scope + "\"); assert( r >= 0 );" << endl;
                        mStreams.mDefs << endl;
                    }
                }
                //cout << "\t\t" << method->getType().getAsString() << endl;
                //cout << "\t\t\t" << declToString( method ) << endl;

                std::string returnName          = method->getResultType().getAsString( printingPolicy );
                std::string methodName          = method->getNameAsString();
                std::string methodQualifiedName = method->getNameAsString();
                
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
                
                // register the method
                //mStreams.mDefs << "\t\t" << "// register " << name << " " << returnName << " " << methodName << "(" << ( numParams > 0 ? " " + params + " " : "" ) << ")" << endl;
                
                mStreams.mDefs << "\t\t" << "r = engine->RegisterObjectMethod( \"" << name << "\", \"" << returnName << " " << methodName << "(" << ( numParams > 0 ? " " + params + " " : "" ) << ")" << "\", asMETHODPR( " << name <<  ", " << methodName << ", " << "(" << ( numParams > 0 ? " " + params + " " : "" ) << ")" << ", " << returnName << " ), asCALL_THISCALL ); assert( r >= 0 );" << endl;
                //mStreams.mDefs << endl;
            }
        }
        
        if( hasPublicMethods ) {
            
            // close the namespace
            if( !scope.empty() ){
                mStreams.mDefs << endl;
                mStreams.mDefs << "\t\t" << "// set back to empty default namespace " << endl;
                mStreams.mDefs << "\t\t" << "r = engine->SetDefaultNamespace(\"\"); assert( r >= 0 );" << endl;
            }
            
            // closing methods function
            mStreams.mDefs << "\t" << "}" << endl;
            mStreams.mDefs << endl;
        }
        
        //cout << endl;
    }
    return true;
}
//! visits functions
bool Parser::Visitor::VisitFunctionDecl(clang::FunctionDecl *declaration)
{
    if( isInMainFile( declaration ) && declaration->getAccess() == AS_none ){
        DeclContext* context = declaration->getParent();
        
        //cout << "\t" << declToString( declaration ) << " " << context->getDeclKindName() << endl;
    }
    return true;
}


//! parses each top-level declarations
/*bool Parser::Consumer::HandleTopLevelDecl(clang::DeclGroupRef group)
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
}*/

// this replaces "HandleTopLevelDecl"
// override this to call our ExampleVisitor on the entire source file
void Parser::Consumer::HandleTranslationUnit( ASTContext &context ) {
    /* we can use ASTContext to get the TranslationUnitDecl, which is
     a single Decl that collectively represents the entire source file */
    mVisitor.TraverseDecl( context.getTranslationUnitDecl() );
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
    return new Consumer( &compiler.getASTContext(), mStreams );
}

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
        
        if( !currentDirName.empty() && currentDirName[0] == '/' ){
            currentDirName = currentDirName.substr( 1 );
        }
        
        // make sure the output directory exists
        if( !fs::exists( mOptions.getOutputDirectory() + "/" + currentDirName ) ){
            fs::create_directory( mOptions.getOutputDirectory() + "/" + currentDirName );
        }
        
        cout << "Parsing " << ( currentDirName.empty() ? "/" : currentDirName + "/" ) + name.string() << endl;
        
        Output output;
        
        runToolOnCodeWithArgs( new FrontendAction( output, mOptions ), code, mOptions.getCompilerFlags(), name.string() );
        
        ofstream sourceFile( mOptions.getOutputDirectory() + "/" + ( currentDirName.empty() ? "" : currentDirName + "/" ) + name.stem().string() + ".cpp" );
        ofstream headerFile( mOptions.getOutputDirectory() + "/" + ( currentDirName.empty() ? "" : currentDirName + "/" ) + name.stem().string() + ".h" );
        
        if( !mOptions.getLicense().empty() ){
            sourceFile << mOptions.getLicense() << endl;
            sourceFile << endl;
            headerFile << mOptions.getLicense() << endl;
            headerFile << endl;
        }
        
        // Header Header
        headerFile << "#pragma once" << endl;
        headerFile << endl;
        headerFile << "class asIScriptEngine;" << endl;
        headerFile << endl;
        headerFile << "namespace as {" << endl;
        headerFile << endl;
        
        
        // Source Header
        sourceFile << "#include \"" << name.string() << "\"" << endl;
        sourceFile << endl;
        sourceFile << "// Avoid having to inform include path if header is already include before" << endl;
        sourceFile << "#ifndef ANGELSCRIPT_H" << endl;
        sourceFile << "\t" << "#include <angelscript.h>" << endl;
        sourceFile << "#endif" << endl;
        sourceFile << endl;
        sourceFile << "#include \"RegistrationHelper.h\"" << endl;
        sourceFile << "#include \"" << "cinder" << "/" << currentDirName << ( currentDirName.empty() ? "" : "/" ) << name.string() << "\"" << endl;
        sourceFile << endl;
        
        sourceFile << endl;
        sourceFile << "using namespace std;" << endl;
        sourceFile << "using namespace ci;" << endl;
        sourceFile << endl;
        sourceFile << "namespace as {" << endl;
        sourceFile << endl;
        
        
        // Types
        if( output.mRegisterTypes.tellp() || output.mEnums.tellp() ){
            headerFile << "\t" << "//! registers " << "cinder" << "/" << currentDirName << ( currentDirName.empty() ? "" : "/" ) << name.string() << " Types" << endl;
            headerFile << "\t" << "void register" << name.stem().string() << "Types( asIScriptEngine* engine );" << endl;
            
            sourceFile << "\t" << "//! registers " << name.stem().string() << " Types" << endl;
            sourceFile << "\t" << "void register" << name.stem().string() << "Types( asIScriptEngine* engine )" << endl;
            sourceFile << "\t" << "{" << endl;
            sourceFile << output.mRegisterTypes.str() << endl;
            if( output.mEnums.tellp() ){
                headerFile << "\t" << "//! registers " << "cinder" << "/" << currentDirName << ( currentDirName.empty() ? "" : "/" ) << name.string() << " Enums" << endl;
                headerFile << "\t" << "void register" << name.stem().string() << "Enums( asIScriptEngine* engine );" << endl;
                
                sourceFile << "\t\t" << "register" << name.stem().string() << "Enums( engine );" << endl;
            }
            sourceFile << "\t" << "}" << endl;
            sourceFile << endl;
        }
        
        // Enums
        if( output.mEnums.tellp() ){
            sourceFile << "\t" << "//! registers " << name.stem().string() << " Enums" << endl;
            sourceFile << "\t" << "void register" << name.stem().string() << "Enums( asIScriptEngine* engine )" << endl;
            sourceFile << "\t" << "{" << endl;
            sourceFile << "\t\t" << "int r;" << endl;
            sourceFile << endl;
            sourceFile << output.mEnums.str() << endl;
            sourceFile << endl;
            sourceFile << "\t\t" << "// set back to empty default namespace " << endl;
            sourceFile << "\t\t" << "r = engine->SetDefaultNamespace(\"\"); assert( r >= 0 );" << endl;
            sourceFile << "\t" << "}" << endl;
            sourceFile << endl;
        }
        
        // Implemntations
        if( output.mRegisterImpls.tellp() ){
            headerFile << "\t" << "//! registers " << "cinder" << "/" << currentDirName << ( currentDirName.empty() ? "" : "/" ) << name.string() << " Implementations" << endl;
            headerFile << "\t" << "void register" << name.stem().string() << "Impls( asIScriptEngine* engine );" << endl;
            
            sourceFile << "\t" << "//! registers " << name.stem().string() << " Implementations" << endl;
            sourceFile << "\t" << "void register" << name.stem().string() << "Impls( asIScriptEngine* engine )" << endl;
            sourceFile << "\t" << "{" << endl;
            sourceFile << output.mRegisterImpls.str() << endl;
            if( output.mFunctions.tellp() ){
                sourceFile << "\t\t" << "register" << name.stem().string() << "Functions( engine );" << endl;
            }
            sourceFile << "\t" << "}" << endl;
            sourceFile << endl;
        }
        
        if( output.mRegisterTypes.tellp() || output.mRegisterImpls.tellp() ){
            headerFile << endl;
        }
        
        // Functions
        if( output.mFunctions.tellp() ){
            headerFile << "\t" << "//! registers " << "cinder" << "/" << currentDirName << ( currentDirName.empty() ? "" : "/" ) << name.string() << " functions" << endl;
            headerFile << "\t" << "void register" << name.stem().string() << "Functions( asIScriptEngine* engine );" << endl;
            
            sourceFile << "\t" << "//! registers " << name.stem().string() << " functions" << endl;
            sourceFile << "\t" << "void register" << name.stem().string() << "Functions( asIScriptEngine* engine )" << endl;
            sourceFile << "\t" << "{" << endl;
            sourceFile << "\t\t" << "int r;" << endl;
            sourceFile << endl;
            sourceFile << output.mFunctions.str() << endl;
            
            // close the namespace
            if( !output.mCurrentFunctionScope.empty() ){
                sourceFile << endl;
                sourceFile << "\t\t" << "// set back to empty default namespace " << endl;
                sourceFile << "\t\t" << "r = engine->SetDefaultNamespace(\"\"); assert( r >= 0 );" << endl;
            }
            
            sourceFile << "\t" << "}" << endl;
            sourceFile << endl;
        }
        
        // Classes
        sourceFile << output.mClassImpls.str() << endl;
        
        // Header definitions
        headerFile << output.mDefinitions.str() << endl;
        headerFile << endl;
        headerFile << "}" << endl;
        
        sourceFile << "}" << endl;
        sourceFile.close();
        headerFile.close();
        
        //cout << endl << endl << endl << output.mDefs.str() << endl << endl << endl;
    }
}

//! visits exceptions
bool Parser::Visitor::VisitCXXThrowExpr(clang::CXXThrowExpr *declaration)
{
    if( isInMainFile( declaration ) ){
    //   ( declaration->getAccess() == AS_public || declaration->getAccess() == AS_none ) ){
        
       // cout << "\tExc:" << declToString( declaration ) << " " << endl;
        
    }
    return true;
}
//! visits enumerators
bool Parser::Visitor::VisitEnumDecl(clang::EnumDecl *declaration)
{
    if( isInMainFile( declaration ) &&
       ( declaration->getAccess() == AS_public || declaration->getAccess() == AS_none ) ){
        
        string name      = declaration->getNameAsString();
        string fullScope = getFullScope( declaration, name );
        if( !fullScope.empty() && mOutput.mCurrentEnumScope != fullScope ){
            
            mOutput.mEnums << "\t\t" << "// set the current namespace " << endl;
            mOutput.mEnums << "\t\t" << "r = engine->SetDefaultNamespace(\"" + fullScope + "\"); assert( r >= 0 );" << endl;
            mOutput.mEnums << endl;
            
            mOutput.mCurrentEnumScope = fullScope;
        }
        
        mOutput.mEnums << "\t\t" << "r = engine->RegisterEnum(\"" + name + "\"); assert( r >= 0 );" << endl;
        
        for( EnumDecl::enumerator_iterator it = declaration->enumerator_begin(), endIt = declaration->enumerator_end(); it != endIt; ++it ){
            EnumConstantDecl* enumDecl = *it;
            
            mOutput.mEnums << "\t\t" << "r = engine->RegisterEnumValue(\"" + name + "\", \"" + enumDecl->getNameAsString() + "\", " + ( fullScope.empty() ? "" : fullScope + "::" ) + ( name.empty() ? "" : name + "::" ) +  enumDecl->getNameAsString() + "); assert( r >= 0 );" << endl;
        }
        mOutput.mEnums << endl;
        
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
   // std::cout << "\tTypedef: " << declaration->getTypeSourceInfo()->getType().getAsString() << " : " << declaration->getNameAsString() << std::endl;
    
    if( isInMainFile( declaration ) &&
       ( declaration->getAccess() == AS_public || declaration->getAccess() == AS_none ) ){
        
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
       //!declaration->isAnonymousStructOrUnion() &&
       //!declaration->isEmpty() &&
       //!declaration->isHidden() &&
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
        if( declaration->getPreviousDecl() != nullptr ){
            cout << "\t" << declaration->getQualifiedNameAsString() << " has PreviousDecl" << endl;
        }
        if( declaration->getMostRecentDecl() != nullptr ){
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
            if( temp != nullptr ){
                cout << "SDFSD" << endl;
            }
            
        }*/
        
        ClassTemplateDecl *temp2 = declaration->getDescribedClassTemplate();
        if( temp2 != nullptr ){
            cout << "\t" << "!Template: ";
            return true;
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
        
        //const SourceManager& sm = mContext->getSourceManager();
        /* cout << "!!! VisitCXXRecordDecl :    " << endl;
         std::cout << declaration->getQualifiedNameAsString() << std::endl;*/
        
        clang::LangOptions langOpts;
        langOpts.CPlusPlus = true;
        
        clang::PrintingPolicy printingPolicy(langOpts);
        printingPolicy.SuppressTagKeyword = true;
        printingPolicy.SuppressScope = true;
        printingPolicy.Bool = true;
        printingPolicy.AnonymousTagLocations = false;
        
        clang::PrintingPolicy printingScopedPolicy(langOpts);
        printingScopedPolicy.SuppressTagKeyword = true;
        printingScopedPolicy.Bool = true;
        printingScopedPolicy.AnonymousTagLocations = false;
        
        clang::PrintingPolicy printingSuppressedPolicy(langOpts);
        printingSuppressedPolicy.SuppressTagKeyword = true;
        printingSuppressedPolicy.SuppressScope = true;
        printingSuppressedPolicy.SuppressTag = true;
        printingSuppressedPolicy.AnonymousTagLocations = false;/*
        printingSuppressedPolicy.SuppressUnwrittenScope = true;
        printingSuppressedPolicy.TerseOutput = true;
        printingSuppressedPolicy.PolishForDeclaration = true;*/
        printingSuppressedPolicy.Bool = true;
        
        
        /*if( mExceptionDecl!=nullptr && declaration->isDerivedFrom(mExceptionDecl) ){
            //cout << "Is derived from std::exception" << endl;
            return true;
        }*/
        
        
        
        
        if( declaration->getDeclContext()->isNamespace() ){
            NamespaceDecl* ns = static_cast<NamespaceDecl*>( declaration->getDeclContext() );
            //mOutput.mClassImpls << "context: " << ns->getNameAsString() << endl;
        }
        
        
        // get names
        string qualifiedName        = declaration->getQualifiedNameAsString( printingScopedPolicy );
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
        
        
        
       /* mOutput.mClassImpls << "!!! name : " << name << endl;
        mOutput.mClassImpls << "!!! qualifiedName : " << qualifiedName << endl;
        mOutput.mClassImpls << "!!! shortQualifiedName : " << shortQualifiedName << endl;
        mOutput.mClassImpls << "!!! qualifiedFuncName : " << qualifiedFuncName << endl;*/
        
        string scope = getFullScope( declaration->getDeclContext(), "" );
        
        if( !declaration->isEmpty() ){
            
            mOutput.mRegisterTypes << "\t\t" << "register" << qualifiedFuncName << "Type( engine );" << endl;
            mOutput.mDefinitions << "\t" << "//! registers " << qualifiedName << " class" << endl;
            mOutput.mDefinitions << "\t" << "void register" << qualifiedFuncName << "Type( asIScriptEngine* engine );" << endl;
            
            // starting type function
            mOutput.mClassImpls << "\t" << "//! registers " << qualifiedName << " class" << endl;
            mOutput.mClassImpls << "\t" << "void register" << qualifiedFuncName << "Type( asIScriptEngine* engine )" << endl;
            mOutput.mClassImpls << "\t" << "{" << endl;
            mOutput.mClassImpls << "\t\t" << "int r;" << endl;
            mOutput.mClassImpls << endl;
            
            // extract scope
            if( !scope.empty() ){
                mOutput.mClassImpls << "\t\t" << "// set the current namespace " << endl;
                mOutput.mClassImpls << "\t\t" << "r = engine->SetDefaultNamespace(\"" + scope + "\"); assert( r >= 0 );" << endl;
                mOutput.mClassImpls << endl;
            }
            
            // register the type
            mOutput.mClassImpls << "\t\t" << "// register the object type " << endl;
            mOutput.mClassImpls << "\t\t" << "r = engine->RegisterObjectType( \"" << name << "\", sizeof(" << declaration->getQualifiedNameAsString() << "), asOBJ_VALUE | asOBJ_APP_CLASS_CDAK ); assert( r >= 0 );" << endl;
            
            // close the namespace
            if( !scope.empty() ){
                mOutput.mClassImpls << endl;
                mOutput.mClassImpls << "\t\t" << "// set back to empty default namespace " << endl;
                mOutput.mClassImpls << "\t\t" << "r = engine->SetDefaultNamespace(\"\"); assert( r >= 0 );" << endl;
            }
            
            // closing type function
            mOutput.mClassImpls << "\t" << "}" << endl;
            mOutput.mClassImpls << endl;
        }
                
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
        bool isStaticNamespace = false;
        
        if( !declaration->field_empty() ){
            for( CXXRecordDecl::field_iterator it = declaration->field_begin(),
                endIt = declaration->field_end(); it != endIt; ++it ){
                FieldDecl* field = *it;
                
                // skip private and implicit fields
                if( field->getAccess() == AS_public && !field->isImplicit() ){
                    if( !hasPublicFields ){
                        hasPublicFields = true;
                        
                        
                        mOutput.mRegisterImpls << "\t\t" << "register" << qualifiedFuncName << "Fields( engine );" << endl;
                        mOutput.mDefinitions << "\t" << "//! registers " << qualifiedName << " fields" << endl;
                        mOutput.mDefinitions << "\t" << "void register" << qualifiedFuncName << "Fields( asIScriptEngine* engine );" << endl;
                        
                        // starting fields function
                        mOutput.mClassImpls << "\t" << "//! registers " << qualifiedName << " fields" << endl;
                        mOutput.mClassImpls << "\t" << "void register" << qualifiedFuncName << "Fields( asIScriptEngine* engine )" << endl;
                        mOutput.mClassImpls << "\t" << "{" << endl;
                        mOutput.mClassImpls << "\t\t" << "int r;" << endl;
                        mOutput.mClassImpls << endl;
                        
                        // set the namespace
                        if( !scope.empty() ){
                            mOutput.mClassImpls << "\t\t" << "// set the current namespace " << endl;
                            mOutput.mClassImpls << "\t\t" << "r = engine->SetDefaultNamespace(\"" + scope + "\"); assert( r >= 0 );" << endl;
                            mOutput.mClassImpls << endl;
                        }
                    }
                    
                    // get field name and type
                    string fieldName = field->getNameAsString();
                    string fieldType = field->getType().getAsString( printingScopedPolicy );
                    
                    // not sure how to remove the "class " in front of some types
                    boost::replace_all( fieldType, "class ", "" );
                    
                    // get field qualified type
                    string fieldQualifiedType = fieldType;
                    
                    // try to extract type scope/namespace
                    string fullScope    = getFullScope( field->getType(), fieldQualifiedType );
                    string classScope   = getClassScope( field->getType(), fieldType );
                    boost::replace_all( fullScope, "::__1", "" );
                    boost::replace_all( classScope, "::__1", "" );
                    
                    // add the scope if it's not there already
                    if( !classScope.empty() && fieldType.find( classScope ) == string::npos ){
                        if( fieldType.find( "const " ) != string::npos ){
                            fieldType =  "const " + classScope + "::" + fieldType.substr( 6 );
                        }
                        else {
                            fieldType =  classScope + "::" + fieldType;
                        }
                    }
                    
                    // add the scope if it's not there already
                    if( !fullScope.empty() && fieldQualifiedType.find( fullScope ) == string::npos ){
                        if( fieldQualifiedType.find( "const " ) != string::npos ){
                            fieldQualifiedType =  "const " + fullScope + "::" + fieldQualifiedType.substr( 6 );
                        }
                        else {
                            fieldQualifiedType =  fullScope + "::" + fieldQualifiedType;
                        }
                    }
                    
                    string fieldDecl = fieldType + " " + fieldName;
                    
                    // register the field
                    //mOutput.mClassImpls << "\t\t" << "// register " << name << " " << fieldDecl << endl;
                    if( !isSupported( fieldDecl ) ){
                        mOutput.mClassImpls << "//";
                    }
                    mOutput.mClassImpls << "\t\t" << "r = engine->RegisterObjectProperty( \"" << name << "\", \"" << fieldDecl << "\", asOFFSET( " << qualifiedName <<  ", " << fieldName << " ) ); assert( r >= 0 );" << endl;
                    //mOutput.mClassImpls << endl;
                }
            }
        }
        
        // if we have public fields we should close the field function
        if( hasPublicFields ) {
            
            // close the namespace
            if( !scope.empty() ){
                mOutput.mClassImpls << endl;
                mOutput.mClassImpls << "\t\t" << "// set back to empty default namespace " << endl;
                mOutput.mClassImpls << "\t\t" << "r = engine->SetDefaultNamespace(\"\"); assert( r >= 0 );" << endl;
            }
            
            // closing type function
            mOutput.mClassImpls << "\t" << "}" << endl;
            mOutput.mClassImpls << endl;
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
                
                std::string r = constructor->getResultType().getAsString( printingScopedPolicy );
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
        
        if( declaration->getDestructor() != nullptr ){
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
            
            if ( llvm::isa<clang::CXXConstructorDecl>( method ) ) {
                cout << "CXXConstructorDecl " << endl;
            }
            else if ( llvm::isa<clang::CXXDestructorDecl>( method ) ) {
                cout << "CXXDestructorDecl " << endl;
            }
            
            // skip private and implicit methods
            if( method->getAccess() == AS_public && !method->isImplicit() && !isConstructor ){
                if( !hasPublicMethods ){
                    hasPublicMethods = true;
                    
                    mOutput.mRegisterImpls << "\t\t" << "register" << qualifiedFuncName << "Methods( engine );" << endl;
                    mOutput.mDefinitions << "\t" << "//! registers " << qualifiedName << " methods" << endl;
                    mOutput.mDefinitions << "\t" << "void register" << qualifiedFuncName << "Methods( asIScriptEngine* engine );" << endl;
                    
                    // starting methods function
                    mOutput.mClassImpls << "\t" << "//! registers " << qualifiedName << " methods" << endl;
                    mOutput.mClassImpls << "\t" << "void register" << qualifiedFuncName << "Methods( asIScriptEngine* engine )" << endl;
                    mOutput.mClassImpls << "\t" << "{" << endl;
                    mOutput.mClassImpls << "\t\t" << "int r;" << endl;
                    mOutput.mClassImpls << endl;
                    
                    // set the namespace
                    if( !scope.empty() ){
                        mOutput.mClassImpls << "\t\t" << "// set the current namespace " << endl;
                        mOutput.mClassImpls << "\t\t" << "r = engine->SetDefaultNamespace(\"" + scope + "\"); assert( r >= 0 );" << endl;
                        mOutput.mClassImpls << endl;
                    }
                }
                
                
                
                // extract function params
                string params               = getFunctionArgList( method );
                string paramsTypes          = getFunctionArgTypeList( method );
                
                // finalize params strings
                params                      = "(" + ( method->getNumParams() > 0 ? " " + params + " " : "" ) + ")";
                paramsTypes                 = "(" + paramsTypes + ")";
                
                // get return qualified type and function name
                string returnQualifiedType  = getFunctionQualifiedReturnType( method );
                string methodName           = getDeclarationName( method );
                
                // register the method
                // if method is not static declare it as ObjectMethod
                if( !method->isStatic() ){
                    if( isStaticNamespace ){
                        mOutput.mClassImpls << endl;
                        mOutput.mClassImpls << "\t\t" << "// set back the current namespace " << endl;
                        mOutput.mClassImpls << "\t\t" << "r = engine->SetDefaultNamespace(\"" + scope + "\"); assert( r >= 0 );" << endl;
                        mOutput.mClassImpls << endl;
                        
                        isStaticNamespace = false;
                    }
                    
                    if( !isSupported( returnQualifiedType + methodName + params + paramsTypes + returnQualifiedType ) ){
                        mOutput.mClassImpls << "//";
                    }
                    mOutput.mClassImpls << "\t\t" << "r = engine->RegisterObjectMethod( \"" << name << "\", \"" << returnQualifiedType << " " << methodName << params << "\", asMETHODPR( " << qualifiedName <<  ", " << methodName << ", " << paramsTypes << ", " << returnQualifiedType << " ), asCALL_THISCALL ); assert( r >= 0 );" << endl;
                }
                // else declare it as GlobalFunction with a namespace
                else {
                    if( !isStaticNamespace ){
                        mOutput.mClassImpls << endl;
                        mOutput.mClassImpls << "\t\t" << "// set static namespace " << endl;
                        mOutput.mClassImpls << "\t\t" << "r = engine->SetDefaultNamespace(\"" + ( !scope.empty() ? scope + "::" : "" ) + name + "\"); assert( r >= 0 );" << endl;
                        mOutput.mClassImpls << endl;
                        
                        isStaticNamespace = true;
                    }
                    if( !isSupported( returnQualifiedType + methodName + params + paramsTypes + returnQualifiedType ) ){
                        mOutput.mClassImpls << "//";
                    }
                    mOutput.mClassImpls << "\t\t" << "r = engine->RegisterGlobalFunction( \"" << returnQualifiedType << " " << methodName << params << "\", asFUNCTIONPR( " << qualifiedName <<  "::" << methodName << ", " << paramsTypes << ", " << returnQualifiedType << " ), asCALL_CDECL ); assert( r >= 0 );" << endl;
                }
            }
        }
        
        // if we have public methods we should close the method function
        if( hasPublicMethods ) {
            
            // close the namespace
            if( !scope.empty() || isStaticNamespace ){
                mOutput.mClassImpls << endl;
                mOutput.mClassImpls << "\t\t" << "// set back to empty default namespace " << endl;
                mOutput.mClassImpls << "\t\t" << "r = engine->SetDefaultNamespace(\"\"); assert( r >= 0 );" << endl;
            }
            
            
            // closing methods function
            mOutput.mClassImpls << "\t" << "}" << endl;
            mOutput.mClassImpls << endl;
        }
    }
    return true;
}
//! visits functions
bool Parser::Visitor::VisitFunctionDecl( clang::FunctionDecl *function )
{
    if( isInMainFile( function ) && function->getAccess() == AS_none ){
        
        // extract function params
        string params               = getFunctionArgList( function );
        string paramsTypes          = getFunctionArgTypeList( function );
        
        // finalize params strings
        params                      = "(" + ( function->getNumParams() > 0 ? " " + params + " " : "" ) + ")";
        paramsTypes                 = "(" + paramsTypes + ")";
        
        // get return qualified type and function name
        string returnQualifiedType  = getFunctionQualifiedReturnType( function );
        string functionName         = getDeclarationName( function );
        string scope                = getFullScope( function->getDeclContext(), "" );
        
        // change scope
        if( !scope.empty() && scope != mOutput.mCurrentFunctionScope ){
            mOutput.mFunctions << "\t\t" << "// set the current namespace " << endl;
            mOutput.mFunctions << "\t\t" << "r = engine->SetDefaultNamespace(\"" + scope + "\"); assert( r >= 0 );" << endl;
            mOutput.mFunctions << endl;
            
            mOutput.mCurrentFunctionScope = scope;
        }
        
        if( !isSupported( returnQualifiedType + params + paramsTypes ) ){
            mOutput.mFunctions << "//";
        }
        
        mOutput.mFunctions << "\t\t" << "r = engine->RegisterGlobalFunction( \"" << returnQualifiedType << " " << functionName << params << "\", asFUNCTIONPR( " << ( scope.empty() ? "" : scope + "::"  ) << functionName << ", " << paramsTypes << ", " << returnQualifiedType << " ), asCALL_CDECL ); assert( r >= 0 );" << endl;
        
    }
    return true;
}





std::string Parser::Visitor::getFullScope( DeclContext* declarationContext, const std::string& currentScope ){
    string scope = "";
    DeclContext* context = declarationContext;
    
    deque<string> contextNames;
    while( context != nullptr && !context->isTranslationUnit() ){
        if( context->isNamespace() ){
            NamespaceDecl* ns = static_cast<NamespaceDecl*>( context );
            
            string nsName = ns->getNameAsString();
            if( mNamespaceAliases.count( nsName ) > 0 ){
                nsName = mNamespaceAliases[nsName]->getNameAsString();
                contextNames.push_front( nsName );
                break;
            }
            if( nsName != "__1" )
                contextNames.push_front( nsName );
        }
        else if( context->isRecord() ){
            CXXRecordDecl* rec = static_cast<CXXRecordDecl*>( context );
            contextNames.push_front( rec->getNameAsString() );
        }
        
        context = context->getParent();
    }
    
    for( int i = 0; i < contextNames.size(); i++ ){
        if( currentScope.find( contextNames[i] ) == string::npos ){
            scope += ( i > 0 ? "::" : "" ) + contextNames[i];
        }
    }
    
    return scope;
}


std::string Parser::Visitor::getFullScope( const clang::QualType& type, const std::string& currentScope ){
    string scope = "";
    if( DeclContext* context = getTypeDeclContext( type ) ){
        scope = getFullScope( context, currentScope );
    }
    return scope;
}

std::string Parser::Visitor::getClassScope( DeclContext* declarationContext, const std::string& currentScope ){
    string scope = "";
    DeclContext* context = declarationContext;
    
    deque<string> contextNames;
    while( context != nullptr && !context->isTranslationUnit() && !context->isNamespace() ){
        if( context->isRecord() ){
            CXXRecordDecl* rec = static_cast<CXXRecordDecl*>( context );
            contextNames.push_front( rec->getNameAsString() );
        }
        
        context = context->getParent();
    }
    
    for( int i = 0; i < contextNames.size(); i++ ){
        if( currentScope.find( contextNames[i] ) == string::npos ){
            scope += ( i > 0 ? "::" : "" ) + contextNames[i];
        }
    }
    
    return scope;
}

std::string Parser::Visitor::getClassScope( const clang::QualType& type, const std::string& currentScope ){
    string scope = "";
    if( DeclContext* context = getTypeDeclContext( type ) ){
        scope = getClassScope( context, currentScope );
    }
    return scope;
}


//! returns the name of a Declaration
std::string Parser::Visitor::getDeclarationName( clang::NamedDecl* declaration )
{
    return declaration->getNameAsString();
}
//! returns the qualified/scoped name of a Declaration
std::string Parser::Visitor::getDeclarationQualifiedName( clang::NamedDecl* declaration )
{
    clang::LangOptions langOpts;
    langOpts.CPlusPlus = true;
    
    clang::PrintingPolicy printingPolicy(langOpts);
    printingPolicy.SuppressTagKeyword = true;
    printingPolicy.SuppressScope = false;
    printingPolicy.Bool = true;
    printingPolicy.AnonymousTagLocations = false;
    
    return declaration->getQualifiedNameAsString( printingPolicy );
}
//! returns the name of a QualType
std::string Parser::Visitor::getTypeName( const clang::QualType &type )
{
    clang::LangOptions langOpts;
    langOpts.CPlusPlus = true;
    
    clang::PrintingPolicy printingPolicy(langOpts);
    printingPolicy.SuppressTagKeyword = true;
    printingPolicy.SuppressScope = false;
    printingPolicy.Bool = true;
    printingPolicy.AnonymousTagLocations = false;
    
    string name     = type.getAsString( printingPolicy );
    string scope    = getClassScope( type, name );
    
    boost::replace_all( scope, "::__1", "" );
    
    // add the scope if it's not there already
    if( !scope.empty() ){
        if( name.find( "const " ) != string::npos ){
            name =  "const " + scope + "::" + name.substr( 6 );
        }
        else {
            name =  scope + "::" + name;
        }
    }
    
    boost::replace_all( name, "class ", "" );
    
    return name;
}
//! returns the qualified/scoped name of a QualType
std::string Parser::Visitor::getTypeQualifiedName( const clang::QualType &type )
{
    clang::LangOptions langOpts;
    langOpts.CPlusPlus = true;
    
    clang::PrintingPolicy printingPolicy(langOpts);
    printingPolicy.SuppressTagKeyword = true;
    printingPolicy.SuppressScope = false;
    printingPolicy.Bool = true;
    printingPolicy.AnonymousTagLocations = false;
    
    string name     = type.getAsString( printingPolicy );
    string scope    = getFullScope( type, name );
    
    boost::replace_all( scope, "::__1", "" );
    
    // add the scope if it's not there already
    if( !scope.empty() ){
        if( name.find( "const " ) != string::npos ){
            name =  "const " + scope + "::" + name.substr( 6 );
        }
        else {
            name =  scope + "::" + name;
        }
    }
    
    boost::replace_all( name, "class ", "" );
    
    return name;
}
//! returns the list of argument name of a function
std::string Parser::Visitor::getFunctionArgList( clang::FunctionDecl *function )
{
    string params;
    int numParams = function->getNumParams();
    for( int i = 0; i < numParams; i++ ){
        ParmVarDecl* p          = function->getParamDecl( i );
        
        // check if there's a default argument value
        Expr* defaultArgExpr    = p->getDefaultArg();
        string defaultArg;
        if( defaultArgExpr ){
            defaultArg          = declToString( defaultArgExpr );
            // make sure with have an =
            if( defaultArg.find( "=" ) == string::npos ){
                defaultArg = " = " + defaultArg;
            }
        }
        
        // get argument name and type
        string argName          = p->getNameAsString();
        string argQualifiedType = getTypeQualifiedName( p->getType() );
        params                  += argQualifiedType + " " + argName + defaultArg + ( i < numParams - 1 ? ", " : "" );
    }
    
    return params;
}
//! returns the qualified/scoped list of argument type name of a function
std::string Parser::Visitor::getFunctionArgTypeList( clang::FunctionDecl *function )
{
    string paramsTypes;
    int numParams = function->getNumParams();
    for( int i = 0; i < numParams; i++ ){
        ParmVarDecl* p          = function->getParamDecl( i );
        string argQualifiedType = getTypeQualifiedName( p->getType() );
        
        paramsTypes             += argQualifiedType + ( i < numParams - 1 ? ", " : "" );
    }
    
    return paramsTypes;
}
//! returns the qualified/scoped list of argument names of a function
std::string Parser::Visitor::getFunctionArgNameList( clang::FunctionDecl *function )
{
    string names;
    int numParams = function->getNumParams();
    for( int i = 0; i < numParams; i++ ){
        ParmVarDecl* p  = function->getParamDecl( i );
        string argName  = p->getNameAsString();
        names          += argName + ( i < numParams - 1 ? ", " : "" );
    }
    return names;
}
//! returns the return type name of a function
std::string Parser::Visitor::getFunctionReturnType( clang::FunctionDecl *function )
{
    return getTypeName( function->getResultType() );
}
//! returns the qualified/scoped return type name of a function
std::string Parser::Visitor::getFunctionQualifiedReturnType( clang::FunctionDecl *function )
{
    return getTypeQualifiedName( function->getResultType() );
}


clang::DeclContext* Parser::Visitor::getTypeDeclContext( const clang::QualType& type )
{
    DeclContext* context = nullptr;
    if( CXXRecordDecl* decl = type.getTypePtr()->getAsCXXRecordDecl() ){
        context = Decl::castToDeclContext( decl );
    }
    else if( const CXXRecordDecl* decl = type.getTypePtr()->getPointeeCXXRecordDecl() ){
        context = Decl::castToDeclContext( decl );
    }
    else if( const TypedefType* typedefType = type.getTypePtr()->getAs<TypedefType>() ){
        context = Decl::castToDeclContext( typedefType->getDecl() );
    }
    
    if( context != nullptr ){
        if( type.getTypePtr()->getTypeClass() == Type::TypeClass::Typedef ){
            context = context->getParent();
        }
        else if( strcmp( context->getDeclKindName(), "ClassTemplateSpecialization" ) == 0 ){
            context = context->getParent();
        }
    }
    
    return context;
}

bool Parser::Visitor::isSupported( const std::string& expr )
{
    const vector<string> &unsupported = mOptions.getUnsupportedTypes();
    for( vector<string>::const_iterator it = unsupported.begin(); it != unsupported.end(); ++it ){
        if( expr.find( *it ) != string::npos ){
            return false;
        }
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
        if( declaration != nullptr ){
            mVisitor.TraverseDecl( declaration );
        }
        //}
    }
 
    return true;
}*/

// this replaces "HandleTopLevelDecl"
// override this to call our ExampleVisitor on the entire source file
void Parser::Consumer::HandleTranslationUnit( ASTContext &context ) {
    
   // cout << context.getSourceManager().getFileEntryForID( context.getSourceManager().getMainFileID() )->getName() << endl;
   // cout << context.getSourceManager().getFileEntryForID( context.getSourceManager().getMainFileID() )->getDir()->getName() << endl;
   // cout << context.getSourceManager().getFileManager().getCanonicalName( context.getSourceManager().getFileEntryForID( context.getSourceManager().getMainFileID() )->getDir() ).str() << endl;
    
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


void Parser::PreprocessorParser::InclusionDirective( clang::SourceLocation HashLoc,
                                                    const clang::Token &IncludeTok,
                                                    clang::StringRef FileName,
                                                    bool IsAngled,
                                                    clang::CharSourceRange FilenameRange,
                                                    const clang::FileEntry *File,
                                                    clang::StringRef SearchPath,
                                                    clang::StringRef RelativePath,
                                                    const clang::Module *Imported)
{
    if( mContext->getSourceManager().isInMainFile( mContext->getFullLoc( HashLoc ) ) ){
        
       // mOutput.mClassImpls << "#include " << ( IsAngled ? "<" : "\"" ) << RelativePath.str() << ( IsAngled ? ">" : "\"" ) << endl;
    }
}

    //! returns our writter consumer
clang::ASTConsumer * Parser::FrontendAction::CreateASTConsumer( clang::CompilerInstance &compiler, clang::StringRef file )
{
    Preprocessor& pp = compiler.getPreprocessor();
    pp.addPPCallbacks( new PreprocessorParser( &compiler.getASTContext(), mOutput ) );
    return new Consumer( &compiler.getASTContext(), mOutput, mOptions );
}

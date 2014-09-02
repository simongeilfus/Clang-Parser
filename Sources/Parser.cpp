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
    
    stringstream globalDeclCalls;
    stringstream globalDefCalls;
    stringstream globalIncludes;
    
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
        
       // cout << "Parsing " << ( currentDirName.empty() ? "/" : currentDirName + "/" ) + name.string() << endl;
        
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
        globalIncludes << "#include \"" << name.string() << "\"" << endl;
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
        if( output.mDeclCalls.tellp() || output.mEnumsDecl.tellp() ){
            headerFile << "\t" << "//! registers " << "cinder" << "/" << currentDirName << ( currentDirName.empty() ? "" : "/" ) << name.string() << " Forward Declarations" << endl;
            headerFile << "\t" << "void registerCinder" << name.stem().string() << "Declarations( asIScriptEngine* engine );" << endl;
            
            sourceFile << "\t" << "//! registers " << name.stem().string() << " Forward Declarations" << endl;
            sourceFile << "\t" << "void registerCinder" << name.stem().string() << "Declarations( asIScriptEngine* engine )" << endl;
            sourceFile << "\t" << "{" << endl;
            sourceFile << output.mDeclCalls.str() << endl;
            if( output.mEnumsDecl.tellp() ){
                sourceFile << "\t\t" << "registerCinder" << name.stem().string() << "Enums( engine );" << endl;
            }
            sourceFile << "\t" << "}" << endl;
            sourceFile << endl;
            
            globalDeclCalls << "\t" << "as::registerCinder" << name.stem().string() << "Declarations( engine );" << endl;
        }
        
        // Implemntations
        if( output.mDefCalls.tellp() ){
            headerFile << "\t" << "//! registers " << "cinder" << "/" << currentDirName << ( currentDirName.empty() ? "" : "/" ) << name.string() << " Definitions" << endl;
            headerFile << "\t" << "void registerCinder" << name.stem().string() << "Definitions( asIScriptEngine* engine );" << endl;
            
            sourceFile << "\t" << "//! registers " << name.stem().string() << " Definitions" << endl;
            sourceFile << "\t" << "void registerCinder" << name.stem().string() << "Definitions( asIScriptEngine* engine )" << endl;
            sourceFile << "\t" << "{" << endl;
            sourceFile << output.mDefCalls.str() << endl;
            if( output.mFunctionDef.tellp() ){
                sourceFile << "\t\t" << "registerCinder" << name.stem().string() << "Functions( engine );" << endl;
            }
            sourceFile << "\t" << "}" << endl;
            sourceFile << endl;
            
            globalDefCalls << "\t" << "as::registerCinder" << name.stem().string() << "Definitions( engine );" << endl;
        }
        
        if( output.mDeclCalls.tellp() || output.mDefCalls.tellp() ){
            headerFile << endl;
        }
        
        
        // Enums
        if( output.mEnumsDecl.tellp() ){
            
            headerFile << "\t" << "//! registers " << "cinder" << "/" << currentDirName << ( currentDirName.empty() ? "" : "/" ) << name.string() << " Enums" << endl;
            headerFile << "\t" << "void registerCinder" << name.stem().string() << "Enums( asIScriptEngine* engine );" << endl;
            
            
            sourceFile << "\t" << "//! registers " << name.stem().string() << " Enums" << endl;
            if( output.mEnumsExtras.tellp() ) sourceFile << endl << output.mEnumsExtras.str() << endl;
            sourceFile << "\t" << "void registerCinder" << name.stem().string() << "Enums( asIScriptEngine* engine )" << endl;
            sourceFile << "\t" << "{" << endl;
            sourceFile << "\t\t" << "int r;" << endl;
            sourceFile << endl;
            sourceFile << output.mEnumsDecl.str() << endl;
            sourceFile << endl;
            sourceFile << "\t\t" << "// set back to empty default namespace " << endl;
            sourceFile << "\t\t" << "r = engine->SetDefaultNamespace(\"\"); assert( r >= 0 );" << endl;
            sourceFile << "\t" << "}" << endl;
            sourceFile << endl;
        }
        
        // Functions
        if( output.mFunctionDef.tellp() ){
            headerFile << "\t" << "//! registers " << "cinder" << "/" << currentDirName << ( currentDirName.empty() ? "" : "/" ) << name.string() << " functions" << endl;
            headerFile << "\t" << "void registerCinder" << name.stem().string() << "Functions( asIScriptEngine* engine );" << endl;
            
            sourceFile << "\t" << "//! registers " << name.stem().string() << " functions" << endl;
            sourceFile << "\t" << "void registerCinder" << name.stem().string() << "Functions( asIScriptEngine* engine )" << endl;
            sourceFile << "\t" << "{" << endl;
            sourceFile << "\t\t" << "int r;" << endl;
            sourceFile << endl;
            sourceFile << output.mFunctionDef.str() << endl;
            
            // close the namespace
            if( !output.mCurrentFunctionScope.empty() ){
                sourceFile << endl;
                sourceFile << "\t\t" << "// set back to empty default namespace " << endl;
                sourceFile << "\t\t" << "r = engine->SetDefaultNamespace(\"\"); assert( r >= 0 );" << endl;
            }
            
            sourceFile << "\t" << "}" << endl;
            sourceFile << endl;
        }
        
        // Header declaration
        if( output.mClassDecl.tellp() ) headerFile << output.mClassDecl.str() << endl;
        if( output.mClassFieldDecl.tellp() ) headerFile << output.mClassFieldDecl.str() << endl;
        if( output.mClassMethodDecl.tellp() ) headerFile << output.mClassMethodDecl.str() << endl;
        if( output.mTemplatesDecl.tellp() ) headerFile << output.mTemplatesDecl.str() << endl;
        
        // Source Definitions
        if( output.mClassExtras.tellp() ) sourceFile << output.mClassExtras.str() << endl;
        if( output.mClassDef.tellp() ) sourceFile << output.mClassDef.str() << endl;
        if( output.mClassFieldDef.tellp() ) sourceFile << output.mClassFieldDef.str() << endl;
        if( output.mClassMethodDef.tellp() ) sourceFile << output.mClassMethodDef.str() << endl;
        if( output.mTemplatesDef.tellp() ) sourceFile << output.mTemplatesDef.str() << endl;
        if( output.mTemplatesSpec.tellp() ) sourceFile << output.mTemplatesSpec.str() << endl;
        
        headerFile << endl;
        headerFile << "}" << endl;
        
        sourceFile << "}" << endl;
        sourceFile.close();
        headerFile.close();
        
        //cout << endl << endl << endl << output.mDefs.str() << endl << endl << endl;
    }
    
    cout << globalIncludes.str() << endl << endl << globalDeclCalls.str() << endl << endl << globalDefCalls.str() << endl;
}

//! visits exceptions
bool Parser::Visitor::VisitCXXThrowExpr(clang::CXXThrowExpr *declaration)
{
    if( isInMainFile( declaration ) ){
    //   ( declaration->getAccess() == AS_public || declaration->getAccess() == AS_none ) ){
        
       cout << "\tExc:" << declToString( declaration ) << " " << endl;
        
    }
    return true;
}
//! visits enumerators
bool Parser::Visitor::VisitEnumDecl(clang::EnumDecl *declaration)
{
    if( isInMainFile( declaration ) &&
       ( declaration->getAccess() == AS_public || declaration->getAccess() == AS_none ) ){
        
        string name      = declaration->getNameAsString();
        if( name.empty() ){
            if( TypedefNameDecl* typedefNamedDecl = declaration->getTypedefNameForAnonDecl() ){
                 name = typedefNamedDecl->getNameAsString();
            }
        }
        
        string fullScope = getFullScope( declaration, name );
        if( !fullScope.empty() && mOutput.mCurrentEnumScope != fullScope ){
            
            mOutput.mEnumsDecl << "\t\t" << "// set the current namespace " << endl;
            mOutput.mEnumsDecl << "\t\t" << "r = engine->SetDefaultNamespace( " + quote( fullScope ) + " ); assert( r >= 0 );" << endl;
            mOutput.mEnumsDecl << endl;
            
            mOutput.mCurrentEnumScope = fullScope;
        }
        
        if( !name.empty() ){
            mOutput.mEnumsDecl << "\t\t" << "r = engine->RegisterEnum( " + quote( name ) + " ); assert( r >= 0 );" << endl;
        }
        
        for( EnumDecl::enumerator_iterator it = declaration->enumerator_begin(), endIt = declaration->enumerator_end(); it != endIt; ++it ){
            EnumConstantDecl* enumDecl = *it;
            
            if( !name.empty() ){
                mOutput.mEnumsDecl << "\t\t" << "r = engine->RegisterEnumValue( " + quote( name ) + ", " + quote( enumDecl->getNameAsString() ) + ", " + ( fullScope.empty() ? "" : fullScope + "::" ) + ( name.empty() ? "" : name + "::" ) +  enumDecl->getNameAsString() + "); assert( r >= 0 );" << endl;
            }
            else {
                string constName = "AS_CONST_" + enumDecl->getNameAsString();
                mOutput.mEnumsExtras << "\t" << "static int " << constName << " = " << ( fullScope.empty() ? "" : fullScope + "::" ) + enumDecl->getNameAsString() << ";"  << endl;
                mOutput.mEnumsDecl << "\t\t" << "r = engine->RegisterGlobalProperty( " << quote( "const int " + enumDecl->getNameAsString() ) << ", &" << constName << "); assert( r >= 0 );" << endl;
            }
        }
        mOutput.mEnumsDecl << endl;
        
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

// TODO Move somewhere else? As an option?
static map<string, string> typeSuffixes = {
    { "float", "f" },
    { "double", "d" },
    { "int", "i" },
    { "uint8_t", "i" }
};


//! visits typedefs
bool Parser::Visitor::VisitTypedefDecl(clang::TypedefDecl *declaration)
{
    
    if( isInMainFile( declaration ) ){ //&&
       //( declaration->getAccess() == AS_public || declaration->getAccess() == AS_none ) ){
        
        string scope = getFullScope( declaration->getDeclContext() );
        
        QualType type = declaration->getTypeSourceInfo()->getType();
        if( const Type *typePtr = type.getTypePtr() ){
            if( const TemplateSpecializationType *templateType = typePtr->getAs<TemplateSpecializationType>() ){
                // cout << "\t Found a typedef of ";
                
                string templateQualifiedName;
                string templateMangleName;
                if( TemplateDecl *templateDecl = templateType->getTemplateName().getAsTemplateDecl() ){
                    templateQualifiedName   = getDeclarationQualifiedName( templateDecl );
                    templateMangleName      = getMangleName( templateDecl );
                }
                /*if( CXXRecordDecl *recordDecl = templateType->getAsCXXRecordDecl() ){
                    
                    string recordName = styleScopedName( getDeclarationQualifiedName( recordDecl ) );
                    cout << getDeclarationName( declaration ) << " | " << recordName << " : " << declaration->getNameAsString() << endl;
                    //cout << visitedRecords.size() << " " << boolalpha << ( find( visitedRecords.begin(), visitedRecords.end(), getMangleName( declaration ) ) != visitedRecords.end() ) << endl;
                    
                    cout << "Looking for " << getMangleName( declaration ) << " in ";
                    for( auto mangle : visitedRecords ){
                        cout << mangle << ", ";
                    }
                    cout << endl;
                    
                    string registerTypeString = "void register" + recordName +  "Type( asIScriptEngine* engine, const std::string &name )";
                   // cout << "LookingFor: " << registerTypeString << endl << endl;
                   // cout << "In:         " << mOutput.mClassDecl.str() << endl << endl;
                   // cout << mOutput.mClassDecl.tellp() << endl;
                    if( mOutput.mClassDecl.str().find( registerTypeString ) != string::npos ){
                        cout << "SDFSDFSDFSD" << endl;
                    }
                    //mOutput.mDeclCalls << "\t\t" << "register" << classQualifiedStyledName << "Type( engine );" << endl;
                    
                }*/
                
                string templateArgs;
                int numArgs = templateType->getNumArgs();
                for( int i = 0; i < numArgs; i++ ){
                    const TemplateArgument &arg = templateType->getArg( i );
                    if( arg.getKind() == TemplateArgument::ArgKind::Type ){
                        QualType argType            = arg.getAsType();
                        templateArgs += getTypeName( argType ) + ( i + 1 < numArgs ? ", " : "" );
                    }
                }
                
                if( !templateQualifiedName.empty() && !templateArgs.empty() && numArgs == 1 ){
                    
                    if( find( mOutput.mClasses.begin(), mOutput.mClasses.end(), templateMangleName ) != mOutput.mClasses.end() ){
                        mOutput.mDeclCalls << "\t\t" << "register" << styleScopedName( templateQualifiedName ) <<  "Type<" << templateArgs << ">( engine, " << quote( declaration->getNameAsString() ) << " );" << endl;
                        
                        string templateSpecialization = "template void register" + styleScopedName( templateQualifiedName ) +  "Type<" + templateArgs + ">( asIScriptEngine*, const std::string & );";
                        if( mOutput.mTemplatesSpec.str().find( templateSpecialization ) == string::npos ){
                            
                            mOutput.mTemplatesSpec << endl << "\t" << "// " << templateQualifiedName << " template specializations so we can keep our implementation in the cpp file" << endl;
                           // mOutput.mTemplatesSpec << "\t" << "template class " << styleScopedName( templateQualifiedName ) << "Factory<" << templateArgs <<">;" << endl;
                            //mOutput.mTemplatesSpec << "\t" << "template<> std::map<" << templateQualifiedName << "<" << templateArgs << ">*, uint32_t> " << styleScopedName( templateQualifiedName ) << "Factory<" << templateArgs <<">::sRefs;" << endl;

                            //mOutput.mTemplatesSpec << "\t" << "class " << styleScopedName( templateQualifiedName ) << "Factory<" << templateArgs << "> ;" << endl;
                            //mOutput.mTemplatesSpec << "\t" << "public:" << endl;
                            //mOutput.mTemplatesSpec << "\t\t" << "std::map<" << templateQualifiedName << "<" << templateArgs << ">" << "*, uint32_t> sRefs;" << endl;
                            //mOutput.mTemplatesSpec << "\t" << "};" << endl;
                            
                            //mOutput.mTemplatesSpec << "\t" << "template<typename T> std::map<" << templateQualifiedName << "<T>*, uint32_t> " << styleScopedName( templateQualifiedName ) << "Factory<T>::sRefs;" << endl;
                            //mOutput.mTemplatesSpec << endl;
                            mOutput.mTemplatesSpec << "\t" << templateSpecialization << endl;
                        }

                    }
                    if( find( mOutput.mClassesWithFields.begin(), mOutput.mClassesWithFields.end(), templateMangleName ) != mOutput.mClassesWithFields.end() ){
                        mOutput.mDefCalls << "\t\t" << "register" << styleScopedName( templateQualifiedName ) <<  "Fields<" << templateArgs << ">( engine, " << quote( declaration->getNameAsString() ) << ", " << quote( templateArgs ) << " );" << endl;
                        
                        string templateSpecialization = "template void register" + styleScopedName( templateQualifiedName ) +  "Fields<" + templateArgs + ">";
                        if( mOutput.mTemplatesSpec.str().find( templateSpecialization ) == string::npos ){
                            mOutput.mTemplatesSpec << "\t" << templateSpecialization << "( asIScriptEngine*, const std::string &, const std::string & );" << endl;
                        }
                    }
                    if( find( mOutput.mClassesWithMethods.begin(), mOutput.mClassesWithMethods.end(), templateMangleName ) != mOutput.mClassesWithMethods.end() ){
                        mOutput.mDefCalls << "\t\t" << "register" << styleScopedName( templateQualifiedName ) <<  "Methods<" << templateArgs << ">( engine, " << quote( declaration->getNameAsString() ) << ", " << quote( templateArgs ) << ", " << quote( typeSuffixes[ templateArgs ] ) << " );" << endl;
                        
                        string templateSpecialization = "template void register" + styleScopedName( templateQualifiedName ) +  "Methods<" + templateArgs + ">";
                        if( mOutput.mTemplatesSpec.str().find( templateSpecialization ) == string::npos ){
                            mOutput.mTemplatesSpec << "\t" << templateSpecialization <<  "( asIScriptEngine*, const std::string &, const std::string &, const std::string & );" << endl;
                        }
                    }
                }
                
            }
            // else std::cout << "\tFound unknown typedef: " << declToString( declaration ) << std::endl;
            //else std::cout << "\tFound unknown typedef: " << scope << " / " << declaration->getNameAsString() << std::endl;
            //else cout << "\t\t That's something else" << endl;
        }
        
        /*if( const Type *type = declaration->getTypeForDecl() ){
            cout << "Found type" << endl;
            if( CXXRecordDecl *decl = type->getAsCXXRecordDecl() ){
                cout << "Found decl " << getDeclarationName( decl ) << endl;
            }
        }*/
        
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
       !declaration->isAnonymousStructOrUnion() &&
       //!declaration->isEmpty() &&
       //!declaration->isHidden() &&
       ( declaration->getAccess() == AS_public || declaration->getAccess() == AS_none ) ){
        
        // get class names
        string className                    = getDeclarationName( declaration );
        string classQualifiedName           = replaceNamespacesByAliases( getDeclarationQualifiedName( declaration ) );
        string classQualifiedStyledName     = styleScopedName( getDeclarationQualifiedName( declaration ) );
        string classScope                   = getFullScope( declaration->getDeclContext() );
        string templateClassName            = className;
        string templateClassQualifiedName   = classQualifiedName;
        string mangleName                   = getMangleName( declaration );
        
        
        bool isTemplate = false;
        int numTemplateParameters = 0;
        vector<string> templateParameterNames;
        if( ClassTemplateDecl *classTemplateDecl = declaration->getDescribedClassTemplate() ){
            TemplateParameterList* params = classTemplateDecl->getTemplateParameters();
            templateClassQualifiedName  += "<";
            templateClassName           += "<";
            
            for( TemplateParameterList::iterator it = params->begin(), itEnd = params->end(); it != itEnd; ++it ){
                string templateParamName = (*it)->getNameAsString();
                templateParameterNames.push_back( templateParamName );
                templateClassQualifiedName  += templateParamName + ( it + 1 != itEnd ? ", " : "" );
                templateClassName           += templateParamName + ( it + 1 != itEnd ? ", " : "" );
                numTemplateParameters++;
            }
            templateClassQualifiedName  += ">";
            templateClassName           += ">";
            
            if( numTemplateParameters <= 1 ){
                
                /*if( templateClassQualifiedName[templateClassQualifiedName.length()-1] == 'T' ){
                    templateClassQualifiedName = templateClassQualifiedName.substr( 0, templateClassQualifiedName.length()-1 ) + "<T>";
                }*/
                
                isTemplate = true;
            }
            else return true;
        }
        
        stringstream *declStream;
        stringstream *defStream;
        
        // Usually singletons and classes with only static members are defined as empty
        // so we don't need to bind the actual type to use the static members.
        if( !declaration->isEmpty() ){
            
            declStream      = &mOutput.mClassDecl;
            defStream       = &mOutput.mClassDef;
            
            if( isTemplate ){
                declStream  = &mOutput.mTemplatesDecl;
                defStream   = &mOutput.mTemplatesDef;
            }
            
            
            // create object factory
            string qualifiedName = classQualifiedName;
            mOutput.mClassExtras << "\t" << "//! " << qualifiedName << " RefCounting and Object Factories" << endl;
            if( isTemplate ){
                qualifiedName = templateClassQualifiedName;
                mOutput.mClassExtras << "\t" << "template<typename T>" << endl;
            }
            mOutput.mClassExtras << "\t" << "class " << classQualifiedStyledName << "Factory {" << endl;
            mOutput.mClassExtras << "\t" << "public:" << endl;
            
            // starting type function
            if( !isTemplate ){
                (*defStream) << "\t" << "//! registers " << classQualifiedName << " class" << endl;
                (*defStream) << "\t" << "void register" << classQualifiedStyledName << "Type( asIScriptEngine* engine )" << endl;
            }
            else {
                (*defStream) << "\t" << "//! registers " << classQualifiedName << " template" << endl;
                (*defStream) << "\t" << "template<typename T>" << endl;
                (*defStream) << "\t" << "void register" << classQualifiedStyledName << "Type( asIScriptEngine* engine, const std::string &name )" << endl;
            }
            
            (*defStream) << "\t" << "{" << endl;
            (*defStream) << "\t\t" << "int r;" << endl;
            (*defStream) << endl;
            
            // add scope
            if( !classScope.empty() ){
                (*defStream) << "\t\t" << "// set the current namespace " << endl;
                (*defStream) << "\t\t" << "r = engine->SetDefaultNamespace( " + quote( classScope ) + " ); assert( r >= 0 );" << endl;
                (*defStream) << endl;
            }
            
            // register the type
            (*defStream) << "\t\t" << "// register the object type " << endl;
            
            if( !isTemplate ){
                mOutput.mDeclCalls << "\t\t" << "register" << classQualifiedStyledName << "Type( engine );" << endl;
                
                (*declStream) << "\t" << "//! registers " << classQualifiedName << " class" << endl;
                (*declStream) << "\t" << "void register" << classQualifiedStyledName << "Type( asIScriptEngine* engine );" << endl;
                
                //(*defStream) << "\t\t" << "r = engine->RegisterObjectType( " << quote( className ) << ", sizeof(" << declaration->getQualifiedNameAsString() << "), asOBJ_VALUE | asOBJ_APP_CLASS_CDAK ); assert( r >= 0 );" << endl;
                
                (*defStream) << "\t\t" << "r = engine->RegisterObjectType( " << quote( className ) << ", 0, asOBJ_REF ); assert( r >= 0 );" << endl;
                (*defStream) << "\t\t" << "r = engine->RegisterObjectBehaviour( " << quote( className ) << ", asBEHAVE_ADDREF, " << quote( "void f()" ) << ", asFUNCTION( " << classQualifiedStyledName << "Factory::addRef ), asCALL_CDECL_OBJLAST ); assert( r >= 0 );" << endl;
                (*defStream) << "\t\t" << "r = engine->RegisterObjectBehaviour( " << quote( className ) << ", asBEHAVE_RELEASE, " << quote( "void f()" ) << ", asFUNCTION( " << classQualifiedStyledName << "Factory::release ), asCALL_CDECL_OBJLAST ); assert( r >= 0 );" << endl;
            }
            else {
                (*declStream) << "\t" << "//! registers " << classQualifiedName << " template" << endl;
                (*declStream) << "\t" << "template<typename T>" << endl;
                (*declStream) << "\t" << "void register" << classQualifiedStyledName << "Type( asIScriptEngine* engine, const std::string &name );" << endl;
                
                //(*defStream) << "\t\t" << "r = engine->RegisterObjectType( name.c_str(), sizeof(" << templateClassQualifiedName << "), asOBJ_VALUE | asOBJ_APP_CLASS_CDAK ); assert( r >= 0 );" << endl;
                (*defStream) << "\t\t" << "r = engine->RegisterObjectType( name.c_str(), 0, asOBJ_REF ); assert( r >= 0 );" << endl;
                (*defStream) << "\t\t" << "r = engine->RegisterObjectBehaviour( name.c_str(), asBEHAVE_ADDREF, " << quote( "void f()" ) << ", asFUNCTION( " << classQualifiedStyledName << "Factory<T>::addRef ), asCALL_CDECL_OBJLAST ); assert( r >= 0 );" << endl;
                (*defStream) << "\t\t" << "r = engine->RegisterObjectBehaviour( name.c_str(), asBEHAVE_RELEASE, " << quote( "void f()" ) << ", asFUNCTION( " << classQualifiedStyledName << "Factory<T>::release ), asCALL_CDECL_OBJLAST ); assert( r >= 0 );" << endl;
            }
            
            // close the namespace
            if( !classScope.empty() ){
                (*defStream) << endl;
                (*defStream) << "\t\t" << "// set back to empty default namespace " << endl;
                (*defStream) << "\t\t" << "r = engine->SetDefaultNamespace(\"\"); assert( r >= 0 );" << endl;
            }
            
            // closing type function
            (*defStream) << "\t" << "}" << endl;
            (*defStream) << endl;
            
            mOutput.mClasses.push_back( mangleName );
        }
        
        
        bool hasPublicFields = false;
        bool hasPublicConstructors = false;
        bool hasPublicDestructor = false;
        bool hasPublicMethods = false;
        bool isStaticNamespace = false;
        
        // Fields
        
        if( !declaration->field_empty() ){
            for( CXXRecordDecl::field_iterator it = declaration->field_begin(), endIt = declaration->field_end(); it != endIt; ++it ){
                FieldDecl* field = *it;
                
                // skip private and implicit fields
                if( field->getAccess() == AS_public && !field->isImplicit() ){
                    
                    declStream      = &mOutput.mClassFieldDecl;
                    defStream       = &mOutput.mClassFieldDef;
                    
                    if( isTemplate ){
                        declStream  = &mOutput.mTemplatesDecl;
                        defStream   = &mOutput.mTemplatesDef;
                    }
                    
                    if( !hasPublicFields ){
                        hasPublicFields = true;
                        
                        if( !isTemplate ){
                            mOutput.mDefCalls << "\t\t" << "register" << classQualifiedStyledName << "Fields( engine );" << endl;
                            
                            (*declStream) << "\t" << "//! registers " << classQualifiedName << " fields" << endl;
                            (*declStream) << "\t" << "void register" << classQualifiedStyledName << "Fields( asIScriptEngine* engine );" << endl;
                            (*defStream) << "\t" << "//! registers " << classQualifiedName << " fields" << endl;
                            (*defStream) << "\t" << "void register" << classQualifiedStyledName << "Fields( asIScriptEngine* engine )" << endl;
                        }
                        else {
                            (*declStream) << "\t" << "//! registers " << classQualifiedName << " template fields" << endl;
                            (*declStream) << "\t" << "template<typename T>" << endl;
                            (*declStream) << "\t" << "void register" << classQualifiedStyledName << "Fields( asIScriptEngine* engine, const std::string &name, const std::string &type );" << endl;
                            (*defStream) << "\t" << "//! registers " << classQualifiedName << " fields" << endl;
                            (*defStream) << "\t" << "template<typename T>" << endl;
                            (*defStream) << "\t" << "void register" << classQualifiedStyledName << "Fields( asIScriptEngine* engine, const std::string &name, const std::string &type )" << endl;
                        }
                        
                        (*defStream) << "\t" << "{" << endl;
                        (*defStream) << "\t\t" << "int r;" << endl;
                        (*defStream) << endl;
                        
                        // set the namespace
                        if( !classScope.empty() ){
                            (*defStream) << "\t\t" << "// set the current namespace " << endl;
                            (*defStream) << "\t\t" << "r = engine->SetDefaultNamespace( " + quote( classScope ) + " ); assert( r >= 0 );" << endl;
                            (*defStream) << endl;
                        }
                    }
                    
                    // get field name, type and field qualified type
                    string fieldName            = getDeclarationName( field );
                    string fieldType            = getTypeName( field->getType() );
                    string fieldDecl            = fieldType + " " + fieldName;
                    
                    // register the field
                    //mOutput.mClassImpls << "\t\t" << "// register " << name << " " << fieldDecl << endl;
                    if( !isSupported( fieldDecl ) ){
                        (*defStream) << "//";
                    }
                    
                    if( !isTemplate ){
                        (*defStream) << "\t\t" << "r = engine->RegisterObjectProperty( " << quote( className ) << ", " << quote( fieldDecl ) << ", asOFFSET( " << classQualifiedName <<  ", " << fieldName << " ) ); assert( r >= 0 );" << endl;
                    }
                    else {
                        (*defStream) << "\t\t" << "r = engine->RegisterObjectProperty( name.c_str(), ( type + " << quote( " " + fieldName ) << " ).c_str(), asOFFSET( " << templateClassQualifiedName <<  ", " << fieldName << " ) ); assert( r >= 0 );" << endl;
                    }
                    //mOutput.mClassImpls << endl;
                }
            }
        }
        
        // if we have public fields we should close the field function
        if( hasPublicFields ) {
            
            // close the namespace
            if( !classScope.empty() ){
                (*defStream) << endl;
                (*defStream) << "\t\t" << "// set back to empty default namespace " << endl;
                (*defStream) << "\t\t" << "r = engine->SetDefaultNamespace(\"\"); assert( r >= 0 );" << endl;
            }
            
            // closing type function
            (*defStream) << "\t" << "}" << endl;
            (*defStream) << endl;
            
            mOutput.mClassesWithFields.push_back( mangleName );
        }
        
        // Methods
        for( CXXRecordDecl::method_iterator it = declaration->method_begin(), endIt = declaration->method_end(); it != endIt; ++it ){
            CXXMethodDecl* method   = *it;
            bool isConstructor      = llvm::isa<clang::CXXConstructorDecl>( method );
            bool isDestructor       = llvm::isa<clang::CXXDestructorDecl>( method );
            
            // skip private and implicit methods
            if( method->getAccess() == AS_public && !method->isImplicit() ) {//&& !isConstructor && !isDestructor ){
                
                declStream      = &mOutput.mClassMethodDecl;
                defStream       = &mOutput.mClassMethodDef;
                
                if( isTemplate ){
                    declStream  = &mOutput.mTemplatesDecl;
                    defStream   = &mOutput.mTemplatesDef;
                }
                
                
                if( !hasPublicMethods ){
                    hasPublicMethods = true;
                    
                    if( !isTemplate ){
                        mOutput.mDefCalls << "\t\t" << "register" << classQualifiedStyledName << "Methods( engine );" << endl;
                        
                        (*declStream) << "\t" << "//! registers " << classQualifiedName << " methods" << endl;
                        (*declStream) << "\t" << "void register" << classQualifiedStyledName << "Methods( asIScriptEngine* engine );" << endl;
                        (*defStream) << "\t" << "//! registers " << classQualifiedName << " methods" << endl;
                        (*defStream) << "\t" << "void register" << classQualifiedStyledName << "Methods( asIScriptEngine* engine )" << endl;
                    }
                    else {
                        (*declStream) << "\t" << "//! registers " << classQualifiedName << " template methods" << endl;
                        (*declStream) << "\t" << "template<typename T>" << endl;
                        (*declStream) << "\t" << "void register" << classQualifiedStyledName << "Methods( asIScriptEngine* engine, const std::string &name, const std::string &type, const std::string &suffix );" << endl;
                        (*defStream) << "\t" << "//! registers " << classQualifiedName << " template methods" << endl;
                        (*defStream) << "\t" << "template<typename T>" << endl;
                        (*defStream) << "\t" << "void register" << classQualifiedStyledName << "Methods( asIScriptEngine* engine, const std::string &name, const std::string &type, const std::string &suffix )" << endl;
                    }
                    
                    (*defStream) << "\t" << "{" << endl;
                    (*defStream) << "\t\t" << "int r;" << endl;
                    (*defStream) << endl;
                    
                    // set the namespace
                    if( !classScope.empty() ){
                        (*defStream) << "\t\t" << "// set the current namespace " << endl;
                        (*defStream) << "\t\t" << "r = engine->SetDefaultNamespace( " + quote( classScope ) + " ); assert( r >= 0 );" << endl;
                        (*defStream) << endl;
                    }
                }
                
                // extract function params
                string params               = "(" + ( method->getNumParams() > 0 ? " " + getFunctionArgList( method ) + " " : "" ) + ")" + ( method->isConst() ? " const" : "" );
                string paramsTypes          = "(" + getFunctionArgTypeList( method ) + ")" + ( method->isConst() ? " const" : "" );
                
                // get return qualified type and function name
                string returnQualifiedType  = getFunctionQualifiedReturnType( method );
                string methodName           = getDeclarationName( method );
                string methodCXXName        = methodName;
                string asReturnType         = returnQualifiedType;
                
                if( method->getResultType().getTypePtr()->getTypeClass() != Type::TypeClass::Builtin && asReturnType.find( "const" ) == string::npos && asReturnType.find( "&" ) == string::npos ){
                    asReturnType += "@";
                }
                
                // change operators in methodName to supported operators
                if( methodName.find( "operator" ) != string::npos ){
                    map<string,string> operators = mOptions.getSupportedOperators();
                    for( auto op : operators ){
                        string newOp = methodName;
                        boost::replace_all( newOp, op.first, "" );
                        if( newOp.empty() ){
                            boost::replace_all( methodName, op.first, op.second );
                        }
                    }
                }
                
                // register the method
                // if method is not static declare it as ObjectMethod
                if( !method->isStatic() ){
                    if( isStaticNamespace ){
                        (*defStream) << endl;
                        (*defStream) << "\t\t" << "// set back the current namespace " << endl;
                        (*defStream) << "\t\t" << "r = engine->SetDefaultNamespace( " + quote( classScope ) + " ); assert( r >= 0 );" << endl;
                        (*defStream) << endl;
                        
                        isStaticNamespace = false;
                    }
                    
                    // comment if we detect an unsupported type
                    bool isCommented = false;
                    if( !isSupported( returnQualifiedType + methodName + params + paramsTypes + returnQualifiedType ) ){
                        (*defStream) << "//";
                        isCommented = true;
                    }
                    
                    // if we still have "operator" in the method, then it means that it's not supported so comment it
                    if( methodName.find( "operator" ) != string::npos ){
                        (*defStream) << "//";
                        isCommented = true;
                    }
                    
                    // output the method
                    string asMethodDecl = quote( asReturnType + " " + methodName + params );
                    
                    // TODO (Add as options?)
                    boost::replace_all( asMethodDecl, "std::string", "string" );
                    
                        if( !isTemplate ){
                            if( isConstructor ){
                                mOutput.mClassExtras << "\t\t" << "static " << classQualifiedName << "* create" << params << endl;
                                mOutput.mClassExtras << "\t\t" << "{" << endl;
                                mOutput.mClassExtras << "\t\t\t" << classQualifiedName << " *ref = new " << classQualifiedName << "();" << endl;
                                mOutput.mClassExtras << "\t\t\t" << "addRef( ref );" << endl;
                                mOutput.mClassExtras << "\t\t\t" << "return ref;" << endl;
                                mOutput.mClassExtras << "\t\t" << "}" << endl;
                                (*defStream) << "\t\t" << "r = engine->RegisterObjectBehaviour( " << quote( className ) << ", asBEHAVE_FACTORY, " << quote( className + "@ f" + paramsTypes  ) << ", asFUNCTIONPR( " << classQualifiedStyledName << "Factory::create, " << paramsTypes << "," << classQualifiedName << "* ), asCALL_CDECL ); assert( r >= 0 );" << endl;
                            }
                            else if( !isDestructor ){
                                (*defStream) << "\t\t" << "r = engine->RegisterObjectMethod( " << quote( className ) << ", " << asMethodDecl << ", asMETHODPR( " << classQualifiedName <<  ", " << methodCXXName << ", " << paramsTypes << ", " << returnQualifiedType << " ), asCALL_THISCALL ); assert( r >= 0 );" << endl;
                            }
                        }
                        else {
                            string paramsAsTypes = paramsTypes;
                            if( !isCommented ){
                                boost::replace_all( asMethodDecl, templateClassQualifiedName, "\" + name + \"" );
                                boost::replace_all( asMethodDecl, templateClassName, "\" + name + \"" );
                                boost::replace_all( asMethodDecl, classQualifiedName, "\" + name + \"" );
                                boost::replace_all( asMethodDecl, "<T>", "\" + suffix + \"" );
                                boost::replace_all( asMethodDecl, " T", "\" + type + \"" );
                                boost::replace_all( asMethodDecl, "\"\" + ", "" );
                                boost::replace_all( asMethodDecl, " + \"\"", "" );
                                
                                boost::replace_all( paramsAsTypes, templateClassQualifiedName, "\" + name + \"" );
                                boost::replace_all( paramsAsTypes, templateClassName, "\" + name + \"" );
                                boost::replace_all( paramsAsTypes, classQualifiedName, "\" + name + \"" );
                                boost::replace_all( paramsAsTypes, "<T>", "\" + suffix + \"" );
                                boost::replace_all( paramsAsTypes, "T", "\" + type + \"" );
                                boost::replace_all( paramsAsTypes, "\"\" + ", "" );
                                boost::replace_all( paramsAsTypes, " + \"\"", "" );
                            }
                            
                            if( isConstructor ){
                                mOutput.mClassExtras << "\t\t" << "static " << templateClassQualifiedName << "* create" << params << endl;
                                mOutput.mClassExtras << "\t\t" << "{" << endl;
                                mOutput.mClassExtras << "\t\t\t" << templateClassQualifiedName << " *ref = new " << templateClassQualifiedName << "();" << endl;
                                mOutput.mClassExtras << "\t\t\t" << "addRef( ref );" << endl;
                                mOutput.mClassExtras << "\t\t\t" << "return ref;" << endl;
                                mOutput.mClassExtras << "\t\t" << "}" << endl;
                                (*defStream) << "\t\t" << "r = engine->RegisterObjectBehaviour( name.c_str(), asBEHAVE_FACTORY, std::string( name + \"@ f" << paramsAsTypes << "\" ).c_str(), asFUNCTIONPR( " << classQualifiedStyledName << "Factory<T>::create, " << paramsTypes << ", " << templateClassQualifiedName << "* ), asCALL_CDECL ); assert( r >= 0 );" << endl;
                            }
                            else if( !isDestructor ){
                                asMethodDecl = "std::string( " + asMethodDecl + " ).c_str()";
                                (*defStream) << "\t\t" << "r = engine->RegisterObjectMethod( name.c_str(), " << asMethodDecl << ", asMETHODPR( " << templateClassQualifiedName <<  ", " << methodCXXName << ", " << paramsTypes << ", " << returnQualifiedType << " ), asCALL_THISCALL ); assert( r >= 0 );" << endl;
                            }
                        }
                    
                    
                }
                // else declare it as GlobalFunction with a namespace
                else {
                    if( !isStaticNamespace ){
                        (*defStream) << endl;
                        (*defStream) << "\t\t" << "// set static namespace " << endl;
                        if( !isTemplate ){
                            (*defStream) << "\t\t" << "r = engine->SetDefaultNamespace( " + quote( ( !classScope.empty() ? classScope + "::" : "" ) + className ) + "); assert( r >= 0 );" << endl;
                        }
                        else {
                            (*defStream) << "\t\t" << "r = engine->SetDefaultNamespace( std::string(" + quote( ( !classScope.empty() ? classScope + "::" : "" ) ) << " + name  ).c_str() ); assert( r >= 0 );" << endl;
                        }
                        (*defStream) << endl;
                        
                        isStaticNamespace = true;
                    }
                    
                    // comment if we detect an unsupported type
                    if( !isSupported( returnQualifiedType + methodName + params + paramsTypes + returnQualifiedType ) ){
                        (*defStream) << "//";
                    }
                    
                    if( !isTemplate ){
                        (*defStream) << "\t\t" << "r = engine->RegisterGlobalFunction( " << quote( returnQualifiedType + " " + methodName + params ) << ", asFUNCTIONPR( " << templateClassQualifiedName <<  "::" << methodName << ", " << paramsTypes << ", " << returnQualifiedType << " ), asCALL_CDECL ); assert( r >= 0 );" << endl;
                    }
                    else {
                        
                    }
                }
            }
        }
        
        // if we have public methods we should close the method function
        if( hasPublicMethods ) {
            
            // close the namespace
            if( !classScope.empty() || isStaticNamespace ){
                (*defStream) << endl;
                (*defStream) << "\t\t" << "// set back to empty default namespace " << endl;
                (*defStream) << "\t\t" << "r = engine->SetDefaultNamespace(\"\"); assert( r >= 0 );" << endl;
            }
            
            
            // closing methods function
            (*defStream) << "\t" << "}" << endl;
            (*defStream) << endl;
            
            mOutput.mClassesWithMethods.push_back( mangleName );
        }
        
        
        if( !declaration->isEmpty() ){
            
            // finish object factory
            string qualifiedName = classQualifiedName;
            if( isTemplate ){
                qualifiedName = templateClassQualifiedName;
            }
            mOutput.mClassExtras << "\t\t" << "static void addRef( " << qualifiedName << " *ptr )" << endl;
            mOutput.mClassExtras << "\t\t" << "{" << endl;
            mOutput.mClassExtras << "\t\t\t" << "typename std::map<" << qualifiedName << "*,uint32_t>::iterator it = sRefs.find( ptr );" << endl;
            mOutput.mClassExtras << "\t\t\t" << "if( it != sRefs.end() ){" << endl;
            mOutput.mClassExtras << "\t\t\t\t" << "it->second++;" << endl;
            mOutput.mClassExtras << "\t\t\t" << "}" << endl;
            mOutput.mClassExtras << "\t\t\t" << "else {" << endl;
            mOutput.mClassExtras << "\t\t\t\t" << "sRefs.insert( std::make_pair( ptr, 1 ) );" << endl;
            mOutput.mClassExtras << "\t\t\t" << "}" << endl;
            mOutput.mClassExtras << "\t\t" << "}" << endl;
            mOutput.mClassExtras << endl;
            mOutput.mClassExtras << "\t\t" << "static void release( " << qualifiedName << " *ptr )" << endl;
            mOutput.mClassExtras << "\t\t" << "{" << endl;
            mOutput.mClassExtras << "\t\t" << "}" << endl;
            mOutput.mClassExtras << endl;
            mOutput.mClassExtras << "\t" << "protected:" << endl;
            mOutput.mClassExtras << "\t\t" << "static std::map<" << qualifiedName << "*, uint32_t> sRefs;" << endl;
            mOutput.mClassExtras << "\t" << "};" << endl;
            if( !isTemplate ) mOutput.mClassExtras << "\t" << "std::map<" << qualifiedName << "*, uint32_t> " << classQualifiedStyledName << "Factory::sRefs;" << endl;
            else mOutput.mClassExtras << "\t" << "template<typename T> std::map<" << qualifiedName << "*, uint32_t> " << classQualifiedStyledName << "Factory<T>::sRefs;" << endl;
            mOutput.mClassExtras << endl;
            
        }
    }
    return true;
}
//! visits functions
bool Parser::Visitor::VisitFunctionDecl( clang::FunctionDecl *function )
{
    if( isInMainFile( function ) && function->getAccess() == AS_none ){
        
        // extract function params
        string params               = "(" + ( function->getNumParams() > 0 ? " " + getFunctionArgList( function ) + " " : "" ) + ")";
        string paramsTypes          = "(" + getFunctionArgTypeList( function ) + ")";
        
        // get return qualified type and function name
        string returnQualifiedType  = getFunctionQualifiedReturnType( function );
        string functionName         = getDeclarationName( function );
        string scope                = getFullScope( function->getDeclContext() );
        
        // change scope
        if( !scope.empty() && scope != mOutput.mCurrentFunctionScope ){
            mOutput.mFunctionDef << "\t\t" << "// set the current namespace " << endl;
            mOutput.mFunctionDef << "\t\t" << "r = engine->SetDefaultNamespace( " + quote( scope ) + " ); assert( r >= 0 );" << endl;
            mOutput.mFunctionDef << endl;
            
            mOutput.mCurrentFunctionScope = scope;
        }
        
        if( !isSupported( returnQualifiedType + params + paramsTypes ) || function->getTemplatedKind() != FunctionDecl::TemplatedKind::TK_NonTemplate ){
            mOutput.mFunctionDef << "//";
        }
        
        mOutput.mFunctionDef << "\t\t" << "r = engine->RegisterGlobalFunction( " << quote( returnQualifiedType + " " + functionName + params ) << ", asFUNCTIONPR( " << ( scope.empty() ? "" : scope + "::"  ) << functionName << ", " << paramsTypes << ", " << returnQualifiedType << " ), asCALL_CDECL ); assert( r >= 0 );" << endl;
        
    }
    return true;
}


//! returns the full scope from a DeclContext
std::string Parser::Visitor::getFullScope( DeclContext* declarationContext, const std::string& currentScope ){
    string scope = "";
    DeclContext* context = declarationContext;
    
    deque<string> contextNames;
    while( context != nullptr && !context->isTranslationUnit() ){
        if( context->isNamespace() ){
            NamespaceDecl* ns = static_cast<NamespaceDecl*>( context );
            
            // TODO remove! Quick hack to get rid of namespaces
            break;
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
//! returns the full scope from a QualType
std::string Parser::Visitor::getFullScope( const clang::QualType& type, const std::string& currentScope ){
    string scope = "";
    if( DeclContext* context = getTypeDeclContext( type ) ){
        scope = getFullScope( context, currentScope );
    }
    return scope;
}
//! returns the class scope from a DeclContext
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
//! returns the class scope from a QualType
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
//! returns the name of a Declaration
std::string Parser::Visitor::getDeclarationName( const clang::NamedDecl* declaration )
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
//! returns the unique name of a declaration
std::string Parser::Visitor::getMangleName( clang::NamedDecl *declaration ){
    MangleContext* mangle = mContext->createMangleContext();
    string name;
    llvm::raw_string_ostream stringOstream( name );
    mangle->mangleCXXName( declaration, stringOstream );
    return stringOstream.str();
}

//! replaces namespaces by aliases and returns the corrected string
std::string Parser::Visitor::replaceNamespacesByAliases( const std::string &declaration )
{
    string shortName = declaration;
    
    // check if there's namespace names to replace by aliases
    for( auto alias : mNamespaceAliases ){
        if( shortName.find( alias.first ) != string::npos ){
            boost::replace_all( shortName, alias.first, alias.second->getNameAsString() );
        }
    }
    
    return shortName;
}

//! makes each word first char upper case, removes the :: and returns the styled string
std::string Parser::Visitor::styleScopedName( const std::string &declaration )
{
    string styledName = declaration;
    
    // make first char of each word uppercase
    styledName[0] = std::toupper( styledName[0] );
    for( int i = 1; i < styledName.length(); i++ ){
        if( styledName[i-1] == ':' && styledName[i] != ':' ){
            styledName[i] = std::toupper( styledName[i] );
        }
    }
    boost::replace_all( styledName, "::", "" );
    
    return styledName;
}
//! returns quoted string
std::string Parser::Visitor::quote( const std::string &declaration )
{
    return "\"" + declaration + "\"";
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

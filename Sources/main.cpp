#include "Parser.h"

int main(int argc, const char * argv[])
{
    Parser::Options options;
    options.compilerFlags( {
        "-x",
        "c++",
        "-fsyntax-only",
        "-fmessage-length=0",
        "-fdiagnostics-show-note-include-stack",
        "-fmacro-backtrace-limit=0",
        "-std=c++11",
        "-stdlib=libc++",
        "-Wno-trigraphs",
        "-fpascal-strings",
        "-O0",
        "-Wno-missing-field-initializers",
        "-Wno-missing-prototypes",
        "-Wno-non-virtual-dtor",
        "-Wno-overloaded-virtual",
        "-Wno-exit-time-destructors",
        "-Wno-missing-braces",
        "-Wparentheses",
        "-Wswitch",
        "-Wno-unused-function",
        "-Wno-unused-label",
        "-Wno-unused-parameter",
        "-Wunused-variable",
        "-Wunused-value",
        "-Wno-empty-body",
        "-Wno-uninitialized",
        "-Wno-unknown-pragmas",
        "-Wno-shadow",
        "-Wno-four-char-constants",
        "-Wno-conversion",
        "-Wno-constant-conversion",
        "-Wno-int-conversion",
        "-Wno-bool-conversion",
        "-Wno-enum-conversion",
        "-Wno-shorten-64-to-32",
        "-Wno-newline-eof",
        "-Wno-c++11-extensions",
        "-DDEBUG=1",
        "-fasm-blocks",
        "-fstrict-aliasing",
        "-Wdeprecated-declarations",
        "-Winvalid-offsetof",
        "-mmacosx-version-min=10.7",
        "-g",
        "-fvisibility-inlines-hidden",
        "-Wno-sign-conversion",
        "-I/Frameworks/Clang/clang 3.4.2/include",
        "-I/Frameworks/Clang/clang 3.4.2/include/clang",
        "-I/Frameworks/Clang/clang 3.4.2/include/c++/v1",
        "-I/Frameworks/Clang/clang 3.4.2/lib/clang/3.4.2/include",
        "-isysroot/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.8.sdk",
        "-iquote/Frameworks/Cinder/cinder_master/include",
        "-I/Frameworks/Cinder/cinder_master/boost",
        "-w"
    } )
    
    // !!! last char can't be a / !!!
    .inputDirectory( "/Frameworks/Cinder/cinder_master/include/cinder" )
    .outputDirectory( "/Users/simongeilfus/Desktop/RegistrationTest/src" )
    .inputFileList( {
        /*"/Frameworks/Cinder/cinder_master/include/cinder/Arcball.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/Area.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/AxisAlignedBox.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/Camera.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/Clipboard.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/ConvexHull.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/Display.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/Easing.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/Filter.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/Font.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/gl/DisplayList.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/gl/Fbo.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/gl/gl.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/gl/GlslProg.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/gl/Light.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/gl/Material.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/gl/Texture.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/gl/TextureFont.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/gl/Vbo.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/ImageIo.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/Json.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/ObjLoader.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/Rect.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/Vector.h",*/
        "/Frameworks/Cinder/cinder_master/include/cinder/Color.h"//,
        //"/Frameworks/Cinder/cinder_master/include/cinder/Matrix44.h"
    } )
    .excludeDirectoryList( {
        // directories
        "/Frameworks/Cinder/cinder_master/include/cinder/audio/cocoa",
        "/Frameworks/Cinder/cinder_master/include/cinder/audio/msw",
        "/Frameworks/Cinder/cinder_master/include/cinder/audio",
        "/Frameworks/Cinder/cinder_master/include/cinder/cocoa",
        "/Frameworks/Cinder/cinder_master/include/cinder/dx",
        "/Frameworks/Cinder/cinder_master/include/cinder/msw",
        
        // files
        "/Frameworks/Cinder/cinder_master/include/cinder/app/AppBasic.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/app/AppCocoaTouch.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/app/AppCocoaView.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/app/AppImplCocoaBasic.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/app/AppImplCocoaRendererGl.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/app/AppImplCocoaRendererQuartz.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/app/AppImplCocoaScreenSaver.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/app/AppImplCocoaTouchRendererGl.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/app/AppImplCocoaTouchRendererQuartz.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/app/AppImplMsw.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/app/AppImplMswBasic.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/app/AppImplMswRenderer.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/app/AppImplMswRendererDx.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/app/AppImplMswRendererGdi.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/app/AppImplMswRendererGl.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/app/AppImplMswScreenSaver.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/app/AppImplWinRT.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/app/AppImplWinRTBasic.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/app/AppNative.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/app/AppScreenSaver.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/app/CinderView.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/app/CinderViewCocoaTouch.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/app/Renderer.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/app/RendererDx.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/app/WinRTApp.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/CaptureImplAvFoundation.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/CaptureImplCocoaDummy.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/CaptureImplDirectShow.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/CaptureImplQtKit.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/ConcurrentCircularBuffer.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/CurrentFunction.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/Exception.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/Filesystem.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/ImageSourceFileQuartz.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/ImageSourceFileWic.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/ImageSourcePng.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/ImageTargetFileQuartz.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/ImageTargetFileWic.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/MatrixAlgo.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/MatrixStack.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/qtime/QuickTimeUtils.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/UrlImplCocoa.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/UrlImplCurl.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/UrlImplWinInet.h",
        "/Frameworks/Cinder/cinder_master/include/cinder/WinRTUtils.h"
    })
    .license( "/*\n\
Copyright (c) 2014, Simon Geilfus, All rights reserved.\n\
\n\
This code is intended for use with the Cinder C++ library: http://libcinder.org\n\
\n\
Redistribution and use in source and binary forms, with or without modification, are permitted provided that\n\
the following conditions are met:\n\
\n\
* Redistributions of source code must retain the above copyright notice, this list of conditions and\n\
the following disclaimer.\n\
* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and\n\
the following disclaimer in the documentation and/or other materials provided with the distribution.\n\
\n\
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" AND ANY EXPRESS OR IMPLIED\n\
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A\n\
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR\n\
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED\n\
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)\n\
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING\n\
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE\n\
POSSIBILITY OF SUCH DAMAGE.\n\
*/" )
    .unsupportedTypes( {
        "*",
        "~",
        "Ref",
        "std::vector",
        "unspecified_bool_type",
        "std::pair",
        "boost::container",
        "NodeType",
        "std::ostream",
        "type-parameter",
        "typename",
        "DIST",
        "Vec3f"
    })
    .supportedOperators( {
        { "operator++",	"opPostInc" },
        { "operator–",	"opPostDec" },
        { "operator==",	"opEquals" },
        //{ "operator!=",	"opEquals" },
        { "operator<",	"opCmp" },
        { "operator<=",	"opCmp" },
        { "operator>",	"opCmp" },
        { "operator>=",	"opCmp" },
        { "operator=",	"opAssign" },
        { "operator+=",	"opAddAssign" },
        { "operator-=",	"opSubAssign" },
        { "operator*=",	"opMulAssign" },
        { "operator/=",	"opDivAssign" },
        { "operator%=",	"opModAssign" },
        { "operator**=",	"opPowAssign" },
        { "operator&=",	"opAndAssign" },
        { "operator|=",	"opOrAssign" },
        { "operator^=",	"opXorAssign" },
        { "operator<<=",	"opShlAssign" },
        { "operator>>=",	"opShrAssign" },
        { "operator>>>=",	"opUShrAssign" },
        { "operator+",	"opAdd" },
        { "operator-",	"opSub" },
        { "operator*",	"opMul" },
        { "operator/",	"opDiv" },
        { "operator%",	"opMod" },
        { "operator**",	"opPow" },
        { "operator&",	"opAnd" },
        { "operator|",	"opOr" },
        { "operator^",	"opXor" },
        { "operator<<",	"opShl" },
        { "operator>>",	"opShr" },
        { "operator>>>",	"opUShr" }    
    })
    ;
    
    Parser parser( options );
    
    
    return 0;
}


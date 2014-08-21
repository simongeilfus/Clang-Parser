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
        "-iquote/Frameworks/Cinder/cinder_master/include/",
        "-I/Frameworks/Cinder/cinder_master/boost/"
    } )
    .inputDirectory( "/Frameworks/Cinder/cinder_master/include/cinder/" )
    .inputFileList( {
        "Vbo.h"
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
    ;
    
    Parser parser( options );
    
    
    return 0;
}


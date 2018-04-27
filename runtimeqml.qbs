import qbs 1.0

StaticLibrary {
    name: "runtimeqml"
    files: [
        "runtimeqml.cpp",
        "runtimeqml.h",
    ]
    Depends { name: 'cpp' }
    Depends { name: "Qt.core" }
    Depends { name: "Qt.quick" }

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: [product.sourceDirectory]
        cpp.defines: ['QRC_SOURCE_PATH="'+path+'/.."']
    }
}

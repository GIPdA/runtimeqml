import qbs 1.0

StaticLibrary {
    name: "runtimeqml"
    Depends { name: 'cpp' }
    Depends { name: "Qt.core" }
    Depends { name: "Qt.quick" }

    files: [
        "runtimeqml.cpp",
        "runtimeqml.hpp",
    ]

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: [exportingProduct.sourceDirectory]
        cpp.defines: ['QRC_SOURCE_PATH="'+path+'/.."']
    }
}

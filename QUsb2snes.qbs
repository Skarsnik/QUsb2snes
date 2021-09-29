import qbs

Project {
    minimumQbsVersion: "1.7.1"

    QtApplication {
        name : "QUsb2Snes"
        cpp.cxxLanguageVersion: "c++11"
        cpp.includePaths: ["devices/EmuNWAccess-qt", "core", "."]
        consoleApplication: false
        files: [
            "core/aclient.h",
            "core/aclientprovider.h",
            "core/adevice.cpp",
            "core/adevice.h",
            "core/devicefactory.cpp",
            "core/devicefactory.h",
            "core/types.h",
            "core/usb2snes.h",
            "core/websocketclient.cpp",
            "core/websocketclient.h",
            "core/websocketprovider.cpp",
            "core/websocketprovider.h",
            "core/wsserver.cpp",
            "core/wsserver.h",
            "core/wsservercommands.cpp",
            "ui/appui.cpp",
            "ui/appui.h",
            "ui/tempdeviceselector.cpp",
            "ui/tempdeviceselector.h",
            "ui/tempdeviceselector.ui",
            "main.cpp",
            "qusb2snes.rc",
            "ressources.qrc",
            "backward.hpp",
            "devices/deviceerror.cpp",
            "devices/deviceerror.h",
            "devices/EmuNWAccess-qt/emunwaccessclient.cpp",
            "devices/EmuNWAccess-qt/emunwaccessclient.h",
            "devices/emunetworkaccessdevice.cpp",
            "devices/emunetworkaccessdevice.h",
            "devices/emunetworkaccessfactory.cpp",
            "devices/emunetworkaccessfactory.h",
            "devices/retroarchfactory.cpp",
            "devices/retroarchfactory.h",
            "devices/retroarchhost.cpp",
            "devices/retroarchhost.h",
            "devices/sd2snesfactory.cpp",
            "devices/sd2snesfactory.h",
            "devices/snesclassicfactory.cpp",
            "devices/snesclassicfactory.h",
            "devices/luabridge.cpp",
            "devices/luabridge.h",
            "devices/luabridgedevice.cpp",
            "devices/luabridgedevice.h",
            "devices/sd2snesdevice.cpp",
            "devices/sd2snesdevice.h",
            "devices/snesclassic.cpp",
            "devices/snesclassic.h",
            "devices/retroarchdevice.cpp",
            "devices/retroarchdevice.h",
            "utils/ipsparse.cpp",
            "utils/ipsparse.h",
            "utils/rommapping/mapping_hirom.c",
            "utils/rommapping/mapping_lorom.c",
            "utils/rommapping/rommapping.c",
            "utils/rommapping/rominfo.c",
        ]

        Group {
            name: "OSX"
            condition: qbs.targetPlatform.contains("macos")
            files: [
                "osx/appnap.h",
                "osx/appnap.mm"
            ]
        }

        Group {     // Properties for the produced executable
            fileTagsFilter: "application"
            qbs.install: true
        }
        Depends {
            name : "Qt";
            submodules : ["gui", "core", "widgets", "network", "serialport", "websockets"]
        }
        Properties {
            condition: qbs.targetPlatform.contains("macos")
            cpp.dynamicsLibraries: ["/System/Library/Frameworks/Foundation.framework/Versions/Current/Resources/BridgeSupport/Foundation.dylib"]
            cpp.frameworks: ["Foundation"]
        }
    }

    QtApplication {
        condition: qbs.targetPlatform.contains("windows")
        name : "TestQUsb2Snes"
        cpp.cxxLanguageVersion: "c++11"
        files: [
            "testmain.cpp",
            "client/usb2snes.h",
            "client/usb2snes.cpp"
        ]

        Group {
            fileTagsFilter: "application"
            qbs.install: true
        }

        Depends {
            name: "Qt"
            submodules: ["core", "network", "websockets"]
        }
    }

    /*
    Product {
        name : "deploy"
        Depends {
            name : "QUsb2Snes"
        }
        Rule {
            inputsFromDependencies: ["application"]
            prepare : {
                var cmd = new Command("windeploy-qt", ["--no-translations",
                                                       "--no-system-d3d-compiler",
                                                       "--no-opengl",
                                                       "--no-svg",
                                                       "--no-webkit",
                                                       "--no-webkit2",
                                                       inputs[0].filePath])
                cmd.description = "Running windeply-qt"
                return cmd
            }
        }

    }*/
}

import qbs

Project {
    minimumQbsVersion: "1.7.1"

    QtApplication {
        name : "QUsb2Snes"
        cpp.cxxLanguageVersion: "c++11"
        consoleApplication: false
        files: [
            "adevice.cpp",
            "adevice.h",
            "appui.cpp",
            "appui.h",
            "devicefactory.cpp",
            "devicefactory.h",
            "devices/sd2snesfactory.cpp",
            "devices/sd2snesfactory.h",
            "devices/snesclassicfactory.cpp",
            "devices/snesclassicfactory.h",
            "ipsparse.cpp",
            "ipsparse.h",
            "devices/luabridge.cpp",
            "devices/luabridge.h",
            "devices/luabridgedevice.cpp",
            "devices/luabridgedevice.h",
            "main.cpp",
            "qusb2snes.rc",
            "ressources.qrc",
            "devices/retroarchdevice.cpp",
            "devices/retroarchdevice.h",
            "rommapping/mapping_hirom.c",
            "rommapping/mapping_lorom.c",
            "rommapping/rommapping.c",
            "devices/sd2snesdevice.cpp",
            "devices/sd2snesdevice.h",
            "devices/snesclassic.cpp",
            "devices/snesclassic.h",
            "usb2snes.h",
            "wsserver.cpp",
            "wsserver.h",
            "wsservercommands.cpp",
        ]

        Group {     // Properties for the produced executable
            fileTagsFilter: "application"
            qbs.install: true
        }
        Depends {
            name : "Qt";
            submodules : ["gui", "core", "widgets", "network", "serialport", "websockets"]
        }
    }

    QtApplication {
        name : "TestQUsb2Snes"
        cpp.cxxLanguageVersion: "c++11"
        files: [
            "testmain.cpp",
            "client/usb2snes.h",
            "client/usb2snes.cpp",
        ]

        Group {     // Properties for the produced executable
            fileTagsFilter: "application"
            qbs.install: true
        }
        Depends {
            name : "Qt";
            submodules : ["core", "network", "websockets"]
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

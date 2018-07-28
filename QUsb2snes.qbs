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
            "main.cpp",
            "qusb2snes.rc",
            "ressources.qrc",
            "retroarchdevice.cpp",
            "retroarchdevice.h",
            "usb2snes.h",
            "usbconnection.cpp",
            "usbconnection.h",
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
    }/*
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

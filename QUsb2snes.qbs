import qbs

Project {
    minimumQbsVersion: "1.7.1"

    QtApplication {
        cpp.cxxLanguageVersion: "c++11"
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
            submodules : ["core", "widgets", "gui", "network", "serialport", "websockets"]
        }
    }
}

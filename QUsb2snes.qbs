import qbs

Project {
    minimumQbsVersion: "1.7.1"

    QtApplication {
        cpp.cxxLanguageVersion: "c++11"
        files: [
            "main.cpp",
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

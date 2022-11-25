import qbs

Project {
    minimumQbsVersion: "1.7.1"

    QtApplication {
        name : "QUsb2Snes"
        cpp.cxxLanguageVersion: "c++11"
        cpp.includePaths: ["devices/EmuNWAccess-qt", "./"]
        consoleApplication: false
        files: [
            "TODO",
            "devices/EmuNWAccess-qt/emunwaccessclient.cpp",
            "devices/EmuNWAccess-qt/emunwaccessclient.h",
            "adevice.cpp",
            "adevice.h",
            "ui/appui.cpp",
            "ui/appuimenu.cpp",
            "ui/appui.h",
            "ui/appuipoptracker.cpp",
            "ui/tempdeviceselector.cpp",
            "ui/tempdeviceselector.h",
            "ui/tempdeviceselector.ui",
            "ui/diagnosticdialog.cpp",
            "ui/diagnosticdialog.h",
            "ui/diagnosticdialog.ui",
            "backward.hpp",
            "devicefactory.cpp",
            "devicefactory.h",
            "devices/deviceerror.cpp",
            "devices/deviceerror.h",
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
            "ipsparse.cpp",
            "ipsparse.h",
            "devices/luabridge.cpp",
            "devices/luabridge.h",
            "devices/luabridgedevice.cpp",
            "devices/luabridgedevice.h",
            "localstorage.cpp",
            "localstorage.h",
            "main.cpp",
            "qskarsnikringlist.hpp",
            "qusb2snes.rc",
            "ressources.qrc",
            "devices/retroarchdevice.cpp",
            "devices/retroarchdevice.h",
            "rommapping/mapping_hirom.c",
            "rommapping/mapping_lorom.c",
            "rommapping/rommapping.c",
            "rommapping/rominfo.c",
            "devices/sd2snesdevice.cpp",
            "devices/sd2snesdevice.h",
            "devices/snesclassic.cpp",
            "devices/snesclassic.h",
            "ui/wizard/deviceselectorpage.cpp",
            "ui/wizard/deviceselectorpage.h",
            "ui/wizard/deviceselectorpage.ui",
            "ui/wizard/devicesetupwizard.cpp",
            "ui/wizard/devicesetupwizard.h",
            "ui/wizard/devicesetupwizard.ui",
            "ui/wizard/lastpage.cpp",
            "ui/wizard/lastpage.h",
            "ui/wizard/lastpage.ui",
            "ui/wizard/retroarchpage.cpp",
            "ui/wizard/retroarchpage.h",
            "ui/wizard/retroarchpage.ui",
            "ui/wizard/sd2snespage.cpp",
            "ui/wizard/sd2snespage.h",
            "ui/wizard/sd2snespage.ui",
            "usb2snes.h",
            "wsserver.cpp",
            "wsserver.h",
            "wsservercommands.cpp",
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
}

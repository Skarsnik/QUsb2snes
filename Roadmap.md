Roadmap

This is a roadmap for futures version of QUsb2Snes

0.7.24 : Some usb2snesw proto change

Introducing a "V2" to the websocket protocol for client connection on the 27xxx port
There will be no backward compact, it's an imposed behavior.
The AppVersion command will tell you to expect v2 thing
Basicly change will be :
                - Every command returns something (looking at you Attach)
                - Error text message will be send before a disconnection
                    Error will have class like DeviceError, ProtocolError
Sorry it's not a full revamp of the protocol, I don't have time for it.
And a replacement protocol could be like a proxy software that work with NWA proto.

0.8 : Real first "stable"

Focused on a 'bug' free release for everything that already in QUsb2Snes
Switch to Qt 5.15.2 for Windows releases with maybe an installer.

0.8.* : Bug fix and Linux package

Not big change again, just fixes and working toward making RPM/Deb

0.9 : Unstable change

Ignored by the windows updater if not on a 0.9 release already
Revamp of the code (already on going on the move branch)
Introducing the NWA 'emulation'. Basicly making devices supported
on QUsb also NWA emulators.

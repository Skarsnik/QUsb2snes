%global debug_package %{nil}
Name:     QUsb2snes
Version:  0.7.26
Release:  1%{?dist}
Summary:  websocket server protocol for accessing hardware/software that act like a SNES (or are a SNES)
License:  GPLv3
URL:      https://skarsnik.github.io/%{name}/
Source:   https://github.com/Skarsnik/%{name}/archive/refs/tags/v%{version}.tar.gz
BuildRequires: qt6-qtbase-devel
BuildRequires: qt6-qtwebsockets-devel
BuildRequires: qt6-qtserialport-devel
BuildRequires: git

Requires: qt6-qtbase
Requires: qt6-qtwebsockets
Requires: qt6-qtserialport

%description
QUsb2Snes is a websocket server that provide an unified protocol for accessing hardware/software that act like a SNES (or are a SNES). 
A classic usage is to use the FileViewer client to upload roms to your SD2SNES. 
But it allows for more advanced usage like reading/writing the memory of the SNES.

%prep
%autosetup -n %{name}

%conf
qmake %{name}.pro CONFIG+='release'
cd QFile2Snes && qmake QFile2Snes.pro CONFIG+='release' && cd -

%build
%make_build
cd QFile2Snes && make && cd -

%install
mkdir -p %{buildroot}/usr/bin/
install -m 755 QUsb2Snes %{buildroot}/usr/bin/QUsb2Snes
install -m 755 QFile2Snes/QFile2Snes %{buildroot}/usr/bin/QFile2Snes

%files
/usr/bin/QUsb2Snes
/usr/bin/QFile2Snes

%license LICENSE
%doc COMPILING.adoc README.md CONTRIBUTING.adoc TODO

%check
make check

%changelog
* Thu Jan 19 2023 Thibault Delattre
- Initial version of the package

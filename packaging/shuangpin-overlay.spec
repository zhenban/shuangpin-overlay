Name:           shuangpin-overlay
Version:        0.1.1
Release:        1%{?dist}
Summary:        Floating Shuangpin key chart that follows what you type

License:        GPL-3.0-or-later
URL:            https://github.com/zhenban/shuangpin-overlay
Source0:        %{url}/archive/v%{version}/%{name}-%{version}.tar.gz

BuildRequires:  cmake
BuildRequires:  ninja-build
BuildRequires:  gcc-c++
BuildRequires:  qt6-qtbase-devel
BuildRequires:  qt6-qttools-devel
BuildRequires:  layer-shell-qt-devel
BuildRequires:  desktop-file-utils
BuildRequires:  appstream

Requires:       hicolor-icon-theme
Recommends:     fcitx5-chinese-addons

%description
A floating key chart for learning Shuangpin. It follows what you type: pressed
keys light up, and after the first key of a syllable only the finals that can
follow it stay lit. It appears while a Shuangpin input method is active and
hides when the keyboard is idle.

Eight schemes are built in, and the one set in Fcitx5 is used by default.

Reading the keyboard needs membership of the "input" group.


%prep
%autosetup


%build
%cmake -G Ninja
%cmake_build


%install
%cmake_install


%check
%ctest
desktop-file-validate %{buildroot}%{_datadir}/applications/*.desktop
appstreamcli validate --no-net %{buildroot}%{_metainfodir}/*.metainfo.xml


%files
%license LICENSE
%doc README.md README.zh-CN.md CHANGELOG.md
%{_bindir}/%{name}
%{_datadir}/applications/io.github.zhenban.ShuangpinOverlay.desktop
%{_metainfodir}/io.github.zhenban.ShuangpinOverlay.metainfo.xml
%{_datadir}/icons/hicolor/scalable/apps/io.github.zhenban.ShuangpinOverlay.svg


%changelog
* Mon Nov 16 2026 zhenban - 0.1.1-1
- Packages for Fedora 44 and 45

* Mon Nov 16 2026 zhenban - 0.1.0-1
- First version

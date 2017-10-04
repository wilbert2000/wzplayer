Name:           wzplayer
Version:        %{version}
Release:        1%{?dist}
Summary:        A great media player

Group:          Applications/Multimedia
License:        GPL-2.0+
URL:            https://github.com/wilbert2000/wzplayer
Source0:        https://github.com/wilbert2000/wzplayer/archive/%{version}.tar.gz

%if 0%{?suse_version}
BuildRequires:  libqt5-devel
BuildRequires:  hicolor-icon-theme
%else
BuildRequires:  qt5-devel
#BuildRequires:  qtwebkit-devel
%endif
BuildRequires:  desktop-file-utils
BuildRequires:  gcc-c++

Requires:       (mplayer or mpv)
%{?_qt5_version:Requires: qt5%{?_isa} >= %{_qt5_version}}

%description
WZPlayer is a video player based on SMPlayer, http://www.smplayer.info,
by Ricardo Villalba. It simplifies playing videos with MPlayer and MPV.

%prep
%setup -a3 -a4 -qn %{name}-%{version}

# correction for wrong-file-end-of-line-encoding
%{__sed} -i 's/\r//' *.txt

%build
make \
%if !0%{?suse_version}
    QMAKE=%{_qt5_qmake} \
    LRELEASE=%{_bindir}/lrelease-qt5 \
%endif
	PREFIX=%{_prefix} \
    DOC_PATH="\\\"%{_docdir}/%{name}/\\\""

%install
make PREFIX=%{_prefix} DESTDIR=%{buildroot}/ DOC_PATH=%{_docdir}/%{name}/ install

%post
touch --no-create %{_datadir}/icons/hicolor
if [ -x %{_bindir}/gtk-update-icon-cache ]; then
  %{_bindir}/gtk-update-icon-cache --quiet %{_datadir}/icons/hicolor || :
fi
update-desktop-database &> /dev/null || :

%postun
touch --no-create %{_datadir}/icons/hicolor
if [ -x %{_bindir}/gtk-update-icon-cache ]; then
  %{_bindir}/gtk-update-icon-cache --quiet %{_datadir}/icons/hicolor || :
fi
update-desktop-database &> /dev/null || :

%files
%defattr(-,root,root)
%{_bindir}/wzplayer
%{_datadir}/applications/*.desktop
%dir %{_datadir}/icons/hicolor/*/
%dir %{_datadir}/icons/hicolor/*/apps/
%{_datadir}/icons/hicolor/*/apps/%{name}.*
%{_datadir}/wzplayer/
%{_mandir}/man1/wzplayer.1.gz
%{_docdir}/%{name}/

%changelog
* Mon Apr 11 2016 Wilbert Hengst <wplayer@xs4all.nl>
- For changes see https://github.com/wilbert2000/wzplayer/commits/master
* Thu Jun 14 2007 Sebastian Vahl <fedora@deadbabylon.de> - 0.5.7-1
- Initial Release

Name:           wzplayer
Version:        %{version}
Release:        1%{?dist}
Summary:        A great media player

Group:          Applications/Multimedia
License:        GPL-2.0+
URL:            https://github.com/wilbert2000/wzplayer
Source0:        https://github.com/wilbert2000/wzplayer/releases/latest

%if 0%{?suse_version}
BuildRequires:  libqt4-devel
BuildRequires:  hicolor-icon-theme
%else
BuildRequires:  qt4-devel
#BuildRequires:  qtwebkit-devel
%endif
BuildRequires:  desktop-file-utils
BuildRequires:  gcc-c++

Requires:       (mplayer or mpv)
%{?_qt4_version:Requires: qt4%{?_isa} >= %{_qt4_version}}

%description
WZPlayer is a graphical user interface for MPV and MPlayer based upon
SMPlayer, http://www.smplayer.info, by Ricardo Villalba. With improved
performance, scaling, DVDNAV support, no setup needed, lots of setup
available. No support and lots of source.

%prep
%setup -a3 -a4 -qn %{name}-%{version}

# correction for wrong-file-end-of-line-encoding
%{__sed} -i 's/\r//' *.txt
# fix files which are not UTF-8 
iconv -f Latin1 -t UTF-8 -o Changelog.utf8 Changelog 
mv Changelog.utf8 Changelog

%build
make \
%if !0%{?suse_version}
	QMAKE=%{_qt4_qmake} \
	LRELEASE=%{_bindir}/lrelease-qt4 \
%endif
	PREFIX=%{_prefix} \
	DOC_PATH="\\\"%{_docdir}/%{name}/\\\"" \
	QMAKE_OPTS=DEFINES+=NO_DEBUG_ON_CONSOLE

#touch src/smplayer
#touch src/translations/smplayer_es.qm

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
- For the changelog see https://github.com/wilbert2000/wzplayer/commits/master
- To find your version search for the digits following the g in %{version}
* Thu Jun 14 2007 Sebastian Vahl <fedora@deadbabylon.de> - 0.5.7-1
- Initial Release

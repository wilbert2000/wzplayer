
#TODO: add build dir

PREFIX=/usr/local
#PREFIX=/tmp/wzplayer

CONF_PREFIX=$(PREFIX)

DATA_PATH=$(PREFIX)/share/wzplayer
DOC_PATH=$(PREFIX)/share/doc/packages/wzplayer
TRANSLATION_PATH=$(DATA_PATH)/translations
THEMES_PATH=$(DATA_PATH)/themes
SHORTCUTS_PATH=$(DATA_PATH)/shortcuts

#KDE_PREFIX=`kde-config --prefix`
#KDE_PREFIX=/tmp/wzplayer/kde/
KDE_PREFIX=$(PREFIX)

KDE_ICONS=$(KDE_PREFIX)/share/icons/hicolor/
KDE_APPLNK=$(KDE_PREFIX)/share/applications/

QMAKE=qmake
LRELEASE=lrelease

DEFS=DATA_PATH=\\\"$(DATA_PATH)\\\" \
     TRANSLATION_PATH=\\\"$(TRANSLATION_PATH)\\\" \
     DOC_PATH=\\\"$(DOC_PATH)\\\" THEMES_PATH=\\\"$(THEMES_PATH)\\\" \
     SHORTCUTS_PATH=\\\"$(SHORTCUTS_PATH)\\\"


ifdef KDE_SUPPORT

# KDE paths, change if necessary

KDE_INCLUDE_PATH=`kde-config --prefix`/include/
KDE_LIB_PATH=`kde-config --prefix`/lib/
KDE_LIBS=-lkio

QMAKE_OPTS=DEFINES+=KDE_SUPPORT INCLUDEPATH+=$(KDE_INCLUDE_PATH) \
           LIBS+="$(KDE_LIBS) -L$(KDE_LIB_PATH)"

endif

src/wzplayer:
	+cd src && $(QMAKE) $(QMAKE_OPTS) && $(DEFS) make
	cd src && $(LRELEASE) wzplayer.pro

clean:
	if [ -f src/Makefile ]; then cd src && make distclean; fi
	-rm src/translations/*.qm

install: src/wzplayer
	-install -d $(DESTDIR)$(PREFIX)/bin/
	install -m 755 src/wzplayer $(DESTDIR)$(PREFIX)/bin/
	-install -d $(DESTDIR)$(DATA_PATH)
	install -m 644 src/input.conf $(DESTDIR)$(DATA_PATH)
	-install -d $(DESTDIR)$(TRANSLATION_PATH)
	install -m 644 src/translations/*.qm $(DESTDIR)$(TRANSLATION_PATH)
	-install -d $(DESTDIR)$(DOC_PATH)
	install -m 644 Changelog *.txt $(DESTDIR)$(DOC_PATH)

	-install -d $(DESTDIR)$(DOC_PATH)
	tar -C docs/ --exclude=.svn -c -f - . | tar -C $(DESTDIR)$(DOC_PATH) -x -f -

	-install -d $(DESTDIR)$(SHORTCUTS_PATH)
	cp src/shortcuts/* $(DESTDIR)$(SHORTCUTS_PATH)

#	-install -d $(DESTDIR)$(THEMES_PATH)
#	-tar -C src/themes/ --exclude=.svn -c -f - . | tar -C $(DESTDIR)$(THEMES_PATH) -x -f -

	-install -d $(DESTDIR)$(KDE_ICONS)/512x512/apps/
	-install -d $(DESTDIR)$(KDE_ICONS)/256x256/apps/
	-install -d $(DESTDIR)$(KDE_ICONS)/192x192/apps/
	-install -d $(DESTDIR)$(KDE_ICONS)/128x128/apps/
	-install -d $(DESTDIR)$(KDE_ICONS)/64x64/apps/
	-install -d $(DESTDIR)$(KDE_ICONS)/32x32/apps/
	-install -d $(DESTDIR)$(KDE_ICONS)/22x22/apps/
	-install -d $(DESTDIR)$(KDE_ICONS)/16x16/apps/
	-install -d $(DESTDIR)$(KDE_ICONS)/scalable/apps/
	install -m 644 icons/wzplayer_icon512.png $(DESTDIR)$(KDE_ICONS)/512x512/apps/wzplayer.png
	install -m 644 icons/wzplayer_icon256.png $(DESTDIR)$(KDE_ICONS)/256x256/apps/wzplayer.png
	install -m 644 icons/wzplayer_icon192.png $(DESTDIR)$(KDE_ICONS)/192x192/apps/wzplayer.png
	install -m 644 icons/wzplayer_icon128.png $(DESTDIR)$(KDE_ICONS)/128x128/apps/wzplayer.png
	install -m 644 icons/wzplayer_icon64.png $(DESTDIR)$(KDE_ICONS)/64x64/apps/wzplayer.png
	install -m 644 icons/wzplayer_icon32.png $(DESTDIR)$(KDE_ICONS)/32x32/apps/wzplayer.png
	install -m 644 icons/wzplayer_icon22.png $(DESTDIR)$(KDE_ICONS)/22x22/apps/wzplayer.png
	install -m 644 icons/wzplayer_icon16.png $(DESTDIR)$(KDE_ICONS)/16x16/apps/wzplayer.png
	install -m 644 icons/wzplayer.svg $(DESTDIR)$(KDE_ICONS)/scalable/apps/wzplayer.svg
	-install -d $(DESTDIR)$(KDE_APPLNK)
	install -m 644 wzplayer.desktop $(DESTDIR)$(KDE_APPLNK)
	install -m 644 wzplayer_enqueue.desktop $(DESTDIR)$(KDE_APPLNK)
	-install -d $(DESTDIR)$(PREFIX)/share/man/man1/
	install -m 644 man/wzplayer.1 $(DESTDIR)$(PREFIX)/share/man/man1/
	gzip -9 -f $(DESTDIR)$(PREFIX)/share/man/man1/wzplayer.1

uninstall:
	-rm -f $(PREFIX)/bin/wzplayer
	-rm -f $(DATA_PATH)/input.conf
	-rm -f $(TRANSLATION_PATH)/*.qm
	-rm -f $(DOC_PATH)/Changelog
	-rm -f $(DOC_PATH)/*.txt
	-rm -f $(SHORTCUTS_PATH)/*.keys
	-rm -f $(KDE_ICONS)/64x64/apps/wzplayer.png
	-rm -f $(KDE_ICONS)/32x32/apps/wzplayer.png
	-rm -f $(KDE_ICONS)/22x22/apps/wzplayer.png
	-rm -f $(KDE_ICONS)/16x16/apps/wzplayer.png
	-rm -f $(KDE_APPLNK)/wzplayer.desktop
	-rm -f $(PREFIX)/share/man/man1/wzplayer.1.gz
	-rmdir $(SHORTCUTS_PATH)/
	-rmdir $(TRANSLATION_PATH)/
#	-for file in docs/*/*; do \
#	    rm -f $(DOC_PATH)/$${file/docs/}; \
#	done;
#	-for file in docs/*; do \
#	    rmdir $(DOC_PATH)/$${file/docs/}; \
#	done;
	-(cd docs && find -iname '*.html') | (cd $(DESTDIR)$(DOC_PATH) && xargs rm)
	-(cd docs && find -type d -name '??') | (cd $(DESTDIR)$(DOC_PATH) && xargs rmdir)
	-rmdir $(DOC_PATH)/
	-rmdir $(DATA_PATH)/



PREFIX=/usr/local
#PREFIX=/tmp/$(NAME)

NAME=wzplayer
PROJECT=$(NAME).pro
# qmake needs full project path
PROJECT_PATH=$(CURDIR)/src/$(PROJECT)

BINDIR=$(DESTDIR)$(PREFIX)/bin/
MANDIR=$(DESTDIR)$(PREFIX)/share/man/man1/

KDE_PREFIX=$(PREFIX)
KDE_ICONS=$(DESTDIR)$(KDE_PREFIX)/share/icons/hicolor/
KDE_APPLNK=$(DESTDIR)$(KDE_PREFIX)/share/applications/

QMAKE=qmake
LRELEASE=lrelease

BUILDDIR=build
CHANGELOG=Changelog

# Pass without DESTDIR to build
DATA_PATH=$(PREFIX)/share/$(NAME)
DOC_PATH=$(PREFIX)/share/doc/packages/$(NAME)
TRANSLATION_PATH=$(DATA_PATH)/translations
THEMES_PATH=$(DATA_PATH)/themes
SHORTCUTS_PATH=$(DATA_PATH)/shortcuts

DEFS=DATA_PATH=\\\"$(DATA_PATH)\\\" \
 DOC_PATH=\\\"$(DOC_PATH)\\\" \
 TRANSLATION_PATH=\\\"$(TRANSLATION_PATH)\\\" \
 THEMES_PATH=\\\"$(THEMES_PATH)\\\" \
 SHORTCUTS_PATH=\\\"$(SHORTCUTS_PATH)\\\"

# Convenience vars with DESTDIR
FDATA_PATH=$(DESTDIR)$(DATA_PATH)
FTRANSLATION_PATH=$(DESTDIR)$(TRANSLATION_PATH)
FSHORTCUTS_PATH=$(DESTDIR)$(SHORTCUTS_PATH)
FDOC_PATH=$(DESTDIR)$(DOC_PATH)


ifdef KDE_SUPPORT

KDE_INCLUDE_PATH=`kde-config --prefix`/include/
KDE_LIB_PATH=`kde-config --prefix`/lib/
KDE_LIBS=-lkio

QMAKE_OPTS=DEFINES+=KDE_SUPPORT INCLUDEPATH+=$(KDE_INCLUDE_PATH) \
           LIBS+="$(KDE_LIBS) -L$(KDE_LIB_PATH)"

endif

$(BUILDDIR)/$(NAME): $(CHANGELOG)
	-mkdir $(BUILDDIR)
	cd $(BUILDDIR) && $(QMAKE) $(QMAKE_OPTS) -o Makefile "$(PROJECT_PATH)"
	cd $(BUILDDIR) && $(DEFS) make
	cd $(BUILDDIR) && $(LRELEASE) "$(PROJECT_PATH)"

$(CHANGELOG):
	echo "See https://github.com/wilbert2000/wzplayer/commits/master" > $(CHANGELOG)

clean:
	-rm -r $(BUILDDIR)
	-rm src/translations/*.qm
	-rm $(CHANGELOG)

install: $(BUILDDIR)/$(NAME)
	-install -d $(BINDIR)
	install -m 755 $(BUILDDIR)/$(NAME) $(BINDIR)
	-install -d $(FDATA_PATH)
	install -m 644 src/input.conf $(FDATA_PATH)
	-install -d $(FTRANSLATION_PATH)
	install -m 644 src/translations/*.qm $(FTRANSLATION_PATH)
	-install -d $(FDOC_PATH)
	install -m 644 Changelog *.txt $(FDOC_PATH)
	tar -C docs/ -c -f - . | tar -C $(FDOC_PATH) -x -f -
	-install -d $(FSHORTCUTS_PATH)
	cp src/shortcuts/* $(FSHORTCUTS_PATH)

	-install -d $(KDE_ICONS)/512x512/apps/
	-install -d $(KDE_ICONS)/256x256/apps/
	-install -d $(KDE_ICONS)/192x192/apps/
	-install -d $(KDE_ICONS)/128x128/apps/
	-install -d $(KDE_ICONS)/64x64/apps/
	-install -d $(KDE_ICONS)/32x32/apps/
	-install -d $(KDE_ICONS)/22x22/apps/
	-install -d $(KDE_ICONS)/16x16/apps/
	-install -d $(KDE_ICONS)/scalable/apps/
	install -m 644 icons/$(NAME)_icon512.png $(KDE_ICONS)/512x512/apps/$(NAME).png
	install -m 644 icons/$(NAME)_icon256.png $(KDE_ICONS)/256x256/apps/$(NAME).png
	install -m 644 icons/$(NAME)_icon192.png $(KDE_ICONS)/192x192/apps/$(NAME).png
	install -m 644 icons/$(NAME)_icon128.png $(KDE_ICONS)/128x128/apps/$(NAME).png
	install -m 644 icons/$(NAME)_icon64.png $(KDE_ICONS)/64x64/apps/$(NAME).png
	install -m 644 icons/$(NAME)_icon32.png $(KDE_ICONS)/32x32/apps/$(NAME).png
	install -m 644 icons/$(NAME)_icon22.png $(KDE_ICONS)/22x22/apps/$(NAME).png
	install -m 644 icons/$(NAME)_icon16.png $(KDE_ICONS)/16x16/apps/$(NAME).png
	install -m 644 icons/$(NAME).svg $(KDE_ICONS)/scalable/apps/$(NAME).svg

	-install -d $(KDE_APPLNK)
	install -m 644 $(NAME).desktop $(KDE_APPLNK)
	install -m 644 $(NAME)_enqueue.desktop $(KDE_APPLNK)

	-install -d $(MANDIR)
	install -m 644 man/$(NAME).1 $(MANDIR)
	gzip -9 -f $(MANDIR)$(NAME).1

uninstall:
	-rm -f $(BINDIR)$(NAME)
	-rm -rf $(FDATA_PATH)
	-rm -rf $(FDOC_PATH)
	-rm -f $(KDE_ICONS)/64x64/apps/$(NAME).png
	-rm -f $(KDE_ICONS)/32x32/apps/$(NAME).png
	-rm -f $(KDE_ICONS)/22x22/apps/$(NAME).png
	-rm -f $(KDE_ICONS)/16x16/apps/$(NAME).png
	-rm -f $(KDE_APPLNK)/$(NAME).desktop
	-rm -f $(KDE_APPLNK)/$(NAME)_enqueue.desktop.desktop
	-rm -f $(MANDIR)$(NAME).1.gz


NAME=wzplayer
PROJECTFILE=$(CURDIR)/src/$(NAME).pro
BUILDDIR=build
CHANGELOG=Changelog

# Installation paths
PREFIX=/usr/local
BINDIR=$(PREFIX)/bin/
MANDIR=$(PREFIX)/share/man/man1/
ICONDIR=$(PREFIX)/share/icons/hicolor/
APPDIR=$(PREFIX)/share/applications/

DATA_PATH=$(PREFIX)/share/$(NAME)
TRANSLATION_PATH=$(DATA_PATH)/translations
THEMES_PATH=$(DATA_PATH)/themes
SHORTCUTS_PATH=$(DATA_PATH)/shortcuts
DOC_PATH=$(PREFIX)/share/doc/packages/$(NAME)

# Qt tools
QMAKE=qmake-qt5
LRELEASE=lrelease-qt5


# Default target
$(BUILDDIR)/$(NAME): $(CHANGELOG)
	-mkdir $(BUILDDIR)
	cd $(BUILDDIR) && $(QMAKE) $(QMAKE_OPTS) -o Makefile "$(PROJECTFILE)"
	cd $(BUILDDIR) && make
	cd $(BUILDDIR) && $(LRELEASE) "$(PROJECTFILE)"
	echo "build done"


$(CHANGELOG):
	echo "See https://github.com/wilbert2000/wzplayer/commits/master" > $(CHANGELOG)


clean:
	-rm -r $(BUILDDIR)
	-rm src/translations/*.qm
	-rm $(CHANGELOG)


install: $(BUILDDIR)/$(NAME)
	-install -d $(BINDIR)
	install -m 755 $(BUILDDIR)/$(NAME) $(BINDIR)
	-install -d $(DATA_PATH)
	install -m 644 src/input.conf $(DATA_PATH)
	-install -d $(TRANSLATION_PATH)
	install -m 644 src/translations/*.qm $(TRANSLATION_PATH)
	-install -d $(DOC_PATH)
	install -m 644 Changelog *.txt $(DOC_PATH)
	tar -C docs/ -c -f - . | tar -C $(DOC_PATH) -x -f -
	-install -d $(SHORTCUTS_PATH)
	cp src/shortcuts/* $(SHORTCUTS_PATH)

	-install -d $(ICONDIR)/512x512/apps/
	-install -d $(ICONDIR)/256x256/apps/
	-install -d $(ICONDIR)/192x192/apps/
	-install -d $(ICONDIR)/128x128/apps/
	-install -d $(ICONDIR)/64x64/apps/
	-install -d $(ICONDIR)/32x32/apps/
	-install -d $(ICONDIR)/22x22/apps/
	-install -d $(ICONDIR)/16x16/apps/
	-install -d $(ICONDIR)/scalable/apps/
	install -m 644 icons/$(NAME)_icon512.png $(ICONDIR)/512x512/apps/$(NAME).png
	install -m 644 icons/$(NAME)_icon256.png $(ICONDIR)/256x256/apps/$(NAME).png
	install -m 644 icons/$(NAME)_icon192.png $(ICONDIR)/192x192/apps/$(NAME).png
	install -m 644 icons/$(NAME)_icon128.png $(ICONDIR)/128x128/apps/$(NAME).png
	install -m 644 icons/$(NAME)_icon64.png $(ICONDIR)/64x64/apps/$(NAME).png
	install -m 644 icons/$(NAME)_icon32.png $(ICONDIR)/32x32/apps/$(NAME).png
	install -m 644 icons/$(NAME)_icon22.png $(ICONDIR)/22x22/apps/$(NAME).png
	install -m 644 icons/$(NAME)_icon16.png $(ICONDIR)/16x16/apps/$(NAME).png
	install -m 644 icons/$(NAME).svg $(ICONDIR)/scalable/apps/$(NAME).svg

	-install -d $(APPDIR)
	install -m 644 $(NAME).desktop $(APPDIR)
	install -m 644 $(NAME)_enqueue.desktop $(APPDIR)

	-install -d $(MANDIR)
	install -m 644 man/$(NAME).1 $(MANDIR)
	gzip -9 -f $(MANDIR)/$(NAME).1


uninstall:
	-rm -f $(BINDIR)$(NAME)
	-rm -rf $(DATA_PATH)
	-rm -rf $(DOC_PATH)
	-rm -f $(ICONDIR)/64x64/apps/$(NAME).png
	-rm -f $(ICONDIR)/32x32/apps/$(NAME).png
	-rm -f $(ICONDIR)/22x22/apps/$(NAME).png
	-rm -f $(ICONDIR)/16x16/apps/$(NAME).png
	-rm -f $(APPDIR)/$(NAME).desktop
	-rm -f $(APPDIR)/$(NAME)_enqueue.desktop.desktop
	-rm -f $(MANDIR)/$(NAME).1.gz

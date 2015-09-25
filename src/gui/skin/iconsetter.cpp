/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2006-2015 Ricardo Villalba <rvm@users.sourceforge.net>
    umplayer, Copyright (C) 2010 Ori Rejwan

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "gui/skin/iconsetter.h"
#include "gui/skin/icon.h"
#include "gui/skin/actiontools.h"
#include "images.h"


#define ICON_ADD(icon, png) { icon.addPixmap(Images::icon(png, 16), QIcon::Normal, QIcon::Off); \
                              icon.addPixmap(Images::icon(png, 16), QIcon::Active, QIcon::Off); \
                              icon.addPixmap(Images::icon(png, 16), QIcon::Selected, QIcon::Off); \
                              icon.addPixmap(Images::icon(png, 16), QIcon::Disabled, QIcon::Off);}

namespace Gui {
namespace Skin {

TIconSetter* TIconSetter::m_instance = 0;

TIconSetter::TIconSetter(QWidget *parent) :
    QWidget(parent)
{
}

TIconSetter* TIconSetter::instance()
{
    if(m_instance == 0)
    {
		m_instance = new TIconSetter();
    }
    return m_instance;
}

void TIconSetter::removeInstance()
{
    if(m_instance)
        delete m_instance;
    m_instance = 0;
}

void TIconSetter::setActionIcon(QPixmap pixmap )
{
#if 0
//#define SAVE_ICONS 1
#define SAVE(name) { QPixmap p = pixmap.copy(n*24, 0, 24, 24); \
                     QString s = "/tmp/" name ".png"; \
                     p.save(s); }

    for(int n = 0; n < 10; ++n )
    {
        QIcon icon;
        icon.addPixmap(pixmap.copy(n*24, 0, 24, 24), QIcon::Normal, QIcon::Off);
        icon.addPixmap(pixmap.copy(n*24, 24, 24, 24), QIcon::Active, QIcon::Off);
        icon.addPixmap(pixmap.copy(n*24, 48, 24, 24), QIcon::Selected, QIcon::Off);
        icon.addPixmap(pixmap.copy(n*24, 72, 24, 24), QIcon::Disabled, QIcon::Off);
        QAction * action = 0;
		//TActionTools::findAction("aaa", toolbar_actions);
        switch(n)
        {
        case 0: action = TActionTools::findAction("open_file", toolbar_actions);
				if (action) ICON_ADD(icon, "file")
				#if SAVE_ICONS
				SAVE("open")
				#endif
                break;
        case 1: action = TActionTools::findAction("open_directory", toolbar_actions);
				if (action) ICON_ADD(icon, "file")
				#if SAVE_ICONS
				SAVE("open_folder")
				#endif
                break;
        case 2: action = TActionTools::findAction("open_dvd", toolbar_actions);
				if (action) ICON_ADD(icon, "file")
				#if SAVE_ICONS
				SAVE("dvd")
				#endif
                break;
        case 3: action = TActionTools::findAction("open_url", toolbar_actions);
				if (action) ICON_ADD(icon, "file")
				#if SAVE_ICONS
				SAVE("url")
				#endif
                break;
        case 4: action = TActionTools::findAction("screenshot", toolbar_actions);
				if (action) ICON_ADD(icon, "file")
				#if SAVE_ICONS
				SAVE("screenshot")
				#endif
                break;
        case 5: action = TActionTools::findAction("show_file_properties", toolbar_actions);
				if (action) ICON_ADD(icon, "file")
				#if SAVE_ICONS
				SAVE("info")
				#endif
                break;
        case 6: action = TActionTools::findAction("show_find_sub_dialog", toolbar_actions);
				if (action) ICON_ADD(icon, "file")
				#if SAVE_ICONS
				SAVE("download_subs")
				#endif
                break;
        case 7: action = TActionTools::findAction("show_preferences", toolbar_actions);
				if (action) ICON_ADD(icon, "file")
				#if SAVE_ICONS
				SAVE("prefs")
				#endif
                break;
        }
        if (action) action->setIcon(icon);
    }
#endif
}

void TIconSetter::buttonIcon(int buttonNo, QPixmap pix )
{
    TIcon icon;
    int w = pix.width();
    int h = pix.height();
    icon.setPixmap(pix.copy(0, 0, w, h/4 ), TIcon::Normal, TIcon::Off);
    icon.setPixmap(pix.copy(0, h/4, w, h/4 ), TIcon::MouseOver, TIcon::Off);
    icon.setPixmap(pix.copy(0, h/2, w, h/4 ), TIcon::MouseDown, TIcon::Off);
    icon.setPixmap(pix.copy(0, 3*h/4, w, h/4 ), TIcon::Disabled, TIcon::Off);
    TIcon icon2;
    switch(buttonNo)
    {
    case 1:
        playControl->setBackwardIcon(icon);
        break;
    case 2:
        playControl->setPreviousIcon(icon);break;
    case 3:        
        icon2.setPixmap(pix.copy(0, 0, w/2, h/4 ), TIcon::Normal, TIcon::Off);
        icon2.setPixmap(pix.copy(0, h/4, w/2, h/4 ), TIcon::MouseOver, TIcon::Off);
        icon2.setPixmap(pix.copy(0, h/2, w/2, h/4 ), TIcon::MouseDown, TIcon::Off);
        icon2.setPixmap(pix.copy(0, 3*h/4, w/2, h/4 ), TIcon::Disabled, TIcon::Off);

        icon2.setPixmap(pix.copy(w/2, 0, w/2, h/4 ), TIcon::Normal, TIcon::On);
        icon2.setPixmap(pix.copy(w/2, h/4, w/2, h/4 ), TIcon::MouseOver, TIcon::On);
        icon2.setPixmap(pix.copy(w/2, h/2, w/2, h/4 ), TIcon::MouseDown, TIcon::On);
        icon2.setPixmap(pix.copy(w/2, 3*h/4, w/2, h/4 ), TIcon::Disabled, TIcon::On);

        playControl->setPlayPauseIcon(icon2);
        break;
    case 4:
        playControl->setStopIcon(icon);break;
    case 5:
        playControl->setRecordIcon(icon);break;
    case 6:
        playControl->setNextIcon(icon);break;
    case 7:
        playControl->setForwardIcon(icon);break;

    }
}


void TIconSetter::mediaPanelButtonIcon( int n, QPixmap pix)
{
    if(pix.isNull()) return;
    TIcon icon;
    int w = pix.width();
    int h = pix.height();
    icon.setPixmap(pix.copy(0, 0, w/2, h/4 ), TIcon::Normal, TIcon::Off);
    icon.setPixmap(pix.copy(0, h/4, w/2, h/4 ), TIcon::MouseOver, TIcon::Off);
    icon.setPixmap(pix.copy(0, h/2, w/2, h/4 ), TIcon::MouseDown, TIcon::Off);
    icon.setPixmap(pix.copy(0, 3*h/4, w/2, h/4 ), TIcon::Disabled, TIcon::Off);

    icon.setPixmap(pix.copy(w/2, 0, w/2, h/4 ), TIcon::Normal, TIcon::On);
    icon.setPixmap(pix.copy(w/2, h/4, w/2, h/4 ), TIcon::MouseOver, TIcon::On);
    icon.setPixmap(pix.copy(w/2, h/2, w/2, h/4 ), TIcon::MouseDown, TIcon::On);
    icon.setPixmap(pix.copy(w/2, 3*h/4, w/2, h/4 ), TIcon::Disabled, TIcon::On);

    switch(n)
    {
    case 1:
        mediaPanel->setShuffleIcon(icon);break;
    case 2:
        mediaPanel->setRepeatIcon(icon);break;
    }
}

} // namesapce Skin
} // namespace Gui

#include "moc_iconsetter.cpp"

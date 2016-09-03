/*  WZPlayer, GUI front-end for mplayer and MPV.
    Parts copyright (C) 2006-2015 Ricardo Villalba <rvm@users.sourceforge.net>

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

#ifndef GUI_FILECHOOSER_H
#define GUI_FILECHOOSER_H

#include "gui/lineedit_with_icon.h"
#include <QFileDialog>

class QToolButton;

namespace Gui {

class TFileChooser : public TLineEditWithIcon {
    Q_OBJECT
	Q_PROPERTY(QString text READ text WRITE setText)
	Q_PROPERTY(QString caption READ caption WRITE setCaption)
	Q_PROPERTY(QString filter READ filter WRITE setFilter)
	Q_PROPERTY(DialogType dialogType READ dialogType WRITE setDialogType)
	Q_PROPERTY(QFileDialog::Options options READ options WRITE setOptions)

public:
	enum DialogType { GetFileName = 0, GetDirectory = 1 };

    TFileChooser(QWidget* parent = 0);
    virtual ~TFileChooser();

	QString caption() const { return _caption; }
	QString filter() const { return _filter; }
	DialogType dialogType() const { return _type; }
	QFileDialog::Options options() const { return _options; }

public slots:
	void setCaption(const QString & caption) { _caption = caption; }
	void setFilter(const QString & filter) { _filter = filter; }
	void setDialogType(DialogType type) { _type = type; }
	void setOptions(QFileDialog::Options options) { _options = options; }

signals:
	void fileChanged(QString file);

protected:
	virtual void setupButton();

protected slots:
	virtual void openFileDialog();
	void onEditingFinished();

protected:
	QString _caption;
	QString _filter;
	DialogType _type;
	QFileDialog::Options _options;

	static QString last_dir;
};

} // namespace Gui

#endif // GUI_FILECHOOSER_H

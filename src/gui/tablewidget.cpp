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

#include "gui/tablewidget.h"
#include <QTableWidgetItem>
#include <QDebug>

#define BE_VERBOSE 0

namespace Gui {

TTableWidget::TTableWidget(QWidget* parent) : QTableWidget(parent) {
}

TTableWidget::TTableWidget(int rows, int columns, QWidget* parent)
	: QTableWidget(rows, columns, parent) {
}

QTableWidgetItem* TTableWidget::getItem(int row, int column, bool* existed) {
#if BE_VERBOSE
	qDebug("Gui::TTableWidget::getItem: %d, %d", row, column);
#endif
	QTableWidgetItem* i = item(row, column);
	if (existed != 0) *existed = (i!=0); // Returns if the item already existed or not
	if (i != 0) return i; else return createItem(column);
}

QTableWidgetItem* TTableWidget::createItem(int /*col*/) {
#if BE_VERBOSE
	qDebug("Gui::TTableWidget::createItem");
#endif
	QTableWidgetItem* i = new QTableWidgetItem();
	i->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
	return i;
}

void TTableWidget::setText(int row, int column, const QString & text) {
#if BE_VERBOSE
	qDebug("Gui::TTableWidget::setText: %d, %d", row, column);
#endif
	bool existed;
	QTableWidgetItem* i = getItem(row, column, &existed);
	i->setText(text);
	if (!existed) setItem(row, column, i);
}

QString TTableWidget::text(int row, int column) {
#if BE_VERBOSE
	qDebug("Gui::TTableWidget::text: %d, %d", row, column);
#endif
	return getItem(row, column)->text();
}

void TTableWidget::setIcon(int row, int column, const QIcon & icon) {
#if BE_VERBOSE
	qDebug("Gui::TTableWidget::setIcon %d, %d", row, column);
#endif
	bool existed;
	QTableWidgetItem* i = getItem(row, column, &existed);
	i->setIcon(icon);
	if (!existed) setItem(row, column, i);
}

QIcon TTableWidget::icon(int row, int column) {
	return getItem(row, column)->icon();
}

bool TTableWidget::isSelected(int row, int column) {
	return getItem(row, column)->isSelected();
}

void TTableWidget::selRow(int row, bool sel) {

	if (row >= 0 && row < rowCount()) {
		QItemSelectionModel::SelectionFlags flags;
		if (sel)
			flags = QItemSelectionModel::Select;
		else
			flags = QItemSelectionModel::Deselect;
		QModelIndex tl = model()->index(row, 0, rootIndex());
		QModelIndex br = model()->index(row, columnCount() - 1, rootIndex());
		selectionModel()->select(QItemSelection(tl, br), flags);
	}
}

} // namespace Gui

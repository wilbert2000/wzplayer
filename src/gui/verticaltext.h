/* Taken from KMix */
/* Copyright (C) 2003-2004 Christian Esken <esken@kde.org> */


#ifndef GUI_VERTICALTEXT_H
#define GUI_VERTICALTEXT_H

#include <QWidget>
#include <QPaintEvent>

namespace Gui {

class TVerticalText : public QWidget
{
public:
    TVerticalText(QWidget* parent, Qt::WindowFlags f = 0);

    void setText(QString s) { _label = s; }
    QString text() { return _label; }
    QSize sizeHint() const;
    QSizePolicy sizePolicy () const;
    
protected:
    void paintEvent (QPaintEvent* event);
    QString _label;
};

} // namespace Gui

#endif // GUI_VERTICALTEXT_H

#ifndef GUI_VIDEOWINDOW_H
#define GUI_VIDEOWINDOW_H

#include <QWidget>



namespace Gui {

// Window for video player
class TVideoWindow : public QWidget {
    Q_OBJECT
public:
    explicit TVideoWindow(QWidget* parent);

    bool normalBackground;

    void setFastBackground();
    void restoreNormalBackground();

protected:
    virtual void paintEvent(QPaintEvent*) override;
};

} // namespace GUI
#endif // GUI_VIDEOWINDOW_H

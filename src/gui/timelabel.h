#ifndef GUI_TIMELABEL_H
#define GUI_TIMELABEL_H

#include <QLabel>


namespace Gui {

class TTimeLabel : public QLabel {
    Q_OBJECT
public:
    enum TTimeResolution {
        RES_SECONDS = 0,
        RES_MS = 1,
        RES_FRAMES = 2
    };

    explicit TTimeLabel(QWidget* parent);

    TTimeResolution timeResolution() const { return resolution; }

public slots:
    void setPositionMS(int ms);
    void setDurationMS(int ms);
    void setTimeResolution(int aResolution);

private:
    TTimeResolution resolution;
    QString positionText;
    QString durationText;
    int lastSec;

    QString getSuffix(int ms, int secs, const QString& fuzzyTime);
    void setPosMS(int ms, bool changed);
};

} // namespace Gui
#endif // GUI_TIMELABEL_H

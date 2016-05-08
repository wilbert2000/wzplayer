#ifndef ICONPROVIDER
#define ICONPROVIDER

#include <QFileIconProvider>
#include <QFileInfo>

class QStyle;

class TIconProvider: public QFileIconProvider {
public:
    TIconProvider();

    QIcon folderIcon;

    virtual QIcon icon(IconType type) const;
    virtual QIcon icon(const QFileInfo& info) const;

    QIcon iconForFile(const QString& filename) const;

    void setStyle(QStyle* aStyle);

private:
    QStyle* style;
};

extern TIconProvider iconProvider;

#endif

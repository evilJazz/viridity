#ifndef VIRIDITYQTQUICKDISPLAYMANAGER_H
#define VIRIDITYQTQUICKDISPLAYMANAGER_H

#include <QObject>

#ifdef KCL_QTQUICK2
    #include <QQmlParserStatus>
#else
    #include <QDeclarativeParserStatus>
#endif

class PrivateQtQuickDisplayManager;

class ViridityQtQuickDisplay :
    public QObject,
#ifdef KCL_QTQUICK2
    public QQmlParserStatus
#else
    public QDeclarativeParserStatus
#endif
{
    Q_OBJECT
#ifdef KCL_QTQUICK2
    Q_INTERFACES(QQmlParserStatus)
#else
    Q_INTERFACES(QDeclarativeParserStatus)
#endif
    Q_PROPERTY(QString targetId READ targetId WRITE setTargetId)
    Q_PROPERTY(bool autoSize READ autoSize WRITE setAutoSize)
    Q_PROPERTY(QObject *displayItem READ displayItem WRITE setDisplayItem)
    Q_CLASSINFO("DefaultProperty", "displayItem")
public:
    ViridityQtQuickDisplay(QObject *parent = NULL);
    virtual ~ViridityQtQuickDisplay();

    virtual void classBegin();
    virtual void componentComplete();

    QObject *displayItem();
    void setDisplayItem(QObject *displayItem);

    QString targetId() const;
    bool autoSize() const;

public slots:
    void setTargetId(QString targetId);
    void setAutoSize(bool autoSize);

private:
    PrivateQtQuickDisplayManager *manager_;

    QObject *displayItem_;
    QString targetId_;
    bool autoSize_;
};

#endif // VIRIDITYQTQUICKDISPLAYMANAGER_H

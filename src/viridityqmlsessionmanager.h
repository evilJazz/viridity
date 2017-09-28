#ifndef VIRIDITYQMLSESSIONMANAGER_H
#define VIRIDITYQMLSESSIONMANAGER_H

#include <QObject>
#include <QUrl>

#include <viriditysessionmanager.h>

#ifdef VIRIDITY_USE_QTQUICK1
#include <QDeclarativeEngine>
#include <QDeclarativeContext>
#else
#include <QQmlEngine>
#include <QQmlContext>
#endif

class ViridityQmlSessionManager : public AbstractViriditySessionManager
{
    Q_OBJECT
public:
    ViridityQmlSessionManager(const QUrl &globalLogicUrl, const QUrl &sessionLogicUrl, QObject *parent = 0);
    virtual ~ViridityQmlSessionManager();

#ifdef VIRIDITY_USE_QTQUICK1
    QDeclarativeEngine *engine();
#else
    QQmlEngine *engine();
#endif

    QObject *globalLogic();
    void startUpGlobalLogic() { globalLogic(); }

protected:
    void initSession(ViriditySession *session);

private:
#ifdef VIRIDITY_USE_QTQUICK1
    QDeclarativeEngine *engine_;
#else
    QQmlEngine *engine_;
#endif

    QObject *globalLogic_;

    QUrl globalLogicUrl_;
    QUrl sessionLogicUrl_;
};

#endif // VIRIDITYQMLSESSIONMANAGER_H

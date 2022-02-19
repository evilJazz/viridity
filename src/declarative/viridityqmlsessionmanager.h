#ifndef VIRIDITYQMLSESSIONMANAGER_H
#define VIRIDITYQMLSESSIONMANAGER_H

#include <QObject>
#include <QPointer>
#include <QUrl>

#include <viriditysessionmanager.h>

#ifdef VIRIDITY_USE_QTQUICK1
#include <QDeclarativeEngine>
#include <QDeclarativeContext>
#include <QDeclarativeComponent>
#else
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlContext>
#include <QtQml/QQmlComponent>
#endif

class ViridityQmlSessionManager : public AbstractViriditySessionManager
{
    Q_OBJECT
public:
#ifdef VIRIDITY_USE_QTQUICK1
    ViridityQmlSessionManager(const QUrl &globalLogicUrl, const QUrl &sessionLogicUrl, QObject *parent = 0, QDeclarativeContext *context = 0);
    ViridityQmlSessionManager(QDeclarativeComponent *globalLogicComponent, QDeclarativeComponent *sessionLogicComponent, QObject *parent = 0, QDeclarativeContext *context = 0);
#else
    ViridityQmlSessionManager(const QUrl &globalLogicUrl, const QUrl &sessionLogicUrl, QObject *parent = 0, QQmlContext *context = 0);
    ViridityQmlSessionManager(QQmlComponent *globalLogicComponent, QQmlComponent *sessionLogicComponent, QObject *parent = 0, QQmlContext *context = 0);
#endif

    virtual ~ViridityQmlSessionManager();

#ifdef VIRIDITY_USE_QTQUICK1
    QDeclarativeEngine *engine();
    QDeclarativeContext *context();
#else
    QQmlEngine *engine();
    QQmlContext *context();
#endif

    QObject *globalLogic();
    void startUpGlobalLogic() { globalLogic(); }

protected:
    void initSession(ViriditySession *session);

private:
#ifdef VIRIDITY_USE_QTQUICK1
    QPointer<QDeclarativeEngine> engine_;

    QPointer<QDeclarativeContext> externalContext_;
    QPointer<QDeclarativeComponent> externalGlobalLogicComponent_;
    QPointer<QDeclarativeComponent> externalSessionLogicComponent_;
#else
    QPointer<QQmlEngine> engine_;

    QPointer<QQmlContext> externalContext_;
    QPointer<QQmlComponent> externalGlobalLogicComponent_;
    QPointer<QQmlComponent> externalSessionLogicComponent_;
#endif

    QObject *globalLogic_;

    QUrl globalLogicUrl_;
    QUrl sessionLogicUrl_;

};

#endif // VIRIDITYQMLSESSIONMANAGER_H

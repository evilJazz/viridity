#ifndef VIRIDITYQMLWEBSERVER_H
#define VIRIDITYQMLWEBSERVER_H

#include <QObject>
#include <QPointer>

#include "viriditywebserver.h"
#include "viridityqmlsessionmanager.h"

#ifdef VIRIDITY_USE_QTQUICK1
#include <QDeclarativeEngine>
#include <QDeclarativeParserStatus>
#else
#include <QQmlEngine>
#include <QQmlParserStatus>
#endif

class ViridityQmlWebServer :
    public QObject,
#ifdef VIRIDITY_USE_QTQUICK1
    public QDeclarativeParserStatus
#else
    public QQmlParserStatus
#endif
{
    Q_OBJECT
#ifdef VIRIDITY_USE_QTQUICK1
    Q_INTERFACES(QDeclarativeParserStatus)
    Q_PROPERTY(QDeclarativeComponent *globalLogicComponent READ globalLogicComponent WRITE setGlobalLogicComponent NOTIFY globalLogicComponentChanged)
    Q_PROPERTY(QDeclarativeComponent *sessionLogicComponent READ sessionLogicComponent WRITE setSessionLogicComponent NOTIFY sessionLogicComponentChanged)
#else
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(QQmlComponent *globalLogicComponent READ globalLogicComponent WRITE setGlobalLogicComponent NOTIFY globalLogicComponentChanged)
    Q_PROPERTY(QQmlComponent *sessionLogicComponent READ sessionLogicComponent WRITE setSessionLogicComponent NOTIFY sessionLogicComponentChanged)
#endif
    Q_PROPERTY(QUrl globalLogicSource READ globalLogicSource WRITE setGlobalLogicSource NOTIFY globalLogicSourceChanged)
    Q_PROPERTY(QUrl sessionLogicSource READ sessionLogicSource WRITE setSessionLogicSource NOTIFY sessionLogicSourceChanged)

    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(QString bindAddress READ bindAddress WRITE setBindAddress NOTIFY bindAddressChanged)
    Q_PROPERTY(int port READ port WRITE setPort NOTIFY portChanged)

    Q_PROPERTY(ViridityQmlSessionManager *sessionManager READ sessionManager NOTIFY enabledChanged)

    Q_CLASSINFO("DefaultProperty", "globalLogicComponent")
public:
    explicit ViridityQmlWebServer(QObject *parent = nullptr);

    bool enabled() const { return enabled_; }
    void setEnabled(bool enabled);

    QString bindAddress() const;
    QHostAddress bindHostAddress() const;

    void setBindAddress(const QString &address);
    void setBindAddress(const QHostAddress &address);

    int port() const { return port_; }
    void setPort(int port);

    Q_INVOKABLE bool listen();
    Q_INVOKABLE bool close();

#ifdef VIRIDITY_USE_QTQUICK1
    QDeclarativeComponent *globalLogicComponent() { return globalLogic_.data(); }
    void setGlobalLogicComponent(QDeclarativeComponent *globalLogic);

    QDeclarativeComponent *sessionLogicComponent() { return sessionLogic_.data(); }
    void setSessionLogicComponent(QDeclarativeComponent *sessionLogic);

    QDeclarativeEngine *engine();
    QDeclarativeContext *context();
#else
    QQmlComponent *globalLogicComponent() { return globalLogicComponent_.data(); }
    void setGlobalLogicComponent(QQmlComponent *globalLogic);

    QQmlComponent *sessionLogicComponent() { return sessionLogicComponent_.data(); }
    void setSessionLogicComponent(QQmlComponent *sessionLogic);

    QQmlEngine *engine();
    QQmlContext *context();
#endif

    ViridityWebServer *webserver() { return webServer_.data(); }
    ViridityQmlSessionManager *sessionManager() { return sessionManager_.data(); }

    QUrl globalLogicSource() { return globalLogicSource_; }
    void setGlobalLogicSource(const QUrl &globalLogicSource);

    QUrl sessionLogicSource() { return sessionLogicSource_; }
    void setSessionLogicSource(const QUrl &sessionLogicSource);

    virtual void classBegin();
    virtual void componentComplete();

signals:
    void enabledChanged();
    void bindAddressChanged();
    void portChanged();

    void globalLogicComponentChanged();
    void sessionLogicComponentChanged();

    void globalLogicSourceChanged();
    void sessionLogicSourceChanged();

    void initialized();
    void opened();

    void closingDown();
    void closed();

private:
    bool enabled_;
    bool declarativeConstructionRunning_;
    QPointer<ViridityWebServer> webServer_;
    QPointer<ViridityQmlSessionManager> sessionManager_;
    QHostAddress bindToAddress_;
    int port_;

#ifdef VIRIDITY_USE_QTQUICK1
    QPointer<QDeclarativeComponent> globalLogicComponent_;
    QPointer<QDeclarativeComponent> sessionLogicComponent_;
    QPointer<QDeclarativeContext> context_;
#else
    QPointer<QQmlComponent> globalLogicComponent_;
    QPointer<QQmlComponent> sessionLogicComponent_;
    QPointer<QQmlContext> context_;
#endif

    QUrl globalLogicSource_;
    QUrl sessionLogicSource_;

    void updateState();

    void cleanUpContext();
    void cleanUp();
};

#endif // VIRIDITYQMLWEBSERVER_H

#include <QCoreApplication>
#include <QDebug>

#include "commandbridge.h"


static CommandBridge *instance = NULL;

CommandBridge &CommandBridge::singleton()
{
    if (!instance)
    {
        instance = new CommandBridge();
        instance->moveToThread(qApp->thread());
    }

    return *instance;
}

CommandBridge::CommandBridge(QObject *parent) :
    QObject(parent),
    response_(QString::null)
{
}

QString CommandBridge::handleCommandReady(const QString &id, const QString &command)
{
qDebug("CommandBridge got command %s for display ID %s", command.toUtf8().constData(), id.toUtf8().constData());
    emit commandReceived(id, command);

qDebug("CommandBridge answers with %s for display ID %s", response_.toUtf8().constData(), id.toUtf8().constData());
    QString result = response_;
    response_ = QString::null;
    return result;
}


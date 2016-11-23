#ifndef VIRIDITYCONNECTION_H
#define VIRIDITYCONNECTION_H

#include <QObject>

class ViridityConnection : public QObject
{
    Q_OBJECT
public:
    explicit ViridityConnection(QObject *parent = 0);

signals:

public slots:
};

#endif // VIRIDITYCONNECTION_H
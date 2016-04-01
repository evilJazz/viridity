#ifndef VIRIDITYDECLARATIVE_H
#define VIRIDITYDECLARATIVE_H

class ViridityDeclarative
{
public:
    /*!
     * Registers all types provided by Viridity with the declarative engine, i.e. the QML/QtQuick 1.x or 2.x engine.
     * Call this once before setting up the first instance of \a QDeclarativeEngine or \a QQmlEngine.
     */
    static void registerTypes();
};

#endif // VIRIDITYDECLARATIVE_H

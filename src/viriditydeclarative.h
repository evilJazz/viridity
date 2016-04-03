#ifndef VIRIDITYDECLARATIVE_H
#define VIRIDITYDECLARATIVE_H

/*!
 * Helper class that contains methods used in the declarative/QML context.
 */

class ViridityDeclarative
{
public:
    /*!
     * Registers all types provided by Viridity with the QML engine.
     * Call this once before setting up the first instance of \a QDeclarativeEngine or \a QQmlEngine.
     */
    static void registerTypes();
};

#endif // VIRIDITYDECLARATIVE_H

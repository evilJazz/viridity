#ifndef VIRIDITY_GLOBAL_H
#define VIRIDITY_GLOBAL_H

#include <QtCore/QtGlobal>

#ifdef VIRIDITY_STATIC
    #define VIRIDITY_EXPORT
#else
    #if defined(VIRIDITY_LIBRARY)
    #  define VIRIDITY_EXPORT Q_DECL_EXPORT
    #else
    #  define VIRIDITY_EXPORT Q_DECL_IMPORT
    #endif
#endif

#endif // VIRIDITY_GLOBAL_H

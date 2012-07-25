/* -*- mode: c++; tab-width: 4; c-basic-offset: 4 -*- */
/*
 * Copyright (C) 2006-2012 Andre Beckedorf <evilJazz _AT_ katastrophos _DOT_ net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef DEBUG_H_INCLUDED
#define DEBUG_H_INCLUDED

#include <time.h>

#include <qobject.h>
#include <qstring.h>
#ifdef QT4
#include <QTime>
#else
#include <qdatetime.h>
#endif

//#define DEBUG
//#undef DEBUG

void kaDebugEnterMethod(const QString &name);
void kaDebugExitMethod(const QString &name);
void kaDebug(const QString &msg);
void kaFatal(const QString &msg);
void kaPrintMemStat();

void kaProfileAddRecord(const QString &name, int elapsed);
void kaProfilePrintStat();
void kaProfileClearStat();

#ifdef DEBUG

class KaDebugGuard
{
public:
    KaDebugGuard(const QString string, bool timed = false, bool silence = false, bool profile = false) :
        string_(string),
        silence_(silence),
        profile_(profile),
        timer_(NULL)
    {
        if (timed)
        {
            timer_ = new QTime();
            timer_->start();
        }

        if (!silence_)
            kaDebugEnterMethod(string);
    }

    virtual ~KaDebugGuard()
    {
        if (timer_)
        {
            int elapsed = timer_->elapsed();

            if (profile_)
                kaProfileAddRecord(string_, elapsed);

            string_ += QString().sprintf(" - [Timing] %5d ms", elapsed);
            delete timer_;
            timer_ = NULL;
        }

        if (!silence_)
            kaDebugExitMethod(string_);
    }

private:
    QString string_;
    bool silence_;
    bool profile_;
    QTime *timer_;
};

  #define DOP(x...) x
  #define DHOP(x...)
#else
  #define DOP(x...)
  #define DHOP(x...)
#endif

#define DGUARDMETHOD DOP(volatile KaDebugGuard guard("[" + QString().sprintf("%s:%d", __FILE__, __LINE__) + "] : " + QString(__PRETTY_FUNCTION__)))
#define DGUARDMETHODTIMED DOP(volatile KaDebugGuard guard("[" + QString().sprintf("%s:%d", __FILE__, __LINE__) + "] : " + QString(__PRETTY_FUNCTION__), true))
#define DGUARDMETHODPROFILED DOP(volatile KaDebugGuard guard("[" + QString().sprintf("%s:%d", __FILE__, __LINE__) + "] : " + QString(__PRETTY_FUNCTION__), true, true, true))

#define DPROFILEPRINTSTAT DOP(kaProfilePrintStat())
#define DPROFILECLEARSTAT DOP(kaProfileClearStat())

#define DGUARDMETHODF(x...) DOP(volatile KaDebugGuard guard("[" + QString().sprintf("%s:%d", __FILE__, __LINE__) + "] : " + QString(__PRETTY_FUNCTION__) + " : " + QString().sprintf(x)))
#define DGUARDMETHODFTIMED(x...) DOP(volatile KaDebugGuard guard("[" + QString().sprintf("%s:%d", __FILE__, __LINE__) + "] : " + QString(__PRETTY_FUNCTION__) + " : " + QString().sprintf(x), true))

#define DHGUARDMETHOD DHOP(DGUARDMETHOD)
#define DHGUARDMETHODF(x...) DHOP(DGUARDMETHODF(x))

#define DHGUARDMETHODTIMED DHOP(DGUARDMETHODTIMED)
#define DHGUARDMETHODFTIMED(x...) DHOP(DGUARDMETHODFTIMED(x))

#define DENTERMETHOD DOP(kaDebugEnterMethod("[" + QString().sprintf("%s:%d", __FILE__, __LINE__) + "] : " + QString(__PRETTY_FUNCTION__)))
#define DEXITMETHOD DOP(kaDebugExitMethod("[" + QString().sprintf("%s:%d", __FILE__, __LINE__) + "] : " + QString(__PRETTY_FUNCTION__)))

#define DENTERMETHODF(x...) DOP(kaDebugEnterMethod("[" + QString().sprintf("%s:%d", __FILE__, __LINE__) + "] : " + QString(__PRETTY_FUNCTION__) + " : " + QString().sprintf(x)))
#define DEXITMETHODF(x...) DOP(kaDebugExitMethod("[" + QString().sprintf("%s:%d", __FILE__, __LINE__) + "] : " + QString(__PRETTY_FUNCTION__) + " : " + QString().sprintf(x)))

#define DHENTERMETHOD DHOP(DENTHERMETHOD)
#define DHEXITMETHOD DHOP(DEXITMETHOD)

#define DHENTERMETHODF(x...) DHOP(DENTERMETHODF(x))
#define DHEXITMETHODF(x...) DHOP(DEXITMETHODF(x))

#define DPRINTF(x...) DOP(kaDebug(QString().sprintf(x)))
#define DHPRINTF(x...) DHOP(DPRINTF(x))

#define DTIMERINIT(timername) DOP(QTime timername; timername.start())
#define DTIMERSTART(timername) DOP(timername.start())
#define DTIMERPRINT(timername, description) DOP(kaDebug(QString().sprintf("[Timing] %-30s: %5d ms", description, timername.elapsed())))

#define DMEMSTAT() DOP(kaPrintMemStat())

#define DASSERT(cond, text) ((!(cond)) ? kaFatal(QString().sprintf("Assertion failed: %s, %s [in %s %s:%d]", #cond, text, __PRETTY_FUNCTION__, __FILE__, __LINE__)) : qt_noop())

#define WTF(x...) kaFatal(QString().sprintf(x))

#endif /* DEBUG_H_INCLUDED */

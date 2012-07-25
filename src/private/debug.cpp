/* -*- mode: c++; tab-width: 4; c-basic-offset: 4 -*- */
/*
 * Copyright (C) 2006-2012 Andre Beckedorf <evilJazz _AT_ katatstrophos _DOT_ net>
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

#include "debug.h"

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

static int indentlevel = -1;

void kaDebugEnterMethod(const QString &name)
{
    qDebug("%*s>> %s", ++indentlevel * 3, "", (const char*)name.toUtf8());
}

void kaDebugExitMethod(const QString &name)
{
    qDebug("%*s<< %s", indentlevel-- * 3, "", (const char*)name.toUtf8());
}

void kaDebug(const QString &msg)
{
    qDebug("%*s%s", (indentlevel == -1 ? 0 : indentlevel * 3 + 3), "", (const char*)msg.toUtf8());
}

void kaFatal(const QString &msg)
{
    qFatal("FATAL: %*s%s", (indentlevel == -1 ? 0 : indentlevel * 3 + 3), "", (const char*)msg.toUtf8());
}

void kaPrintMemStat()
{
#ifdef Q_OS_LINUX
    char           buf[256];
    FILE         * file;
    unsigned int   pages;
    unsigned int   mem_size;

    snprintf(buf, sizeof(buf), "/proc/%d/statm", (unsigned int)getpid());

    if ((file = fopen( buf, "r" )) == NULL)
    {
        perror("open");
        return;
    }

    fgets(buf, sizeof(buf), file);

    fclose(file);

    sscanf(buf, "%u", &pages);
    mem_size = ((unsigned long)pages) * ((unsigned long)getpagesize());

    kaDebug(QString().sprintf("Memory used: %d bytes / %d kbytes", mem_size, mem_size / 1024));
#endif
}

#include <QHash>

struct ProfileRecord
{
    QString name;
    int timesCalled;
    qint64 totalRuntime;

    bool operator<(const ProfileRecord &other) const
    {
        return timesCalled < other.timesCalled;
    }
};

typedef QHash<QString, ProfileRecord> ProfileRecords;
ProfileRecords profileRecords;

void kaProfileAddRecord(const QString &name, int elapsed)
{
    ProfileRecord &record = profileRecords[name];
    if (record.name.isEmpty())
        record.name = name;

    ++record.timesCalled;
    record.totalRuntime += elapsed;
}

void kaProfilePrintStat()
{
    QList<ProfileRecord> records = profileRecords.values();
    qSort(records);

    kaDebug("");
    kaDebug("Profile stats:");
    kaDebug("=============================");
    for (int i = 0; i < records.count(); ++i)
    {
        const ProfileRecord &record = records.at(i);
        kaDebug(QString().sprintf("%5d -> %100s, times called: %6d,  total runtime: %8d ms,  average runtime: %8.2f ms",
            i + 1,
            record.name.toLatin1().constData(),
            record.timesCalled,
            record.totalRuntime,
            (float)record.totalRuntime / record.timesCalled
        ));
    }
    kaDebug("=============================");
    kaDebug("");
}

void kaProfileClearStat()
{
    profileRecords.clear();
}

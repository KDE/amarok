/*
 *   Copyright 2006-2007 Aaron Seigo <aseigo@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef DATAENGINE_P_H
#define DATAENGINE_P_H

#include <QQueue>
#include <QTime>
#include <KPluginInfo>

class QTime;

namespace Plasma
{

class DataEnginePrivate
{
    public:
        DataEnginePrivate(DataEngine *e, KService::Ptr service);
        ~DataEnginePrivate();
        DataContainer *source(const QString &sourceName, bool createWhenMissing = true);
        void connectSource(DataContainer *s, QObject *visualization, uint pollingInterval,
                           Plasma::IntervalAlignment align, bool immediateCall = true);
        DataContainer *requestSource(const QString &sourceName, bool *newSource = 0);
        void trimQueue();
        void queueUpdate();
        void internalUpdateSource(DataContainer*);

        /**
         * Reference counting method. Calling this method increases the count
         * by one.
         **/
        void ref();

        /**
         * Reference counting method. Calling this method decreases the count
         * by one.
         **/
        void deref();

        /**
         * Reference counting method. Used to determine if this DataEngine is
         * used.
         * @return true if the reference count is non-zero
         **/
        bool isUsed() const;

        DataEngine *q;
        KPluginInfo dataEngineDescription;
        int refCount;
        int updateTimerId;
        int minPollingInterval;
        QTime updateTimestamp;
        DataEngine::SourceDict sources;
        QQueue<DataContainer*> sourceQueue;
        QTimer *updateTimer;
        QString icon;
        uint limit;
        bool valid;
        DataEngineScript *script;
        QString engineName;
        Package *package;
};

} // Plasma namespace

#endif // multiple inclusion guard

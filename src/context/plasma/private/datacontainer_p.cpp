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

#include "datacontainer.h"
#include "datacontainer_p.h"

namespace Plasma
{

SignalRelay *DataContainerPrivate::signalRelay(const DataContainer *dc, QObject *visualization,
                                               uint pollingInterval,
                                               Plasma::IntervalAlignment align,
                                               bool immediateUpdate)
{
    QMap<uint, SignalRelay *>::const_iterator relayIt = relays.find(pollingInterval);
    SignalRelay *relay = 0;

    //FIXME what if we have two applets with the same interval and different alignment?
    if (relayIt == relays.end()) {
        relay = new SignalRelay(const_cast<DataContainer*>(dc), this,
                                pollingInterval, align, immediateUpdate);
        relays[pollingInterval] = relay;
    } else {
        relay = relayIt.value();
    }

    relayObjects[visualization] = relay;
    return relay;
}

bool DataContainerPrivate::hasUpdates()
{
    if (cached) {
        // SignalRelay needs us to pretend we did an update
        cached = false;
        return true;
    }

    return dirty;
}

SignalRelay::SignalRelay(DataContainer *parent, DataContainerPrivate *data, uint ival,
                         Plasma::IntervalAlignment align, bool immediateUpdate)
    : QObject(parent),
      dc(parent),
      d(data),
      m_interval(ival),
      m_align(align),
      m_resetTimer(true),
      m_queued(true)
{
    //kDebug() << "signal relay with time of" << m_timerId << "being set up";
    m_timerId = startTimer(immediateUpdate ? 0 : m_interval);
    if (m_align != Plasma::NoAlignment) {
        checkAlignment();
    }
}

int SignalRelay::receiverCount() const
{
    return receivers(SIGNAL(dataUpdated(QString,Plasma::DataEngine::Data)));
}

bool SignalRelay::isUnused()
{
    return receivers(SIGNAL(dataUpdated(QString,Plasma::DataEngine::Data))) < 1;
}

void SignalRelay::checkAlignment()
{
    int newTime = 0;

    QTime t = QTime::currentTime();
    if (m_align == Plasma::AlignToMinute) {
        int seconds = t.second();
        if (seconds > 2) {
            newTime = ((60 - seconds) * 1000) + 500;
        }
    } else if (m_align == Plasma::AlignToHour) {
        int minutes = t.minute();
        int seconds = t.second();
        if (minutes > 1 || seconds > 10) {
            newTime = ((60 - minutes) * 1000 * 60) +
                ((60 - seconds) * 1000) + 500;
        }
    }

    if (newTime) {
        killTimer(m_timerId);
        m_timerId = startTimer(newTime);
        m_resetTimer = true;
    }
}

void SignalRelay::checkQueueing()
{
    if (m_queued) {
        emit dataUpdated(dc->objectName(), d->data);
        m_queued = false;
        //TODO: should we re-align our timer at this point, to avoid
        //      constant queueing due to more-or-less constant time
        //      async update time? this might make sense for
        //      staggered accesses to the same source by multiple
        //      visualizations causing a minimumPollingInterval violation.
        //      it may not make sense for purely async-and-takes-a-while
        //      type operations (e.g. network fetching).
        //      we need more real world data before making such a change
        //      change
        //
        // killTimer(m_timerId);
        // m_timerId = startTime(m_interval);
    }
}

void SignalRelay::timerEvent(QTimerEvent *event)
{
    if (m_resetTimer) {
        killTimer(m_timerId);
        m_timerId = startTimer(m_interval);
        m_resetTimer = false;
    }

    if (m_align != Plasma::NoAlignment) {
        checkAlignment();
    }

    emit dc->updateRequested(dc);
    if (d->hasUpdates()) {
        //kDebug() << "emitting data updated directly" << d->data;
        emit dataUpdated(dc->objectName(), d->data);
        m_queued = false;
    } else {
        // the source wasn't actually updated; so let's put ourselves in the queue
        // so we get a dataUpdated() call when the data does arrive
        //kDebug() << "queued";
        m_queued = true;
    }
    event->accept();
}

} // Plasma namespace

#include "datacontainer_p.moc"


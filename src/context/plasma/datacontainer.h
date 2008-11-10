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

#ifndef PLASMA_DATACONTAINER_H
#define PLASMA_DATACONTAINER_H

#include <QtCore/QHash>
#include <QtCore/QObject>

#include <plasma/plasma_export.h>
#include <plasma/dataengine.h>

namespace Plasma
{

class DataContainerPrivate;

/**
 * @class DataContainer plasma/datacontainer.h <Plasma/DataContainer>
 *
 * @brief A set of data exported via a DataEngine
 *
 * Plasma::DataContainer wraps the data exported by a DataEngine
 * implementation, providing a generic wrapper for the data.
 *
 * A DataContainer may have zero or more associated pieces of data which
 * are keyed by strings. The data itself is stored as QVariants. This allows
 * easy and flexible retrieval of the information associated with this object
 * without writing DataContainer or DataEngine specific code in visualizations.
 *
 * If you are creating your own DataContainer objects (and are passing them to
 * DataEngine::addSource()), you normally just need to listen to the
 * updateRequested() signal (as well as any other methods you might have of
 * being notified of new data) and call setData() to actually update the data.
 * Then you need to either trigger the scheduleSourcesUpdated signal of the
 * parent DataEngine or call checkForUpdate() on the DataContainer.
 *
 * You also need to set a suitable name for the source with setObjectName().
 * See DataEngine::addSource() for more information.
 *
 * Note that there is normally no need to subclass DataContainer, except as
 * a way of encapsulating the data retreival for a source, since all notifications
 * are done via signals rather than virtual methods.
 **/
class PLASMA_EXPORT DataContainer : public QObject
{
    friend class DataEngine;
    friend class DataEnginePrivate;
    Q_OBJECT

    public:
        /**
         * Constructs a default DataContainer that has no name or data
         * associated with it
         **/
        explicit DataContainer(QObject *parent = 0);
        virtual ~DataContainer();

        /**
         * Returns the data for this DataContainer
         **/
        const DataEngine::Data data() const;

        /**
         * Set a value for a key.
         *
         * This also marks this source as needing to signal an update.
         *
         * If you call setData() directly on a DataContainer, you need to
         * either trigger the scheduleSourcesUpdated() slot for the
         * data engine it belongs to or call checkForUpdate() on the
         * DataContainer.
         *
         * @param key a string used as the key for the data
         * @param value a QVariant holding the actual data. If a null or invalid
         *              QVariant is passed in and the key currently exists in the
         *              data, then the data entry is removed
         **/
        void setData(const QString &key, const QVariant &value);

        /**
         * Removes all data currently associated with this source
         *
         * If you call removeAllData() on a DataContainer, you need to
         * either trigger the scheduleSourcesUpdated() slot for the
         * data engine it belongs to or call checkForUpdate() on the
         * DataContainer.
         **/
        void removeAllData();

        /**
         * @return true if the visualization is currently connected
         */
        bool visualizationIsConnected(QObject *visualization) const;

        /**
         * Connects an object to this DataContainer.
         *
         * May be called repeatedly for the same visualization without
         * side effects
         *
         * @param visualization the object to connect to this DataContainer
         * @param pollingInterval the time in milliseconds between updates
         **/
        void connectVisualization(QObject *visualization, uint pollingInterval,
                                  Plasma::IntervalAlignment alignment);

    public Q_SLOTS:
        /**
         * Disconnects an object from this DataContainer.
         *
         * Note that if this source was created by DataEngine::sourceRequestEvent(),
         * it will be deleted by DataEngine once control returns to the event loop.
         **/
        void disconnectVisualization(QObject *visualization);

    Q_SIGNALS:
        /**
         * Emitted when the data has been updated, allowing visualizations to
         * reflect the new data.
         *
         * Note that you should not normally emit this directly.  Instead, use
         * checkForUpdates() or the DataEngine::scheduleSourcesUpdated() slot.
         *
         * @param source the objectName() of the DataContainer (and hence the name
         *               of the source) that updated its data
         * @param data   the updated data
         **/
        void dataUpdated(const QString &source, const Plasma::DataEngine::Data &data);

        /**
         * Emitted when the last visualization is disconnected.
         *
         * Note that if this source was created by DataEngine::sourceRequestEvent(),
         * it will be deleted by DataEngine once control returns to the event loop
         * after this signal is emitted.
         *
         * @param source  the name of the source that became unused
         **/
        void becameUnused(const QString &source);

        /**
         * Emitted when an update is requested.
         *
         * If a polling interval was passed connectVisualization(), this signal
         * will be emitted every time the interval expires.
         *
         * Note that if you create your own DataContainer (and pass it to
         * DataEngine::addSource()), you will need to listen to this signal
         * and refresh the data when it is triggered.
         *
         * @param source  the datacontainer the update was requested for.  Useful
         *                for classes that update the data for several containers.
         **/
        void updateRequested(DataContainer *source);

    protected:
        /**
         * Checks whether any data has changed and, if so, emits dataUpdated().
         **/
        void checkForUpdate();

        /**
         * Returns how long ago, in msecs, that the data in this container was last updated.
         *
         * This is used by DataEngine to compress updates that happen more quickly than the
         * minimum polling interval by calling setNeedsUpdate() instead of calling
         * updateSourceEvent() immediately.
         **/
        uint timeSinceLastUpdate() const;

        /**
         * Indicates that the data should be treated as dirty the next time hasUpdates() is called.
         *
         * This is needed for the case where updateRequested() is triggered but we don't want to
         * update the data immediately because it has just been updated.  The second request won't
         * be fulfilled in this case, because we never updated the data and so never called
         * checkForUpdate().  So we claim it needs an update anyway.
         **/
        void setNeedsUpdate(bool update = true);

    protected Q_SLOTS:
        /**
         * Check if the DataContainer is still in use.
         *
         * If not the signal "becameUnused" will be emitted.
         *
         * Warning: The DataContainer may be invalid after calling this function, because a listener
         * to becameUnused() may have deleted it.
         **/
        void checkUsage();

    private:
        friend class SignalRelay;
        DataContainerPrivate *const d;
};

} // Plasma namespace

#endif // multiple inclusion guard

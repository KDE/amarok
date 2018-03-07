/****************************************************************************************
 * Copyright (c) 2010 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_LOGGER_H
#define AMAROK_LOGGER_H

#include "core/amarokcore_export.h"

#include <QMetaType>
#include <QObject>

class KJob;
class QNetworkReply;

namespace Amarok
{
    /**
      * This interface provides methods that allow backend components to notify the user.
      * Users of this class may not make assumptions about the kind of notifications that
      * will be sent to the user.
      *
      * The class name is up for discussion btw.
      */
    class AMAROK_CORE_EXPORT Logger
    {
    public:

        enum MessageType { Information, Warning, Error };

        Logger() {}
        virtual ~Logger() {}

    public Q_SLOTS:

        /**
          * Informs the user about the progress of a job, i.e. a download job.
          * At the very least, the user is notified about the start and end of the job.
          *
          * @param job The job whose progress should be monitored
          * @param text An additional text that will be part of the notification
          * @param obj The object that will be called if the user cancels the job. If not set, the job will not be cancellable
          * @param slot The slot on the given object that will be called if the user cancels the job. No slot will be called if not set.
          * The signal will be emitted from the GUI thread. The receiver may not make assumptions about the sender
          * @param type The Qt connection type to use for the connection to the receiving slot. Defaults to Qt::AutoConnection
          */
        virtual void newProgressOperation( KJob *job, const QString &text, QObject *obj = 0, const char *slot = 0, Qt::ConnectionType type = Qt::AutoConnection ) = 0;

        /**
          * Informs the user about the progress of a network request.
          * At the very least, the user is notified about the start and end of the request.
          *
          * @param reply The network reply object whose progress should be monitored
          * @param text An additional text that will be part of the notification
          * @param obj The object that will be called if the user cancels the network request. If not set, the progress will not be cancellable
          * @param slot The slot on the given object that will be called if the user cancels the network request. No slot will be called if not set.
          * The signal will be emitted from the GUI thread. The receiver may not make assumptions about the sender
          * @param type The Qt connection type to use for the connection to the receiving slot. Defaults to Qt::AutoConnection
          */
        virtual void newProgressOperation( QNetworkReply *reply, const QString &text, QObject *obj = 0, const char *slot = 0, Qt::ConnectionType type = Qt::AutoConnection ) = 0;

        /**
         * Informs the user about the progress of a generic QObject
         *
         * @param sender The object sending the required signals. This sender must emit singals
         *        incrementProgress() and endProgressOperation() and optionally totalSteps().
         * @param text An additional text that will be part of the notification
         * @param maximum The maximum value of the progress operation
         * @param obj The object that will be called if the user cancels the network request. If not
         *        set, the progress will not be cancellable
         * @param slot The slot on the given object that will be called if the user cancels the
         *             network request. No slot will be called if not set.
         * The signal will be emitted from the GUI thread. The receiver may not make assumptions
         * about the sender
         * @param type The Qt connection type to use for the connection to the receiving slot.
         *             Defaults to Qt::AutoConnection
         */
        virtual void newProgressOperation( QObject *sender, const QString &text, int maximum = 100,
                                           QObject *obj = 0, const char *slot = 0,
                                           Qt::ConnectionType type = Qt::AutoConnection ) = 0;

        /**
          * Sends a notification to the user.
          * This method will send a notification containing the given text to the user.
          *
          * @param text The text that the notification will contain
          */
        virtual void shortMessage( const QString &text ) = 0;

        /**
          * Send a notification to the user with an additional context.
          * A notification will be send to the user containing the given text. Additionally, it will convey the context given by @p type.
          * @param text The text that the notification will contain
          * @param type The context of the notification
          */
        virtual void longMessage( const QString &text, MessageType type = Information ) = 0;
    };
}

#endif

/****************************************************************************************
 * Copyright (c) 2009 Alejandro Wainzinger <aikawarazuni@gmail.com>                     *
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

#ifndef MEDIADEVICEHANDLER_CAPABILITY_H
#define MEDIADEVICEHANDLER_CAPABILITY_H

#include "core-impl/collections/mediadevicecollection/support/mediadevicecollection_export.h"

#include <QObject>

/**
 * Base class for all media device Capabilities
 *
 * Following rules apply when working with capabilities:
 *  * Capabilities get deleted along their media device handler. Therefore use
 *    QPointer everywhere to detect that.
 *  * the one who creates capability using create<Type>() must deleteLater() it when no
 *    longer used.
 */
namespace Handler
{
    class MEDIADEVICECOLLECTION_EXPORT Capability : public QObject
    {
        Q_OBJECT

        public:
            //add additional capabilities here
            enum Type { Unknown = 0
                        , Readable = 1 // can read from device
                        , Writable = 2 // can write to device
                        , Playlist = 3 // can read/write playlists
                        , Artwork = 4 // can read/write artwork
                        , Podcast = 5 // can read/write podcasts
                      };
            Q_ENUM( Type )

            /**
             * @param handler should be set to associated MediaDeviceHandler or Collection.
             *
             * The capability sets its parent to handler, so that it can be guaranteed that
             * the handler is valid for Capability's lifetime.
             */
            explicit Capability( QObject *handler );
            ~Capability() override;

        Q_SIGNALS:
            /**
             * Signals that parent of this object should be set to @param parent
             */
            void signalSetParent( QObject *parent );

        private Q_SLOTS:
            /**
             * Simply calls setParent( parent ); needed for cases where moveToThread() is
             * called in constructor - setting parent needs to be done in the new thread.
             */
            void slotSetParent( QObject *parent );
    };
}

#endif

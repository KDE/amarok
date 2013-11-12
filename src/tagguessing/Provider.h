/****************************************************************************************
 * Copyright (c) 2013 Vedant Agarwala <vedant.kota@gmail.com>                           *
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

#ifndef TAGGUESSING_PROVIDER_H
#define TAGGUESSING_PROVIDER_H

#include "core/meta/forward_declarations.h"
#include "network/NetworkAccessManagerProxy.h"

#include <QPointer>

namespace TagGuessing{

    /**
     * Base abstract class for all "providers" of the Tag Guessing
     * Every Provider will be created on the main thread by the TagGuessing::Finder
     * Providers will have to create their own threads/jobs as required so as not to block the main thread
     */
    class AMAROK_EXPORT Provider : public QObject
    {
        Q_OBJECT

    public:
        virtual ~Provider();

        virtual bool isRunning() const = 0;

    signals:
        void progressStep();
        void trackFound( const Meta::TrackPtr &track, const QVariantMap &tags );
        void done();

    public slots:
        virtual void run( const Meta::TrackList &tracks ) = 0;

        virtual void lookUpByPUID( const Meta::TrackPtr &track, const QString &puid ) = 0;

    protected:
        /**
         * This class should never be instantized. Only subclasses can call the constructor.
         * Also, make sure this object is created on the main thread
         */
        Provider( QObject *parent );

    };

    typedef QPointer<Provider> ProviderPtr; // TODO check the need to change this to a, say, QExplicitlySharedDataPointer
    typedef QPair<Meta::TrackPtr, QVariantMap> TrackInfo;
}

#endif // TAGGUESSING_PROVIDER_H

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

#ifndef FINDER_H
#define FINDER_H

#include "Provider.h"
#include "Meta.h"

#include <QObject>
#include <QList>

namespace TagGuessing {

    /**
     * Handles the finding of tags from the available providers.
     * It calls the various methods of Provider objects, always on the main thread
     * To create a new Provider, subclass Provider and add edit the private function Finder::addProviders()
     */
    class Finder : public QObject
    {
        Q_OBJECT

    public:
        Finder( QObject *parent );
        ~Finder();
        bool isRunning();
        int getProgressBarSizeMultiplier() const;
    signals:

    public slots:
        void run( const Meta::TrackList &tracks );

        void lookUpByPUID( const Meta::TrackPtr &track, const QString &puid );

    private:
        void addProviders();
        QList <ProviderPtr> m_providers;
    };
} // Namespace TagGuessing

#endif // FINDER_H

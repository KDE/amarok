/****************************************************************************************
 * Copyright (c) 2008 Daniel Jones <danielcjones@gmail.com>                             *
 * Copyright (c) 2009 Leo Franchi <lfranchi@kde.org>                                    *
 * Copyright (c) 2010, 2011 Ralf Engels <ralf-engels@gmx.de>                            *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_IFELSEBIAS_H
#define AMAROK_IFELSEBIAS_H

#include "dynamic/Bias.h"
#include "dynamic/BiasFactory.h"

namespace Dynamic
{

    /** The if-else bias works like an or bias.
        If the first sub-bias doesn't return a valid result it will go to the second
        sub-bias and so on.
    */
    class IfElseBias : public OrBias
    {
        Q_OBJECT

        public:
            IfElseBias();

            static QString sName();
            QString name() const override;
            QString toString() const override;

            void paintOperator( QPainter* painter, const QRect &rect, Dynamic::AbstractBias* bias ) override;

            TrackSet matchingTracks( const Meta::TrackList& playlist,
                                             int contextCount, int finalCount,
                                             const TrackCollectionPtr &universe ) const override;

            void resultReceived( const Dynamic::TrackSet &tracks ) override;

        private:
            /** Removes duplicate tracks from m_tracks
                Removes duplicate tracks (depending on m_position, m_playlist and
                AmarokConfig::dynamicDuplicates()) from m_tracks
            */
            void removeDuplicate() const;

            // buffered by matchingTracks
            mutable Meta::TrackList m_playlist;
            mutable int m_contextCount;
            mutable int m_finalCount;
            mutable Dynamic::TrackCollectionPtr m_universe;

            Q_DISABLE_COPY(IfElseBias)
    };


    class AMAROK_EXPORT IfElseBiasFactory : public Dynamic::AbstractBiasFactory
    {
        public:
            QString i18nName() const override;
            QString name() const override;
            QString i18nDescription() const override;
            BiasPtr createBias() override;
    };

}


#endif


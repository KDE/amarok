/****************************************************************************************
 * Copyright (c) 2008 Daniel Jones <danielcjones@gmail.com>                             *
 * Copyright (c) 2009 Leo Franchi <lfranchi@kde.org>                                    *
 * Copyright (c) 2010, 2011 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#include "Bias.h"
#include "BiasFactory.h"

namespace Dynamic
{

    /** The if-else bias works like an or bias.
        If the first sub-bias doesn't return a valid result it
        will go to the second sub-bias and so on.
    */
    class IfElseBias : public OrBias
    {
        Q_OBJECT

        public:
            IfElseBias( bool empty = false );
            IfElseBias( QXmlStreamReader *reader );

            virtual void toXml( QXmlStreamWriter *writer ) const;

            static QString sName();
            virtual QString name() const;
            virtual QString toString() const;

            virtual void paintOperator( QPainter* painter, const QRect &rect, Dynamic::AbstractBias* bias );

            virtual TrackSet matchingTracks( int position,
                                             const Meta::TrackList& playlist,
                                             int contextCount,
                                             const TrackCollectionPtr universe ) const;

            virtual void resultReceived( const Dynamic::TrackSet &tracks );

        private:

            // buffered by matchingTracks
            mutable int m_position;
            mutable Meta::TrackList m_playlist;
            mutable int m_contextCount;
            mutable Dynamic::TrackCollectionPtr m_universe;

            Q_DISABLE_COPY(IfElseBias)
    };


    class AMAROK_EXPORT IfElseBiasFactory : public Dynamic::AbstractBiasFactory
    {
        public:
            virtual QString i18nName() const;
            virtual QString name() const;
            virtual QString i18nDescription() const;
            virtual BiasPtr createBias();
            virtual BiasPtr createBias( QXmlStreamReader *reader );
    };

}


#endif


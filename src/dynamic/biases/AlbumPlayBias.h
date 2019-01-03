/****************************************************************************************
 * Copyright (c) 2011 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#ifndef AMAROK_ALBUMPLAY_BIAS_H
#define AMAROK_ALBUMPLAY_BIAS_H

#include "dynamic/Bias.h"
#include "dynamic/BiasFactory.h"
#include "widgets/MetaQueryWidget.h"

namespace Dynamic
{
    class AlbumPlayBias : public RandomBias
    {
        Q_OBJECT

        public:
            enum FollowType
            {
                DirectlyFollow,
                Follow,
                DontCare
            };

            AlbumPlayBias();

            void fromXml( QXmlStreamReader *reader ) override;
            void toXml( QXmlStreamWriter *writer ) const override;

            static QString sName();
            QString name() const override;
            QString toString() const override;

            QWidget* widget( QWidget* parent = nullptr ) override;

            TrackSet matchingTracks( const Meta::TrackList& playlist,
                                             int contextCount, int finalCount,
                                             const TrackCollectionPtr universe ) const override;

            bool trackMatches( int position,
                                       const Meta::TrackList& playlist,
                                       int contextCount ) const override;

            FollowType follow() const;
            void setFollow( FollowType value );

        protected Q_SLOTS:
            void selectionChanged( int );

        protected:
            static QString nameForFollow( FollowType match );
            static FollowType followForName( const QString &name );
            virtual bool sameTrack( Meta::TrackPtr track1, Meta::TrackPtr track2 ) const;

            FollowType m_follow;

        private:
            Q_DISABLE_COPY(AlbumPlayBias)
    };


    class AMAROK_EXPORT AlbumPlayBiasFactory : public Dynamic::AbstractBiasFactory
    {
        public:
            QString i18nName() const override;
            QString name() const override;
            QString i18nDescription() const override;
            BiasPtr createBias() override;
    };

}

#endif


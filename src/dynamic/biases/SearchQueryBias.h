/****************************************************************************************
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

#ifndef AMAROK_SEARCHQUERYBIAS_H
#define AMAROK_SEARCHQUERYBIAS_H

#include "shared/amarok_export.h"
#include "Bias.h"
#include "BiasFactory.h"
#include "TagMatchBias.h"

namespace Dynamic
{

    /** A bias that accepts a query string from the search bar.
        This query would be straight forward SimpleMatchBias except
        that I added the "unique" track functionality.
        This adds a lot complexity to it.
    */
    class SearchQueryBias : public SimpleMatchBias
    {
        Q_OBJECT

        public:
            SearchQueryBias( QString filter = QString("genre: Rock") );

            virtual void fromXml( QXmlStreamReader *reader );
            virtual void toXml( QXmlStreamWriter *writer ) const;

            static QString sName();
            virtual QString name() const;
            virtual QString toString() const;

            virtual QWidget* widget( QWidget* parent = 0 );

            Dynamic::TrackSet matchingTracks( int position,
                                              const Meta::TrackList& playlist,
                                              int contextCount,
                                              Dynamic::TrackCollectionPtr universe ) const;

            void updateFinished();

            bool trackMatches( int position,
                               const Meta::TrackList& playlist,
                               int contextCount ) const;

            bool unique() const;
            QString filter() const;

        public slots:
            void setUnique( bool value );
            void setFilter( const QString &filter );

        protected slots:
            virtual void newQuery();

        private:
            bool m_unique;
            QString m_filter;

            // tracks before the position at the last "matchingTracks" call
            mutable QStringList m_existingTracks;

            Q_DISABLE_COPY(SearchQueryBias)
    };


    class AMAROK_EXPORT SearchQueryBiasFactory : public Dynamic::AbstractBiasFactory
    {
        public:
            virtual QString i18nName() const;
            virtual QString name() const;
            virtual QString i18nDescription() const;
            virtual BiasPtr createBias();
    };

}

#endif


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

#ifndef AMAROK_METATAGBIAS_H
#define AMAROK_METATAGBIAS_H

#include "amarok_export.h"
#include "core/collections/QueryMaker.h"
#include "dynamic/Bias.h"
#include "dynamic/BiasFactory.h"
#include "widgets/MetaQueryWidget.h"

#include <QDateTime>

class QWidget;
class QCheckBox;

namespace Dynamic
{

    class TagMatchBias;

    /** An abstract bias that will check matching tracks agains the results from a query maker.
        You can use this base class for writing your own biases.
        In all cases you have to implement newQuery which creates a
        QueryMaker and starts it.
    */
    class AMAROK_EXPORT SimpleMatchBias : public AbstractBias
    {
        Q_OBJECT

        public:
            SimpleMatchBias();

            virtual void fromXml( QXmlStreamReader *reader );
            virtual void toXml( QXmlStreamWriter *writer ) const;

            virtual TrackSet matchingTracks( const Meta::TrackList& playlist,
                                             int contextCount, int finalCount,
                                             const TrackCollectionPtr universe ) const;

            virtual bool trackMatches( int position,
                                       const Meta::TrackList& playlist,
                                       int contextCount ) const;

            /** Is the result inverted (e.g. does NOT contain) */
            bool isInvert() const;
            void setInvert( bool value );

        public Q_SLOTS:
            virtual void invalidate();

        protected Q_SLOTS:
            /** Called when we get new uids from the query maker */
            virtual void updateReady( QStringList uids );

            /** Called when the querymaker is finished */
            virtual void updateFinished();

            /** Creates a new query to get matching tracks. */
            virtual void newQuery() = 0;

        protected:
            MetaQueryWidget::Filter m_filter;

            mutable QScopedPointer<Collections::QueryMaker> m_qm;

            mutable TrackSet m_tracks;

            /** Time when we got the query result.
                We don't want results to be valid forever.
                Fix 311906 */
            QDateTime m_tracksTime;

            /** Returns true if the tracks in m_tracks reflect the
                current filter.
                Tracks are only valid for a short time.
            */
            bool tracksValid() const;

            /** The results are reported inverted (tracks that not match) */
            bool m_invert;

        private:
            Q_DISABLE_COPY(SimpleMatchBias)
    };

    /** A bias widget for the TagMatchBias */
    class TagMatchBiasWidget : public QWidget
    {
        Q_OBJECT

        public:
            explicit TagMatchBiasWidget( Dynamic::TagMatchBias* bias, QWidget* parent = 0 );

        private Q_SLOTS:
            void syncControlsToBias();
            void syncBiasToControls();

        private:
            QCheckBox* m_invertBox;
            MetaQueryWidget* m_queryWidget;

            Dynamic::TagMatchBias* m_bias;
    };


    /** A bias that matches tracks against a MetaQueryWidget filter. */
    class TagMatchBias : public SimpleMatchBias
    {
        Q_OBJECT

        public:
            TagMatchBias();

            virtual void fromXml( QXmlStreamReader *reader );
            virtual void toXml( QXmlStreamWriter *writer ) const;

            static QString sName();
            virtual QString name() const;
            virtual QString toString() const;

            virtual QWidget* widget( QWidget* parent = 0 );

            virtual bool trackMatches( int position,
                                       const Meta::TrackList& playlist,
                                       int contextCount ) const;


            MetaQueryWidget::Filter filter() const;
            void setFilter( const MetaQueryWidget::Filter &filter );

        protected Q_SLOTS:
            virtual void newQuery();

        protected:
            static QString nameForCondition( MetaQueryWidget::FilterCondition cond );
            static MetaQueryWidget::FilterCondition conditionForName( const QString &name );

            bool matches( const Meta::TrackPtr &track ) const;

        private:
            Q_DISABLE_COPY(TagMatchBias)
    };


    class AMAROK_EXPORT TagMatchBiasFactory : public Dynamic::AbstractBiasFactory
    {
        public:
            virtual QString i18nName() const;
            virtual QString name() const;
            virtual QString i18nDescription() const;
            virtual BiasPtr createBias();
    };

}

#endif


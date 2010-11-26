/****************************************************************************************
 * Copyright (c) 2008-2010 Soren Harward <stharward@gmail.com>                          *
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

#ifndef APG_TAGMATCH_CONSTRAINT
#define APG_TAGMATCH_CONSTRAINT

#include "ui_TagMatchEditWidget.h"

#include "Matching.h"

#include "core/meta/Meta.h"
#include "widgets/MetaQueryWidget.h"

#include <QAbstractListModel>
#include <QBitArray>
#include <QHash>
#include <QString>
#include <QVariant>

class Constraint;
class ConstraintFactoryEntry;

namespace Collections {
    class QueryMaker;
}

namespace ConstraintTypes {

    class TagMatchFieldsModel;

    /* Puts tracks with the specified tag into the playlist.  "Tag" is used a
     * bit loosely here; a more accurate term would probably be "metadata",
     * since the matchable properties include parameters like "rating" and
     * "last played" that aren't in the actual file that corresponds to the
     * track being played. -- sth */

    class TagMatch : public MatchingConstraint
    {
        Q_OBJECT

        public:
            static Constraint* createFromXml(QDomElement&, ConstraintNode*);
            static Constraint* createNew(ConstraintNode*);
            static ConstraintFactoryEntry* registerMe();

            virtual QWidget* editWidget() const;
            virtual void toXml(QDomDocument&, QDomElement&) const;

            virtual QString getName() const;

            virtual Collections::QueryMaker* initQueryMaker(Collections::QueryMaker*) const;
            virtual double satisfaction(const Meta::TrackList&);
            virtual double deltaS_insert(const Meta::TrackList&, const Meta::TrackPtr, const int) const;
            virtual double deltaS_replace(const Meta::TrackList&, const Meta::TrackPtr, const int) const;
            virtual double deltaS_delete(const Meta::TrackList&, const int) const;
            virtual double deltaS_swap(const Meta::TrackList&, const int, const int) const;
            virtual void insertTrack(const Meta::TrackList&, const Meta::TrackPtr, const int);
            virtual void replaceTrack(const Meta::TrackList&, const Meta::TrackPtr, const int);
            virtual void deleteTrack(const Meta::TrackList&, const int);
            virtual void swapTracks(const Meta::TrackList&, const int, const int);

            virtual ConstraintNode::Vote* vote( const Meta::TrackList&, const Meta::TrackList& ) const;

#ifndef KDE_NO_DEBUG_OUTPUT
            virtual void audit( const Meta::TrackList& ) const;
#endif

            // Implementation of MatchingConstraint virtuals
            const QBitArray whatTracksMatch( const Meta::TrackList& );
            int constraintMatchType() const;

        private slots:
            void setFilter( const MetaQueryWidget::Filter &filter );
            void setInvert( bool );
            void setStrictness( int );

        private:
            TagMatch( QDomElement&, ConstraintNode* );
            TagMatch( ConstraintNode* );
            ~TagMatch();

            // constraint parameters
            bool m_invert;
            double m_strictness;

            MetaQueryWidget::Filter m_filter;

            // internal state data
            double m_satisfaction;

            // convenience functions
            QString valueToString() const;

            // int conditionToComparison( MetaQueryWidget::FilterCondition condition ) const;
            bool matches( const Meta::TrackPtr ) const; // match values are fuzzily calculated
            mutable QHash<Meta::TrackPtr, bool> m_matchCache; // internal cache for per-track true/false data

            /** Support class that does fuzzy comparisons for the TagMatch. */
            class Comparer {
                public:
                    static int rangeNum( const double strictness, const qint64 field );
                    static double compareNum( const double test,
                                              MetaQueryWidget::FilterCondition condition,
                                              double target,
                                              const double strictness,
                                              const qint64 field );
                    static double compareStr( const QString& test,
                                              MetaQueryWidget::FilterCondition condition,
                                              const QString& target );

                    static double compareLabels( const Meta::TrackPtr t,
                                                 MetaQueryWidget::FilterCondition condition,
                                                 const QString& target );
                protected:
                    /** Returns the normal fuzzy range for a field.
                        e.g. For valScore (range 0-100) returns 20.0 */
                    static double fieldWeight( qint64 field );
                    static double fuzzyProb( const double a, const double b,
                                             const double strictness, const double weight );
            };

    };

    class TagMatchEditWidget : public QWidget
    {
        Q_OBJECT

        public:
            TagMatchEditWidget( const MetaQueryWidget::Filter &filter, bool invert, int strictness);
            ~TagMatchEditWidget();

        signals:
            void filterChanged( const MetaQueryWidget::Filter &filter );
            void invertChanged( bool );
            void strictnessChanged( int );

        private slots:
            void slotUpdateStrictness();

        private:
            Ui::TagMatchEditWidget ui;
    };

} // namespace ConstraintTypes

typedef QPair<int,int> DateRange;
Q_DECLARE_METATYPE( DateRange )

#endif

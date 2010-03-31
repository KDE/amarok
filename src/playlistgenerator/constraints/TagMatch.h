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

#ifndef APG_GLOBALMATCH_CONSTRAINT
#define APG_GLOBALMATCH_CONSTRAINT

#include "ui_TagMatchEditWidget.h"

#include "Matching.h"

#include "core/meta/Meta.h"

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

    /* Puts tracks with the specified tag into the playlist.  "Tag" is used a
     * bit loosely here; a more accurate term would probably be "metadata",
     * since the matchable properties include parameters like "rating" and
     * "last played" that aren't in the actual file that corresponds to the
     * track being played. -- sth */

    class TagMatch : public MatchingConstraint {
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

            // Implementation of MatchingConstraint virtuals
            const QBitArray whatTracksMatch( const Meta::TrackList& );
            int constraintMatchType() const;

        private slots:
            void setComparison( int );
            void setField( const QString& );
            void setInvert( bool );
            void setStrictness( int );
            void setValue( const QVariant& );

        private:
            TagMatch( QDomElement&, ConstraintNode* );
            TagMatch( ConstraintNode* );

            // constraint parameters
            int m_comparison;
            QString m_field;
            bool m_invert;
            double m_strictness;
            QVariant m_value;

            // internal state data
            double m_satisfaction;

            // convenience functions
            double dateComparison( uint ) const;
            double labelComparison( Meta::TrackPtr ) const;
            QString comparisonToString() const;
            QString valueToString() const;

            bool matches( const Meta::TrackPtr ) const; // match values are fuzzily calculated
            mutable QHash<QString, bool> m_matchCache; // internal cache for per-track true/false data
    };

    class TagMatchEditWidget : public QWidget {
        Q_OBJECT

        public:
            TagMatchEditWidget( const int, const QString&, const bool, const int, const QVariant& );

        signals:
            void comparisonChanged( int );
            void fieldChanged( const QString& );
            void invertChanged( bool );
            void strictnessChanged( int );
            void valueChanged( const QVariant& );
            void updated();

        private slots:
            // comparison
            void on_comboBox_ComparisonDate_currentIndexChanged( int );
            void on_comboBox_ComparisonInt_currentIndexChanged( int );
            void on_comboBox_ComparisonRating_currentIndexChanged( int );
            void on_comboBox_ComparisonString_currentIndexChanged( int );
            void on_comboBox_ComparisonTime_currentIndexChanged( int );

            // field
            void on_comboBox_Field_currentIndexChanged( const QString& );

            // invert
            void on_checkBox_Invert_clicked( bool );

            // strictness
            void on_slider_StrictnessDate_valueChanged( int );
            void on_slider_StrictnessInt_valueChanged( int );
            void on_slider_StrictnessRating_valueChanged( int );
            void on_slider_StrictnessTime_valueChanged( int );

            // value
            void on_kdatewidget_DateSpecific_changed( const QDate& );
            void on_spinBox_ValueDateValue_valueChanged( int );
            void on_comboBox_ValueDateUnit_currentIndexChanged( int );
            void on_spinBox_ValueInt_valueChanged( int );
            void on_lineEdit_StringValue_textChanged( const QString& );
            void on_rating_RatingValue_ratingChanged( int );
            void on_timeEdit_TimeValue_timeChanged( const QTime& );

        private:
            Ui::TagMatchEditWidget ui;
    };

} // namespace ConstraintTypes

typedef QPair<int,int> DateRange;
Q_DECLARE_METATYPE( DateRange );

#endif

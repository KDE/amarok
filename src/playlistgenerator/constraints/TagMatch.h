/****************************************************************************************
 * Copyright (c) 2008-2012 Soren Harward <stharward@gmail.com>                          *
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

#include "core/meta/forward_declarations.h"

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

    class TagMatch : public MatchingConstraint {
        Q_OBJECT

        /* support classes declared below */
        class Comparer;

        public:
            enum FieldTypes { FieldTypeInt, FieldTypeDate, FieldTypeString };
            enum NumComparison { CompareNumLessThan, CompareNumEquals, CompareNumGreaterThan };
            enum StrComparison { CompareStrEquals, CompareStrStartsWith, CompareStrEndsWith, CompareStrContains, CompareStrRegExp };
            enum DateComparison { CompareDateBefore, CompareDateOn, CompareDateAfter, CompareDateWithin };

            static Constraint* createFromXml(QDomElement&, ConstraintNode*);
            static Constraint* createNew(ConstraintNode*);
            static ConstraintFactoryEntry* registerMe();

            virtual QWidget* editWidget() const;
            virtual void toXml(QDomDocument&, QDomElement&) const;

            virtual QString getName() const;

            virtual Collections::QueryMaker* initQueryMaker(Collections::QueryMaker*) const;
            virtual double satisfaction(const Meta::TrackList&) const;

            // Implementation of MatchingConstraint virtuals
            const QBitArray whatTracksMatch( const Meta::TrackList& );
            int constraintMatchType() const;

        private Q_SLOTS:
            void setComparison( int );
            void setField( const QString& );
            void setInvert( bool );
            void setStrictness( int );
            void setValue( const QVariant& );

        private:
            TagMatch( QDomElement&, ConstraintNode* );
            TagMatch( ConstraintNode* );
            ~TagMatch();

            // constraint parameters
            int m_comparison;
            QString m_field;
            bool m_invert;
            double m_strictness;
            QVariant m_value;

            // convenience classes
            const Comparer* const m_comparer;
            const TagMatchFieldsModel* const m_fieldsModel;

            // convenience functions
            QString comparisonToString() const;
            QString valueToString() const;

            bool matches( const Meta::TrackPtr ) const; // match values are fuzzily calculated
            mutable QHash<Meta::TrackPtr, bool> m_matchCache; // internal cache for per-track true/false data


        /* support class that does fuzzy comparisons */
        class Comparer {
            public:
                Comparer();
                ~Comparer();

                double compareNum( const double, const int, const double, const double, const qint64 ) const;
                double compareStr( const QString&, const int, const QString& ) const;
                double compareDate( const uint, const int, const QVariant&, const double ) const;
                double compareLabels( const Meta::TrackPtr, const int, const QString& ) const;

                // rough inverses of the comparison
                uint rangeDate( const double ) const;
                int rangeNum( const double, const qint64 ) const;

            private:
                QHash<qint64, double> m_numFieldWeight;
                const double m_dateWeight;

                double fuzzyProb( const double, const double, const double, const double ) const;
        };

    };

    /* support class that manages data relationships for the various fields */
    class TagMatchFieldsModel : public QAbstractListModel {
        Q_OBJECT

        public:
            TagMatchFieldsModel();
            ~TagMatchFieldsModel();

            // required by QAbstractListModel
            QVariant data( const QModelIndex&, int role = Qt::DisplayRole ) const;
            int rowCount( const QModelIndex& parent = QModelIndex() ) const;

            bool contains( const QString& ) const;
            int index_of( const QString& ) const;
            QString field_at( int ) const;
            qint64 meta_value_of( const QString& ) const;
            QString pretty_name_of( const QString& ) const;
            TagMatch::FieldTypes type_of( const QString& ) const;

        private:
            QList<QString> m_fieldNames;
            QHash<QString, TagMatch::FieldTypes> m_fieldTypes;
            QHash<QString, qint64> m_fieldMetaValues;
            QHash<QString, QString> m_fieldPrettyNames;
    };

    class TagMatchEditWidget : public QWidget {
        Q_OBJECT

        public:
            TagMatchEditWidget( const int, const QString&, const bool, const int, const QVariant& );
            ~TagMatchEditWidget();

        Q_SIGNALS:
            void comparisonChanged( int );
            void fieldChanged( const QString& );
            void invertChanged( bool );
            void strictnessChanged( int );
            void valueChanged( const QVariant& );
            void updated();

        private Q_SLOTS:
            // comparison
            void on_comboBox_ComparisonDate_currentIndexChanged( int );
            void on_comboBox_ComparisonInt_currentIndexChanged( int );
            void on_comboBox_ComparisonRating_currentIndexChanged( int );
            void on_comboBox_ComparisonString_currentIndexChanged( int );
            void on_comboBox_ComparisonTime_currentIndexChanged( int );

            // field
            void on_comboBox_Field_currentIndexChanged( int );

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
            void slotUpdateComboBoxLabels( int );

        private:
            Ui::TagMatchEditWidget ui;
            TagMatchFieldsModel* const m_fieldsModel;
    };

} // namespace ConstraintTypes

typedef QPair<int,int> DateRange;
Q_DECLARE_METATYPE( DateRange )

#endif

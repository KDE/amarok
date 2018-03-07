/****************************************************************************************
 * Copyright (c) 2008 Daniel Caleb Jones <danielcjones@gmail.com>                       *
 * Copyright (c) 2009 Mark Kretschmann <kretschmann@kde.org>                            *
 * Copyright (c) 2010 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#ifndef AMAROK_METAQUERY_H
#define AMAROK_METAQUERY_H

#include <QWidget>
#include <QPointer>
#include "core/meta/forward_declarations.h"
#include "core/meta/support/MetaConstants.h"

#include <KComboBox>
#include <QSpinBox>

class QFrame;
class QGridLayout;
class QHBoxLayout;
class QLabel;
class QToolButton;
class QVBoxLayout;
class KToolBar;

namespace Collections
{
    class QueryMaker;
}

/**
 *  A class that allows to select a time distance.
 */
class TimeDistanceWidget : public QWidget
{
    Q_OBJECT

public:
    TimeDistanceWidget( QWidget *parent = 0 );
    qint64 timeDistance() const;
    void setTimeDistance( qint64 value );

    template<typename Func>
    void connectChanged( typename QtPrivate::FunctionPointer<Func>::Object *receiver, Func slot )
    {
        connect( m_timeEdit, QOverload<int>::of(&QSpinBox::valueChanged),
                 receiver, slot );
        connect( m_unitSelection, QOverload<int>::of(&QComboBox::currentIndexChanged),
                 receiver, slot );
    }

protected:
    QSpinBox *m_timeEdit;
    QComboBox *m_unitSelection;

private Q_SLOTS:
    void slotUpdateComboBoxLabels( int value );
};

class MetaQueryWidget : public QWidget
{
    Q_PROPERTY( bool hideFieldSelector READ isFieldSelectorHidden WRITE setFieldSelectorHidden )

    Q_OBJECT

    public:
        /** Creates a MetaQueryWidget which can be used to select one meta query filter.
         *  @param onlyNumeric If set to true the widget will only display numeric fields.
         *  @param noCondition If set to true no condition can be selected.
         */
        explicit MetaQueryWidget( QWidget* parent = 0, bool onlyNumeric = false, bool noCondition = false );
        ~MetaQueryWidget();

        enum FilterCondition
        {
            Equals       =  0,
            GreaterThan  =  1,
            LessThan     =  2,
            Between      =  3,
            OlderThan    =  4,
            NewerThan    =  5,
            Contains     =  6
        };

        struct Filter
        {
            Filter()
                  : m_field( 0 )
                  , numValue( 0 )
                  , numValue2( 0 )
                  , condition( Contains )
            {}

            qint64 field() const { return m_field; }
            void setField( qint64 newField );

            /** Returns a textual representation of the field.
             */
            QString fieldToString() const;

            /** Returns a textual representation of the filter.
             *  Used for the edit filter dialog (or for debugging)
             */
            QString toString( bool invert = false ) const;

            bool isNumeric() const
            { return MetaQueryWidget::isNumeric( m_field ); }

            bool isDate() const
            { return MetaQueryWidget::isDate( m_field ); }

            /** Returns the minimum allowed value for the field type */
            static qint64 minimumValue( quint64 field );
            static qint64 maximumValue( quint64 field );
            static qint64 defaultValue( quint64 field );

        private:
            qint64 m_field;

        public:
            QString  value;
            qint64   numValue;
            qint64   numValue2;
            FilterCondition condition;
        };

        /** Returns the current filter value.
         */
        Filter filter() const;

        /** Returns true if the given field is a numeric field */
        static bool isNumeric( qint64 field );

        /** Returns true if the given field is a date field */
        static bool isDate( qint64 field );

        /** Returns a localized text of the condition.
            @param field Needed in order to test whether the field is a date, numeric or a string since the texts differ slightly
        */
        static QString conditionToString( FilterCondition condition, qint64 field );


    public Q_SLOTS:
        void setFilter(const MetaQueryWidget::Filter &value);

        void setField( const qint64 field );
        /** Field Selector combo box visibility state
         */
        bool isFieldSelectorHidden() const;
        void setFieldSelectorHidden( const bool hidden );

    Q_SIGNALS:
        void changed(const MetaQueryWidget::Filter &value);

    private Q_SLOTS:
        void fieldChanged( int );
        void compareChanged( int );
        void valueChanged( const QString& );
        void numValueChanged( int );
        void numValue2Changed( int );
        void numValueChanged( qint64 );
        void numValue2Changed( qint64 );
        void numValueChanged( const QTime& );
        void numValue2Changed( const QTime& );
        void numValueDateChanged();
        void numValue2DateChanged();
        void numValueTimeDistanceChanged();
        void numValueFormatChanged( int );

        void populateComboBox( QStringList );
        void comboBoxPopulated();

    private:
        void makeFieldSelection();

        /** Adds the value selection widgets to the layout.
         *  Adds m_compareSelection, m_valueSelection1, m_valueSelection2 to the layout.
         */
        void setValueSelection();

        void makeCompareSelection();
        void makeValueSelection();

        void makeGenericComboSelection( bool editable, Collections::QueryMaker* populateQuery );
        void makeMetaComboSelection( qint64 field );

        void makeFormatComboSelection();
        void makeGenericNumberSelection( qint64 field, const QString& unit = "" );
        void makePlaycountSelection();
        void makeRatingSelection();
        void makeLengthSelection();
        void makeDateTimeSelection();
        void makeFilenameSelection();

        bool m_onlyNumeric;
        bool m_noCondition;

        bool m_settingFilter; // if set to true we are just setting the filter

        QVBoxLayout* m_layoutMain;
        QHBoxLayout* m_layoutValue;
        QVBoxLayout* m_layoutValueLabels;
        QVBoxLayout* m_layoutValueValues;

        QComboBox*   m_fieldSelection;
        QLabel*      m_andLabel;
        QComboBox*   m_compareSelection;
        QWidget*     m_valueSelection1;
        QWidget*     m_valueSelection2;

        Filter m_filter;

        QMap< QObject*, QPointer<KComboBox> > m_runningQueries;
};

#endif


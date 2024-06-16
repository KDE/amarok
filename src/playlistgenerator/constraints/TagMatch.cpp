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

#define DEBUG_PREFIX "Constraint::TagMatch"

#include "TagMatch.h"

#include "playlistgenerator/Constraint.h"
#include "playlistgenerator/ConstraintFactory.h"

#include "core/collections/QueryMaker.h"
#include "core/meta/Meta.h"
#include "core/meta/Statistics.h"
#include "core/support/Debug.h"

#include <QRandomGenerator>

#include <cmath>
#include <cstdlib>

Constraint*
ConstraintTypes::TagMatch::createFromXml( QDomElement& xmlelem, ConstraintNode* p )
{
    if ( p )
        return new TagMatch( xmlelem, p );
    else
        return nullptr;
}

Constraint*
ConstraintTypes::TagMatch::createNew( ConstraintNode* p )
{
    if ( p )
        return new TagMatch( p );
    else
        return nullptr;
}

ConstraintFactoryEntry*
ConstraintTypes::TagMatch::registerMe()
{
    return new ConstraintFactoryEntry( QStringLiteral("TagMatch"),
                                       i18n("Match Tags"),
                                       i18n("Make all tracks in the playlist match the specified characteristic"),
                                       &TagMatch::createFromXml, &TagMatch::createNew );
}

ConstraintTypes::TagMatch::TagMatch( QDomElement& xmlelem, ConstraintNode* p )
        : MatchingConstraint( p )
        , m_comparer( new Comparer() )
        , m_fieldsModel( new TagMatchFieldsModel() )
{
    QDomAttr a;

    a = xmlelem.attributeNode( QStringLiteral("field") );
    if ( !a.isNull() ) {
        if ( m_fieldsModel->contains( a.value() ) )
            m_field = a.value();
    }

    a = xmlelem.attributeNode( QStringLiteral("comparison") );
    if ( !a.isNull() ) {
        m_comparison = a.value().toInt();
    }

    a = xmlelem.attributeNode( QStringLiteral("value") );
    if ( !a.isNull() ) {
        if ( m_fieldsModel->type_of( m_field ) == FieldTypeInt ) {
            m_value = a.value().toInt();
        } else if ( m_fieldsModel->type_of( m_field ) == FieldTypeDate ) {
            if ( m_comparison == CompareDateWithin ) {
                QStringList parts = a.value().split(' ');
                if ( parts.size() == 2 ) {
                    int u = parts.at( 0 ).toInt();
                    int v = 0;
                    if ( parts.at( 1 ) == QLatin1String("months") )
                        v = 1;
                    else if ( parts.at( 1 ) == QLatin1String("years") )
                        v = 2;
                    m_value = QVariant::fromValue( DateRange( u, v ) );
                } else
                    m_value = QVariant::fromValue( DateRange( 0, 0 ) );
            } else
                m_value = QDate::fromString( a.value(), Qt::ISODate );
        } else { // String type
            m_value = a.value();
        }
    }

    a = xmlelem.attributeNode( QStringLiteral("invert") );
    if ( !a.isNull() && a.value() == QLatin1String("true") )
        m_invert = true;
    else
        m_invert = false;

    a = xmlelem.attributeNode( QStringLiteral("strictness") );
    if ( !a.isNull() )
        m_strictness = a.value().toDouble();
}

ConstraintTypes::TagMatch::TagMatch( ConstraintNode* p )
        : MatchingConstraint( p )
        , m_comparison( CompareStrEquals )
        , m_field( QStringLiteral("title") )
        , m_invert( false )
        , m_strictness( 1.0 )
        , m_value()
        , m_comparer( new Comparer() )
        , m_fieldsModel( new TagMatchFieldsModel() )
{
}

ConstraintTypes::TagMatch::~TagMatch()
{
    delete m_comparer;
    delete m_fieldsModel;
}

QWidget*
ConstraintTypes::TagMatch::editWidget() const
{
    TagMatchEditWidget* e = new TagMatchEditWidget(
                                            m_comparison,
                                            m_field,
                                            m_invert,
                                            static_cast<int>( m_strictness * 10 ),
                                            m_value );
    connect( e, &TagMatchEditWidget::comparisonChanged, this, &TagMatch::setComparison );
    connect( e, &TagMatchEditWidget::fieldChanged, this, &TagMatch::setField );
    connect( e, &TagMatchEditWidget::invertChanged, this, &TagMatch::setInvert );
    connect( e, &TagMatchEditWidget::strictnessChanged, this, &TagMatch::setStrictness );
    connect( e, &TagMatchEditWidget::valueChanged, this, &TagMatch::setValue );
    return e;
}

void
ConstraintTypes::TagMatch::toXml( QDomDocument& doc, QDomElement& elem ) const
{
    QDomElement c = doc.createElement( QStringLiteral("constraint") );

    c.setAttribute( QStringLiteral("type"), QStringLiteral("TagMatch") );
    c.setAttribute( QStringLiteral("field"), m_field );
    c.setAttribute( QStringLiteral("comparison"), m_comparison );
    c.setAttribute( QStringLiteral("value"), valueToString() );

    if ( m_invert )
        c.setAttribute( QStringLiteral("invert"), QStringLiteral("true") );
    else
        c.setAttribute( QStringLiteral("invert"), QStringLiteral("false") );

    c.setAttribute( QStringLiteral("strictness"), QString::number( m_strictness ) );

    elem.appendChild( c );
}

QString
ConstraintTypes::TagMatch::getName() const
{
    KLocalizedString v( ki18nc( "%1 = empty string or \"not\"; "
                                "%2 = a metadata field, like \"title\" or \"artist name\"; "
                                "%3 = a predicate, can be equals, starts with, ends with or contains; "
                                "%4 = a string to match; "
                                "Example: Match tag: not title contains \"foo\"", "Match tag:%1 %2 %3 %4") );
    v = v.subs( m_invert ? i18n(" not") : QLatin1String("") );
    v = v.subs( m_fieldsModel->pretty_name_of( m_field ) );
    v = v.subs( comparisonToString() );
    if ( m_field == QLatin1String("rating") ) {
        double r = m_value.toDouble() / 2.0;
        v = v.subs( i18ncp("number of stars in the rating of a track", "%1 star", "%1 stars", r) );
    } else if ( m_field == QLatin1String("length") ) {
        v = v.subs( QTime(0, 0, 0).addMSecs( m_value.toInt() ).toString( QStringLiteral("H:mm:ss") ) );
    } else {
        if ( m_fieldsModel->type_of( m_field ) == FieldTypeString ) {
            // put quotes around any strings (eg, track title or artist name) ...
            QString s = i18nc("an arbitrary string surrounded by quotes", "\"%1\"", valueToString() );
            v = v.subs( s );
        } else {
            // ... but don't quote put quotes around anything else
            v = v.subs( valueToString() );
        }
    }
    return v.toString();
}

Collections::QueryMaker*
ConstraintTypes::TagMatch::initQueryMaker( Collections::QueryMaker* qm ) const
{
    if ( ( m_fieldsModel->type_of( m_field ) == FieldTypeInt ) ) {
        int v = m_value.toInt();
        int range = static_cast<int>( m_comparer->rangeNum( m_strictness, m_fieldsModel->meta_value_of( m_field ) ) );
        if ( m_comparison == CompareNumEquals ) {
            if ( !m_invert ) {
                if ( m_strictness < 0.99 ) { // fuzzy approximation of "1.0"
                    qm->beginAnd();
                    qm->addNumberFilter( m_fieldsModel->meta_value_of( m_field ), v - range, Collections::QueryMaker::GreaterThan );
                    qm->addNumberFilter( m_fieldsModel->meta_value_of( m_field ), v + range, Collections::QueryMaker::LessThan );
                    qm->endAndOr();
                } else {
                    qm->addNumberFilter( m_fieldsModel->meta_value_of( m_field ), v, Collections::QueryMaker::Equals );
                }
            } else {
                if ( m_strictness > 0.99 ) {
                    qm->excludeNumberFilter( m_fieldsModel->meta_value_of( m_field ), v, Collections::QueryMaker::Equals );
                }
            }
        } else if ( m_comparison == CompareNumGreaterThan ) {
            if ( m_invert )
                qm->excludeNumberFilter( m_fieldsModel->meta_value_of( m_field ), v + range, Collections::QueryMaker::GreaterThan );
            else
                qm->addNumberFilter( m_fieldsModel->meta_value_of( m_field ), v - range, Collections::QueryMaker::GreaterThan );
        } else if ( m_comparison == CompareNumLessThan ) {
            if ( m_invert )
                qm->excludeNumberFilter( m_fieldsModel->meta_value_of( m_field ), v - range, Collections::QueryMaker::LessThan );
            else
                qm->addNumberFilter( m_fieldsModel->meta_value_of( m_field ), v + range, Collections::QueryMaker::LessThan );
        }
    } else if ( m_fieldsModel->type_of( m_field ) == FieldTypeDate ) {
        uint referenceDate = 0;
        int range = m_comparer->rangeDate( m_strictness );
        if ( m_comparison == CompareDateBefore ) {
            referenceDate = m_value.toDateTime().toSecsSinceEpoch();
            if ( m_invert )
                qm->excludeNumberFilter( m_fieldsModel->meta_value_of( m_field ), referenceDate - range, Collections::QueryMaker::LessThan );
            else
                qm->addNumberFilter( m_fieldsModel->meta_value_of( m_field ), referenceDate + range, Collections::QueryMaker::LessThan );
        } else if ( m_comparison == CompareDateOn ) {
            referenceDate = m_value.toDateTime().toSecsSinceEpoch();
            if ( !m_invert ) {
                qm->beginAnd();
                qm->addNumberFilter( m_fieldsModel->meta_value_of( m_field ), referenceDate - range, Collections::QueryMaker::GreaterThan );
                qm->addNumberFilter( m_fieldsModel->meta_value_of( m_field ), referenceDate + range, Collections::QueryMaker::LessThan );
                qm->endAndOr();
            }
        } else if ( m_comparison == CompareDateAfter ) {
            referenceDate = m_value.toDateTime().toSecsSinceEpoch();
            if ( m_invert )
                qm->excludeNumberFilter( m_fieldsModel->meta_value_of( m_field ), referenceDate + range, Collections::QueryMaker::GreaterThan );
            else
                qm->addNumberFilter( m_fieldsModel->meta_value_of( m_field ), referenceDate - range, Collections::QueryMaker::GreaterThan );
        } else if ( m_comparison == CompareDateWithin ) {
            QDateTime now = QDateTime::currentDateTime();
            DateRange r = m_value.value<DateRange>();
            switch ( r.second ) {
                case 0:
                    referenceDate = now.addDays( -1 * r.first ).toSecsSinceEpoch();
                    break;
                case 1:
                    referenceDate = now.addMonths( -1 * r.first ).toSecsSinceEpoch();
                    break;
                case 2:
                    referenceDate = now.addYears( -1 * r.first ).toSecsSinceEpoch();
                    break;
                default:
                    break;
            }
            if ( m_invert )
                qm->excludeNumberFilter( m_fieldsModel->meta_value_of( m_field ), referenceDate + range, Collections::QueryMaker::GreaterThan );
            else
                qm->addNumberFilter( m_fieldsModel->meta_value_of( m_field ), referenceDate - range, Collections::QueryMaker::GreaterThan );
        }
    } else if ( m_fieldsModel->type_of( m_field ) == FieldTypeString ) {
        if ( m_comparison == CompareStrEquals ) {
            if ( m_invert )
                qm->excludeFilter( m_fieldsModel->meta_value_of( m_field ), m_value.toString(), true, true );
            else
                qm->addFilter( m_fieldsModel->meta_value_of( m_field ), m_value.toString(), true, true );
        } else if ( m_comparison == CompareStrStartsWith ) {
            if ( m_invert )
                qm->excludeFilter( m_fieldsModel->meta_value_of( m_field ), m_value.toString(), true, false );
            else
                qm->addFilter( m_fieldsModel->meta_value_of( m_field ), m_value.toString(), true, false );
        } else if ( m_comparison == CompareStrEndsWith ) {
            if ( m_invert )
                qm->excludeFilter( m_fieldsModel->meta_value_of( m_field ), m_value.toString(), false, true );
            else
                qm->addFilter( m_fieldsModel->meta_value_of( m_field ), m_value.toString(), false, true );
        } else if ( m_comparison == CompareStrContains ) {
            if ( m_invert )
                qm->excludeFilter( m_fieldsModel->meta_value_of( m_field ), m_value.toString(), false, false );
            else
                qm->addFilter( m_fieldsModel->meta_value_of( m_field ), m_value.toString(), false, false );
        }
        // TODO: regexp
    } else {
        error() << "TagMatch cannot initialize QM for unknown type";
    }

    return qm;
}

double
ConstraintTypes::TagMatch::satisfaction( const Meta::TrackList& tl ) const
{
    double satisfaction = 0.0;
    for( Meta::TrackPtr t : tl )
    {
        if ( matches( t ) ) {
            satisfaction += 1.0;
        }
    }
    satisfaction /= ( double )tl.size();
    return satisfaction;
}

const QBitArray
ConstraintTypes::TagMatch::whatTracksMatch( const Meta::TrackList& tl )
{
    QBitArray match = QBitArray( tl.size() );
    for ( int i = 0; i < tl.size(); i++ ) {
        if ( matches( tl.at( i ) ) )
            match.setBit( i, true );
    }
    return match;
}

int
ConstraintTypes::TagMatch::constraintMatchType() const
{
    return ( 0 << 28 ) + m_fieldsModel->index_of( m_field );
}


QString
ConstraintTypes::TagMatch::comparisonToString() const
{
    if ( m_fieldsModel->type_of( m_field ) == FieldTypeInt ) {
        if ( m_comparison == CompareNumEquals ) {
            return i18nc("a numerical tag (like year or track number) equals a value","equals");
        } else if ( m_comparison == CompareNumGreaterThan ) {
            return i18n("greater than");
        } else if ( m_comparison == CompareNumLessThan ) {
            return i18n("less than");
        }
    } else if ( m_fieldsModel->type_of( m_field ) == FieldTypeDate ) {
        if ( m_comparison == CompareDateBefore ) {
            return i18n("before");
        } else if ( m_comparison == CompareDateOn ) {
            return i18n("on");
        } else if ( m_comparison == CompareDateAfter ) {
            return i18n("after");
        } else if ( m_comparison == CompareDateWithin ) {
            return i18n("within");
        }
    } else {
        if ( m_comparison == CompareStrEquals ) {
            return i18nc("an alphabetical tag (like title or artist name) equals some string","equals");
        } else if ( m_comparison == CompareStrStartsWith ) {
            return i18nc("an alphabetical tag (like title or artist name) starts with some string","starts with");
        } else if ( m_comparison == CompareStrEndsWith ) {
            return i18nc("an alphabetical tag (like title or artist name) ends with some string","ends with");
        } else if ( m_comparison == CompareStrContains ) {
            return i18nc("an alphabetical tag (like title or artist name) contains some string","contains");
        } else if ( m_comparison == CompareStrRegExp ) {
            return i18n("regexp");
        }
    }
    return i18n("unknown comparison");
}

QString
ConstraintTypes::TagMatch::valueToString() const
{
    if ( m_fieldsModel->type_of( m_field ) == FieldTypeDate ) {
        if ( m_comparison != CompareDateWithin ) {
            return m_value.toDate().toString( Qt::ISODate );
        } else {
            KLocalizedString unit;
            switch ( m_value.value<DateRange>().second ) {
                case 0:
                    unit = ki18np("%1 day", "%1 days");
                    break;
                case 1:
                    unit = ki18np("%1 month", "%1 months");
                    break;
                case 2:
                    unit = ki18np("%1 year", "%1 years");
                    break;
                default:
                    break;
            }
            return unit.subs( m_value.value<DateRange>().first ).toString();
        }
    } else {
        return m_value.toString();
    }
}

bool
ConstraintTypes::TagMatch::matches( Meta::TrackPtr track ) const
{
    if ( !m_matchCache.contains( track ) ) {
        double v = 0.0;
        qint64 fmv = m_fieldsModel->meta_value_of( m_field );
        switch ( fmv ) {
            case Meta::valUrl:
                v = m_comparer->compareStr( track->prettyUrl(), m_comparison, m_value.toString() );
                break;
            case Meta::valTitle:
                v = m_comparer->compareStr( track->prettyName(), m_comparison, m_value.toString() );
                break;
            case Meta::valArtist:
                v = m_comparer->compareStr( track->artist()->prettyName(), m_comparison, m_value.toString() );
                break;
            case Meta::valAlbum:
                v = m_comparer->compareStr( track->album()->prettyName(), m_comparison, m_value.toString() );
                break;
            case Meta::valGenre:
                v = m_comparer->compareStr( track->genre()->prettyName(), m_comparison, m_value.toString() );
                break;
            case Meta::valComposer:
                v = m_comparer->compareStr( track->composer()->prettyName(), m_comparison, m_value.toString() );
                break;
            case Meta::valYear:
                v = m_comparer->compareNum( track->year()->prettyName().toInt(), m_comparison, m_value.toInt(), m_strictness, fmv );
                break;
            case Meta::valComment:
                v = m_comparer->compareStr( track->comment(), m_comparison, m_value.toString() );
                break;
            case Meta::valTrackNr:
                v = m_comparer->compareNum( track->trackNumber(), m_comparison, m_value.toInt(), m_strictness, fmv );
                break;
            case Meta::valDiscNr:
                v = m_comparer->compareNum( track->discNumber(), m_comparison, m_value.toInt(), m_strictness, fmv );
                break;
            case Meta::valLength:
                v = m_comparer->compareNum( track->length(), m_comparison, m_value.toInt(), m_strictness, fmv );
                break;
            case Meta::valBitrate:
                v = m_comparer->compareNum( track->bitrate(), m_comparison, m_value.toInt(), m_strictness, fmv );
                break;
            case Meta::valFilesize:
                v = m_comparer->compareNum( track->filesize(), m_comparison, m_value.toInt(), m_strictness, fmv );
                break;
            case Meta::valCreateDate:
                v = m_comparer->compareDate( track->createDate().toSecsSinceEpoch(), m_comparison, m_value, m_strictness );
                break;
            case Meta::valScore:
                v = m_comparer->compareNum( track->statistics()->score(), m_comparison, m_value.toDouble(), m_strictness, fmv );
                break;
            case Meta::valRating:
                v = m_comparer->compareNum( track->statistics()->rating(), m_comparison, m_value.toInt(), m_strictness, fmv );
                break;
            case Meta::valFirstPlayed:
                v = m_comparer->compareDate( track->statistics()->firstPlayed().toSecsSinceEpoch(), m_comparison, m_value, m_strictness );
                break;
            case Meta::valLastPlayed:
                v = m_comparer->compareDate( track->statistics()->lastPlayed().toSecsSinceEpoch(), m_comparison, m_value, m_strictness );
                break;
            case Meta::valPlaycount:
                v = m_comparer->compareNum( track->statistics()->playCount(), m_comparison, m_value.toInt(), m_strictness, fmv );
                break;
            case Meta::valLabel:
                v = m_comparer->compareLabels( track, m_comparison, m_value.toString() );
                break;
            default:
                v = 0.0;
                break;
        }
        if ( m_invert )
            v = 1.0 - v;

        m_matchCache.insert( track, ( v > ( (double)QRandomGenerator::global()->generate() / (double)std::numeric_limits<quint32>::max() ) ) );
    }
    return m_matchCache.value( track );
}

void
ConstraintTypes::TagMatch::setComparison( int c )
{
    m_comparison = c;
    m_matchCache.clear();
    Q_EMIT dataChanged();
}

void
ConstraintTypes::TagMatch::setField( const QString& s )
{
    m_field = s;
    m_matchCache.clear();
    Q_EMIT dataChanged();
}

void
ConstraintTypes::TagMatch::setInvert( bool v )
{
    if ( m_invert != v ) {
        for( const Meta::TrackPtr &t : m_matchCache.keys() )
            m_matchCache.insert( t, !m_matchCache.value( t ) );
    }
    m_invert = v;
    Q_EMIT dataChanged();
}

void
ConstraintTypes::TagMatch::setStrictness( int v )
{
    m_strictness = static_cast<double>( v ) / 10.0;
    m_matchCache.clear();
}

void
ConstraintTypes::TagMatch::setValue( const QVariant& v )
{
    m_value = v;
    m_matchCache.clear();
    Q_EMIT dataChanged();
}

/******************************
 * Edit Widget                *
 ******************************/

ConstraintTypes::TagMatchEditWidget::TagMatchEditWidget(
                        const int comparison,
                        const QString& field,
                        const bool invert,
                        const int strictness,
                        const QVariant& value )
        : QWidget( nullptr )
        , m_fieldsModel( new TagMatchFieldsModel() )
{
    ui.setupUi( this );

    // plural support in combobox labels
    connect( ui.spinBox_ValueDateValue, QOverload<int>::of(&QSpinBox::valueChanged),
             this, &TagMatchEditWidget::slotUpdateComboBoxLabels );
    ui.comboBox_ValueDateUnit->insertItem(0, i18ncp("within the last %1 days", "day", "days", 0));
    ui.comboBox_ValueDateUnit->insertItem(1, i18ncp("within the last %1 months", "month", "months", 0));
    ui.comboBox_ValueDateUnit->insertItem(2, i18ncp("within the last %1 years", "year", "years", 0));

    // fill in appropriate defaults for some attributes
    ui.qcalendarwidget_DateSpecific->setSelectedDate( QDate::currentDate() );

    // fill in user-specified values before the slots have been connected to we don't have to call back to the constraint a dozen times
    ui.comboBox_Field->setModel( m_fieldsModel );
    ui.checkBox_Invert->setChecked( invert );

    if ( field == QLatin1String("rating") ) {
        ui.comboBox_ComparisonRating->setCurrentIndex( comparison );
        ui.slider_StrictnessRating->setValue( strictness );
        ui.rating_RatingValue->setRating( value.toInt() );
    } else if ( field == QLatin1String("length") ) {
        ui.comboBox_ComparisonTime->setCurrentIndex( comparison );
        ui.slider_StrictnessTime->setValue( strictness );
        ui.timeEdit_TimeValue->setTime( QTime(0, 0, 0).addMSecs( value.toInt() ) );
    } else if ( m_fieldsModel->type_of( field ) == TagMatch::FieldTypeInt ) {
        ui.comboBox_ComparisonInt->setCurrentIndex( comparison );
        ui.slider_StrictnessInt->setValue( strictness );
        ui.spinBox_ValueInt->setValue( value.toInt() );
    } else if ( m_fieldsModel->type_of( field ) == TagMatch::FieldTypeDate ) {
        ui.comboBox_ComparisonDate->setCurrentIndex( comparison );
        ui.slider_StrictnessDate->setValue( strictness );
        if ( comparison == TagMatch::CompareDateWithin ) {
            ui.stackedWidget_Date->setCurrentIndex( 1 );
            ui.spinBox_ValueDateValue->setValue( value.value<DateRange>().first );
            ui.comboBox_ValueDateUnit->setCurrentIndex( value.value<DateRange>().second );
        } else {
            ui.stackedWidget_Date->setCurrentIndex( 0 );
            ui.qcalendarwidget_DateSpecific->setSelectedDate( value.toDate() );
        }
    } else if ( m_fieldsModel->type_of( field ) == TagMatch::FieldTypeString ) {
        ui.comboBox_ComparisonString->setCurrentIndex( comparison );
        ui.lineEdit_StringValue->setText( value.toString() );
    }

    // set this after the slot has been connected so that it also sets the field page correctly
    ui.comboBox_Field->setCurrentIndex( m_fieldsModel->index_of( field ) );
}

ConstraintTypes::TagMatchEditWidget::~TagMatchEditWidget()
{
    delete m_fieldsModel;
}

// ComboBox slots for comparisons
void
ConstraintTypes::TagMatchEditWidget::on_comboBox_ComparisonDate_currentIndexChanged( int c )
{
    if ( c == TagMatch::CompareDateWithin )
        ui.stackedWidget_Date->setCurrentIndex( 1 );
    else
        ui.stackedWidget_Date->setCurrentIndex( 0 );
    Q_EMIT comparisonChanged( c );
}

void
ConstraintTypes::TagMatchEditWidget::on_comboBox_ComparisonInt_currentIndexChanged( int c )
{
    Q_EMIT comparisonChanged( c );
}

void
ConstraintTypes::TagMatchEditWidget::on_comboBox_ComparisonRating_currentIndexChanged( int c )
{
    Q_EMIT comparisonChanged( c );
}

void
ConstraintTypes::TagMatchEditWidget::on_comboBox_ComparisonString_currentIndexChanged( int c )
{
    Q_EMIT comparisonChanged( c );
}

void
ConstraintTypes::TagMatchEditWidget::on_comboBox_ComparisonTime_currentIndexChanged( int c )
{
    Q_EMIT comparisonChanged( c );
}

// ComboBox slots for field
void
ConstraintTypes::TagMatchEditWidget::on_comboBox_Field_currentIndexChanged( int idx )
{
    QString field = m_fieldsModel->field_at( idx );
    int c = 0;
    int s = 0;
    QVariant v;
    if ( field == QLatin1String("length") ) {
        ui.stackedWidget_Field->setCurrentIndex( 3 );
        c = ui.comboBox_ComparisonTime->currentIndex();
        s = ui.slider_StrictnessTime->value();
        v = QTime(0, 0, 0).msecsTo( ui.timeEdit_TimeValue->time() );
    } else if ( field == QLatin1String("rating") ) {
        ui.stackedWidget_Field->setCurrentIndex( 4 );
        c = ui.comboBox_ComparisonRating->currentIndex();
        s = ui.slider_StrictnessRating->value();
        v = ui.rating_RatingValue->rating();
    } else {
        if ( m_fieldsModel->type_of( field ) == TagMatch::FieldTypeInt ) {
            ui.stackedWidget_Field->setCurrentIndex( 0 );
            c = ui.comboBox_ComparisonInt->currentIndex();
            s = ui.slider_StrictnessInt->value();
            v = ui.spinBox_ValueInt->value();
        } else if ( m_fieldsModel->type_of( field ) == TagMatch::FieldTypeDate ) {
            ui.stackedWidget_Field->setCurrentIndex( 1 );
            c = ui.comboBox_ComparisonDate->currentIndex();
            s = ui.slider_StrictnessDate->value();
            if ( c == TagMatch::CompareDateWithin ) {
                ui.stackedWidget_Date->setCurrentIndex( 1 );
                int a = ui.spinBox_ValueDateValue->value();
                int b = ui.comboBox_ValueDateUnit->currentIndex();
                v = QVariant::fromValue( DateRange( a, b ) );
            } else {
                ui.stackedWidget_Date->setCurrentIndex( 0 );
                v = ui.qcalendarwidget_DateSpecific->selectedDate();
            }
        } else if ( m_fieldsModel->type_of( field ) == TagMatch::FieldTypeString ) {
            ui.stackedWidget_Field->setCurrentIndex( 2 );
            c = ui.comboBox_ComparisonString->currentIndex();
            s = 1.0;
            v = ui.lineEdit_StringValue->text();
        }
    }

    // TODO: set range limitations and default values depending on field

    Q_EMIT fieldChanged( field );
    Q_EMIT valueChanged( v );
    Q_EMIT comparisonChanged( c );
    Q_EMIT strictnessChanged( s );
}

// Invert checkbox slot
void
ConstraintTypes::TagMatchEditWidget::on_checkBox_Invert_clicked( bool v )
{
    Q_EMIT invertChanged( v );
}

// Strictness Slider slots
void
ConstraintTypes::TagMatchEditWidget::on_slider_StrictnessDate_valueChanged( int v )
{
    Q_EMIT strictnessChanged( v );
}

void
ConstraintTypes::TagMatchEditWidget::on_slider_StrictnessInt_valueChanged( int v )
{
    Q_EMIT strictnessChanged( v );
}

void
ConstraintTypes::TagMatchEditWidget::on_slider_StrictnessRating_valueChanged( int v )
{
    Q_EMIT strictnessChanged( v );
}

void
ConstraintTypes::TagMatchEditWidget::on_slider_StrictnessTime_valueChanged( int v )
{
    Q_EMIT strictnessChanged( v );
}

// various value slots
void
ConstraintTypes::TagMatchEditWidget::on_kdatewidget_DateSpecific_changed( const QDate& v )
{
    Q_EMIT valueChanged( QVariant( v ) );
}

void
ConstraintTypes::TagMatchEditWidget::on_comboBox_ValueDateUnit_currentIndexChanged( int u )
{
    int v = ui.spinBox_ValueDateValue->value();
    Q_EMIT valueChanged( QVariant::fromValue( DateRange( v, u ) ) );
}

void
ConstraintTypes::TagMatchEditWidget::on_spinBox_ValueDateValue_valueChanged( int v )
{
    int u = ui.comboBox_ValueDateUnit->currentIndex();
    Q_EMIT valueChanged( QVariant::fromValue( DateRange( v, u ) ) );
}

void
ConstraintTypes::TagMatchEditWidget::on_spinBox_ValueInt_valueChanged( int v )
{
    Q_EMIT valueChanged( QVariant( v ) );
}

void
ConstraintTypes::TagMatchEditWidget::on_lineEdit_StringValue_textChanged( const QString& v )
{
    Q_EMIT valueChanged( QVariant( v ) );
}

void
ConstraintTypes::TagMatchEditWidget::on_rating_RatingValue_ratingChanged( int v )
{
    Q_EMIT valueChanged( QVariant( v ) );
}

void
ConstraintTypes::TagMatchEditWidget::on_timeEdit_TimeValue_timeChanged( const QTime& t )
{
    int v = QTime(0, 0, 0).msecsTo( t );
    Q_EMIT valueChanged( QVariant( v ) );
}

void
ConstraintTypes::TagMatchEditWidget::slotUpdateComboBoxLabels( int value )
{
    ui.comboBox_ValueDateUnit->setItemText(0, i18ncp("within the last %1 days", "day", "days", value));
    ui.comboBox_ValueDateUnit->setItemText(1, i18ncp("within the last %1 months", "month", "months", value));
    ui.comboBox_ValueDateUnit->setItemText(2, i18ncp("within the last %1 years", "year", "years", value));
}

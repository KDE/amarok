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

#define DEBUG_PREFIX "Constraint::TagMatch"

#include "TagMatch.h"

#include "playlistgenerator/Constraint.h"
#include "playlistgenerator/ConstraintFactory.h"

#include "core/collections/QueryMaker.h"
#include "core/support/Debug.h"

#include <QtGlobal>
#include <QtGui>

#include <math.h>
#include <stdlib.h>

Constraint*
ConstraintTypes::TagMatch::createFromXml( QDomElement& xmlelem, ConstraintNode* p )
{
    if ( p )
        return new TagMatch( xmlelem, p );
    else
        return 0;
}

Constraint*
ConstraintTypes::TagMatch::createNew( ConstraintNode* p )
{
    if ( p )
        return new TagMatch( p );
    else
        return 0;
}

ConstraintFactoryEntry*
ConstraintTypes::TagMatch::registerMe()
{
    return new ConstraintFactoryEntry( i18n("TagMatch"),
                    i18n("Make all tracks in the playlist match the specified characteristic"),
                    &TagMatch::createFromXml, &TagMatch::createNew );
}

ConstraintTypes::TagMatch::TagMatch( QDomElement& xmlelem, ConstraintNode* p )
        : MatchingConstraint( p )
        , m_fieldsModel( new TagMatchFieldsModel() )
{
    QDomAttr a;

    a = xmlelem.attributeNode( "field" );
    if ( !a.isNull() ) {
        if ( m_fieldsModel->contains( a.value() ) )
            m_field = a.value();
        else
            debug() << a.value() << "is not a recognized field name" << endl;
    }

    a = xmlelem.attributeNode( "comparison" );
    if ( !a.isNull() ) {
        m_comparison = a.value().toInt();
    }

    a = xmlelem.attributeNode( "value" );
    if ( !a.isNull() ) {
        if ( m_fieldsModel->type_of( m_field ) == FieldTypeInt ) {
            m_value = a.value().toInt();
        } else if ( m_fieldsModel->type_of( m_field ) == FieldTypeDate ) {
            if ( m_comparison == Constraint::CompareDateWithin ) {
                QStringList parts = a.value().split(" ");
                if ( parts.size() == 2 ) {
                    int u = parts.at( 0 ).toInt();
                    int v = 0;
                    if ( parts.at( 1 ) == "months" )
                        v = 1;
                    else if ( parts.at( 1 ) == "years" )
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

    a = xmlelem.attributeNode( "invert" );
    if ( !a.isNull() && a.value() == "true" )
        m_invert = true;
    else
        m_invert = false;

    a = xmlelem.attributeNode( "strictness" );
    if ( !a.isNull() )
        m_strictness = a.value().toDouble();
}

ConstraintTypes::TagMatch::TagMatch( ConstraintNode* p )
        : MatchingConstraint( p )
        , m_comparison( Constraint::CompareStrEquals )
        , m_field( "title" )
        , m_invert( false )
        , m_strictness( 1.0 )
        , m_value()
        , m_fieldsModel( new TagMatchFieldsModel() )
{
}

ConstraintTypes::TagMatch::~TagMatch()
{
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
    connect( e, SIGNAL( comparisonChanged( int ) ), this, SLOT( setComparison( int ) ) );
    connect( e, SIGNAL( fieldChanged( const QString& ) ), this, SLOT( setField( const QString& ) ) );
    connect( e, SIGNAL( invertChanged( bool ) ), this, SLOT( setInvert( bool ) ) );
    connect( e, SIGNAL( strictnessChanged( int ) ), this, SLOT( setStrictness( int ) ) );
    connect( e, SIGNAL( valueChanged( const QVariant& ) ), this, SLOT( setValue( const QVariant& ) ) );
    return e;
}

void
ConstraintTypes::TagMatch::toXml( QDomDocument& doc, QDomElement& elem ) const
{
    QDomElement c = doc.createElement( "constraint" );
    QDomText t = doc.createTextNode( getName() );

    c.appendChild( t );
    c.setAttribute( "type", "TagMatch" );
    c.setAttribute( "field", m_field );
    c.setAttribute( "comparison", m_comparison );
    c.setAttribute( "value", valueToString() );

    if ( m_invert )
        c.setAttribute( "invert", "true" );
    else
        c.setAttribute( "invert", "false" );

    c.setAttribute( "strictness", QString::number( m_strictness ) );

    elem.appendChild( c );
}

QString
ConstraintTypes::TagMatch::getName() const
{
    QString v( "Tag Match:%1 %2 %3 %4" );
    v = v.arg( ( m_invert ? i18n(" not") : "" ), m_fieldsModel->pretty_name_of( m_field ), comparisonToString() );
    if ( m_field == "rating" ) {
        double r = m_value.toDouble() / 2.0;
        return v.arg( QString("%1 stars").arg( r ) );
    } else if ( m_field == i18n("length") ) {
        return v.arg( QTime().addMSecs( m_value.toInt() ).toString( "H:mm:ss" ) );
    } else {
        return v.arg( valueToString() );
    }
}

Collections::QueryMaker*
ConstraintTypes::TagMatch::initQueryMaker( Collections::QueryMaker* qm ) const
{
    if ( ( m_fieldsModel->type_of( m_field ) == FieldTypeInt ) ) {
        int v = m_value.toInt();

        double factor;
        int range;
        if ( m_field != i18n("length") ) {
            // compute fuzzy ranges -- this marks the boundary beyond which the fuzzy match probability is less than 1%
            factor = exp( 2.0 * m_strictness ) / ( sqrt(( double )v ) + 1.0 ); // duplicated from Constraint::compare()
            range = (int)ceil( 4.6051702 / factor );
        } else {
            // small kludge to get fuzziness to play better in the case of track lengths
            factor = exp( 2.0 * m_strictness ) / ( sqrt(( double )v/1000.0 ) + 1.0 );
            range = (int)ceil( 4605.1702 / factor );
        }
        if ( m_comparison == Constraint::CompareNumEquals ) {
            if ( !m_invert ) {
                qm->beginAnd();
                qm->addNumberFilter( m_fieldsModel->meta_value_of( m_field ), v - range, Collections::QueryMaker::GreaterThan );
                qm->addNumberFilter( m_fieldsModel->meta_value_of( m_field ), v + range, Collections::QueryMaker::LessThan );
                qm->endAndOr();
            }
        } else if ( m_comparison == Constraint::CompareNumGreaterThan ) {
            if ( m_invert )
                qm->excludeNumberFilter( m_fieldsModel->meta_value_of( m_field ), v + range, Collections::QueryMaker::GreaterThan );
            else
                qm->addNumberFilter( m_fieldsModel->meta_value_of( m_field ), v - range, Collections::QueryMaker::GreaterThan );
        } else if ( m_comparison == Constraint::CompareNumLessThan ) {
            if ( m_invert )
                qm->excludeNumberFilter( m_fieldsModel->meta_value_of( m_field ), v - range, Collections::QueryMaker::LessThan );
            else
                qm->addNumberFilter( m_fieldsModel->meta_value_of( m_field ), v + range, Collections::QueryMaker::LessThan );
        }
    } else if ( m_fieldsModel->type_of( m_field ) == FieldTypeDate ) {
        double factor = ( exp( 5.0 * m_strictness ) ) / 1e6; // duplicated from this::dateComparison()
        uint range = (uint)ceil( 4.6051702 / factor );
        uint referenceDate;
        if ( m_comparison == Constraint::CompareDateBefore ) {
            referenceDate = m_value.toDateTime().toTime_t();
            if ( m_invert )
                qm->excludeNumberFilter( m_fieldsModel->meta_value_of( m_field ), referenceDate - range, Collections::QueryMaker::LessThan );
            else
                qm->addNumberFilter( m_fieldsModel->meta_value_of( m_field ), referenceDate + range, Collections::QueryMaker::LessThan );
        } else if ( m_comparison == Constraint::CompareDateOn ) {
            referenceDate = m_value.toDateTime().toTime_t();
            if ( !m_invert ) {
                qm->beginAnd();
                qm->addNumberFilter( m_fieldsModel->meta_value_of( m_field ), referenceDate - range, Collections::QueryMaker::GreaterThan );
                qm->addNumberFilter( m_fieldsModel->meta_value_of( m_field ), referenceDate + range, Collections::QueryMaker::LessThan );
                qm->endAndOr();
            }
        } else if ( m_comparison == Constraint::CompareDateAfter ) {
            referenceDate = m_value.toDateTime().toTime_t();
            if ( m_invert )
                qm->excludeNumberFilter( m_fieldsModel->meta_value_of( m_field ), referenceDate + range, Collections::QueryMaker::GreaterThan );
            else
                qm->addNumberFilter( m_fieldsModel->meta_value_of( m_field ), referenceDate - range, Collections::QueryMaker::GreaterThan );
        } else if ( m_comparison == Constraint::CompareDateWithin ) {
            QDateTime now = QDateTime::currentDateTime();
            DateRange r = m_value.value<DateRange>();
            switch ( r.second ) {
                case 0:
                    now.addDays( -1 * r.first );
                    break;
                case 1:
                    now.addMonths( -1 * r.first );
                    break;
                case 2:
                    now.addYears( -1 * r.first );
                    break;
                default:
                    break;
            }
            referenceDate = now.toTime_t();
            if ( m_invert )
                qm->excludeNumberFilter( m_fieldsModel->meta_value_of( m_field ), referenceDate + range, Collections::QueryMaker::GreaterThan );
            else
                qm->addNumberFilter( m_fieldsModel->meta_value_of( m_field ), referenceDate - range, Collections::QueryMaker::GreaterThan );
        }
    } else if ( m_fieldsModel->type_of( m_field ) == FieldTypeString ) {
        if ( m_comparison == Constraint::CompareStrEquals ) {
            if ( m_invert )
                qm->excludeFilter( m_fieldsModel->meta_value_of( m_field ), m_value.toString(), true, true );
            else
                qm->addFilter( m_fieldsModel->meta_value_of( m_field ), m_value.toString(), true, true );
        } else if ( m_comparison == Constraint::CompareStrStartsWith ) {
            if ( m_invert )
                qm->excludeFilter( m_fieldsModel->meta_value_of( m_field ), m_value.toString(), true, false );
            else
                qm->addFilter( m_fieldsModel->meta_value_of( m_field ), m_value.toString(), true, false );
        } else if ( m_comparison == Constraint::CompareStrEndsWith ) {
            if ( m_invert )
                qm->excludeFilter( m_fieldsModel->meta_value_of( m_field ), m_value.toString(), false, true );
            else
                qm->addFilter( m_fieldsModel->meta_value_of( m_field ), m_value.toString(), false, true );
        } else if ( m_comparison == Constraint::CompareStrContains ) {
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
ConstraintTypes::TagMatch::satisfaction( const Meta::TrackList& tl )
{
    m_satisfaction = 0.0;
    foreach( Meta::TrackPtr t, tl ) {
        if ( matches( t ) ) {
            m_satisfaction += 1.0;
        }
    }
    m_satisfaction /= ( double )tl.size();
    return m_satisfaction;
}

double
ConstraintTypes::TagMatch::deltaS_insert( const Meta::TrackList& tl, const Meta::TrackPtr t, const int ) const
{
    double s = m_satisfaction * tl.size();
    if ( matches( t ) ) {
        s += 1.0;
    }
    return s / ( double )( tl.size() + 1 ) - m_satisfaction;
}

double
ConstraintTypes::TagMatch::deltaS_replace( const Meta::TrackList& tl, const Meta::TrackPtr t, const int i ) const
{
    bool oldT = matches( tl.at( i ) );
    bool newT = matches( t );
    if ( !( oldT ^ newT ) ) {
        return 0.0;
    } else if ( newT ) {
        return 1.0 / ( double )tl.size();
    } else if ( oldT ) {
        return -1.0 / ( double )tl.size();
    } else {
        return 0.0;
    }
}

double
ConstraintTypes::TagMatch::deltaS_delete( const Meta::TrackList& tl, const int i ) const
{
    double s = m_satisfaction * tl.size();
    if ( matches( tl.at( i ) ) ) {
        s -= 1.0;
    }
    return s / ( double )( tl.size() - 1 ) - m_satisfaction;
}

double
ConstraintTypes::TagMatch::deltaS_swap( const Meta::TrackList&, const int, const int ) const
{
    return 0.0;
}

void
ConstraintTypes::TagMatch::insertTrack( const Meta::TrackList& tl, const Meta::TrackPtr t, const int i )
{
    m_satisfaction += deltaS_insert( tl, t, i );
}

void
ConstraintTypes::TagMatch::replaceTrack( const Meta::TrackList& tl, const Meta::TrackPtr t, const int i )
{
    m_satisfaction += deltaS_replace( tl, t, i );
}

void
ConstraintTypes::TagMatch::deleteTrack( const Meta::TrackList& tl, const int i )
{
    m_satisfaction += deltaS_delete( tl, i );
}

void
ConstraintTypes::TagMatch::swapTracks( const Meta::TrackList&, const int, const int ) {}

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

double
ConstraintTypes::TagMatch::dateComparison( uint trackDate ) const
{
    /* comparing dates is a little bit tricky, so I split it off into its own function */
    int comp;
    uint referenceDate;
    if ( m_comparison == Constraint::CompareDateWithin ) {
        comp = Constraint::CompareDateAfter;
        QDateTime now = QDateTime::currentDateTime();
        DateRange r = m_value.value<DateRange>();
        switch ( r.second ) {
            case 0:
                now.addDays( -1 * r.first );
                break;
            case 1:
                now.addMonths( -1 * r.first );
                break;
            case 2:
                now.addYears( -1 * r.first );
                break;
            default:
                break;
        }
        referenceDate = now.toTime_t();
    } else {
        comp = m_comparison;
        referenceDate = m_value.toDateTime().toTime_t();
    }

    /* I decided to keep the comparison logic here instead of passing it down
     * to Constraint::compare() because ::compare() calculates the strictness
     * factor in proportion to the values that were passed to it.  The numbers
     * involved in date calculations (ie, seconds since the epoch) are so large
     * that they make ::compare()'s factor pretty much useless, plus the
     * strictness factor gets smaller when later dates are chosen.  This is a
     * more useful strictness factor for date calculations. -- stharward */

    double r = 0.0;
    double factor = ( exp( 5.0 * m_strictness ) ) / 1e6;
    if ( comp == Constraint::CompareDateOn ) {
        // fuzzy equals -- within 18 hours
        if ( qAbs( (double)trackDate - (double)referenceDate ) < ( 64800.0 ) )
            r = 1.0;
        else if ( trackDate > referenceDate )
            r = exp( factor * ( (double)referenceDate - (double)trackDate ) );
        else
            r = exp( factor * ( (double)trackDate - (double)referenceDate ) );
    } else if ( comp == Constraint::CompareDateAfter ) {
        r = ( trackDate > referenceDate ) ? 1.0 : exp( factor * ( (double)trackDate - (double)referenceDate ) );
    } else if ( comp == Constraint::CompareDateBefore ) {
        r = ( trackDate < referenceDate ) ? 1.0 : exp( factor * ( (double)referenceDate - (double)trackDate ) );
    } else {
        r = 0.0;
    }

    return r;
}

double
ConstraintTypes::TagMatch::labelComparison( Meta::TrackPtr t ) const
{
    DEBUG_BLOCK
    Meta::LabelList labelList = t->labels();

    double v = 0.0;
    foreach ( Meta::LabelPtr label, labelList ) {
        // this is correct ...
        // v = qMax( compare( label, m_comparison, m_value.toString() ), v );

        // ... but as long as compare() returns only 0.0 or 1.0, the following is faster:
        v = compare( label->prettyName(), m_comparison, m_value.toString() );
        if ( v == 1.0 ) {
            return 1.0;
        }
    }

    return v;
}

QString
ConstraintTypes::TagMatch::comparisonToString() const
{
    if ( m_fieldsModel->type_of( m_field ) == FieldTypeInt ) {
        if ( m_comparison == Constraint::CompareNumEquals ) {
            return QString( i18n("equals") );
        } else if ( m_comparison == Constraint::CompareNumGreaterThan ) {
            return QString( i18n("greater than") );
        } else if ( m_comparison == Constraint::CompareNumLessThan ) {
            return QString( i18n("less than") );
        }
    } else if ( m_fieldsModel->type_of( m_field ) == FieldTypeDate ) {
        if ( m_comparison == Constraint::CompareDateBefore ) {
            return QString( i18n("before") );
        } else if ( m_comparison == Constraint::CompareDateOn ) {
            return QString( i18n("on") );
        } else if ( m_comparison == Constraint::CompareDateAfter ) {
            return QString( i18n("after") );
        } else if ( m_comparison == Constraint::CompareDateWithin ) {
            return QString( i18n("within") );
        }
    } else {
        if ( m_comparison == Constraint::CompareStrEquals ) {
            return QString( i18n("equals") );
        } else if ( m_comparison == Constraint::CompareStrStartsWith ) {
            return QString( i18n("starts with") );
        } else if ( m_comparison == Constraint::CompareStrEndsWith ) {
            return QString( i18n("ends with") );
        } else if ( m_comparison == Constraint::CompareStrContains ) {
            return QString( i18n("contains") );
        } else if ( m_comparison == Constraint::CompareStrRegExp ) {
            return QString( i18n("regexp") );
        }
    }
    return QString( i18n("unknown comparison") );
}

QString
ConstraintTypes::TagMatch::valueToString() const
{
    if ( m_fieldsModel->type_of( m_field ) == FieldTypeDate )
        if ( m_comparison != Constraint::CompareDateWithin ) {
            return m_value.toDate().toString( Qt::ISODate );
        } else {
            QString unit;
            switch ( m_value.value<DateRange>().second ) {
                case 0:
                    unit = "days";
                    break;
                case 1:
                    unit = "months";
                    break;
                case 2:
                    unit = "years";
                    break;
                default:
                    break;
            }
            return QString("%1 %2").arg( m_value.value<DateRange>().first ).arg( unit );
        }
    else
        return m_value.toString();
}
    
bool
ConstraintTypes::TagMatch::matches( Meta::TrackPtr track ) const
{
    if ( !m_matchCache.contains( track->uidUrl() ) ) {
        double v = 0.0;
        int lengthInSec, targetInSec; // these are used for track length calculations below
        switch ( m_fieldsModel->meta_value_of( m_field ) ) {
            case Meta::valUrl:
                v = compare( track->prettyUrl(), m_comparison, m_value.toString() );
                break;
            case Meta::valTitle:
                v = compare( track->prettyName(), m_comparison, m_value.toString() );
                break;
            case Meta::valArtist:
                v = compare( track->artist()->prettyName(), m_comparison, m_value.toString() );
                break;
            case Meta::valAlbum:
                v = compare( track->album()->prettyName(), m_comparison, m_value.toString() );
                break;
            case Meta::valGenre:
                v = compare( track->genre()->prettyName(), m_comparison, m_value.toString() );
                break;
            case Meta::valComposer:
                v = compare( track->composer()->prettyName(), m_comparison, m_value.toString() );
                break;
            case Meta::valYear:
                v = compare<int>( track->year()->prettyName().toInt(), m_comparison, m_value.toInt() );
                break;
            case Meta::valComment:
                v = compare( track->comment(), m_comparison, m_value.toString() );
                break;
            case Meta::valTrackNr:
                v = compare<int>( track->trackNumber(), m_comparison, m_value.toInt(), m_strictness );
                break;
            case Meta::valDiscNr:
                v = compare<int>( track->discNumber(), m_comparison, m_value.toInt(), m_strictness );
                break;
            case Meta::valLength:
                // the strictness factor doesn't handle milliseconds very well
                lengthInSec = track->length() / 1000;
                targetInSec = m_value.toInt() / 1000;
                v = compare<int>( lengthInSec, m_comparison, targetInSec, m_strictness );
                break;
            case Meta::valBitrate:
                v = compare<int>( track->bitrate(), m_comparison, m_value.toInt(), m_strictness );
                break;
            case Meta::valFilesize:
                v = compare<int>( track->filesize(), m_comparison, m_value.toInt(), m_strictness );
                break;
            case Meta::valCreateDate:
                v = dateComparison( track->createDate().toTime_t() );
                break;
            case Meta::valScore:
                v = compare<double>( track->score(), m_comparison, m_value.toDouble(), m_strictness );
                break;
            case Meta::valRating:
                v = compare<int>( track->rating(), m_comparison, m_value.toInt(), m_strictness );
                break;
            case Meta::valFirstPlayed:
                v = dateComparison( track->firstPlayed() );
                break;
            case Meta::valLastPlayed:
                v = dateComparison( track->lastPlayed() );
                break;
            case Meta::valPlaycount:
                v = compare<int>( track->playCount(), m_comparison, m_value.toInt(), m_strictness );
                break;
            case Meta::valLabel:
                v = labelComparison( track );
                break;
            default:
                v = 0.0;
                break;
        }
        if ( m_invert )
            v = 1.0 - v;

        m_matchCache.insert( track->uidUrl(), ( v > ( (double)qrand() / (double)RAND_MAX ) ) );
    }
    return m_matchCache.value( track->uidUrl() );
}

void
ConstraintTypes::TagMatch::setComparison( int c )
{
    m_comparison = c;
    m_matchCache.clear();
    emit dataChanged();
}

void
ConstraintTypes::TagMatch::setField( const QString& s )
{
    m_field = s;
    m_matchCache.clear();
    emit dataChanged();
}

void
ConstraintTypes::TagMatch::setInvert( bool v )
{
    if ( m_invert != v ) {
        foreach( const QString& s, m_matchCache.keys() ) {
            m_matchCache.insert( s, !m_matchCache.value( s ) );
        }
    }
    m_invert = v;
    emit dataChanged();
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
    emit dataChanged();
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
        : QWidget( 0 )
        , m_fieldsModel( new TagMatchFieldsModel() )
{
    ui.setupUi( this );

    // fill in appropriate defaults for some attributes
    ui.kdatewidget_DateSpecific->setDate( QDate::currentDate() );

    // fill in user-specified values before the slots have been connected to we don't have to call back to the constraint a dozen times
    ui.comboBox_Field->setModel( m_fieldsModel );
    ui.checkBox_Invert->setChecked( invert );

    if ( field == i18n("rating") ) {
        ui.comboBox_ComparisonRating->setCurrentIndex( comparison );
        ui.slider_StrictnessRating->setValue( strictness );
        ui.rating_RatingValue->setRating( value.toInt() );
    } else if ( field == i18n("length") ) {
        ui.comboBox_ComparisonTime->setCurrentIndex( comparison );
        ui.slider_StrictnessTime->setValue( strictness );
        ui.timeEdit_TimeValue->setTime( QTime().addMSecs( value.toInt() ) );
    } else if ( m_fieldsModel->type_of( field ) == TagMatch::FieldTypeInt ) {
        ui.comboBox_ComparisonInt->setCurrentIndex( comparison );
        ui.slider_StrictnessInt->setValue( strictness );
        ui.spinBox_ValueInt->setValue( value.toInt() );
    } else if ( m_fieldsModel->type_of( field ) == TagMatch::FieldTypeDate ) {
        ui.comboBox_ComparisonDate->setCurrentIndex( comparison );
        ui.slider_StrictnessDate->setValue( strictness );
        if ( comparison == Constraint::CompareDateWithin ) {
            ui.stackedWidget_Date->setCurrentIndex( 1 );
            ui.spinBox_ValueDateValue->setValue( value.value<DateRange>().first );
            ui.comboBox_ValueDateUnit->setCurrentIndex( value.value<DateRange>().second );
        } else {
            ui.stackedWidget_Date->setCurrentIndex( 0 );
            ui.kdatewidget_DateSpecific->setDate( value.toDate() );
        }
    } else if ( m_fieldsModel->type_of( field ) == TagMatch::FieldTypeString ) {
        ui.comboBox_ComparisonString->setCurrentIndex( comparison );
        ui.lineEdit_StringValue->setText( value.toString() );
    }

    // set this after the slot has been connected so that it also sets the field page correctly
    ui.comboBox_Field->setCurrentIndex( m_fieldsModel->index_of( field ) );
}

// ComboBox slots for comparisons
void
ConstraintTypes::TagMatchEditWidget::on_comboBox_ComparisonDate_currentIndexChanged( int c )
{
    if ( c == Constraint::CompareDateWithin )
        ui.stackedWidget_Date->setCurrentIndex( 1 );
    else
        ui.stackedWidget_Date->setCurrentIndex( 0 );
    emit comparisonChanged( c );
}

void
ConstraintTypes::TagMatchEditWidget::on_comboBox_ComparisonInt_currentIndexChanged( int c )
{
    emit comparisonChanged( c );
}

void
ConstraintTypes::TagMatchEditWidget::on_comboBox_ComparisonRating_currentIndexChanged( int c )
{
    emit comparisonChanged( c );
}

void
ConstraintTypes::TagMatchEditWidget::on_comboBox_ComparisonString_currentIndexChanged( int c )
{
    emit comparisonChanged( c );
}

void
ConstraintTypes::TagMatchEditWidget::on_comboBox_ComparisonTime_currentIndexChanged( int c )
{
    emit comparisonChanged( c );
}

// ComboBox slots for field
void
ConstraintTypes::TagMatchEditWidget::on_comboBox_Field_currentIndexChanged( int idx )
{
    QString field = m_fieldsModel->field_at( idx );
    if ( field == i18n("length") )
        ui.stackedWidget_Field->setCurrentIndex( 3 );
    else if ( field == i18n("rating") )
        ui.stackedWidget_Field->setCurrentIndex( 4 );
    else
        ui.stackedWidget_Field->setCurrentIndex( m_fieldsModel->type_of( field ) );

    // TODO: set range limitations and default values
    // FIXME: when the field changes, it should also change the comparison and the value

    emit fieldChanged( field );
}

// Invert checkbox slot
void
ConstraintTypes::TagMatchEditWidget::on_checkBox_Invert_clicked( bool v )
{
    emit invertChanged( v );
}

// Strictness Slider slots
void
ConstraintTypes::TagMatchEditWidget::on_slider_StrictnessDate_valueChanged( int v )
{
    emit strictnessChanged( v );
}

void
ConstraintTypes::TagMatchEditWidget::on_slider_StrictnessInt_valueChanged( int v )
{
    emit strictnessChanged( v );
}

void
ConstraintTypes::TagMatchEditWidget::on_slider_StrictnessRating_valueChanged( int v )
{
    emit strictnessChanged( v );
}

void
ConstraintTypes::TagMatchEditWidget::on_slider_StrictnessTime_valueChanged( int v )
{
    emit strictnessChanged( v );
}

// various value slots
void
ConstraintTypes::TagMatchEditWidget::on_kdatewidget_DateSpecific_changed( const QDate& v )
{
    emit valueChanged( QVariant( v ) );
}

void
ConstraintTypes::TagMatchEditWidget::on_comboBox_ValueDateUnit_currentIndexChanged( int u )
{
    int v = ui.spinBox_ValueDateValue->value();
    emit valueChanged( QVariant::fromValue( DateRange( v, u ) ) );
}

void
ConstraintTypes::TagMatchEditWidget::on_spinBox_ValueDateValue_valueChanged( int v )
{
    int u = ui.comboBox_ValueDateUnit->currentIndex();
    emit valueChanged( QVariant::fromValue( DateRange( v, u ) ) );
}

void
ConstraintTypes::TagMatchEditWidget::on_spinBox_ValueInt_valueChanged( int v )
{
    emit valueChanged( QVariant( v ) );
}

void
ConstraintTypes::TagMatchEditWidget::on_lineEdit_StringValue_textChanged( const QString& v )
{
    emit valueChanged( QVariant( v ) );
}

void
ConstraintTypes::TagMatchEditWidget::on_rating_RatingValue_ratingChanged( int v )
{
    emit valueChanged( QVariant( v ) );
}

void
ConstraintTypes::TagMatchEditWidget::on_timeEdit_TimeValue_timeChanged( const QTime& t )
{
    int v = QTime().msecsTo( t );
    emit valueChanged( QVariant( v ) );
}

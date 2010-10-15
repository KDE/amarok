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

#include "core-impl/collections/support/CollectionManager.h"
#include "core/support/Debug.h"
#include "core/collections/MetaQueryMaker.h"
#include "core/collections/QueryMaker.h"
#include "widgets/MetaQueryWidget.h"
#include "widgets/kdatecombo.h"
#include "widgets/kratingwidget.h"
#include "shared/FileType.h"

#include <typeinfo>

#include <QWidget>
#include <QLineEdit>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QListView>
#include <QTimeEdit>

#include <KComboBox>
#include <KIcon>
#include <KDateTime> // for local time
#include <KNumInput>
#include <klocale.h>

using namespace Amarok;

TimeDistanceWidget::TimeDistanceWidget( QWidget *parent )
    : QWidget( parent )
{
    m_timeEdit = new KIntSpinBox(this);
    m_timeEdit->setMinimum( 0 );
    m_timeEdit->setMaximum( 600 );

    m_unitSelection = new KComboBox(this);
    connect( m_timeEdit, SIGNAL(valueChanged(int)), this, SLOT(slotUpdateComboBoxLabels(int)) );
    m_unitSelection->addItem( i18np( "second", "seconds", 0 ) );
    m_unitSelection->addItem( i18np( "minute", "minutes", 0 ) );
    m_unitSelection->addItem( i18np( "hour", "hours", 0 ) );
    m_unitSelection->addItem( i18np( "day", "days", 0 ) );

    QHBoxLayout *hLayout = new QHBoxLayout(this);
    hLayout->setContentsMargins(0, 0, 0, 0);
    hLayout->addWidget( m_timeEdit );
    hLayout->addWidget( m_unitSelection );
}

qint64 TimeDistanceWidget::timeDistance() const
{
    qint64 time = m_timeEdit->value();
    switch( m_unitSelection->currentIndex() )
    {
    case 3:
        time *= 24; // days
    case 2:
        time *= 60; // hours
    case 1:
        time *= 60; // minutes
    }

    return time;
}

void TimeDistanceWidget::setTimeDistance( qint64 value )
{
    int unit = 0;
    if( value > 600 || !(value % 60) ) {
        unit++;
        value /= 60;

        if( value > 600 || !(value % 60) ) {
            unit++;
            value /= 60;

            if( value > 600 || !(value % 24) ) {
                unit++;
                value /= 24;
            }
        }
    }

    m_unitSelection->setCurrentIndex( unit );
    m_timeEdit->setValue( value );
}

void TimeDistanceWidget::connectChanged( QObject *receiver, const char *slot )
{
    connect( m_timeEdit, SIGNAL(valueChanged(const QString&)), receiver, slot );
    connect( m_unitSelection, SIGNAL(currentIndexChanged(int)), receiver, slot );
}


void TimeDistanceWidget::slotUpdateComboBoxLabels( int value )
{
    m_unitSelection->setItemText(0, i18np("second", "seconds", value));
    m_unitSelection->setItemText(1, i18np("minute", "minutes", value));
    m_unitSelection->setItemText(2, i18np("hour", "hours", value));
    m_unitSelection->setItemText(3, i18np("day", "days", value));
}


MetaQueryWidget::MetaQueryWidget( QWidget* parent, bool onlyNumeric, bool noCondition )
    : QWidget( parent )
    , m_onlyNumeric( onlyNumeric )
    , m_noCondition( noCondition )
    , m_settingFilter( false )
    , m_andLabel(0)
    , m_compareSelection(0)
    , m_valueSelection1(0)
    , m_valueSelection2(0)
{
    // note: we are using the strange layout structure because the KRatingWidget size depends on the height.
    m_layoutMain = new QVBoxLayout( this );
    m_layoutMain->setContentsMargins(0, 0, 0, 0);

    makeFieldSelection();
    m_layoutMain->addWidget( m_fieldSelection );

    m_layoutValue = new QHBoxLayout();
    m_layoutMain->addLayout(m_layoutValue);

    m_layoutValueLabels = new QVBoxLayout();
    m_layoutValue->addLayout(m_layoutValueLabels, 0);
    m_layoutValueValues = new QVBoxLayout();
    m_layoutValue->addLayout(m_layoutValueValues, 1);

    if( m_onlyNumeric ) {
        m_filter.field = Meta::valYear;
        m_filter.condition = Equals;

    } else {
        m_filter.field = 0;
        m_filter.condition = Contains;
    }

    setFilter(m_filter);
}

MetaQueryWidget::~MetaQueryWidget()
{
}

MetaQueryWidget::Filter
MetaQueryWidget::filter() const
{
    // special handling for between
    if( m_filter.condition == Contains )
    {
        Filter f = m_filter;
        f.numValue  = qMin(m_filter.numValue, m_filter.numValue2) - 1;
        f.numValue2 = qMax(m_filter.numValue, m_filter.numValue2) + 1;
    }
    return m_filter;
}

void
MetaQueryWidget::setFilter( const MetaQueryWidget::Filter &value )
{
    m_settingFilter = true;
    m_filter = value;

    // correct filter
    if( !isNumeric( m_filter.field ) )
        m_filter.condition = Contains;
    else
        if( m_filter.condition == Contains )
            m_filter.condition = Equals;

    int index = m_fieldSelection->findData( (int)m_filter.field );
    m_fieldSelection->setCurrentIndex( index == -1 ? 0 : index );

    if( !m_noCondition )
        makeCompareSelection();
    makeValueSelection();
    setValueSelection();

    m_settingFilter = false;
    emit changed(m_filter);
}

void
MetaQueryWidget::makeFieldSelection()
{
    m_fieldSelection = new KComboBox( this );
    if (!m_onlyNumeric)
    {
        m_fieldSelection->addItem( i18n( "Simple Search" ), 0 );
        m_fieldSelection->addItem( KIcon("filename-space-amarok"), i18nc( "The name of the file this track is stored in", "File Name" ), Meta::valUrl );
        // note: what about directory?
        m_fieldSelection->addItem( KIcon("filename-title-amarok"), i18n( "Title" ), Meta::valTitle );
        // note: album artist is not handled here. could be added
        m_fieldSelection->addItem( KIcon("filename-artist-amarok"), i18n( "Artist" ), Meta::valArtist );
        m_fieldSelection->addItem( KIcon("filename-album-amarok"), i18n( "Album" ), Meta::valAlbum );
        m_fieldSelection->addItem( KIcon("filename-genre-amarok"), i18n( "Genre" ), Meta::valGenre );
        m_fieldSelection->addItem( KIcon("filename-composer-amarok"),i18n( "Composer" ), Meta::valComposer );
    }
    m_fieldSelection->addItem( KIcon("filename-year-amarok"), i18n( "Year" ), Meta::valYear );
    if (!m_onlyNumeric)
        m_fieldSelection->addItem( KIcon("filename-comment-amarok"), i18n( "Comment" ), Meta::valComment );
    m_fieldSelection->addItem( KIcon("filename-track-amarok"), i18n( "Track Number" ), Meta::valTrackNr );
    m_fieldSelection->addItem( KIcon("filename-discnumber-amarok"), i18n( "Disc Number" ), Meta::valDiscNr );
    m_fieldSelection->addItem( KIcon("filename-bpm-amarok"), i18n( "BPM" ), Meta::valBpm );
    m_fieldSelection->addItem( KIcon("filename-group-length"), i18n( "Track Length" ), Meta::valLength );
    m_fieldSelection->addItem( KIcon("application-octet-stream"), i18n( "Bit Rate" ), Meta::valBitrate );
    m_fieldSelection->addItem( KIcon("filename-sample-rate"), i18n( "Sample Rate" ), Meta::valSamplerate );
    m_fieldSelection->addItem( i18n( "File Size" ), Meta::valFilesize );
    if (!m_onlyNumeric)
        m_fieldSelection->addItem( KIcon("filename-filetype-amarok"), i18n( "Format" ), Meta::valFormat );
    m_fieldSelection->addItem( i18n( "Added" ), Meta::valCreateDate );
    m_fieldSelection->addItem( KIcon("emblem-favorite"), i18n( "Score" ), Meta::valScore );
    m_fieldSelection->addItem( KIcon("rating"), i18n( "Rating" ), Meta::valRating );
    m_fieldSelection->addItem( i18n( "First Played" ), Meta::valFirstPlayed );
    m_fieldSelection->addItem( KIcon("filename-last-played"), i18n( "Last Played" ), Meta::valLastPlayed );
    m_fieldSelection->addItem( KIcon("filename-comment-amarok"), i18n( "Play Count" ), Meta::valPlaycount );
    if (!m_onlyNumeric)
        m_fieldSelection->addItem( /* KIcon("filename-labels-amarok"),*/ i18n( "Label" ), Meta::valLabel );
    connect( m_fieldSelection, SIGNAL(currentIndexChanged(int)), this, SLOT(fieldChanged(int)) );
}

void
MetaQueryWidget::fieldChanged( int i )
{
    if( m_settingFilter )
        return;

    qint64 field = qvariant_cast<qint64>( m_fieldSelection->itemData( i ) );
    if( m_filter.field == field )
        return; // nothing to do

    if( isNumeric( m_filter.field ) != isNumeric( field ) )
        m_filter.value.clear();
    m_filter.numValue = 0;
    m_filter.numValue2 = 0;

    m_filter.field = field;

    if( isNumeric( field ) && m_filter.condition == Contains )
        m_filter.condition = Equals;
    else if( !isNumeric( field ) && m_filter.condition != Contains )
        m_filter.condition = Contains;

    // in the fieldChanged slot we assume that the field was really changed,
    // so we don't have a problem with throwing away all the old widgets

    if( !m_noCondition )
        makeCompareSelection();
    makeValueSelection();

    setValueSelection();

    emit changed(m_filter);
}

void
MetaQueryWidget::compareChanged( int index )
{
    FilterCondition condition = (FilterCondition) m_compareSelection->itemData( index ).toInt();

    if( m_filter.condition == condition )
        return; // nothing to do

    m_filter.condition = condition;

    // need to re-generate the value selection fields
    makeValueSelection();

    setValueSelection();

    emit changed(m_filter);
}

void
MetaQueryWidget::valueChanged( const QString& value )
{
    m_filter.value = value;

    emit changed(m_filter);
}

void
MetaQueryWidget::numValueChanged( int value )
{
    m_filter.numValue = value;

    emit changed(m_filter);
}

void
MetaQueryWidget::numValue2Changed( int value )
{
    m_filter.numValue2 = value;

    emit changed(m_filter);
}

void
MetaQueryWidget::numValueChanged( qint64 value )
{
    m_filter.numValue = value;

    emit changed(m_filter);
}

void
MetaQueryWidget::numValue2Changed( qint64 value )
{
    m_filter.numValue2 = value;

    emit changed(m_filter);
}

void
MetaQueryWidget::numValueChanged( const QTime& value )
{
    m_filter.numValue = qAbs( value.secsTo( QTime(0,0,0) ) );

    emit changed(m_filter);
}

void
MetaQueryWidget::numValue2Changed( const QTime& value )
{
    m_filter.numValue2 = qAbs( value.secsTo( QTime(0,0,0) ) );

    emit changed(m_filter);
}

void
MetaQueryWidget::numValueDateChanged()
{
    KDateCombo* dateSelection = qobject_cast<KDateCombo*>( sender() );
    if( dateSelection )
    {
        QDate date;
        dateSelection->getDate( &date );
        m_filter.numValue = QDateTime( date ).toTime_t();

        emit changed(m_filter);
    }
}

void
MetaQueryWidget::numValue2DateChanged()
{
    KDateCombo* dateSelection = qobject_cast<KDateCombo*>( sender() );
    if( dateSelection )
    {
        QDate date;
        dateSelection->getDate( &date );
        m_filter.numValue2 = QDateTime( date ).toTime_t();

        emit changed(m_filter);
    }
}

void
MetaQueryWidget::numValueTimeDistanceChanged()
{
    // static_cast. Remember: the TimeDistanceWidget does not have a Q_OBJECT macro
    TimeDistanceWidget* distanceSelection = static_cast<TimeDistanceWidget*>( sender()->parent() );
    m_filter.numValue = distanceSelection->timeDistance();

    emit changed(m_filter);
}

void
MetaQueryWidget::numValueFormatChanged(int index)
{
    KComboBox* combo = static_cast<KComboBox*>(sender());
    if( combo ) {
        m_filter.numValue = combo->itemData( index ).toInt();

        emit changed(m_filter);
    }
}

void
MetaQueryWidget::setValueSelection()
{
    if( m_compareSelection )
        m_layoutValueLabels->addWidget( m_compareSelection );

    if( m_filter.condition == Between )
    {
        delete m_andLabel; // delete the old label
        m_andLabel = new QLabel( i18n( "and" ), this );
        m_layoutValueLabels->addWidget( m_andLabel );
    }
    else
    {
        delete m_andLabel;
        m_andLabel = 0;
    }

    if( m_valueSelection1 )
        m_layoutValueValues->addWidget( m_valueSelection1 );

    if( m_valueSelection2 )
        m_layoutValueValues->addWidget( m_valueSelection2 );
}


void
MetaQueryWidget::makeCompareSelection()
{
    delete m_compareSelection;
    m_compareSelection = 0;

    qint64 field = m_filter.field;

    // no compare selection for text fields
    if( !isNumeric(field) || field == Meta::valFormat )
        return;

    m_compareSelection = new KComboBox();
    m_compareSelection->addItem( i18n( "Smaller Than" ), (int)LessThan );
    m_compareSelection->addItem( i18n( "Equal To" ), (int)Equals );
    m_compareSelection->addItem( i18n( "Larger Than" ), (int)GreaterThan );
    m_compareSelection->addItem( i18n( "Between" ), (int)Between );

    if( isDate( field ) )
        m_compareSelection->addItem( i18nc( "The date lies before the given one", "Older Than" ), (int)OlderThan );

    connect( m_compareSelection, SIGNAL(currentIndexChanged(int)),
            SLOT(compareChanged(int)) );

    FilterCondition condition = m_filter.condition;

    if( condition == LessThan )
        m_compareSelection->setCurrentIndex( 0 );
    else if( condition == Equals )
        m_compareSelection->setCurrentIndex( 1 );
    else if( condition == GreaterThan )
        m_compareSelection->setCurrentIndex( 2 );
    else if( condition == Between )
        m_compareSelection->setCurrentIndex( 3 );
    else if( condition == OlderThan )
        m_compareSelection->setCurrentIndex( 4 );
}

void
MetaQueryWidget::makeValueSelection()
{
    delete m_valueSelection1;
    m_valueSelection1 = 0;
    delete m_valueSelection2;
    m_valueSelection2 = 0;

    qint64 field = m_filter.field;
    if( field == Meta::valUrl )
        makeFilenameSelection();
    else if( field == Meta::valTitle )
        // We,re not going to populate this. There tends to be too many titles.
        makeGenericComboSelection( true, 0 );
    else if( field == Meta::valArtist ||
        field == Meta::valAlbum ||
        field == Meta::valGenre ||
        field == Meta::valComposer )
        makeMetaComboSelection( field );
    else if( field == Meta::valYear )
        makeGenericNumberSelection( 1900, 2300, 1976 );
    else if( field == Meta::valComment )
        makeGenericComboSelection( true, 0 );
    else if( field == Meta::valTrackNr )
        makeGenericNumberSelection( 0, 100, 0 );
    else if( field == Meta::valDiscNr )
        makeGenericNumberSelection( 0, 10, 0 );
    else if( field == Meta::valBpm )
        makeGenericNumberSelection( 60, 200, 80 );
    else if( field == Meta::valLength )
        makeLengthSelection();
    else if( field == Meta::valBitrate )
        makeGenericNumberSelection( 60, 400, 160 );
    else if( field == Meta::valSamplerate )
        makeGenericNumberSelection( 8000, 48000, 44000 );
    else if( field == Meta::valFilesize )
        makeGenericNumberSelection( 0, 200000, 1000 );
    else if( field == Meta::valFormat )
        makeFormatComboSelection();
    else if( field == Meta::valCreateDate )
        makeDateTimeSelection();
    else if( field == Meta::valScore )
        makeGenericNumberSelection( 0, 100, 0 );
    else if( field == Meta::valRating )
        makeRatingSelection();
    else if( field == Meta::valFirstPlayed )
        makeDateTimeSelection();
    else if( field == Meta::valLastPlayed )
        makeDateTimeSelection();
    else if( field == Meta::valPlaycount )
        makeGenericNumberSelection( 0, 1000, 0 );
    else if( field == Meta::valLabel )
        makeGenericComboSelection( true, 0 );
    else // e.g. the simple search
        makeGenericComboSelection( true, 0 );
}

void
MetaQueryWidget::makeGenericComboSelection( bool editable, Collections::QueryMaker* populateQuery )
{
    KComboBox* combo = new KComboBox( this );
    combo->setEditable( editable );

    if( populateQuery != 0 )
    {
        m_runningQueries.insert(populateQuery, QPointer<KComboBox>(combo));
        connect( populateQuery, SIGNAL(newResultReady(QString,QStringList)),
                SLOT(populateComboBox(QString,QStringList)) );
        connect( populateQuery, SIGNAL(queryDone()),
                SLOT(comboBoxPopulated()) );

        populateQuery->run();
    }
    combo->setEditText( m_filter.value );

    connect( combo, SIGNAL(editTextChanged( const QString& ) ),
            SLOT(valueChanged(const QString&)) );

    combo->completionObject()->setIgnoreCase( true );
    combo->setCompletionMode( KGlobalSettings::CompletionPopup );
    combo->setInsertPolicy( QComboBox::InsertAtTop );
    m_valueSelection1 = combo;
}

void
MetaQueryWidget::makeMetaComboSelection( qint64 field )
{
    Collections::QueryMaker* qm = new Collections::MetaQueryMaker( CollectionManager::instance()->queryableCollections() );
    qm->setQueryType( Collections::QueryMaker::Custom );
    qm->addReturnValue( field );
    qm->setAutoDelete( true );
    makeGenericComboSelection( true, qm );
}

void
MetaQueryWidget::populateComboBox( QString collectionId, QStringList results )
{
    Q_UNUSED(collectionId);

    QObject* query = sender();
    if( !query )
        return;

    QPointer<KComboBox> combo = m_runningQueries.value(query);
    if( combo.isNull() )
        return;

    // note: adding items seems to reset the edit text, so we have
    //   to take care of that.
    disconnect( combo, 0, this, 0 );

    // want the results unique and sorted
    const QSet<QString> dataSet = results.toSet();
    QStringList dataList = dataSet.toList();
    dataList.sort();
    combo->addItems( dataList );

    KCompletion* comp = combo->completionObject();
    comp->setItems( dataList );

    // reset the text and re-enable the signal
    combo->setEditText( m_filter.value );
    connect( combo, SIGNAL(editTextChanged( const QString& ) ),
            SLOT(valueChanged(const QString&)) );
}

void
MetaQueryWidget::makeFormatComboSelection()
{
    KComboBox* combo = new KComboBox( this );
    combo->setSizePolicy( QSizePolicy::Ignored, QSizePolicy::Preferred );

    combo->addItem( "mp3", (int)Amarok::Mp3 );
    combo->addItem( "flac", (int)Amarok::Flac );
    combo->addItem( "mp4", (int)Amarok::Mp4 );
    combo->addItem( "ogg", (int)Amarok::Ogg );

    int index = m_fieldSelection->findData( (int)m_filter.numValue );
    combo->setCurrentIndex( index == -1 ? 0 : index );

    connect( combo,
             SIGNAL( currentIndexChanged(int) ),
             SLOT( numValueFormatChanged(int) ) );

    m_valueSelection1 = combo;
}

void
MetaQueryWidget::comboBoxPopulated()
{
    QObject* query = sender();
    if( !query )
        return;

    m_runningQueries.remove( query );
}

void
MetaQueryWidget::makeFilenameSelection()
{
    // Don't populate the combobox. Too many urls.
    makeGenericComboSelection( true, 0 );
}


void
MetaQueryWidget::makeRatingSelection()
{
    KRatingWidget* ratingWidget = new KRatingWidget();
    ratingWidget->setRating( (int)m_filter.numValue );
    connect( ratingWidget, SIGNAL(ratingChanged(int)),
             this, SLOT( numValueChanged(int) ) );

    m_valueSelection1 = ratingWidget;

    if( m_filter.condition != Between )
        return;

    // second KRatingWidget for the between selection
    KRatingWidget* ratingWidget2 = new KRatingWidget();
    ratingWidget2->setRating( (int)m_filter.numValue2 );
    connect( ratingWidget2, SIGNAL(ratingChanged(int)),
             this, SLOT(numValue2Changed(int)) );

    m_valueSelection2 = ratingWidget2;
}


void
MetaQueryWidget::makeLengthSelection()
{
    QTimeEdit* timeSpin = new QTimeEdit();
    timeSpin->setDisplayFormat( "m:ss" );
    timeSpin->setMinimumTime( QTime( 0, 0, 0 ) );
    timeSpin->setMaximumTime( QTime( 0, 60, 59) );
    timeSpin->setTime( QTime().addSecs( m_filter.numValue ) );

    connect( timeSpin, SIGNAL(timeChanged(const QTime&)),
            SLOT(numValueChanged(const QTime&)) );

    m_valueSelection1 = timeSpin;

    if( m_filter.condition != Between )
        return;

    QTimeEdit* timeSpin2 = new QTimeEdit();
    timeSpin2->setDisplayFormat( "m:ss" );
    timeSpin2->setMinimumTime( QTime( 0, 0, 0 ) );
    timeSpin2->setMaximumTime( QTime( 0, 60, 59) );
    timeSpin2->setTime( QTime().addSecs( m_filter.numValue2 ) );

    connect( timeSpin2, SIGNAL(timeChanged(const QTime&)),
            SLOT(numValueChanged(const QTime&)) );

    m_valueSelection2 = timeSpin2;
}

void
MetaQueryWidget::makeGenericNumberSelection( int min, int max, int def )
{
    KIntSpinBox* spin = new KIntSpinBox();
    spin->setMinimum( min );
    spin->setMaximum( max );
    if( m_filter.condition == Contains ||
        m_filter.numValue < min || m_filter.numValue > max )
    {
        spin->setValue( def );
        numValueChanged( def );
    }
    else
        spin->setValue( m_filter.numValue );

    connect( spin, SIGNAL(valueChanged(int)),
            this, SLOT(numValueChanged(int)) );

    m_valueSelection1 = spin;

    if( m_filter.condition != Between )
        return;

    // second spin box for the between selection
    KIntSpinBox* spin2 = new KIntSpinBox();
    spin2->setMinimum( min );
    spin2->setMaximum( max );
    if( m_filter.condition == Contains ||
        m_filter.numValue2 < min || m_filter.numValue2 > max )
    {
        spin2->setValue( def );
        numValue2Changed( def );
    }
    else
        spin2->setValue( m_filter.numValue2 );

    connect( spin2, SIGNAL(valueChanged(int)),
            this, SLOT(numValue2Changed(int)) );

    m_valueSelection2 = spin2;
}


void
MetaQueryWidget::makeDateTimeSelection()
{
    if( m_filter.condition != OlderThan )
    {
        KDateCombo* dateSelection = new KDateCombo();
        QDateTime dt;
        if( m_filter.condition == Contains )
            dt = QDateTime::currentDateTime();
        else
            dt.setTime_t( m_filter.numValue );
        dateSelection->setDate( dt.date() );

        connect( dateSelection, SIGNAL(currentIndexChanged(int)),
                SLOT( numValueDateChanged() ) );

        m_valueSelection1 = dateSelection;

        if( m_filter.condition != Between )
            return;

        // second KDateCombo for the between selection
        KDateCombo* dateSelection2 = new KDateCombo();
        if( m_filter.condition == Contains )
            dt = QDateTime::currentDateTime();
        else
            dt.setTime_t( m_filter.numValue2 );
        dateSelection2->setDate( dt.date() );

        connect( dateSelection2, SIGNAL(currentIndexChanged(int)),
                SLOT( numValue2DateChanged() ) );

        m_valueSelection2 = dateSelection2;
    }
    else
    {
        TimeDistanceWidget* distanceSelection = new TimeDistanceWidget();
        distanceSelection->setTimeDistance( m_filter.numValue);

        distanceSelection->connectChanged( this, SLOT(numValueTimeDistanceChanged()));

        m_valueSelection1 = distanceSelection;
    }
}


bool MetaQueryWidget::isNumeric( qint64 field )
{
    switch( field )
    {
    case Meta::valYear:
    case Meta::valTrackNr:
    case Meta::valDiscNr:
    case Meta::valBpm:
    case Meta::valLength:
    case Meta::valBitrate:
    case Meta::valSamplerate:
    case Meta::valFilesize:
    case Meta::valFormat:
    case Meta::valCreateDate:
    case Meta::valScore:
    case Meta::valRating:
    case Meta::valFirstPlayed:
    case Meta::valLastPlayed:
    case Meta::valPlaycount:
        return true;
    default:
        return false;
    }
}

bool MetaQueryWidget::isDate( qint64 field )
{
    switch( field )
    {
    case Meta::valCreateDate:
    case Meta::valFirstPlayed:
    case Meta::valLastPlayed:
        return true;
    default:
        return false;
    }
}

QString MetaQueryWidget::Filter::fieldToString()
{
    // see also src/browsers/CollectionTreeItemModelBase.cpp

    switch( field )
    {
    case Meta::valUrl:
        return i18nc( "The name of the file this track is stored in", "filename" );
    case Meta::valTitle:
        return i18n( "title" );
    case Meta::valArtist:
        return i18n( "artist" );
    case Meta::valAlbum:
        return i18n( "album" );
    case Meta::valGenre:
        return i18n( "genre" );
    case Meta::valComposer:
        return i18n( "composer" );
    case Meta::valYear:
        return i18n( "year" );
    case Meta::valComment:
        return i18n( "comment" );
    case Meta::valTrackNr:
        return i18n( "tracknumber" );
    case Meta::valDiscNr:
        return i18n( "discnumber" );
    case Meta::valBpm:
        return i18n( "bpm" );
    case Meta::valLength:
        return i18n( "length" );
    case Meta::valBitrate:
        return i18n( "bitrate" );
    case Meta::valSamplerate:
        return i18n( "samplerate" );
    case Meta::valFilesize:
        return i18n( "filesize" );
    case Meta::valFormat:
        return i18n( "format" );
    case Meta::valCreateDate:
        return i18n( "added" );
    case Meta::valScore:
        return i18n( "score" );
    case Meta::valRating:
        return i18n( "rating" );
    case Meta::valFirstPlayed:
        return i18n( "first" );
    case Meta::valLastPlayed:
        return i18n( "played" );
    case Meta::valPlaycount:
        return i18n( "playcount" );

        /* missing here because not usefull:
           unique id,
           track gain,
           track gain peak
           album gain
           album gain peak
           album artist

           see src/core/meta/support/MetaConstants.h
           */
    case Meta::valLabel:
        return i18n( "label" );

    default:
        return "";
    }

    return QString();
}

QString MetaQueryWidget::Filter::toString( bool invert )
{
    // this member is called when there is a keyword that needs numeric attributes
    QString strField = fieldToString();
    QString strValue1 = value;
    QString strValue2 = value;

    if( field == Meta::valFormat )
    {
        if( numValue == (int)Amarok::Mp3 )
            strValue1 = "mp3";
        else if( numValue == (int)Amarok::Flac )
            strValue1 = "flac";
        else if( numValue == (int)Amarok::Mp4 )
            strValue1 = "mp4";
        else if( numValue == (int)Amarok::Ogg )
            strValue1 = "ogg";
    }
    else if( MetaQueryWidget::isDate(field) )
    {
        const QDateTime &today = KDateTime::currentLocalDateTime().dateTime();
        strValue1 = QString::number( QDateTime::fromTime_t( numValue  ).daysTo( today )) + 'd';
        strValue2 = QString::number( QDateTime::fromTime_t( numValue2 ).daysTo( today )) + 'd';
    }
    else if( MetaQueryWidget::isNumeric(field) )
    {
        strValue1 = QString::number( numValue );
        strValue2 = QString::number( numValue2 );
    }

    QString result;

    switch( condition )
    {
    case Equals:
        {
            result = strField + ":" + strValue1;
            if( invert )
                result.prepend( QChar('-') );
            break;
        }

    case GreaterThan:
        {
            result = strField + ":>" + strValue1;
            if( invert )
                result.prepend( QChar('-') );
            break;
        }

    case LessThan:
        {
            result = strField + ":<" + strValue1;
            if( invert )
                result.prepend( QChar('-') );
            break;
        }

    case Between:
        {
            if( invert )
                result = strField + ":<" + strValue1 + " OR " + strField + ":>" + strValue2;
            else
                result = strField + ":>" + strValue1 + " " + strField + ":<" + strValue2;
            break;
        }

    case OlderThan:
        {
            // a human readable time..
            int unit = 0;
            qint64 val = numValue;
            if( val > 600 || !(val % 60) ) {
                unit++;
                val /= 60;

                if( val > 600 || !(val % 60) ) {
                    unit++;
                    val /= 60;

                    if( val > 600 || !(val % 24) ) {
                        unit++;
                        val /= 24;
                    }
                }
            }
            QChar strUnit('s');
            if( unit==1 )
                strUnit = 'M';
            else if( unit==2 )
                strUnit = 'h';
            else if( unit==3 )
                strUnit = 'd';

            result = strField + ":>" + QString::number(val) + strUnit;
            if( invert )
                result.prepend( QChar('-') );
            break;
        }

    case Contains:
        {
            result = value;
            if( invert )
                result.prepend( QChar('-') );
            break;
        }
    }
    return result;
}

#include "MetaQueryWidget.moc"


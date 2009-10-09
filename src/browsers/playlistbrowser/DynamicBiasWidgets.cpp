/****************************************************************************************
 * Copyright (c) 2008 Daniel Caleb Jones <danielcjones@gmail.com>                       *
 * Copyright (c) 2009 Mark Kretschmann <kretschmann@kde.org>                            *
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

#include "DynamicBiasWidgets.h"

#include "App.h"
#include "Bias.h"
#include "BiasedPlaylist.h"
#include "CollectionManager.h"
#include "Debug.h"
#include "DynamicPlaylist.h"
#include "DynamicBiasModel.h"
#include "DynamicModel.h"
#include "MetaQueryMaker.h"
#include "QueryMaker.h"
#include "SliderWidget.h"
#include "SvgHandler.h"
#include "collection/support/XmlQueryWriter.h"
#include "widgets/kratingwidget.h"
#include "widgets/kdatecombo.h"

#include <typeinfo>

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListView>
#include <QPaintEvent>
#include <QPainter>
#include <QSpinBox>
#include <QTimeEdit>
#include <QToolButton>

#include <KComboBox>
#include <KIcon>
#include <KVBox>
#include <klocale.h>

PlaylistBrowserNS::BiasBoxWidget::BiasBoxWidget( QWidget* parent )
    : QWidget( parent ), m_alternate(false)
{
}

void
PlaylistBrowserNS::BiasBoxWidget::paintEvent( QPaintEvent* e )
{
    Q_UNUSED(e)


    // is it showing ?
    QListView* parentList = dynamic_cast<QListView*>(parent());
    if( parentList )
    {
        PlaylistBrowserNS::DynamicBiasModel* model =
            dynamic_cast<PlaylistBrowserNS::DynamicBiasModel*>( parentList->model() );
        if( model )
        {
            QRect rect = parentList->visualRect( model->indexOf( this ) );
            if( rect.x() < 0 || rect.y() < 0 || rect.height() < height() )
            {
                hide();
                return;
            }
        }
    }

    QPainter painter(this);
    painter.setRenderHint( QPainter::Antialiasing, true );

    QPixmap body;

    body = The::svgHandler()->renderSvgWithDividers( "body", width(), height(), "body" );

    painter.drawPixmap( 0, 0, body );

    painter.end();
}

void
PlaylistBrowserNS::BiasBoxWidget::resizeEvent( QResizeEvent* )
{
    emit widgetChanged( this );
}



PlaylistBrowserNS::BiasAddWidget::BiasAddWidget( const QString& caption, const QString& description, QWidget* parent )
    : BiasBoxWidget( parent )
    , m_addButton( new QToolButton( this ) )
{
    DEBUG_BLOCK

    QHBoxLayout* mainLayout = new QHBoxLayout( this );
    m_addButton->setIcon( KIcon( "list-add-amarok" ) );
    m_addButton->setToolTip( i18n( "Add a new bias." ) );
    connect( m_addButton, SIGNAL( clicked() ), SLOT( slotClicked() ) );

    QLabel* descLabel = new QLabel( QString("<b>%1</b><br>%2").arg(caption, description), this );
    descLabel->setAlignment( Qt::AlignCenter );
    descLabel->setWordWrap( true );

    mainLayout->addWidget( m_addButton );
    mainLayout->setStretchFactor( m_addButton, 0 );
    mainLayout->setAlignment( m_addButton, Qt::AlignLeft | Qt::AlignVCenter );
    mainLayout->addWidget( descLabel );
    mainLayout->setStretchFactor( descLabel, 1 );

    setLayout( mainLayout );
}

// TODO: For some reason this only works with double-click. Also, mouseReleaseEvent() doesn't work at all. Don't know why.
void
PlaylistBrowserNS::BiasAddWidget::mousePressEvent( QMouseEvent* event )
{
    DEBUG_BLOCK

    slotClicked();

    BiasBoxWidget::mousePressEvent( event );
}

void
PlaylistBrowserNS::BiasAddWidget::slotClicked()
{
    DEBUG_BLOCK

    emit addBias();
}




PlaylistBrowserNS::BiasWidget::BiasWidget( Dynamic::Bias* b, QWidget* parent )
    : BiasBoxWidget( parent )
    , m_mainLayout( new KVBox( this ) )
    , m_bias(b)
{
    DEBUG_BLOCK

    QHBoxLayout* hLayout = new QHBoxLayout( this );

    QToolButton* removeButton = new QToolButton( this );
    removeButton->setIcon( KIcon( "list-remove-amarok" ) );
    removeButton->setToolTip( i18n( "Remove this bias." ) );
    connect( removeButton, SIGNAL( clicked() ), SLOT( biasRemoved() ), Qt::QueuedConnection );

    hLayout->addWidget( removeButton );
    hLayout->setStretchFactor( removeButton, 0 );
    hLayout->setAlignment( removeButton, Qt::AlignLeft | Qt::AlignVCenter );

    hLayout->addWidget( m_mainLayout );

    setLayout( hLayout );
}

void
PlaylistBrowserNS::BiasWidget::biasRemoved()
{
    emit biasRemoved( m_bias );
}


PlaylistBrowserNS::BiasGlobalWidget::BiasGlobalWidget(
        Dynamic::GlobalBias* bias, QWidget* parent )
    : BiasWidget( bias, parent )
    , m_withLabel(0)
    , m_valueSelection(0)
    , m_compareSelection(0)
    , m_gbias( bias )
    , m_filter( bias->filter() )
{
    DEBUG_BLOCK

    m_controlFrame = new QFrame( this );
    layout()->addWidget( m_controlFrame );
    m_controlLayout = new QGridLayout( m_controlFrame );
    m_controlFrame->setLayout( m_controlLayout );

    QHBoxLayout* sliderLayout = new QHBoxLayout();
    m_controlLayout->addLayout( sliderLayout, 0, 1 );

    m_weightLabel = new QLabel( " 0%", m_controlFrame );
    m_weightSelection = new Amarok::Slider( Qt::Horizontal, 100, m_controlFrame );
    m_weightSelection->setToolTip(
            i18n( "This controls what portion of the playlist should match the criteria" ) );
    connect( m_weightSelection, SIGNAL(valueChanged(int)),
            this, SLOT(weightChanged(int)) );

    m_fieldSelection = new KComboBox( m_controlFrame );
    m_fieldSelection->setPalette( QApplication::palette() );

    m_controlLayout->addWidget( new QLabel( i18n( "Proportion:" ), m_controlFrame ), 0, 0 );
    m_controlLayout->addWidget( new QLabel( i18n( "Match:" ), m_controlFrame ), 1, 0 );

    m_controlLayout->addWidget( m_weightSelection, 0, 1 );
    m_controlLayout->addWidget( m_weightLabel, 0, 1 );
    m_controlLayout->addWidget( m_fieldSelection, 1, 1 );

    sliderLayout->addWidget( m_weightSelection );
    sliderLayout->addWidget( m_weightLabel );

    popuplateFieldSection();
    connect( m_fieldSelection, SIGNAL(currentIndexChanged(int)),
            this, SLOT(fieldChanged(int)) );
    m_fieldSelection->setCurrentIndex( 0 );
    
    syncControlsToBias();
}

void
PlaylistBrowserNS::BiasGlobalWidget::syncBiasToControls()
{
    m_gbias->setQuery( m_filter );
    m_gbias->setActive( true );

    emit biasChanged( m_bias );
}

void
PlaylistBrowserNS::BiasGlobalWidget::syncControlsToBias()
{
    int index = m_fieldSelection->findData( m_filter.field );
    m_fieldSelection->setCurrentIndex( index == -1 ? 0 : index );

    m_weightSelection->setValue( (int)(m_gbias->weight() * 100.0) );
}


void
PlaylistBrowserNS::BiasGlobalWidget::popuplateFieldSection()
{
    m_fieldSelection->addItem( "", (qint64)0 );
    m_fieldSelection->addItem( i18n( "Artist" ), Meta::valArtist );
    m_fieldSelection->addItem( i18n( "Composer" ), Meta::valComposer );
    m_fieldSelection->addItem( i18n( "Album" ), Meta::valAlbum );
    m_fieldSelection->addItem( i18n( "Title" ), Meta::valTitle );
    m_fieldSelection->addItem( i18n( "Genre" ), Meta::valGenre );
    m_fieldSelection->addItem( i18n( "Year" ), Meta::valYear );
    m_fieldSelection->addItem( i18n( "Play Count" ), Meta::valPlaycount );
    m_fieldSelection->addItem( i18n( "Rating" ), Meta::valRating );
    m_fieldSelection->addItem( i18n( "Score" ), Meta::valScore );
    m_fieldSelection->addItem( i18n( "Length" ), Meta::valLength );
    m_fieldSelection->addItem( i18n( "Track #" ), Meta::valTrackNr );
    m_fieldSelection->addItem( i18n( "Disc #" ), Meta::valDiscNr );
    m_fieldSelection->addItem( i18n( "First Played" ), Meta::valFirstPlayed );
    m_fieldSelection->addItem( i18n( "Last Played" ), Meta::valLastPlayed );
    m_fieldSelection->addItem( i18n( "Comment" ), Meta::valComment );
    m_fieldSelection->addItem( i18nc( "The name of the file this track is stored in", "Filename" ), Meta::valUrl );
}

void
PlaylistBrowserNS::BiasGlobalWidget::weightChanged( int ival )
{
    double fval = (double)ival;
    m_weightLabel->setText( QString().sprintf( "%2.0f%%", fval ) );

    m_gbias->setWeight( fval / 100.0 );

    emit biasChanged( m_bias );
}

void
PlaylistBrowserNS::BiasGlobalWidget::fieldChanged( int i )
{
    qint64 field = qvariant_cast<qint64>( m_fieldSelection->itemData( i ) );
    m_filter.field = field;
    if( field != m_gbias->filter().field )
    {
        m_filter.value.clear();
        syncBiasToControls();
    }

    if( field == 0 )
    {
        delete m_compareSelection;
        m_compareSelection = 0;
        delete m_withLabel;
        m_withLabel = 0;
        delete m_valueSelection;
        m_valueSelection = 0;
    }
    else if( field == Meta::valArtist )
        makeArtistSelection();
    else if( field == Meta::valComposer )
        makeComposerSelection();
    else if( field == Meta::valAlbum )
        makeAlbumSelection();
    else if( field == Meta::valTitle )
        makeTitleSelection();
    else if( field == Meta::valGenre )
        makeGenreSelection();
    else if( field == Meta::valYear )
        makeGenericNumberSelection( 0, 3000, 1976 );
    else if( field == Meta::valPlaycount )
        makeGenericNumberSelection( 0, 10000, 0 );
    else if( field == Meta::valRating )
        makeRatingSelection();
    else if( field == Meta::valScore )
        makeGenericNumberSelection( 0, 100, 0 );
    else if( field == Meta::valLength )
        makeLengthSelection();
    else if( field == Meta::valTrackNr )
        makeGenericNumberSelection( 0, 1000, 0 );
    else if( field == Meta::valDiscNr )
        makeGenericNumberSelection( 0, 1000, 0 );
    else if( field == Meta::valFirstPlayed )
        makeDateTimeSelection();
    else if( field == Meta::valLastPlayed )
        makeDateTimeSelection();
    else if( field == Meta::valComment )
        makeGenericComboSelection( true, 0 );
    else if( field == Meta::valUrl )
        makeFilenameSelection();

    //etc
}

void
PlaylistBrowserNS::BiasGlobalWidget::compareChanged( int index )
{
    if( index < 0 )
        m_filter.compare = -1;
    else if( index < m_compareSelection->count() )
    {
        m_filter.compare = 
            qvariant_cast<int>(
                    m_compareSelection->itemData( index ) );
    }

    syncBiasToControls();
}


void
PlaylistBrowserNS::BiasGlobalWidget::setValueSelection( QWidget* w )
{
    if( m_withLabel == 0 )
    {
        m_withLabel = new QLabel( i18n( "With:" ), m_controlFrame );
        m_controlLayout->addWidget( m_withLabel, 2, 0 );
    }

    delete m_valueSelection;
    m_valueSelection = w;
    m_controlLayout->addWidget( m_valueSelection, 2, 1 );

}

void
PlaylistBrowserNS::BiasGlobalWidget::valueChanged( int value )
{
    m_filter.value = QString::number( value );
    syncBiasToControls();
}

void
PlaylistBrowserNS::BiasGlobalWidget::valueChanged( const QString& value )
{
    m_filter.value = value;
    syncBiasToControls();
}

void
PlaylistBrowserNS::BiasGlobalWidget::valueDateChanged()
{
    KDateCombo* dateSelection = qobject_cast<KDateCombo*>( sender() );
    if( dateSelection )
    {
        QDate date;
        dateSelection->getDate( &date );
        QDateTime dt( date );
        m_filter.value = QString::number( dt.toTime_t() );
        syncBiasToControls();
    }
}

void
PlaylistBrowserNS::BiasGlobalWidget::valueChanged( const QTime& value )
{
    m_filter.value = QString::number( qAbs( value.secsTo( QTime(0,0,0) ) ) );
    syncBiasToControls();
}


void
PlaylistBrowserNS::BiasGlobalWidget::makeCompareSelection( QWidget* parent )
{
    m_compareSelection = new KComboBox( parent );
    m_compareSelection->setPalette( QApplication::palette() );
    m_compareSelection->addItem( "", -1 );
    m_compareSelection->addItem( i18n( "less than" ),    (int)QueryMaker::LessThan );
    m_compareSelection->addItem( i18n( "equal to" ),     (int)QueryMaker::Equals );
    m_compareSelection->addItem( i18n( "greater than" ), (int)QueryMaker::GreaterThan );

    connect( m_compareSelection, SIGNAL(currentIndexChanged(int)),
            SLOT(compareChanged(int)) );

    int val = m_gbias->filter().compare;

    if( val == -1 )
        m_compareSelection->setCurrentIndex( 0 );
    if( val == (int)QueryMaker::LessThan )
        m_compareSelection->setCurrentIndex( 1 );
    else if( val == (int)QueryMaker::Equals )
        m_compareSelection->setCurrentIndex( 2 );
    else if( val == (int)QueryMaker::GreaterThan )
        m_compareSelection->setCurrentIndex( 3 );

}

void
PlaylistBrowserNS::BiasGlobalWidget::makeGenericComboSelection( bool editable, QueryMaker* populateQuery )
{
    KComboBox* combo = new KComboBox( m_controlFrame );
    combo->setPalette( QApplication::palette() );
    combo->setSizePolicy( QSizePolicy::Ignored, QSizePolicy::Preferred );
    combo->setEditable( editable );

    combo->addItem( QString() );

    if( populateQuery != 0 )
    {
        m_runningQueries[populateQuery] = combo;
        connect( populateQuery, SIGNAL(newResultReady(QString,QStringList)),
                SLOT(populateComboBox(QString,QStringList)) );
        connect( populateQuery, SIGNAL(queryDone()),
                SLOT(comboBoxPopulated()) );

        populateQuery->run();
    }


    connect( combo, SIGNAL(currentIndexChanged( const QString& )),
            SLOT(valueChanged(const QString&)) );
    connect( combo, SIGNAL(editTextChanged( const QString& ) ),
            SLOT(valueChanged(const QString&)) );

    combo->setEditText( m_gbias->filter().value );

    combo->setCompletionMode( KGlobalSettings::CompletionPopup );
    setValueSelection( combo );
}


void
PlaylistBrowserNS::BiasGlobalWidget::populateComboBox( QString collectionId, QStringList results )
{
    Q_UNUSED(collectionId);

    QueryMaker* query = qobject_cast<QueryMaker*>( sender() );
    if( !query )
        return;

    KComboBox* combo = m_runningQueries[query];
    if( !combo )
        return;

    QSet<QString> dataSet;
    foreach( const QString &r, results )
        dataSet += r;

    QStringList dataList = dataSet.toList();
    dataList.sort();
    foreach( const QString &item, dataList )
        combo->addItem( item );

    KCompletion* comp = combo->completionObject();
    comp->setItems( dataList );
}


void
PlaylistBrowserNS::BiasGlobalWidget::comboBoxPopulated()
{
    QueryMaker* query = qobject_cast<QueryMaker*>( sender() );
    if( !query )
        return;

    m_runningQueries.remove( query );
    query->deleteLater();
}



void
PlaylistBrowserNS::BiasGlobalWidget::makeArtistSelection()
{
    QueryMaker* qm = new MetaQueryMaker( CollectionManager::instance()->queryableCollections() );
    qm->setQueryType( QueryMaker::Custom );
    qm->addReturnValue( Meta::valArtist );
    makeGenericComboSelection( false, qm );
}


void
PlaylistBrowserNS::BiasGlobalWidget::makeComposerSelection()
{
    QueryMaker* qm = new MetaQueryMaker( CollectionManager::instance()->queryableCollections() );
    qm->setQueryType( QueryMaker::Custom );
    qm->addReturnValue( Meta::valComposer );
    makeGenericComboSelection( false, qm );
}


void
PlaylistBrowserNS::BiasGlobalWidget::makeAlbumSelection()
{
    QueryMaker* qm = new MetaQueryMaker( CollectionManager::instance()->queryableCollections() );
    qm->setQueryType( QueryMaker::Custom );
    qm->addReturnValue( Meta::valAlbum );
    makeGenericComboSelection( false, qm );
}


void
PlaylistBrowserNS::BiasGlobalWidget::makeTitleSelection()
{
    // We,re not going to populate this. There tends to be too many titles.
    makeGenericComboSelection( true, 0 );
}


void
PlaylistBrowserNS::BiasGlobalWidget::makeGenreSelection()
{
    QueryMaker* qm = new MetaQueryMaker( CollectionManager::instance()->queryableCollections() );
    qm->setQueryType( QueryMaker::Custom );
    qm->addReturnValue( Meta::valGenre );
    makeGenericComboSelection( false, qm );
}

void
PlaylistBrowserNS::BiasGlobalWidget::makeFilenameSelection()
{
    // Don't populate the combobox. Too many urls.
    makeGenericComboSelection( true, 0 );
}


void
PlaylistBrowserNS::BiasGlobalWidget::makeRatingSelection()
{
    KHBox* hLayout = new KHBox( m_controlFrame );

    makeCompareSelection( hLayout );

    KRatingWidget* ratingWidget = new KRatingWidget( hLayout );

    connect( ratingWidget, SIGNAL(ratingChanged(int)),
             this, SLOT(valueChanged(int)) );
    
    ratingWidget->setRating( m_gbias->filter().value.toInt() );
    setValueSelection( hLayout );
}


void
PlaylistBrowserNS::BiasGlobalWidget::makeLengthSelection()
{
    KHBox* hLayout = new KHBox( m_controlFrame );

    makeCompareSelection( hLayout );

    QTimeEdit* timeSpin = new QTimeEdit( hLayout );
    timeSpin->setDisplayFormat( "m:ss" );
    QTime min( 0, 0, 0 );
    QTime max( 0, 60, 59 );
    timeSpin->setMinimumTime( min );
    timeSpin->setMaximumTime( max );

    connect( timeSpin, SIGNAL(timeChanged(const QTime&)),
            SLOT(valueChanged(const QTime&)) );

    timeSpin->setTime( QTime().addSecs( m_gbias->filter().value.toInt() ) );

    setValueSelection( hLayout );
}

void
PlaylistBrowserNS::BiasGlobalWidget::makeGenericNumberSelection( int min, int max, int def )
{
    KHBox* hLayout = new KHBox( m_controlFrame );

    makeCompareSelection( hLayout );

    QSpinBox* spin = new QSpinBox( hLayout );
    spin->setMinimum( min );
    spin->setMaximum( max );

    connect( spin, SIGNAL(valueChanged( const QString& )),
            this, SLOT(valueChanged(const QString&)) );

    if( m_gbias->filter().value.isEmpty() )
        spin->setValue( def );
    else
        spin->setValue( m_gbias->filter().value.toInt() );

    setValueSelection( hLayout );
}


void
PlaylistBrowserNS::BiasGlobalWidget::makeDateTimeSelection()
{
    KHBox* hLayout = new KHBox( m_controlFrame );

    makeCompareSelection( hLayout );

    KDateCombo* dateSelection = new KDateCombo( hLayout );

    connect( dateSelection, SIGNAL(currentIndexChanged(int)),
            SLOT( valueDateChanged() ) );

    QDateTime dt;
    if( m_gbias->filter().value.isEmpty() )
        dt = QDateTime::currentDateTime();
    else
        dt.setTime_t( m_gbias->filter().value.toUInt() );

    dateSelection->setDate( dt.date() );

    setValueSelection( hLayout );
}




PlaylistBrowserNS::BiasNormalWidget::BiasNormalWidget( Dynamic::NormalBias* bias, QWidget* parent )
    : BiasWidget( bias, parent )
    , m_controlFrame(0)
    , m_controlLayout(0)
    , m_fieldSelection(0)
    , m_withLabel(0)
    , m_valueSelection(0)
    , m_scaleSelection(0)
    , m_scaleLabel(0)
    , m_nbias(bias) 
{
    m_controlFrame = new QFrame( this );
    layout()->addWidget( m_controlFrame );
    m_controlLayout = new QGridLayout( m_controlFrame );
    m_controlFrame->setLayout( m_controlLayout );

    QHBoxLayout* sliderLayout = new QHBoxLayout();
    m_controlLayout->addLayout( sliderLayout, 0, 1 );

    m_scaleLabel = new QLabel( " 0%", m_controlFrame );
    m_scaleSelection = new Amarok::Slider( Qt::Horizontal, 100, m_controlFrame );
    m_scaleSelection->setToolTip(
            i18n( "This controls how strictly to match the given value." ) );
    connect( m_scaleSelection, SIGNAL(valueChanged(int)),
            SLOT(scaleChanged(int)) );

    m_fieldSelection = new KComboBox( m_controlFrame );
    m_fieldSelection->setPalette( QApplication::palette() );

    m_controlLayout->addWidget( new QLabel( i18n( "Strictness:" ), m_controlFrame ), 0, 0 );
    m_controlLayout->addWidget( new QLabel( i18n( "Match:" ), m_controlFrame ), 1, 0 );

    m_controlLayout->addWidget( m_scaleSelection, 0, 1 );
    m_controlLayout->addWidget( m_scaleLabel, 0, 1 );
    m_controlLayout->addWidget( m_fieldSelection, 1, 1 );

    sliderLayout->addWidget( m_scaleSelection );
    sliderLayout->addWidget( m_scaleLabel );

    popuplateFieldSection();
    connect( m_fieldSelection, SIGNAL(currentIndexChanged(int)),
            this, SLOT(fieldChanged(int)) );

    int index = m_fieldSelection->findData( m_nbias->field() );
    m_fieldSelection->setCurrentIndex( index == -1 ? 0 : index );
    m_scaleSelection->setValue( (int)(m_nbias->scale() * 100.0) );
}


void
PlaylistBrowserNS::BiasNormalWidget::popuplateFieldSection()
{
    m_fieldSelection->addItem( "", (qint64)0 );
    m_fieldSelection->addItem( i18n( "Year" ), Meta::valYear );
    m_fieldSelection->addItem( i18n( "Play Count" ), Meta::valPlaycount );
    m_fieldSelection->addItem( i18n( "Rating" ), Meta::valRating );
    m_fieldSelection->addItem( i18n( "Score" ), Meta::valScore );
    m_fieldSelection->addItem( i18n( "Length" ), Meta::valLength );
    m_fieldSelection->addItem( i18n( "Track #" ), Meta::valTrackNr );
    m_fieldSelection->addItem( i18n( "Disc #" ), Meta::valDiscNr );
    m_fieldSelection->addItem( i18n( "First Played" ), Meta::valFirstPlayed );
    m_fieldSelection->addItem( i18n( "Last Played" ), Meta::valLastPlayed );
}

void
PlaylistBrowserNS::BiasNormalWidget::setValueSelection( QWidget* w )
{
    if( m_withLabel == 0 )
    {
        m_withLabel = new QLabel( i18n( "With:" ), m_controlFrame );
        m_controlLayout->addWidget( m_withLabel, 2, 0 );
    }

    delete m_valueSelection;
    m_valueSelection = w;
    m_controlLayout->addWidget( m_valueSelection, 2, 1 );

}

void
PlaylistBrowserNS::BiasNormalWidget::scaleChanged( int ival )
{
    double fval = (double)ival;
    m_scaleLabel->setText( QString().sprintf( "%2.0f%%", fval ) );

    if( fval / 100.0 != m_nbias->scale() )
    {
        m_nbias->setScale( fval / 100.0 );

        emit biasChanged( m_bias );
    }
}


void
PlaylistBrowserNS::BiasNormalWidget::fieldChanged( int i )
{
    qint64 field = qvariant_cast<qint64>( m_fieldSelection->itemData( i ) );

    if( field != m_nbias->field() )
    {
        m_nbias->setField( field );
        emit biasChanged( m_bias );
    }

    if( field == 0 )
    {
        delete m_valueSelection;
        m_valueSelection = 0;
    }
    else if( field == Meta::valYear )
        makeGenericNumberSelection( 0, 3000 );
    else if( field == Meta::valPlaycount )
        makeGenericNumberSelection( 0, 1000 );
    else if( field == Meta::valRating )
        makeRatingSelection();
    else if( field == Meta::valScore )
        makeGenericNumberSelection( 0, 100 );
    else if( field == Meta::valLength )
        makeLengthSelection();
    else if( field == Meta::valTrackNr )
        makeGenericNumberSelection( 0, 50 );
    else if( field == Meta::valDiscNr )
        makeGenericNumberSelection( 0, 50 );
    else if( field == Meta::valFirstPlayed )
        makeDateTimeSelection();
    else if( field == Meta::valLastPlayed )
        makeDateTimeSelection();
}

void
PlaylistBrowserNS::BiasNormalWidget::valueChanged( int value )
{
    m_nbias->setValue( (double)value );
    emit biasChanged( m_bias );
}

void
PlaylistBrowserNS::BiasNormalWidget::valueDateChanged()
{
    KDateCombo* dateSelection = qobject_cast<KDateCombo*>( sender() );
    if( dateSelection )
    {
        QDate date;
        dateSelection->getDate( &date );
        QDateTime dt( date );
        m_nbias->setValue( (double)dt.toTime_t() );
        emit biasChanged( m_bias );
    }
}

void
PlaylistBrowserNS::BiasNormalWidget::valueChanged( const QTime& value )
{
    m_nbias->setValue( (double)qAbs( value.secsTo( QTime(0,0,0) ) ) );
    emit biasChanged( m_bias );
}

void
PlaylistBrowserNS::BiasNormalWidget::makeGenericNumberSelection( int min, int max )
{
    KHBox* hLayout = new KHBox( m_controlFrame );

    QSpinBox* spin = new QSpinBox( hLayout );
    spin->setMinimum( min );
    spin->setMaximum( max );

    connect( spin, SIGNAL(valueChanged(int)),
            SLOT(valueChanged(int)) );

    spin->setValue( m_nbias->value() );

    setValueSelection( hLayout );
}


void
PlaylistBrowserNS::BiasNormalWidget::makeRatingSelection()
{
    KHBox* hLayout = new KHBox( m_controlFrame );

    KRatingWidget* ratingWidget = new KRatingWidget( hLayout );

    connect( ratingWidget, SIGNAL(ratingChanged(int)),
             this, SLOT(valueChanged(int)) );
    
    ratingWidget->setRating( (unsigned int)m_nbias->value() );
    setValueSelection( hLayout );
}

void
PlaylistBrowserNS::BiasNormalWidget::makeLengthSelection()
{
    KHBox* hLayout = new KHBox( m_controlFrame );

    QTimeEdit* timeSpin = new QTimeEdit( hLayout );
    timeSpin->setDisplayFormat( "m:ss" );
    QTime min( 0, 0, 0 );
    QTime max( 0, 60, 59 );
    timeSpin->setMinimumTime( min );
    timeSpin->setMaximumTime( max );

    connect( timeSpin, SIGNAL(timeChanged(const QTime&)),
            SLOT(valueChanged(const QTime&)) );

    timeSpin->setTime( QTime().addSecs( m_nbias->value() ) );

    setValueSelection( hLayout );
}

void
PlaylistBrowserNS::BiasNormalWidget::makeDateTimeSelection()
{
    KHBox* hLayout = new KHBox( m_controlFrame );

    KDateCombo* dateSelection = new KDateCombo( hLayout );

    connect( dateSelection, SIGNAL(currentIndexChanged(int)),
            SLOT( valueDateChanged() ) );

    QDateTime dt;
    if( m_nbias->value() == 0.0 )
        dt = QDateTime::currentDateTime();
    else
        dt.setTime_t( (qint64)m_nbias->value() );

    dateSelection->setDate( dt.date() );

    setValueSelection( hLayout );
}

#include "DynamicBiasWidgets.moc"


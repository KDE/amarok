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
#include "core-impl/collections/support/CollectionManager.h"
#include "core/support/Debug.h"
#include "DynamicPlaylist.h"
#include "DynamicBiasModel.h"
#include "DynamicModel.h"
#include "core/collections/MetaQueryMaker.h"
#include "core/collections/QueryMaker.h"
#include "SliderWidget.h"
#include "SvgHandler.h"
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
#include <Nepomuk/KRatingWidget>

/**
 *  A class that allows to select a time distance.
 *  Used for GlobalBiasWidget.
 */
class TimeDistanceWidget : public KHBox
{
public:
    TimeDistanceWidget( QWidget *parent = 0 )
        : KHBox( parent )
    {
        m_timeEdit = new QSpinBox(this);
        m_timeEdit->setMinimum( 0 );
        m_timeEdit->setMaximum( 600 );

        m_unitSelection = new KComboBox(this);
        m_unitSelection->addItem( i18n( "seconds" ) );
        m_unitSelection->addItem( i18n( "minutes" ) );
        m_unitSelection->addItem( i18n( "hours" ) );
        m_unitSelection->addItem( i18n( "days" ) );
    }

    qint64 timeDistance() const
    {
        qint64 time = m_timeEdit->value();
        switch( m_unitSelection->currentIndex() )
        {
        case 3:
            time *= 24 ; // days
        case 2:
            time *= 60; // hours
        case 1:
            time *= 60; // minutes
        }

        return time;
    }

    void setTimeDistance( qint64 value )
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
                    value /= 60;
                }
            }
        }

        m_unitSelection->setCurrentIndex( unit );
        m_timeEdit->setValue( value );
    }

    // cmake is tricking me here. It will create a private .moc with the same
    // name as the public moc. So no Q_OBJECT macro fro my TimeDistanceWidget
    void connectChanged( QObject *receiver, const char *slot )
    {
        connect( m_timeEdit, SIGNAL(valueChanged(int)), receiver, slot );
        connect( m_unitSelection, SIGNAL(currentIndexChanged(int)), receiver, slot );
    }

protected:
    QSpinBox *m_timeEdit;
    KComboBox *m_unitSelection;
};


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
    , m_bias(b)
{
    DEBUG_BLOCK

    QHBoxLayout* mainLayout = new QHBoxLayout( this );

    QToolButton* removeButton = new QToolButton( this );
    removeButton->setIcon( KIcon( "list-remove-amarok" ) );
    removeButton->setToolTip( i18n( "Remove this bias." ) );
    connect( removeButton, SIGNAL( clicked() ), SLOT( biasRemoved() ), Qt::QueuedConnection );

    mainLayout->addWidget( removeButton );
    mainLayout->setStretchFactor( removeButton, 0 );
    mainLayout->setAlignment( removeButton, Qt::AlignLeft | Qt::AlignVCenter );

    setLayout( mainLayout );
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
    , m_andLabel(0)
    , m_compareSelection(0)
    , m_valueSelection1(0)
    , m_valueSelection2(0)
    , m_gbias( bias )
    , m_filter( bias->filter() )
{
    DEBUG_BLOCK

    m_controlFrame = new QFrame( this );
    m_controlLayout = new QGridLayout( m_controlFrame );
    m_controlFrame->setLayout( m_controlLayout );


    m_weightLabel = new QLabel( " 0%", m_controlFrame );
    m_weightSelection = new Amarok::Slider( Qt::Horizontal, 100, m_controlFrame );
    m_weightSelection->setToolTip(
            i18n( "This controls what portion of the playlist should match the criteria" ) );
    connect( m_weightSelection, SIGNAL(valueChanged(int)),
            this, SLOT(weightChanged(int)) );

    makeFieldSelection();

    QHBoxLayout* sliderLayout = new QHBoxLayout();
    sliderLayout->addWidget( m_weightSelection );
    sliderLayout->addWidget( m_weightLabel );

    m_controlLayout->addWidget( new QLabel( i18n( "Proportion:" ), m_controlFrame ), 0, 0 );
    m_controlLayout->addWidget( new QLabel( i18n( "Match:" ), m_controlFrame ), 1, 0 );

    m_controlLayout->addLayout( sliderLayout, 0, 1, 1, 2 );
    m_controlLayout->addWidget( m_fieldSelection, 1, 1, 1, 2 );

    m_controlLayout->setColumnStretch( 0, 0 );
    m_controlLayout->setColumnStretch( 1, 0 );
    m_controlLayout->setColumnStretch( 2, 1 );

    syncControlsToBias();

    layout()->addWidget( m_controlFrame );
}

PlaylistBrowserNS::BiasGlobalWidget::~BiasGlobalWidget()
{
    // sometimes we don't add all the widgets to the layout, so we need to delete them by hand
    delete m_withLabel;
    delete m_andLabel;
    delete m_compareSelection;
    delete m_valueSelection1;
    delete m_valueSelection2;
}

void
PlaylistBrowserNS::BiasGlobalWidget::syncControlsToBias()
{
    int weight = (int)(m_gbias->weight() * 100.0);
    m_weightSelection->setValue(weight);
    weightChanged(weight); // the widget value might not have changed and thus the signal not fired

    int index = m_fieldSelection->findData( (int)m_filter.field );
    m_fieldSelection->setCurrentIndex( index == -1 ? 0 : index );

    makeCompareSelection();
    makeValueSelection();
    setValueSelection();
}

void
PlaylistBrowserNS::BiasGlobalWidget::syncBiasToControls()
{
    m_gbias->setFilter( m_filter );
    m_gbias->setActive( true );

    emit biasChanged( m_bias );
}

void
PlaylistBrowserNS::BiasGlobalWidget::makeFieldSelection()
{
    m_fieldSelection = new KComboBox( m_controlFrame );
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
    connect( m_fieldSelection, SIGNAL(currentIndexChanged(int)), this, SLOT(fieldChanged(int)) );
}

void
PlaylistBrowserNS::BiasGlobalWidget::weightChanged( int ival )
{
    m_weightLabel->setText( QString().sprintf( "%d%%", ival ) );

    double fval = (double)ival;
    m_gbias->setWeight( fval / 100.0 );

    emit biasChanged( m_bias );
}

void
PlaylistBrowserNS::BiasGlobalWidget::fieldChanged( int i )
{
    qint64 field = qvariant_cast<qint64>( m_fieldSelection->itemData( i ) );
    m_filter.field = field;
    if( m_filter.field != m_gbias->filter().field )
    {
        m_filter.value.clear();
        m_filter.numValue = 0;
        m_filter.numValue2 = 0;

        syncBiasToControls();
    }

    // in the fieldChanged slot we assume that the field was really changed,
    // so we don't have a problem with throwing away all the old widgets

    makeCompareSelection();
    makeValueSelection();

    setValueSelection();
}

void
PlaylistBrowserNS::BiasGlobalWidget::compareChanged( int index )
{
    m_filter.condition = (Dynamic::GlobalBias::FilterCondition)
                m_compareSelection->itemData( index ).toInt();

    if( m_filter.condition != m_gbias->filter().condition )
        syncBiasToControls();

    // need to re-generate the value selection fields
    makeValueSelection();

    setValueSelection();
}

void
PlaylistBrowserNS::BiasGlobalWidget::valueChanged( const QString& value )
{
    m_filter.value = value;
    syncBiasToControls();
}

void
PlaylistBrowserNS::BiasGlobalWidget::numValueChanged( int value )
{
    m_filter.numValue = value;
    syncBiasToControls();
}

void
PlaylistBrowserNS::BiasGlobalWidget::numValue2Changed( int value )
{
    m_filter.numValue2 = value;
    syncBiasToControls();
}

void
PlaylistBrowserNS::BiasGlobalWidget::numValueChanged( qint64 value )
{
    m_filter.numValue = value;
    syncBiasToControls();
}

void
PlaylistBrowserNS::BiasGlobalWidget::numValue2Changed( qint64 value )
{
    m_filter.numValue2 = value;
    syncBiasToControls();
}

void
PlaylistBrowserNS::BiasGlobalWidget::numValueChanged( const QTime& value )
{
    m_filter.numValue = qAbs( value.secsTo( QTime(0,0,0) ) );
    syncBiasToControls();
}

void
PlaylistBrowserNS::BiasGlobalWidget::numValue2Changed( const QTime& value )
{
    m_filter.numValue2 = qAbs( value.secsTo( QTime(0,0,0) ) );
    syncBiasToControls();
}

void
PlaylistBrowserNS::BiasGlobalWidget::numValueDateChanged()
{
    KDateCombo* dateSelection = qobject_cast<KDateCombo*>( sender() );
    if( dateSelection )
    {
        QDate date;
        dateSelection->getDate( &date );
        m_filter.numValue = QDateTime( date ).toTime_t();
        syncBiasToControls();
    }
}

void
PlaylistBrowserNS::BiasGlobalWidget::numValue2DateChanged()
{
    KDateCombo* dateSelection = qobject_cast<KDateCombo*>( sender() );
    if( dateSelection )
    {
        QDate date;
        dateSelection->getDate( &date );
        m_filter.numValue2 = QDateTime( date ).toTime_t();
        syncBiasToControls();
    }
}

void
PlaylistBrowserNS::BiasGlobalWidget::numValueTimeDistanceChanged()
{
    // static_cast. Remember: the TimeDistanceWidget does not have a Q_OBJECT macro
    TimeDistanceWidget* distanceSelection = static_cast<TimeDistanceWidget*>( sender()->parent() );
    m_filter.numValue = distanceSelection->timeDistance();
    syncBiasToControls();
}

void
PlaylistBrowserNS::BiasGlobalWidget::setValueSelection()
{
    if( m_withLabel == 0 )
    {
        m_withLabel = new QLabel( i18n( "With:" ), m_controlFrame );
        m_controlLayout->addWidget( m_withLabel, 2, 0 );
    }

    if( m_compareSelection )
    {
        m_controlLayout->addWidget( m_compareSelection, 2, 1 );
    }

    if( m_valueSelection1 )
    {
        if( m_compareSelection )
            m_controlLayout->addWidget( m_valueSelection1, 2, 2 );
        else
            m_controlLayout->addWidget( m_valueSelection1, 2, 1, 1, 2 );
    }

    if( m_filter.condition == Dynamic::GlobalBias::Between && m_valueSelection2 )
    {
        m_andLabel = new QLabel( i18n( "and" ), m_controlFrame );
        m_andLabel->setMinimumHeight( m_fieldSelection->minimumHeight()); // rating will look bad without this.
        m_controlLayout->addWidget( m_andLabel, 3, 1 );

        if( m_compareSelection )
            m_controlLayout->addWidget( m_valueSelection2, 3, 2 );
        else
            m_controlLayout->addWidget( m_valueSelection2, 3, 1, 1, 2 );
    }
}


void
PlaylistBrowserNS::BiasGlobalWidget::makeCompareSelection()
{
    delete m_withLabel;
    m_withLabel = 0;
    delete m_compareSelection;
    m_compareSelection = 0;

    // no compare selection for the following fields
    qint64 field = m_filter.field;
    if( field == Meta::valArtist ||
        field == Meta::valComposer ||
        field == Meta::valAlbum ||
        field == Meta::valTitle ||
        field == Meta::valGenre ||
        field == Meta::valComment ||
        field == Meta::valUrl )
        return;

    m_compareSelection = new KComboBox();
    m_compareSelection->addItem( i18n( "Smaller Than" ),    (int)Dynamic::GlobalBias::LessThan );
    m_compareSelection->addItem( i18n( "Equal To" ),     (int)Dynamic::GlobalBias::Equals );
    m_compareSelection->addItem( i18n( "Larger Than" ), (int)Dynamic::GlobalBias::GreaterThan );

    // those fields can get a between comparation
    if( field == Meta::valYear ||
        field == Meta::valPlaycount ||
        field == Meta::valRating ||
        field == Meta::valScore ||
        field == Meta::valLength ||
        field == Meta::valTrackNr ||
        field == Meta::valDiscNr ||
        field == Meta::valFirstPlayed ||
        field == Meta::valLastPlayed )
        m_compareSelection->addItem( i18n( "Between" ),      (int)Dynamic::GlobalBias::Between );

    // and those fields can get an older than comparation
    if( field == Meta::valFirstPlayed ||
        field == Meta::valLastPlayed )
        m_compareSelection->addItem( i18n( "Older Than" ),      (int)Dynamic::GlobalBias::OlderThan );

    connect( m_compareSelection, SIGNAL(currentIndexChanged(int)),
            SLOT(compareChanged(int)) );

    Dynamic::GlobalBias::FilterCondition condition = m_gbias->filter().condition;

    if( condition == Dynamic::GlobalBias::LessThan )
        m_compareSelection->setCurrentIndex( 0 );
    else if( condition == Dynamic::GlobalBias::Equals )
        m_compareSelection->setCurrentIndex( 1 );
    else if( condition == Dynamic::GlobalBias::GreaterThan )
        m_compareSelection->setCurrentIndex( 2 );
    else if( condition == Dynamic::GlobalBias::Between )
        m_compareSelection->setCurrentIndex( 3 );
    else if( condition == Dynamic::GlobalBias::OlderThan )
        m_compareSelection->setCurrentIndex( 4 );
}

void
PlaylistBrowserNS::BiasGlobalWidget::makeValueSelection()
{
    delete m_andLabel;
    m_andLabel = 0;

    delete m_valueSelection1;
    m_valueSelection1 = 0;
    delete m_valueSelection2;
    m_valueSelection2 = 0;

    qint64 field = m_filter.field;
    if( field == Meta::valArtist )
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
        makeGenericNumberSelection( 1900, 2300, 1976 );
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
}

void
PlaylistBrowserNS::BiasGlobalWidget::makeGenericComboSelection( bool editable, Collections::QueryMaker* populateQuery )
{
    delete m_compareSelection;
    m_compareSelection = 0;
    m_filter.condition = Dynamic::GlobalBias::Contains;

    KComboBox* combo = new KComboBox( m_controlFrame );
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

    connect( combo, SIGNAL(editTextChanged( const QString& ) ),
            SLOT(valueChanged(const QString&)) );

    combo->setCompletionMode( KGlobalSettings::CompletionPopup );
    m_valueSelection1 = combo;
}


void
PlaylistBrowserNS::BiasGlobalWidget::populateComboBox( QString collectionId, QStringList results )
{
    Q_UNUSED(collectionId);

    Collections::QueryMaker* query = qobject_cast<Collections::QueryMaker*>( sender() );
    if( !query )
        return;

    KComboBox* combo = m_runningQueries[query];
    if( !combo )
        return;

    const QSet<QString> dataSet = results.toSet();
    QStringList dataList = dataSet.toList();
    dataList.sort();
    combo->addItems( dataList );

    KCompletion* comp = combo->completionObject();
    comp->setItems( dataList );

    const QString fieldValue = m_gbias->filter().value;
    combo->setCurrentIndex( combo->findText( fieldValue ) );

    connect( combo,
             SIGNAL( currentIndexChanged( const QString& ) ),
             SLOT( valueChanged( const QString& ) ) );
}


void
PlaylistBrowserNS::BiasGlobalWidget::comboBoxPopulated()
{
    Collections::QueryMaker* query = qobject_cast<Collections::QueryMaker*>( sender() );
    if( !query )
        return;

    m_runningQueries.remove( query );
    query->deleteLater();
}



void
PlaylistBrowserNS::BiasGlobalWidget::makeArtistSelection()
{
    Collections::QueryMaker* qm = new Collections::MetaQueryMaker( CollectionManager::instance()->queryableCollections() );
    qm->setQueryType( Collections::QueryMaker::Custom );
    qm->addReturnValue( Meta::valArtist );
    makeGenericComboSelection( false, qm );
}


void
PlaylistBrowserNS::BiasGlobalWidget::makeComposerSelection()
{
    Collections::QueryMaker* qm = new Collections::MetaQueryMaker( CollectionManager::instance()->queryableCollections() );
    qm->setQueryType( Collections::QueryMaker::Custom );
    qm->addReturnValue( Meta::valComposer );
    makeGenericComboSelection( false, qm );
}


void
PlaylistBrowserNS::BiasGlobalWidget::makeAlbumSelection()
{
    Collections::QueryMaker* qm = new Collections::MetaQueryMaker( CollectionManager::instance()->queryableCollections() );
    qm->setQueryType( Collections::QueryMaker::Custom );
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
    Collections::QueryMaker* qm = new Collections::MetaQueryMaker( CollectionManager::instance()->queryableCollections() );
    qm->setQueryType( Collections::QueryMaker::Custom );
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
    KRatingWidget* ratingWidget = new KRatingWidget();
    ratingWidget->setRating( (int)m_gbias->filter().numValue );
    connect( ratingWidget, SIGNAL(ratingChanged(int)),
             this, SLOT(numValueChanged(int)) );

    m_valueSelection1 = ratingWidget;

    if( m_filter.condition != Dynamic::GlobalBias::Between )
        return;

    // second KRatingWidget for the between selection
    KRatingWidget* ratingWidget2 = new KRatingWidget();
    ratingWidget2->setRating( (int)m_gbias->filter().numValue2 );
    connect( ratingWidget2, SIGNAL(ratingChanged(int)),
             this, SLOT(numValue2Changed(int)) );

    m_valueSelection2 = ratingWidget2;
}


void
PlaylistBrowserNS::BiasGlobalWidget::makeLengthSelection()
{
    QTimeEdit* timeSpin = new QTimeEdit();
    timeSpin->setDisplayFormat( "m:ss" );
    timeSpin->setMinimumTime( QTime( 0, 0, 0 ) );
    timeSpin->setMaximumTime( QTime( 0, 60, 59) );
    timeSpin->setTime( QTime().addSecs( m_gbias->filter().value.toInt() ) );

    connect( timeSpin, SIGNAL(timeChanged(const QTime&)),
            SLOT(valueChanged(const QTime&)) );

    m_valueSelection1 = timeSpin;

    if( m_filter.condition != Dynamic::GlobalBias::Between )
        return;

    QTimeEdit* timeSpin2 = new QTimeEdit();
    timeSpin2->setDisplayFormat( "m:ss" );
    timeSpin2->setMinimumTime( QTime( 0, 0, 0 ) );
    timeSpin2->setMaximumTime( QTime( 0, 60, 59) );
    timeSpin2->setTime( QTime().addSecs( m_gbias->filter().value.toInt() ) );

    connect( timeSpin2, SIGNAL(timeChanged(const QTime&)),
            SLOT(valueChanged(const QTime&)) );

    m_valueSelection2 = timeSpin2;
}

void
PlaylistBrowserNS::BiasGlobalWidget::makeGenericNumberSelection( int min, int max, int def )
{
    QSpinBox* spin = new QSpinBox();
    spin->setMinimum( min );
    spin->setMaximum( max );
    if( m_gbias->filter().condition == Dynamic::GlobalBias::Contains )
        spin->setValue( def );
    else
        spin->setValue( m_gbias->filter().numValue );

    connect( spin, SIGNAL(valueChanged(int)),
            this, SLOT(valueChanged(int)) );

    m_valueSelection1 = spin;

    if( m_filter.condition != Dynamic::GlobalBias::Between )
        return;

    // second spin box for the between selection
    QSpinBox* spin2 = new QSpinBox();
    spin2->setMinimum( min );
    spin2->setMaximum( max );
    if( m_gbias->filter().condition == Dynamic::GlobalBias::Contains )
        spin2->setValue( def );
    else
        spin2->setValue( m_gbias->filter().numValue2 );

    connect( spin2, SIGNAL(valueChanged(int)),
            this, SLOT(numValue2Changed(int)) );

    m_valueSelection2 = spin2;
}


void
PlaylistBrowserNS::BiasGlobalWidget::makeDateTimeSelection()
{
    if( m_filter.condition != Dynamic::GlobalBias::OlderThan )
    {
        KDateCombo* dateSelection = new KDateCombo();
        QDateTime dt;
        if( m_gbias->filter().condition == Dynamic::GlobalBias::Contains )
            dt = QDateTime::currentDateTime();
        else
            dt.setTime_t( m_gbias->filter().numValue );
        dateSelection->setDate( dt.date() );

        connect( dateSelection, SIGNAL(currentIndexChanged(int)),
                SLOT( numValueDateChanged() ) );

        m_valueSelection1 = dateSelection;

        if( m_filter.condition != Dynamic::GlobalBias::Between )
            return;

        // second KDateCombo for the between selection
        KDateCombo* dateSelection2 = new KDateCombo();
        if( m_gbias->filter().condition == Dynamic::GlobalBias::Contains )
            dt = QDateTime::currentDateTime();
        else
            dt.setTime_t( m_gbias->filter().numValue2 );
        dateSelection2->setDate( dt.date() );

        connect( dateSelection2, SIGNAL(currentIndexChanged(int)),
                SLOT( numValue2DateChanged() ) );

        m_valueSelection2 = dateSelection2;
    }
    else
    {
        TimeDistanceWidget* distanceSelection = new TimeDistanceWidget();
        distanceSelection->setTimeDistance( m_gbias->filter().numValue);

        distanceSelection->connectChanged( this, SLOT(numValueTimeDistanceChanged()));

        m_valueSelection1 = distanceSelection;
    }
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
    m_scaleLabel->setText( QString().sprintf( "%d%%", ival ) );

    double fval = (double)ival;
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


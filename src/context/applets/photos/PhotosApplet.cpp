/****************************************************************************************
 * Copyright (c) 2009 Simon Esneault <simon.esneault@gmail.com>                         *
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

#define DEBUG_PREFIX "PhotosApplet"

#include "PhotosApplet.h"
#include "PhotosScrollWidget.h"

// Amarok
#include "core/support/Amarok.h"
#include "EngineController.h"
#include "core/support/Debug.h"
#include "context/ContextView.h"
#include "context/widgets/TextScrollingWidget.h"

// KDE
#include <KAction>
#include <KColorScheme>
#include <KConfigDialog>
#include <KGlobalSettings>
#include <Plasma/BusyWidget>
#include <Plasma/IconWidget>
#include <Plasma/Theme>

// Qt
#include <QGraphicsLinearLayout>
#include <QGraphicsProxyWidget>
#include <QGraphicsTextItem>
#include <QGraphicsWidget>

PhotosApplet::PhotosApplet( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
    , m_stoppedstate( false )
    , m_settingsIcon( 0 )
{
    DEBUG_BLOCK
    setHasConfigurationInterface( true );

    EngineController *engine = The::engineController();
    connect( engine, SIGNAL( trackPlaying( Meta::TrackPtr ) ),
             this, SLOT( trackPlaying() ) );
    connect( engine, SIGNAL( stopped( qint64, qint64 ) ),
             this, SLOT( stopped() ) );
}

void
PhotosApplet::init()
{
    // Call the base implementation.
    Context::Applet::init();

    setBackgroundHints( Plasma::Applet::NoBackground );
    resize( 500, -1 );
    
    // Create label
    QFont labelFont;
    labelFont.setPointSize( labelFont.pointSize() + 2 );
    m_headerText = new TextScrollingWidget( this );
    m_headerText->setBrush( Plasma::Theme::defaultTheme()->color( Plasma::Theme::TextColor ) );
    m_headerText->setFont( labelFont );
    m_headerText->setText( i18n( "Photos" ) );
    m_headerText->setDrawBackground( true );

    // Set the collapse size
    setCollapseHeight( m_headerText->boundingRect().height() + 3 * standardPadding() );
    
    // Icon
    QAction* settingsAction = new QAction( this );
    settingsAction->setIcon( KIcon( "preferences-system" ) );
    settingsAction->setVisible( true );
    settingsAction->setEnabled( true );
    settingsAction->setText( i18n( "Settings" ) );
    m_settingsIcon = addAction( settingsAction );
    connect( m_settingsIcon, SIGNAL( clicked() ), this, SLOT( showConfigurationInterface() ) );

    m_widget = new PhotosScrollWidget( this );
    m_widget->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    m_widget->setContentsMargins( 0, 0, 0, 0 );

    QGraphicsLinearLayout *headerLayout = new QGraphicsLinearLayout;
    headerLayout->addItem( m_headerText );
    headerLayout->addItem( m_settingsIcon );
    headerLayout->setContentsMargins( 0, 4, 0, 2 );

    QGraphicsLinearLayout *layout = new QGraphicsLinearLayout( Qt::Vertical, this );
    layout->addItem( headerLayout );
    layout->addItem( m_widget );
    
    // Read config and inform the engine.
    KConfigGroup config = Amarok::config("Photos Applet");
    m_nbPhotos = config.readEntry( "NbPhotos", "10" ).toInt();
    m_Animation = config.readEntry( "Animation", "Fading" );
    m_KeyWords = config.readEntry( "KeyWords", "" );

    if ( m_Animation == i18nc( "animation type", "Automatic" ) )
        m_widget->setMode( 0 );
    
    if ( m_Animation == i18n( "Interactive" ) )
        m_widget->setMode( 1 );
    
    if ( m_Animation == i18n( "Fading" ) )
        m_widget->setMode( 2 );
  
    constraintsEvent();

    connectSource( "photos" );
    connect( dataEngine( "amarok-photos" ), SIGNAL( sourceAdded( const QString & ) ),
             this, SLOT( connectSource( const QString & ) ) );

    dataEngine( "amarok-photos" )->query( QString( "photos:nbphotos:" ) + QString().setNum( m_nbPhotos ) );
    dataEngine( "amarok-photos" )->query( QString( "photos:keywords:" ) + m_KeyWords );
    setCollapseOn();
}

PhotosApplet::~PhotosApplet()
{
    DEBUG_BLOCK
}

void
PhotosApplet::trackPlaying( )
{
    DEBUG_BLOCK
    m_stoppedstate = false;
    dataEngine( "amarok-photos" )->query( QString( "photos" ) );
}

void
PhotosApplet::stopped()
{
    DEBUG_BLOCK

    m_stoppedstate = true;
    m_headerText->setText( i18n( "Photos" ) + QString( " : " ) + i18n( "No track playing" ) );
    m_widget->clear();
    m_widget->hide();
    setBusy( false );
    setCollapseOn();
    dataEngine( "amarok-photos" )->query( QString( "photos:stopped" ) );
}

void 
PhotosApplet::constraintsEvent( Plasma::Constraints constraints )
{
    Q_UNUSED( constraints );
    m_headerText->setScrollingText( i18n( "Photos" ) );
}

void 
PhotosApplet::paintInterface( QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &contentsRect )
{
    Q_UNUSED( p );
    Q_UNUSED( option );
    Q_UNUSED( contentsRect );
    p->setRenderHint( QPainter::Antialiasing );
    // tint the whole applet
    addGradientToAppletBackground( p );
}

void 
PhotosApplet::connectSource( const QString &source )
{
    if ( source == "photos" )
        dataEngine( "amarok-photos" )->connectSource( "photos", this );
}

void 
PhotosApplet::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data ) // SLOT
{
//    DEBUG_BLOCK
    Q_UNUSED( name )

    if ( data.empty() )
        return;

    if ( m_stoppedstate )
    {
        m_headerText->setText( i18n( "Photos" )  );
        updateConstraints();
        update();
        m_widget->clear();
        m_widget->hide();
        setBusy( false );
        setCollapseOn();
        return;
    }
    // if we get a message, show it
    if ( data.contains( "message" ) && data["message"].toString().contains("Fetching"))
    {
        //FIXME: This should use i18n( "blah %1 blah", foo ). 
        m_headerText->setText( i18n( "Photos" ) + QString( " : " ) + i18n( "Fetching ..." ) );
        updateConstraints();
        update();
        setCollapseOff();
        m_widget->clear();
        m_widget->hide();
        if( canAnimate() )
            setBusy( true );
    }
    else if ( data.contains( "message" ) && data["message"].toString().contains("NA_Collapse") )
    {
        updateConstraints();
        update();
        setCollapseOn();
        m_widget->clear();
        m_widget->hide();
        setBusy( false );
    }
    else if ( data.contains( "message" ) )
    {
        //FIXME: This should use i18n( "blah %1 blah", foo ). 
        m_headerText->setText( i18n( "Photos" ) + " : " + data[ "message" ].toString() );
        updateConstraints();
        update();
        m_widget->hide();
        setCollapseOn();
        setBusy( false );
    }
    else if ( data.contains( "data" ) )
    {
        // Do not show some picture if we're still animating as it can lead to trouble
        // let's hope animating time will be shorter than fetching time of all the picture :/
        // this also prevent the stupid effect of reanimating several time.
        if ( isAppletExtended() )
        {
            //FIXME: This should use i18n( "blah %1 blah", foo ). 
            m_headerText->setText( i18n( "Photos" ) + QString( " : " ) + data[ "artist" ].toString() );
            updateConstraints();
            update();
            setCollapseOff();
            // Send the data to the scrolling widget
            m_widget->setPixmapList( data[ "data" ].value< QList < PhotosInfo * > >() );
            m_widget->show();
            setBusy(false);
            setMinimumHeight( 300 );
            emit sizeHintChanged( Qt::MinimumSize );
            layout()->invalidate();
        }
        else
            return;
    }
    updateConstraints();
    update();
}

void
PhotosApplet::createConfigurationInterface( KConfigDialog *parent )
{
    KConfigGroup configuration = config();
    QWidget *settings = new QWidget;
    ui_Settings.setupUi( settings );

    parent->addPage( settings, i18n( "Photos Settings" ), "preferences-system");

    ui_Settings.animationComboBox->setCurrentIndex( ui_Settings.animationComboBox->findText( m_Animation ) );
    ui_Settings.photosSpinBox->setValue( m_nbPhotos );
    ui_Settings.additionalkeywordsLineEdit->setText( m_KeyWords );
    connect( parent, SIGNAL( accepted() ), this, SLOT( saveSettings( ) ) );
}

void
PhotosApplet::saveSettings()
{
    DEBUG_BLOCK
    KConfigGroup config = Amarok::config("Photos Applet");

    m_nbPhotos = ui_Settings.photosSpinBox->value();
    m_Animation = ui_Settings.animationComboBox->currentText();
    m_KeyWords = ui_Settings.additionalkeywordsLineEdit->text();
    config.writeEntry( "NbPhotos", m_nbPhotos );
    config.writeEntry( "Animation", m_Animation );
    config.writeEntry( "KeyWords", m_KeyWords );

    m_widget->setMode( ui_Settings.animationComboBox->currentIndex() );

    m_widget->clear();
    
    dataEngine( "amarok-photos" )->query( QString( "photos:nbphotos:" ) + QString().setNum( m_nbPhotos ) );
    dataEngine( "amarok-photos" )->query( QString( "photos:keywords:" ) + m_KeyWords );
}


#include "PhotosApplet.moc"


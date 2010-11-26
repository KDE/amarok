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
    , m_settingsIcon( 0 )
{
    DEBUG_BLOCK
    setHasConfigurationInterface( true );
    setBackgroundHints( Plasma::Applet::NoBackground );
}

void
PhotosApplet::init()
{
    DEBUG_BLOCK

    // Call the base implementation.
    Context::Applet::init();
    
    // Create label
    QFont labelFont;
    labelFont.setPointSize( labelFont.pointSize() + 2 );
    m_headerText = new TextScrollingWidget( this );
    m_headerText->setFont( labelFont );
    m_headerText->setText( i18n( "Photos" ) );
    m_headerText->setDrawBackground( true );
    m_headerText->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );

    // Set the collapse size
    setCollapseHeight( m_headerText->boundingRect().height() + 2 * standardPadding() + 2 );

    // Icon
    QAction* settingsAction = new QAction( this );
    settingsAction->setIcon( KIcon( "preferences-system" ) );
    settingsAction->setVisible( true );
    settingsAction->setEnabled( true );
    settingsAction->setText( i18n( "Settings" ) );
    m_settingsIcon = addAction( settingsAction );
    connect( m_settingsIcon, SIGNAL( clicked() ), this, SLOT( showConfigurationInterface() ) );

    m_widget = new PhotosScrollWidget( this );
    m_widget->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );
    m_widget->setContentsMargins( 0, 0, 0, 0 );
    m_widget->setPreferredHeight( 300 );
    // m_widget->setMaximumHeight( 300 );
    setMaximumHeight( 300 );
    setMinimumHeight( 300 );

    QGraphicsLinearLayout *headerLayout = new QGraphicsLinearLayout;
    headerLayout->addItem( m_headerText );
    headerLayout->addItem( m_settingsIcon );
    headerLayout->setContentsMargins( 0, 4, 0, 2 );

    QGraphicsLinearLayout *layout = new QGraphicsLinearLayout( Qt::Vertical, this );
    layout->addItem( headerLayout );
    layout->addItem( m_widget );
    // layout->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );
    
    // Read config and inform the engine.
    KConfigGroup config = Amarok::config("Photos Applet");
    m_nbPhotos = config.readEntry( "NbPhotos", "10" ).toInt();
    m_Animation = config.readEntry( "Animation", "Fading" );
    m_KeyWords = config.readEntry( "KeyWords", QStringList() );

    if( m_Animation == i18nc( "animation type", "Automatic" ) )
        m_widget->setMode( 0 );
    else if( m_Animation == i18n( "Interactive" ) )
        m_widget->setMode( 1 );
    else // fading
        m_widget->setMode( 2 );
  
    Plasma::DataEngine *engine = dataEngine( "amarok-photos" );
    // connect( engine, SIGNAL(sourceAdded(QString)), SLOT(connectSource(QString)) );
    engine->setProperty( "fetchSize", m_nbPhotos );
    engine->setProperty( "keywords", m_KeyWords );
    engine->connectSource( "photos", this );
    // engine->query( QLatin1String( "photos" ) );
    stopped();

    updateConstraints();
}

PhotosApplet::~PhotosApplet()
{
    DEBUG_BLOCK
}

void
PhotosApplet::stopped()
{
    DEBUG_BLOCK

    m_headerText->setScrollingText( i18n( "Photos" ) + QString( " : " ) + i18n( "No track playing" ) );
    m_widget->clear();
    m_widget->hide();
    setBusy( false );
    // dataEngine( "amarok-photos" )->query( QString( "photos:stopped" ) );
    // setMinimumHeight( 0 );
    // emit sizeHintChanged( Qt::MinimumSize );
    setCollapseOn();
    updateConstraints();
}

void 
PhotosApplet::constraintsEvent( Plasma::Constraints constraints )
{
    Context::Applet::constraintsEvent( constraints );
    m_headerText->setScrollingText( i18n( "Photos" ) );
    update();
}

void 
PhotosApplet::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data ) // SLOT
{
    DEBUG_BLOCK
    if( name != QLatin1String("photos") || data.isEmpty() )
        return;

    setBusy( false );

    // if we get a message, show it
    if( data.contains( "message" ) && data["message"].toString().contains("Fetching"))
    {
        debug() << "received message: Fetching";
        //FIXME: This should use i18n( "blah %1 blah", foo ). 
        m_headerText->setScrollingText( i18n( "Photos" ) + QString( " : " ) + i18n( "Fetching ..." ) );
        setCollapseOff();
        m_widget->clear();
        m_widget->hide();
        if( canAnimate() )
            setBusy( true );
    }
    else if( data.contains( "message" ) && data["message"].toString().contains("stopped") )
    {
        debug() << "received message: stopped";
        stopped();
    }
    else if( data.contains( "message" ) && data["message"].toString().contains("NA_Collapse") )
    {
        debug() << "received message: NA_Collapse";
        setCollapseOn();
        m_widget->clear();
        m_widget->hide();
    }
    else if( data.contains( "message" ) )
    {
        debug() << "received message:" << data["message"].toString();
        //FIXME: This should use i18n( "blah %1 blah", foo ). 
        m_headerText->setScrollingText( i18n( "Photos" ) + " : " + data[ "message" ].toString() );
        m_widget->hide();
        setCollapseOn();
    }
    else if( data.contains( "data" ) )
    {
        debug() << "received data for:" << data["artist"].toString();
        m_widget->clear();
        // Do not show some picture if we're still animating as it can lead to trouble
        // let's hope animating time will be shorter than fetching time of all the picture :/
        // this also prevent the stupid effect of reanimating several time.
        // if ( isAppletExtended() )
        // {
            //FIXME: This should use i18n( "blah %1 blah", foo ). 
            m_headerText->setScrollingText( i18n( "Photos" ) + QString( " : " ) + data[ "artist" ].toString() );
            setCollapseOff();
            // Send the data to the scrolling widget
            m_widget->setPhotosInfoList( data[ "data" ].value< PhotosInfo::List >() );
            m_widget->show();
            // setMinimumHeight( 300 );
            // emit sizeHintChanged( Qt::MinimumSize );
            layout()->invalidate();
        // }
        // else
            // return;
    }
    updateConstraints();
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
    ui_Settings.additionalkeywordsLineEdit->setText( m_KeyWords.join(", ") );
    connect( parent, SIGNAL( accepted() ), this, SLOT( saveSettings( ) ) );
}

void
PhotosApplet::saveSettings()
{
    DEBUG_BLOCK
    KConfigGroup config = Amarok::config("Photos Applet");

    m_nbPhotos = ui_Settings.photosSpinBox->value();
    m_Animation = ui_Settings.animationComboBox->currentText();
    m_KeyWords = ui_Settings.additionalkeywordsLineEdit->text().split(", ");
    config.writeEntry( "NbPhotos", m_nbPhotos );
    config.writeEntry( "Animation", m_Animation );
    config.writeEntry( "KeyWords", m_KeyWords );

    m_widget->setMode( ui_Settings.animationComboBox->currentIndex() );
    m_widget->clear();
    
    Plasma::DataEngine *engine = dataEngine( "amarok-photos" );
    engine->setProperty( "fetchSize", m_nbPhotos );
    engine->setProperty( "keywords", m_KeyWords );
    engine->query( QLatin1String( "photos:forceUpdate" ) );
}

#include "PhotosApplet.moc"

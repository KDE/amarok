/****************************************************************************************
 * Copyright (c) 2009 Joffrey Clavel <jclavel@clabert.info>                             *
 * Copyright (c) 2009 Oleksandr Khayrullin <saniokh@gmail.com>                          *
 * Copyright (c) 2009 Ludovic Deveaux <deveaux.ludovic31@gmail.com>                     *
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

#include "UpcomingEventsApplet.h"

#include "Amarok.h"
#include "App.h"
#include "Debug.h"
#include "context/Svg.h"
#include "context/ContextView.h"
#include "PaletteHandler.h"

#include <Plasma/Theme>
#include <plasma/widgets/iconwidget.h>

#include <KConfigDialog>
#include <KStandardDirs>
#include "context/widgets/TextScrollingWidget.h"
#include "context/widgets/DropPixmapItem.h"
#include "context/applets/upcomingevents/LastFmEvent.h"

#include <QDesktopServices>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsGridLayout>
#include <QGraphicsProxyWidget>
#include <QLabel>


UpcomingEventsApplet::UpcomingEventsApplet( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
    , m_aspectRatio( 0 )
    , m_headerAspectRatio( 0.0 )
    , m_headerLabel( 0 )
    , m_settingsIcon( 0 )    
{
    setHasConfigurationInterface( true );
    setBackgroundHints( Plasma::Applet::NoBackground );
}

void
UpcomingEventsApplet::init()
{
    mainLayout = new QGraphicsGridLayout;
    m_headerLabel = new QLabel;
    m_eventParticipants = new QLabel;
    m_bigImage = new QLabel;
    m_eventName = new QLabel;
    m_eventDate = new QLabel;
    m_eventUrl = new QLabel;

    QGraphicsProxyWidget* headerLabel = new QGraphicsProxyWidget( this );
    headerLabel->setWidget( m_headerLabel );
    m_headerLabel->setText( i18n( "Upcoming Events" ) );
    m_headerLabel->setAttribute( Qt::WA_TranslucentBackground, true );
    m_headerLabel->setAlignment( Qt::AlignCenter );

    QGraphicsProxyWidget* bigImage = new QGraphicsProxyWidget( this );
    bigImage->setWidget( m_bigImage );
    m_bigImage->setAttribute( Qt::WA_TranslucentBackground, true );

    QGraphicsProxyWidget* eventName = new QGraphicsProxyWidget( this );
    eventName->setWidget( m_eventName );
    m_eventName->setAttribute( Qt::WA_TranslucentBackground, true );

    QGraphicsProxyWidget* eventParticipants = new QGraphicsProxyWidget( this );
    eventParticipants->setWidget( m_eventParticipants );
    m_eventParticipants->setAttribute( Qt::WA_TranslucentBackground, true );

    QGraphicsProxyWidget* eventDate = new QGraphicsProxyWidget( this );
    eventDate->setWidget( m_eventDate );
    m_eventDate->setAttribute( Qt::WA_TranslucentBackground, true );

    QGraphicsProxyWidget* eventUrl = new QGraphicsProxyWidget( this );
    eventUrl->setWidget( m_eventUrl );
    m_eventUrl->setAttribute( Qt::WA_TranslucentBackground, true );

    mainLayout->addItem( headerLabel, 0, 0, 1, 2 );
    mainLayout->addItem( bigImage, 1, 0, 4, 1 );
    mainLayout->addItem( eventName, 1, 1, 1, 1 );
    mainLayout->addItem( eventParticipants, 2, 1, 1, 1 );
    mainLayout->addItem( eventDate, 3, 1, 1, 1 );
    mainLayout->addItem( eventUrl, 4, 1, 1, 1 );

    mainLayout->setColumnPreferredWidth( 0, this->boundingRect().width() / 2 );
    mainLayout->setColumnPreferredWidth( 1, this->boundingRect().width() / 2 );

    mainLayout->setRowFixedHeight( 0, m_headerLabel->height() );
    
    setLayout( mainLayout );

    // ask for all the CV height
    resize( 500, -1 );

    QAction* settingsAction = new QAction( this );
    settingsAction->setIcon( KIcon( "preferences-system" ) );
    settingsAction->setEnabled( true );
    m_settingsIcon = addAction( settingsAction );
    m_settingsIcon->setToolTip( i18n( "Settings" ) );
    connect( m_settingsIcon, SIGNAL( activated() ), this, SLOT( configure() ) );

    connectSource( "upcomingEvents" );
    connect( dataEngine( "amarok-upcomingEvents" ), SIGNAL( sourceAdded( const QString & ) ), SLOT( connectSource( const QString & ) ) );

    constraintsEvent();

    // Read config and inform the engine.
    KConfigGroup config = Amarok::config("UpcomingEvents Applet");
    m_timeSpan = config.readEntry( "timeSpan", "AllEvents" );
    m_enabledLinks = config.readEntry( "enabledLinks", 0 );
    
}

void
UpcomingEventsApplet::connectSource( const QString &source )
{
    if( source == "upcomingEvents" )
    {
        dataEngine( "amarok-upcomingEvents" )->connectSource( "upcomingEvents", this );
        dataUpdated( source, dataEngine( "amarok-upcomingEvents" )->query( "upcomingEvents" ) );
    }
}

void
UpcomingEventsApplet::constraintsEvent( Plasma::Constraints constraints )
{
    Q_UNUSED( constraints );

    // Icon positionning
    m_settingsIcon->setPos( size().width() - m_settingsIcon->size().width() - standardPadding(), standardPadding() );

    for( int i = 1; i < 5; i++ )
    {
        mainLayout->setRowFixedHeight( i, m_bigImage->pixmap()->height() / 4 );
    }    
}

bool
UpcomingEventsApplet::hasHeightForWidth() const
{
    return true;
}

qreal
UpcomingEventsApplet::heightForWidth( qreal width ) const
{
    return width * m_aspectRatio;
}

void
UpcomingEventsApplet::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data ) // SLOT
{
    Q_UNUSED( name )
    QString artistName = data[ "artist" ].toString();
    if (artistName.compare( "" ) != 0)
        m_headerLabel->setText( i18n( "Upcoming events for " ) + artistName );
    else
        m_headerLabel->setText( i18n( "Upcoming events" ) );

    m_bigImage->setPixmap( data[ "cover" ].value<QPixmap>() );

    if ( m_enabledLinks ) {
        m_eventUrl->setText( data[ "eventUrl" ].toString() );
        m_eventUrl->show();
    }
    else
        m_eventUrl->hide();

    LastFmEvent::LastFmEventList event = data[ "LastFmEvent" ].value< LastFmEvent::LastFmEventList >();
    if ( !event.isEmpty() )
    {
        m_eventName->setText( event.at(0).name() );
        m_eventDate->setText( event.at(0).date() );

        QString artistList;
        for( int i = 0; i < event.at(0).artists().size(); i++ )
        {
            if( i == event.at(0).artists().size() - 1 )
            {
                artistList.append( event.at(0).artists().at( i ) + " - " );
            }
            else
            {
               artistList.append( event.at(0).artists().at( i ) );
            }
        }
        m_eventParticipants->setText( artistList );        
    }
    else
    {
        m_eventName->setText( "" );
        m_eventDate->setText( "" );
        m_eventParticipants->setText( "" );
        m_eventUrl->setText( "" );
    }
    
    updateConstraints();
    update();
}

void
UpcomingEventsApplet::paintInterface( QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &contentsRect )
{
    Q_UNUSED( option )
    Q_UNUSED( contentsRect )

    p->setRenderHint( QPainter::Antialiasing );

    addGradientToAppletBackground( p );

    //draw background of wiki text
    p->save();
    QColor bg( App::instance()->palette().highlight().color() );
    bg.setHsvF( bg.hueF(), 0.07, 1, bg.alphaF() );

    p->restore();
}


void
UpcomingEventsApplet::configure()
{
    DEBUG_BLOCK
    showConfigurationInterface();
}


void
UpcomingEventsApplet::createConfigurationInterface( KConfigDialog *parent )
{
    // for use when gui created
    /*KConfigGroup configuration = config();
    QWidget *settings = new QWidget;
    //ui_Settings.setupUi( settings );

    parent->addPage( settings, i18n( "Upcoming Events Settings" ), "preferences-system");
    connect( ui_Settings.comboBox, SIGNAL( currentIndexChanged( QString ) ), this, SLOT( switchToLang( QString ) ) );*/

    KConfigGroup config = Amarok::config("UpcomingEvents Applet");
    QWidget *settings = new QWidget();
    ui_Settings.setupUi( settings );

    m_temp_timeSpan = m_timeSpan;
    m_temp_enabledLinks = m_enabledLinks;

    // TODO bad, it's done manually ...
    if ( m_timeSpan == "AllEvents" )
        ui_Settings.comboBox->setCurrentIndex( 0 );
    else if ( m_timeSpan == "ThisWeek" )
        ui_Settings.comboBox->setCurrentIndex( 1 );
    else if ( m_timeSpan == "ThisMonth" )
        ui_Settings.comboBox->setCurrentIndex( 2 );
    else if ( m_timeSpan == "ThisYear" )
        ui_Settings.comboBox->setCurrentIndex( 3 );

    if ( m_enabledLinks )
        ui_Settings.checkBox->setCheckState ( Qt::Checked );

    
    
    parent->addPage( settings, i18n( "Upcoming Events Settings" ), "preferences-system");
    connect( ui_Settings.comboBox, SIGNAL( currentIndexChanged( QString ) ), this, SLOT( changeTimeSpan( QString ) ) );
    connect( ui_Settings.checkBox, SIGNAL( stateChanged( int ) ), this, SLOT( setAddressAsLink( int ) ) );
    connect( parent, SIGNAL( okClicked( ) ), this, SLOT( saveSettings( ) ) );    
}

void
UpcomingEventsApplet::changeTimeSpan(QString span)
{
    DEBUG_BLOCK
    // TODO change this b/c it's BAAADDD !!!
    
    if (span == i18n("Automatic") )
        m_temp_timeSpan = "AllEvents";

    else if (span == i18n("This week") )
        m_temp_timeSpan = "ThisWeek";

    else if (span == i18n("This month") )
        m_temp_timeSpan = "ThisMonth";

    else if (span == i18n("This year") )
        m_temp_timeSpan = "ThisYear";

    else if (span == i18n("All events") )
        m_temp_timeSpan = "AllEvents";
}

void
UpcomingEventsApplet::saveTimeSpan()
{
    DEBUG_BLOCK
    m_timeSpan = m_temp_timeSpan;
    dataEngine( "amarok-upcomingEvents" )->query( QString( "upcomingEvents:timeSpan:" ) + m_timeSpan );

    KConfigGroup config = Amarok::config("UpcomingEvents Applet");
    config.writeEntry( "timeSpan", m_timeSpan );
    dataEngine( "amarok-upcomingEvents" )->query( QString( "upcomingEvents:timeSpan:" ) + m_timeSpan );
}

void
UpcomingEventsApplet::setAddressAsLink(int state)
{
    DEBUG_BLOCK

    m_temp_enabledLinks = (state == Qt::Checked);
}

void
UpcomingEventsApplet::saveAddressAsLink()
{
    DEBUG_BLOCK

    m_enabledLinks = m_temp_enabledLinks;

    dataEngine( "amarok-upcomingEvents" )->query( QString( "upcomingEvents:enabledLinks:" ) + m_enabledLinks );

    KConfigGroup config = Amarok::config("UpcomingEvents Applet");
    config.writeEntry( "enabledLinks", m_enabledLinks );
    dataEngine( "amarok-upcomingEvents" )->query( QString( "upcomingEvents:enabledLinks:" ) + m_enabledLinks );
}

void
UpcomingEventsApplet::saveSettings()
{
    saveTimeSpan();
    saveAddressAsLink();
}

#include "UpcomingEventsApplet.moc"




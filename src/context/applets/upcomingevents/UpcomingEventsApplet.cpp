/****************************************************************************************
 * Copyright (c) 2009 Joffrey Clavel <jclavel@clabert.info>                             *
 * Copyright (c) 2009 Oleksandr Khayrullin <saniokh@gmail.com>                          *
 * Copyright (c) 2009-2010 Ludovic Deveaux <deveaux.ludovic31@gmail.com>                *
 * Copyright (c) 2010 Hormiere Guillaume <hormiere.guillaume@gmail.com>                 *
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

#include "core/support/Amarok.h"
#include "App.h"
#include "context/ContextView.h"
#include "core/support/Debug.h"
#include "context/widgets/DropPixmapItem.h"
#include "context/applets/upcomingevents/LastFmEvent.h"
#include "PaletteHandler.h"
#include "context/Svg.h"
#include "context/widgets/TextScrollingWidget.h"

// Qt
#include <QDesktopServices>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsGridLayout>
#include <QGraphicsLayoutItem>
#include <QGraphicsProxyWidget>
#include <QGridLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QScrollBar>

// KDE
#include <plasma/widgets/iconwidget.h>
#include <KConfigDialog>
#include <KDateTime>
#include <KStandardDirs>
#include <Plasma/Theme>

/**
 * \brief Constructor
 *
 * UpcomingEventsApplet constructor
 *
 * \param parent : the UpcomingEventsApplet parent (used by Context::Applet)
 * \param args : (used by Context::Applet)
 */
UpcomingEventsApplet::UpcomingEventsApplet( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
    , m_headerLabel( 0 )
    , m_settingsIcon( 0 )    
{
    setHasConfigurationInterface( true );
    setBackgroundHints( Plasma::Applet::NoBackground );
}

/**
 * \brief Initialization
 *
 * Initializes the UpcomingEventsApplet with default parameters
 */
void
UpcomingEventsApplet::init()
{
    // The widgets are displayed line by line with only one column
    m_mainLayout = new QVBoxLayout;
    m_mainLayout->setSizeConstraint( QLayout::SetFixedSize);
    m_mainLayout->setAlignment( Qt::AlignJustify );
    m_headerLabel = new TextScrollingWidget( this );
    
    setBackgroundHints( Plasma::Applet::NoBackground );

    // Use the same font as the other applets
    QFont labelFont;
    labelFont.setPointSize( labelFont.pointSize() + 2 );
    m_headerLabel->setBrush( Plasma::Theme::defaultTheme()->color( Plasma::Theme::TextColor ) );
    m_headerLabel->setFont( labelFont );
    m_headerLabel->setText( i18n( "Upcoming Events" ) );

    // Use an embedded widget for the applet
    m_scrollProxy = new QGraphicsProxyWidget( this );
    m_scrollProxy->setAttribute( Qt::WA_NoSystemBackground );

    QWidget * scrollContent = new QWidget;
    scrollContent->setAttribute( Qt::WA_NoSystemBackground );
    scrollContent->setLayout( m_mainLayout );
    scrollContent->show();

    m_scroll = new QScrollArea;
    m_scroll->setWidget( scrollContent );
    m_scroll->setFrameShape( QFrame::NoFrame );
    m_scroll->setAttribute( Qt::WA_NoSystemBackground );
    m_scroll->viewport()->setAttribute( Qt::WA_NoSystemBackground );

    m_scrollProxy->setWidget( m_scroll );

    // ask for all the CV height
    resize( 500, -1 );

    QAction* settingsAction = new QAction( this );
    settingsAction->setIcon( KIcon( "preferences-system" ) );
    settingsAction->setEnabled( true );
    m_settingsIcon = addAction( settingsAction );
    m_settingsIcon->setToolTip( i18n( "Settings" ) );
    connect( m_settingsIcon, SIGNAL( clicked() ), this, SLOT( configure() ) );

    connectSource( "upcomingEvents" );
    connect( dataEngine( "amarok-upcomingEvents" ), SIGNAL( sourceAdded( const QString & ) ), SLOT( connectSource( const QString & ) ) );

    constraintsEvent();

    // Read config and inform the engine.
    KConfigGroup config = Amarok::config("UpcomingEvents Applet");
    m_timeSpan = config.readEntry( "timeSpan", "AllEvents" );
    m_enabledLinks = config.readEntry( "enabledLinks", 0 );
}

/**
 * Connects the source to the Upcoming Events engine
 * and calls the dataUpdated function
 */
void
UpcomingEventsApplet::connectSource( const QString &source )
{
    if( source == "upcomingEvents" )
    {
        dataEngine( "amarok-upcomingEvents" )->connectSource( "upcomingEvents", this );
        dataUpdated( source, dataEngine( "amarok-upcomingEvents" )->query( "upcomingEvents" ) );
    }
}

/**
 * Called when any of the geometry constraints have been updated.
 *
 * This is always called prior to painting and should be used as an
 * opportunity to layout the widget, calculate sizings, etc.
 *
 * Do not call update() from this method; an update() will be triggered
 * at the appropriate time for the applet.
 *
 * \param constraints : the type of constraints that were updated
 */
void
UpcomingEventsApplet::constraintsEvent( Plasma::Constraints constraints )
{
    DEBUG_BLOCK
    
    Q_UNUSED( constraints );

    prepareGeometryChange();
    qreal widmax = boundingRect().width() - 2 * m_settingsIcon->size().width() - 6 * standardPadding();
    QRectF rect( ( boundingRect().width() - widmax ) / 2, 0 , widmax, 15 );

    m_headerLabel->setScrollingText( m_headerLabel->text(), rect );
    m_headerLabel->setPos( ( size().width() - m_headerLabel->boundingRect().width() ) / 2 , standardPadding() + 3 );

    m_scrollProxy->setPos( standardPadding(), m_headerLabel->pos().y() + m_headerLabel->boundingRect().height() + standardPadding() );
    QSize artistsSize( size().width() - 2 * standardPadding(), boundingRect().height() - m_scrollProxy->pos().y() - standardPadding() );
    
    m_scrollProxy->setMinimumSize( artistsSize );
    m_scrollProxy->setMaximumSize( artistsSize );
    
    QSize artistSize( artistsSize.width() - 2 * standardPadding() - m_scroll->verticalScrollBar()->size().width(), artistsSize.height() - 2 * standardPadding() );
    m_scroll->widget()->setMinimumSize( artistSize );
    m_scroll->widget()->setMaximumSize( artistSize );

    // Icon positionning
    m_settingsIcon->setPos( size().width() - m_settingsIcon->size().width() - standardPadding(), standardPadding() );

    for( int i = 0; i < m_widgets.size(); i++ )
    {
        m_mainLayout->addWidget( m_widgets.at( i ) );
    }
}

/**
 * Updates the data from the Upcoming Events engine
 *
 * \param name : the name
 * \param data : the engine from where the data are received
 */
void
UpcomingEventsApplet::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data ) // SLOT
{
    DEBUG_BLOCK
    Q_UNUSED( name )
    QString artistName = data[ "artist" ].toString();
    if (artistName.compare( "" ) != 0)
        m_headerLabel->setText( i18n( "Upcoming events for %1", artistName ) );
    else
        m_headerLabel->setText( i18n( "Upcoming events" ) );

    LastFmEvent::LastFmEventList events = data[ "LastFmEvent" ].value< LastFmEvent::LastFmEventList >();

    qDeleteAll( m_widgets );
    m_widgets.clear();

    for( int i = 0; i < events.size(); i++ )
    {
        const QString artistList = events.at( i ).artists().join( " - " );
        KDateTime limite(KDateTime::currentLocalDateTime());
        bool timeSpanDisabled = false;

        if ( this->m_timeSpan == "ThisWeek")
            limite = limite.addDays( 7 );
        else if( this->m_timeSpan == "ThisMonth" )
            limite = limite.addMonths( 1 );
        else if( this->m_timeSpan == "ThisYear" )
            limite = limite.addYears( 1 );
        else
            timeSpanDisabled = true;

        if ( timeSpanDisabled || events.at( i ).date() < limite.dateTime() )
        {
            UpcomingEventsWidget * widget = new UpcomingEventsWidget;
            widget->setName( events.at( i ).name() );
            widget->setDate( KDateTime( events.at( i ).date() ) );
            widget->setLocation( events.at( i ).location() );
            !artistList.isEmpty() ? widget->setParticipants( artistList ) : widget->setParticipants( "No other participants" );
            widget->setUrl( events.at( i ).url() );
            widget->setImage( events.at( i ).smallImageUrl() );
            m_widgets.insert( i, widget );
        }
    }

    if(  0 == m_widgets.size() && artistName.compare( "" ) != 0 )
    {
        m_headerLabel->setText( i18n( "No upcoming events for %1", artistName ) );
    }

    updateConstraints();
    update();
}

/**
 * \brief Paints the interface
 *
 * This method is called when the interface should be painted
 *
 * \param painter : the QPainter to use to do the paintiner
 * \param option : the style options object
 * \param contentsRect : the rect to paint within; automatically adjusted for
 *                     the background, if any
 */
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

    if( !m_headerLabel->isAnimating() )
        drawRoundedRectAroundText( p, m_headerLabel );

    p->restore();
}

/**
 * Show the settings windows
 */
void
UpcomingEventsApplet::configure()
{
    DEBUG_BLOCK
    showConfigurationInterface();
}

/**
 * Reimplement this method so provide a configuration interface,
 * parented to the supplied widget. Ownership of the widgets is passed
 * to the parent widget.
 *
 * \param parent : the dialog which is the parent of the configuration
 *               widgets
 */
void
UpcomingEventsApplet::createConfigurationInterface( KConfigDialog *parent )
{
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

/**
 * Replace the former time span by the new one
 */
void
UpcomingEventsApplet::changeTimeSpan(QString span)
{
    DEBUG_BLOCK
    // TODO change this b/c it's BAAADDD !!!
    
    if (span == i18nc("automatic time span", "Automatic") )
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

/**
 * Save the time span choosen by the user
 */
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

/**
 * Sets the upcoming events as links
 */
void
UpcomingEventsApplet::setAddressAsLink(int state)
{
    DEBUG_BLOCK

    m_temp_enabledLinks = (state == Qt::Checked);
}

/**
 * Displays all the upcoming events addresses as links
 */
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

/**
 * Save all the upcoming events settings
 */
void
UpcomingEventsApplet::saveSettings()
{
    saveTimeSpan();
    saveAddressAsLink();
}

#include "UpcomingEventsApplet.moc"

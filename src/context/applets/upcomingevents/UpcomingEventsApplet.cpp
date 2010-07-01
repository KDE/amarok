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

#define DEBUG_PREFIX "UpcomingEventsApplet"

#include "UpcomingEventsApplet.h"

#include "App.h"
#include "context/Svg.h"
#include "context/applets/upcomingevents/LastFmEvent.h"
#include "context/widgets/TextScrollingWidget.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"

#include <KConfigDialog>
#include <KDateTime>
#include <Plasma/Extender>
#include <Plasma/ExtenderItem>
#include <Plasma/IconWidget>
#include <Plasma/ScrollWidget>
#include <Plasma/Theme>

#include <QDesktopServices>
#include <QGraphicsLayoutItem>
#include <QGraphicsLinearLayout>
#include <QGraphicsProxyWidget>

UpcomingEventsApplet::UpcomingEventsApplet( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
    , m_headerLabel( 0 )
    , m_settingsIcon( 0 )
{
    setHasConfigurationInterface( true );
    setBackgroundHints( Plasma::Applet::NoBackground );
}

void
UpcomingEventsApplet::init()
{
    // Call the base implementation.
    Context::Applet::init();

    setBackgroundHints( Plasma::Applet::NoBackground );

    QAction* settingsAction = new QAction( this );
    settingsAction->setIcon( KIcon( "preferences-system" ) );
    settingsAction->setEnabled( true );
    m_settingsIcon = addAction( settingsAction );
    m_settingsIcon->setToolTip( i18n( "Settings" ) );
    connect( m_settingsIcon, SIGNAL( clicked() ), this, SLOT( configure() ) );

    // Use the same font as the other applets
    QFont labelFont;
    labelFont.setPointSize( labelFont.pointSize() + 2 );
    m_headerLabel = new TextScrollingWidget( this );
    m_headerLabel->setBrush( Plasma::Theme::defaultTheme()->color( Plasma::Theme::TextColor ) );
    m_headerLabel->setFont( labelFont );
    m_headerLabel->setText( i18n( "Upcoming Events" ) );
    m_headerLabel->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Fixed );

    QGraphicsLinearLayout *headerLayout = new QGraphicsLinearLayout( Qt::Horizontal );
    headerLayout->addItem( m_headerLabel );
    headerLayout->addItem( m_settingsIcon );
    headerLayout->setContentsMargins( 0, 4, 0, 2 );

    // The widgets are displayed line by line with only one column
    m_layout = new QGraphicsLinearLayout( Qt::Vertical );
    QGraphicsWidget *content = new QGraphicsWidget( this );
    content->setLayout( m_layout );

    // Use an embedded widget for the applet
    Plasma::ScrollWidget *scrollArea = new Plasma::ScrollWidget( this );
    scrollArea->setWidget( content );

    m_scrollWidget = new Plasma::ExtenderItem( extender() );
    m_scrollWidget->setName( "currentartistevents" );
    m_scrollWidget->setWidget( scrollArea );
    m_scrollWidget->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding );
    extender()->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding );

    QGraphicsLinearLayout *layout = new QGraphicsLinearLayout( Qt::Vertical );
    layout->addItem( headerLayout );
    layout->addItem( extender() );
    setLayout( layout );

    // ask for all the CV height
    resize( 500, -1 );

    connectSource( "upcomingEvents" );
    connect( dataEngine( "amarok-upcomingEvents" ), SIGNAL( sourceAdded( const QString & ) ), SLOT( connectSource( const QString & ) ) );

    // Read config and inform the engine.
    KConfigGroup config = Amarok::config("UpcomingEvents Applet");
    m_timeSpan = config.readEntry( "timeSpan", "AllEvents" );
    m_enabledLinks = config.readEntry( "enabledLinks", 0 );

    updateConstraints();
    update();
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
    prepareGeometryChange();
    qreal padding = standardPadding();
    qreal iconWidth = m_settingsIcon->size().width();
    qreal widmax = size().width() - 2 * iconWidth - 6 * padding;
    QRectF rect( ( size().width() - widmax ) / 2, 0 , widmax, 15 );
    m_headerLabel->setScrollingText( i18n( "Upcoming Events" ) );

    qreal scrollAreaW = size().width();
    qreal scrollAreaH = size().height() - m_headerLabel->boundingRect().height() - extender()->pos().y();
    QSizeF scrollAreaSize( scrollAreaW - 2 * padding, scrollAreaH - 2 * padding );
    Plasma::ScrollWidget *scrollArea = static_cast<Plasma::ScrollWidget*>( m_scrollWidget->widget() );
    scrollArea->setMinimumSize( scrollAreaSize );
    scrollArea->setMaximumSize( scrollAreaSize );
    scrollArea->widget()->resize(0,0);
    m_scrollWidget->setMaximumWidth( scrollAreaSize.width() );
}

void
UpcomingEventsApplet::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data )
{
    DEBUG_BLOCK
    Q_UNUSED( name )
    const QString &artistName = data[ "artist" ].toString();
    const LastFmEvent::List &events = data[ "LastFmEvent" ].value< LastFmEvent::List >();

    QGraphicsLayoutItem *child;
    if( m_layout->count() > 0 )
    {
        while( (child = m_layout->itemAt( 0 )) != 0 )
        {
            m_layout->removeItem( child );
            delete child;
        }
    }

    int eventsAdded( 0 );
    for( int i = 0, count = events.count(); i < count; ++i )
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

        if( timeSpanDisabled || events.at( i ).date() < limite.dateTime() )
        {
            UpcomingEventsWidget *widget = new UpcomingEventsWidget;
            widget->setName( events.at( i ).name() );
            widget->setDate( KDateTime( events.at( i ).date() ) );
            widget->setLocation( events.at( i ).location() );
            widget->setParticipants( artistList );
            widget->setUrl( events.at( i ).url() );
            widget->setImage( events.at( i ).smallImageUrl() );
            widget->setAttribute( Qt::WA_NoSystemBackground );
            m_layout->addItem( widget );

            // can also use Plasma::Separator here but that's in kde 4.4
            QFrame *separator = new QFrame;
            separator->setFrameStyle( QFrame::HLine );
            separator->setAutoFillBackground( false );
            QGraphicsProxyWidget *separatorProxy = new QGraphicsProxyWidget( m_scrollWidget );
            separatorProxy->setWidget( separator );
            m_layout->addItem( separatorProxy );
            ++eventsAdded;
        }
    }

    QString title;
    if( 0 == eventsAdded && !artistName.isEmpty() )
    {
        title = i18n( "No upcoming events for %1", artistName );
    }
    else
    {
        title = artistName.isEmpty() ? i18ncp( "@title:group Number of upcoming events",
                                               "1 event", "%1 events", eventsAdded )
                                     : i18ncp( "@title:group Number of upcoming events for an artist",
                                               "%1: 1 event", "%1: %2 events", artistName, eventsAdded );
    }
    m_scrollWidget->setTitle( title );

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

    if( !m_headerLabel->isAnimating() )
        drawRoundedRectAroundText( p, m_headerLabel );

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
    KConfigGroup config = Amarok::config("UpcomingEvents Applet");
    QWidget *settings = new QWidget();
    ui_Settings.setupUi( settings );

    m_temp_timeSpan = m_timeSpan;
    m_temp_enabledLinks = m_enabledLinks;

    // TODO bad, it's done manually ...
    if( m_timeSpan == "AllEvents" )
        ui_Settings.comboBox->setCurrentIndex( 0 );
    else if( m_timeSpan == "ThisWeek" )
        ui_Settings.comboBox->setCurrentIndex( 1 );
    else if( m_timeSpan == "ThisMonth" )
        ui_Settings.comboBox->setCurrentIndex( 2 );
    else if( m_timeSpan == "ThisYear" )
        ui_Settings.comboBox->setCurrentIndex( 3 );

    if( m_enabledLinks )
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
    const QString enabledLinks = m_enabledLinks ? "true" : "false";

    dataEngine( "amarok-upcomingEvents" )->query( QString( "upcomingEvents:enabledLinks:" ) + enabledLinks );

    KConfigGroup config = Amarok::config("UpcomingEvents Applet");
    config.writeEntry( "enabledLinks", m_enabledLinks );
    dataEngine( "amarok-upcomingEvents" )->query( QString( "upcomingEvents:enabledLinks:" ) + enabledLinks );
}

void
UpcomingEventsApplet::saveSettings()
{
    saveTimeSpan();
    saveAddressAsLink();
}

#include "UpcomingEventsApplet.moc"

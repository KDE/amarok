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
#include <QGraphicsLayoutItem>
#include <QGraphicsProxyWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QScrollBar>

#include <typeinfo>
#include <QGridLayout>


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
    m_mainLayout = new QVBoxLayout;
    m_mainLayout->setSizeConstraint( QLayout::SetMinAndMaxSize );
    m_headerLabel = new TextScrollingWidget( this );
    
    setBackgroundHints( Plasma::Applet::NoBackground );

    QFont labelFont;
    labelFont.setPointSize( labelFont.pointSize() + 2 );
    m_headerLabel->setBrush( Plasma::Theme::defaultTheme()->color( Plasma::Theme::TextColor ) );
    m_headerLabel->setFont( labelFont );
    m_headerLabel->setText( i18n( "Upcoming Events" ) );

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

    debug() << "TAILLE DE M_WIDGETS = " << m_widgets.size();
    for( int i = 0; i < m_widgets.size(); i++ )
    {
        debug() << "ENTREE DANS LA BOUCLE CONSTRAINTSEVENT";
        m_mainLayout->addWidget( m_widgets.at( i ) );
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
    DEBUG_BLOCK
    Q_UNUSED( name )
    QString artistName = data[ "artist" ].toString();
    if (artistName.compare( "" ) != 0)
        m_headerLabel->setText( i18n( "Upcoming events for %1", artistName ) );
    else
        m_headerLabel->setText( i18n( "Upcoming events" ) );

    LastFmEvent::LastFmEventList events = data[ "LastFmEvent" ].value< LastFmEvent::LastFmEventList >();

    foreach( UpcomingEventsWidget *u, m_widgets )
    {
        delete u;
    }
    m_widgets.clear();

    debug() << "TAILLE DE EVENTS = " << events.size();
    for( int i = 0; i < events.size(); i++ )
    {
        debug() << "BOUCLE FOR DATAUPDATED";

        QString artistList;
        for( int j = 0; j < events.at( i ).artists().size(); j++ )
        {
            if( j == events.at( i ).artists().size() - 1 )
            {
                artistList.append( events.at( i ).artists().at( j ) );
            }
            else
            {
               artistList.append( events.at( i ).artists().at( j ) + " - " );
            }
        }

        QDateTime limite(QDateTime::currentDateTime());
        bool timeSpanDisabled = false;

        if ( this->m_timeSpan == "ThisWeek")
            limite = limite.addDays( 7 );
        else if( this->m_timeSpan == "ThisMonth" )
            limite = limite.addMonths( 1 );
        else if( this->m_timeSpan == "ThisYear" )
            limite = limite.addYears( 1 );
        else
            timeSpanDisabled = true;

        if ( timeSpanDisabled || events.at( i ).date() < limite )
            m_widgets.insert( i, new UpcomingEventsWidget( events.at( i ).name(),
                                                           events.at( i ).date(),
                                                           artistList,
                                                           events.at( i ).url(),
                                                           events.at( i ).smallImageUrl()) );


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




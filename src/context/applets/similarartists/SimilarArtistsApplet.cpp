/****************************************************************************************
 * Copyright (c) 2009 Joffrey Clavel <jclavel@clabert.info>                             *
 * Copyright (c) 2009 Oleksandr Khayrullin <saniokh@gmail.com>                          *
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

#include "SimilarArtistsApplet.h"

//Amarok
#include "Amarok.h"
#include "App.h"
#include "Debug.h"
#include "context/Svg.h"
#include "context/ContextView.h"
#include "context/widgets/DropPixmapItem.h"
#include "PaletteHandler.h"
#include "context/widgets/TextScrollingWidget.h"
#include "./SimilarArtist.h"

//Kde
#include <Plasma/Theme>
#include <plasma/widgets/iconwidget.h>
#include <KConfigDialog>
#include <KStandardDirs>

//Qt
#include <QDesktopServices>
#include <QTextEdit>
#include <qgraphicsproxywidget.h>
#include <QLabel>
#include <QGraphicsGridLayout>
#include <QGraphicsScene>


SimilarArtistsApplet::SimilarArtistsApplet( QObject *parent, const QVariantList& args )
    : Context::Applet( parent, args )
    , m_aspectRatio( 0 )
    , m_headerAspectRatio( 0.0 )
    , m_headerLabel( 0 )
    , m_settingsIcon( 0 )    
{
    DEBUG_BLOCK

    setHasConfigurationInterface( true );
    setBackgroundHints( Plasma::Applet::NoBackground );

    m_layout=new QGraphicsGridLayout;
    m_tmp=new QGraphicsGridLayout;
    m_scene=new QGraphicsScene;

    m_stoppedState=true;

    // temporary code
    ArtistWidget *art1=new ArtistWidget;
    ArtistWidget *art2=new ArtistWidget;

    m_artists.append(art1);
    m_artists.append(art2);

    int cpt=1; // the first row (0) is dedicated for the applet title
    foreach(ArtistWidget* art, m_artists)
    {

        QGraphicsProxyWidget *tmp = m_scene->addWidget(art);
        m_layout->addItem(tmp,cpt,0);
        cpt++;
    }

    setLayout(m_layout);
}

void
SimilarArtistsApplet::init()
{
    
    m_headerLabel = new TextScrollingWidget( this );

    // ask for all the CV height
    resize( 500, -1 );

    QFont labelFont;
    labelFont.setPointSize( labelFont.pointSize() + 2 );
    m_headerLabel->setBrush( Plasma::Theme::defaultTheme()->color( Plasma::Theme::TextColor ) );
    m_headerLabel->setFont( labelFont );
    m_headerLabel->setText( i18n( "Similar Artists" ) );

    setCollapseHeight( m_headerLabel->boundingRect().height() + 3 * standardPadding() );

    QAction* settingsAction = new QAction( this );
    settingsAction->setIcon( KIcon( "preferences-system" ) );
    settingsAction->setEnabled( true );
    m_settingsIcon = addAction( settingsAction );
    m_settingsIcon->setToolTip( i18n( "Settings" ) );
    connect( m_settingsIcon, SIGNAL( activated() ), this, SLOT( configure() ) );

    connectSource( "similarArtists" );
    connect( dataEngine( "amarok-similarArtists" ), SIGNAL( sourceAdded( const QString & ) ), SLOT( connectSource( const QString & ) ) );

    constraintsEvent();

    // Read config and inform the engine.
    KConfigGroup config = Amarok::config("SimilarArtists Applet");
    m_maxArtists = config.readEntry( "maxArtists", "20" ).toInt();

}

void
SimilarArtistsApplet::connectSource( const QString &source )
{
    if( source == "similarArtists" ) {
        dataEngine( "amarok-similarArtists" )->connectSource( "similarArtists", this );
        dataUpdated( source, dataEngine( "amarok-similarArtists" )->query( "similarArtists" ) );
    }
}

void
SimilarArtistsApplet::constraintsEvent( Plasma::Constraints constraints )
{
    Q_UNUSED( constraints );

    prepareGeometryChange();
    qreal widmax = boundingRect().width() - 2 * m_settingsIcon->size().width() - 6 * standardPadding();
    QRectF rect( ( boundingRect().width() - widmax ) / 2, 0 , widmax, 15 );

    m_headerLabel->setScrollingText( m_headerLabel->text(), rect );
    m_headerLabel->setPos( ( size().width() - m_headerLabel->boundingRect().width() ) / 2 , standardPadding() + 3 );

    // Icon positionning
    m_settingsIcon->setPos( size().width() - m_settingsIcon->size().width() - standardPadding(), standardPadding() );
    
}

bool
SimilarArtistsApplet::hasHeightForWidth() const
{
    return true;
}

qreal
SimilarArtistsApplet::heightForWidth( qreal width ) const
{
    return width * m_aspectRatio;
}

/**
 * This method was launch when amarok play a new track
 */
void
SimilarArtistsApplet::engineNewTrackPlaying( )
{
    DEBUG_BLOCK

    m_stoppedState = false;
    setCollapseOff();
}

/**
 * This method was launch when amarok stop is playback (ex: The user has clicked on the stop button)
 */
void
SimilarArtistsApplet::enginePlaybackEnded( qint64 finalPosition, qint64 trackLength, PlaybackEndedReason )
{
    Q_UNUSED( finalPosition )
    Q_UNUSED( trackLength )
    DEBUG_BLOCK


    // we clear all artists
    int cpt=0;
    foreach(ArtistWidget* art, m_artists)
    {
      m_layout->removeAt(cpt);
      //delete art;
    }

    //m_artists.clear();
    
    
    m_stoppedState = true;
    m_headerLabel->setText( i18n( "Similar artist" ) + QString( " : " ) + i18n( "No track playing" ) );
    setCollapseOn();
}


void
SimilarArtistsApplet::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data ) // SLOT
{
    Q_UNUSED( name )
    DEBUG_BLOCK


    if ( m_stoppedState )
    {
        //TODO
    } else {

        // the layout begin at the bottom of the applet's title
        m_layout->setRowMinimumHeight(0,m_headerLabel->boundingRect().height()+7);
        m_layout->setRowMaximumHeight(0,m_headerLabel->boundingRect().height()+7);


        QString artistName = data[ "artist" ].toString();

        // we see if the artist name is valid
        if (artistName.compare( "" ) != 0) {
           m_headerLabel->setText( i18n( "Similar artists of " ) + artistName );

           //m_artists[0]->setArtist(artistName,"http://amarok.kde.org");
           m_artists[1]->setArtist(artistName, "http://kde.org");
            
           m_artists[0]->setPhoto(data[ "cover" ].value<QPixmap>());
           m_artists[1]->setPhoto(data[ "cover" ].value<QPixmap>());

           m_artists[0]->setGenres("art1");
           m_artists[1]->setGenres("art2");

           SimilarArtist::SimilarArtistsList similars = data[ "SimilarArtists" ].value<SimilarArtist::SimilarArtistsList>();

           debug()<< "Taille similars : " << similars.size();
           if ( !similars.isEmpty() )
            {
                m_artists[0]->setArtist(similars.at(0).getName(),"http://amarok.kde.org");
//                 m_eventName->setText( event.at(0).name() );
//                 m_eventDate->setText( event.at(0).date().toString( Qt::DefaultLocaleLongDate ) );
// 
//                 QString artistList;
//                 for( int i = 0; i < event.at(0).artists().size(); i++ )
//                 {
//                     if( i == event.at(0).artists().size() - 1 )
//                     {
//                         artistList.append( event.at(0).artists().at( i ) );
//                     }
//                     else
//                     {
//                     artistList.append( event.at(0).artists().at( i ) + " - " );
//                     }
//                 }
//                 m_eventParticipants->setText( artistList );
//                 m_eventUrl->setText( "<html><body><a href=\"" + event.at(0).url().prettyUrl() + "\"><u>" + i18n( "Event website" ) + "</u></a></body></html>" );
            }
            else
            {
                m_artists[0]->clear();
            }

        } else { // the artist name is invalid
            m_headerLabel->setText( i18n( "Similar Artists" ) );
        }

        updateConstraints();
        update();
    }

}

void
SimilarArtistsApplet::paintInterface( QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &contentsRect )
{
    Q_UNUSED( option )
    Q_UNUSED( contentsRect )

    p->setRenderHint( QPainter::Antialiasing );

    addGradientToAppletBackground( p );

    // draw rounded rect around title (only if not animating )
    if ( !m_headerLabel->isAnimating() )
        drawRoundedRectAroundText( p, m_headerLabel );
}


void
SimilarArtistsApplet::configure()
{
    DEBUG_BLOCK
    showConfigurationInterface();
}


void
SimilarArtistsApplet::createConfigurationInterface( KConfigDialog *parent )
{
    KConfigGroup config = Amarok::config("SimilarArtists Applet");
    QWidget *settings = new QWidget();
    ui_Settings.setupUi( settings );

    ui_Settings.spinBox->setValue( m_maxArtists );
    
    parent->addPage( settings, i18n( "Similar Artists Settings" ), "preferences-system");

    connect( ui_Settings.spinBox, SIGNAL( valueChanged( int ) ), this, SLOT( changeMaxArtists( int ) ) );
    connect( parent, SIGNAL( okClicked( ) ), this, SLOT( saveSettings( ) ) );
}

void
SimilarArtistsApplet::changeMaxArtists( int value )
{
DEBUG_BLOCK

    m_temp_maxArtists = value;
}

void
SimilarArtistsApplet::saveMaxArtists()
{
DEBUG_BLOCK

    m_maxArtists = m_temp_maxArtists;

    dataEngine( "amarok-similarArtists" )->query( QString( "similarArtists:maxArtists:" ) + m_maxArtists );

    KConfigGroup config = Amarok::config("SimilarArtists Applet");
    config.writeEntry( "maxArtists", m_maxArtists );
    dataEngine( "amarok-similarArtists" )->query( QString( "similarArtists:maxArtists:" ) + m_maxArtists );
}

void
SimilarArtistsApplet::saveSettings()
{
    saveMaxArtists();
}

#include "SimilarArtistsApplet.moc"




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
#include <QGraphicsProxyWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QScrollBar>



SimilarArtistsApplet::SimilarArtistsApplet( QObject *parent, const QVariantList& args )
    : Context::Applet( parent, args )
    , m_aspectRatio( 0 )
    , m_headerAspectRatio( 0.0 )
    , m_headerLabel( 0 )
    , m_settingsIcon( 0 )    
{
    setHasConfigurationInterface( true );
    setBackgroundHints( Plasma::Applet::NoBackground );

    m_stoppedState=false;
}


SimilarArtistsApplet::~SimilarArtistsApplet()
{
    DEBUG_BLOCK
    debug()<<"1";
    delete m_headerLabel;
    debug()<<"2";
    delete m_settingsIcon;
    debug()<<"3";
    
    debug()<<"4";
    //delete m_layout;
}


void
SimilarArtistsApplet::init()
{    
    // create the layout for dispose the artists widgets in the scrollarea via a widget
    m_layout=new QVBoxLayout;
    m_layout->setSizeConstraint( QLayout::SetMinAndMaxSize );
    //m_layout->setAlignment(Qt::AlignHCenter);
    
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

    // permit to add the scrollarea in this applet
    m_scrollProxy = new QGraphicsProxyWidget( this );
    m_scrollProxy->setAttribute( Qt::WA_NoSystemBackground );

    // this widget contents the artists widgets and it is added on the scrollarea
    QWidget *scrollContent = new QWidget;
    scrollContent->setAttribute( Qt::WA_NoSystemBackground );
    scrollContent->setLayout( m_layout );
    scrollContent->show();
    
    // create a scrollarea
    m_scroll = new QScrollArea();
    m_scroll->setWidget( scrollContent );
    m_scroll->setFrameShape( QFrame::NoFrame );
    m_scroll->setAttribute( Qt::WA_NoSystemBackground );
    m_scroll->viewport()->setAttribute( Qt::WA_NoSystemBackground );

    // add the scrollarea in the applet
    m_scrollProxy->setWidget( m_scroll );

    // Read config and inform the engine.
    KConfigGroup config = Amarok::config("SimilarArtists Applet");
    m_maxArtists = config.readEntry( "maxArtists", "3" ).toInt();

    connectSource( "similarArtists" );
    connect( dataEngine( "amarok-similarArtists" ), SIGNAL( sourceAdded( const QString & ) ), SLOT( connectSource( const QString & ) ) );

    // we connect the geometry changed wit(h a setGeom function which will update the video widget geometry
    connect ( this, SIGNAL(geometryChanged()), SLOT( setGeom() ) );
    
    constraintsEvent();    

    debug()<< "SAA fin init";
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

    DEBUG_BLOCK

    debug()<< "SAA deb const";

    prepareGeometryChange();
    qreal widmax = boundingRect().width() - 2 * m_settingsIcon->size().width() - 6 * standardPadding();
    QRectF rect( ( boundingRect().width() - widmax ) / 2, 0 , widmax, 15 );

    m_headerLabel->setScrollingText( m_headerLabel->text(), rect );
    m_headerLabel->setPos( ( size().width() - m_headerLabel->boundingRect().width() ) / 2 , standardPadding() + 3 );

    // Icon positionning
    m_settingsIcon->setPos( size().width() - m_settingsIcon->size().width() - standardPadding(), standardPadding() );


    // ScrollArea positionning via the proxyWidget
    m_scrollProxy->setPos( standardPadding(), m_headerLabel->pos().y() + m_headerLabel->boundingRect().height() + standardPadding() );

    QSize artistsSize( size().width() - 2 * standardPadding(), boundingRect().height() - m_scrollProxy->pos().y() - standardPadding() );
    m_scrollProxy->setMinimumSize( artistsSize );
    m_scrollProxy->setMaximumSize( artistsSize );

    debug()<< "SAA fin const";
    
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

    // we clear all artists
    foreach(ArtistWidget* art, m_artists)
    {
      m_layout->removeWidget(art);
      delete art;
    }

    m_artists.clear();
    
    
    m_stoppedState = true;
    m_headerLabel->setText( i18n( "Similar artist" ) + QString( " : " ) + i18n( "No track playing" ) );
    setCollapseOn();

    for( int i = 0; i < m_artists.size(); i++ )
    {
        //debug() << "ENTREE DANS LA BOUCLE CONSTRAINTSEVENT";
        m_layout->addWidget( m_artists.at( i ) );
    }
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
        //m_layout->setRowMinimumHeight(0,m_headerLabel->boundingRect().height()+7);
        //m_layout->setRowMaximumHeight(0,m_headerLabel->boundingRect().height()+7);


        QString artistName = data[ "artist" ].toString();

        // we see if the artist name is valid
        if (artistName.compare( "" ) != 0) {

           m_headerLabel->setText( i18n( "Similar artists of %1", artistName ) );
           
           
           SimilarArtist::SimilarArtistsList similars = data[ "SimilarArtists" ].value<SimilarArtist::SimilarArtistsList>();

           if ( !similars.isEmpty() )
            {
                debug()<<"similars";
                // we see the number of artist we need display
                int sizeArtistsDisplay=m_maxArtists>similars.size()?similars.size():m_maxArtists;

                debug()<<"SAA taille a afficher "<<sizeArtistsDisplay;
                // we adapt the list size
                int cpt=m_artists.size()+1; // the first row (0) is dedicated for the applet title
                debug()<<"SAA taille cpt:" <<cpt;
                debug()<<"SAA taille size :" << sizeArtistsDisplay;
                while(sizeArtistsDisplay>=cpt) {
                    debug()<<"SAA1";
                    ArtistWidget *art=new ArtistWidget;
                    debug()<<"SAA2";
                    m_artists.append(art);
                    debug()<<"SAA3";
                    //QGraphicsProxyWidget *tmp = new QGraphicsProxyWidget(this);
                    //tmp->setWidget(m_artists.last());
                    //m_scene->addWidget(art);
                    debug()<<"SAA4";
                    m_layout->addWidget(m_artists.last());//Item(tmp,cpt,0,1,1);
                    //m_artists.last()->setMinimumWidth(size().width() - 2 * standardPadding());
                    debug()<<"SAA5";
                    cpt++;
                    m_artists.last()->show();
                }
                m_scroll->widget()->show();

                debug()<<"SAA Agrandissement de la liste ok";
                //TODO Bug when the number of artist to display decrease
                cpt=sizeArtistsDisplay;
                debug()<<"SAA Nb artist a afficher :" << cpt;
                while(cpt<m_artists.size()) {
                    //m_layout->removeAt(cpt-1);
                    m_layout->removeWidget(m_artists.last());
                    delete m_artists.last();
                    m_artists.removeLast();
                    //cpt--;
                }
                debug()<<"SAA reduction de la liste ok";


                cpt=0; 
                foreach(ArtistWidget* art, m_artists) {
                    art->setArtist(similars.at(cpt).name(), similars.at(cpt).url());

                    debug()<< "SAA artist: " << similars.at(cpt).name();
                    art->setPhoto(similars.at(cpt).urlImage());
                    art->setMatch(similars.at(cpt).match());
                    cpt++;
                }
                debug()<<"SAA modif contenu ok";
                
            }
            else // No similar artist found
            {
                // we clear all artists
                for(int cpt=0;cpt<m_artists.size();cpt++)
                {
                    //m_layout->removeAt(cpt);
                    m_layout->removeWidget(m_artists.at(cpt));
                    delete m_artists.at(cpt);
                }

                m_artists.clear();

                m_headerLabel->setText( i18n( "Similar artist" ) + QString( " : " ) + i18n( "No similar artist found" ) );
                
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

    //simulate a update of data to update the content of the applet accordingly to the new settings
    dataUpdated( NULL, dataEngine( "amarok-similarArtists" )->query( "similarArtists" ) );
}

void
SimilarArtistsApplet::setGeom( )
{
    //updateConstraints();
//     constraintsEvent();
//     updateConstraints();
//     update();
//      foreach(ArtistWidget* art, m_artists)
//     {
//       art->adjustSize();
//       delete art;
//     }
}


#include "SimilarArtistsApplet.moc"




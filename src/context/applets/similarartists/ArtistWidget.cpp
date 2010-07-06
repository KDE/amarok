/****************************************************************************************
 * Copyright (c) 2009-2010 Joffrey Clavel <jclavel@clabert.info>                        *
 * Copyright (c) 2010 Alexandre Mendes <alex.mendes1988@gmail.com>                      *
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

#define DEBUG_PREFIX "ArtistWidget"

#include "ArtistWidget.h"

//Amarok
#include "amarokurls/AmarokUrl.h"
#include "core/collections/Collection.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "core/collections/QueryMaker.h"
#include "core/support/Debug.h"
#include "playlist/PlaylistModelStack.h"
#include "SvgHandler.h"

//KDE
#include <KHBox>
#include <KIcon>

//Qt
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QDesktopServices>
#include <QTextDocument>

ArtistWidget::ArtistWidget( QWidget *parent )
    : QWidget( parent )
{

    // set a fixed size for all widget, for harmonize the similar artists applet display
    this->setMinimumHeight( 115 );
    this->setMaximumHeight( 115 );

    this->setMinimumWidth(350);

    // The background og this widget is transparent
    m_layout = new QGridLayout( this );
    this->setAttribute( Qt::WA_TranslucentBackground, true );

    m_image = new QLabel( this );
    // The background of the QLabel is transparent
    m_image->setAttribute( Qt::WA_TranslucentBackground, true );
    m_image->setAlignment( Qt::AlignCenter );

    m_nameLabel = new QLabel( this );
    // The background of the QLabel is transparent
    m_nameLabel->setAttribute( Qt::WA_TranslucentBackground, true );
    m_nameLabel->setAlignment( Qt::AlignCenter );

    m_genre = new QLabel( this );
    // The background of the QLabel is transparent
    m_genre->setAttribute( Qt::WA_TranslucentBackground, true );
    m_genre->setAlignment( Qt::AlignCenter );

    m_topTrackLabel = new QLabel( this );
    m_topTrackLabel->setWordWrap( true );
    // The background of the QLabel is transparent
    m_topTrackLabel->setAttribute( Qt::WA_TranslucentBackground, true );
    m_topTrackLabel->setAlignment( Qt::AlignLeft );

    KHBox * spacer = new KHBox( this );
    spacer->setFixedHeight( 20 );

    //make sure the buttons are pushed all the way to the right.
    new QWidget( spacer );

    m_topTrackButton = new QPushButton( spacer );
    m_topTrackButton->setIcon( KIcon( "media-track-add-amarok" ) );
    m_topTrackButton->setFlat( true );
    m_topTrackButton->setFixedWidth( 20 );
    m_topTrackButton->setFixedHeight( 20 );
    m_topTrackButton->setToolTip( i18n( "Add top track to the Playlist" ) );
    m_topTrackButton->hide();
    
    connect( m_topTrackButton, SIGNAL( clicked( bool ) ), this, SLOT( addTopTrackToPlaylist() ) );

    m_navigateButton = new QPushButton( spacer );
    m_navigateButton->setIcon( KIcon( "edit-find" ) );
    m_navigateButton->setFlat( true );
    m_navigateButton->setFixedWidth( 20 );
    m_navigateButton->setFixedHeight( 20 );
    m_navigateButton->setToolTip( i18n( "Show in Media Sources" ) );
    m_navigateButton->hide();

    connect( m_navigateButton, SIGNAL( clicked( bool ) ), this, SLOT( navigateToArtist() ) );

    
    m_lastfmStationButton = new QPushButton( spacer );
    m_lastfmStationButton->setIcon( KIcon("view-services-lastfm-amarok") );
    m_lastfmStationButton->setFlat( true );
    m_lastfmStationButton->setFixedWidth( 20 );
    m_lastfmStationButton->setFixedHeight( 20 );
    m_lastfmStationButton->setToolTip( i18n( "Add last.fm artist station to the Playlist" ) );

    connect( m_lastfmStationButton, SIGNAL( clicked( bool ) ), this, SLOT( addLastfmArtistStation() ) );


    m_desc= new QLabel( this );
    m_desc->setWordWrap( true );
    // The background of the QLabel is transparent
    m_desc->setAttribute( Qt::WA_TranslucentBackground, true );
    m_desc->setAlignment( Qt::AlignLeft );
    m_desc->setMinimumHeight( 50 );


    // the image display is extended on two row
    m_layout->addWidget( m_image, 0, 0, 3, 1 );
    m_layout->addWidget( m_nameLabel, 0, 1 );
    m_layout->addWidget( m_genre, 0, 2 );
    m_layout->addWidget( m_topTrackLabel, 1, 1, 1, 2 );
    m_layout->addWidget( spacer, 1, 2, 1, 1 );
    m_layout->addWidget( m_desc, 2, 1, 1, 2 );

    // open the url of the similar artist when his name is clicked
    connect( m_nameLabel, SIGNAL( linkActivated( const QString & ) ), this
             , SLOT( openUrl( const QString  & ) ) );
}


ArtistWidget::~ArtistWidget()
{
    delete m_layout;
    delete m_image;
    delete m_nameLabel;
    delete m_genre;
    delete m_topTrackLabel;
    delete m_desc;
}

void
ArtistWidget::setPhoto( const QPixmap &photo )
{
    m_image->setPixmap( The::svgHandler()->addBordersToPixmap( photo, 5, QString(), true ) );
}

void
ArtistWidget::setPhoto( const KUrl& urlPhoto )
{
    // display a message for the user while the fetch of the picture
    m_image->clear();
    m_image->setText( i18n( "Loading the picture..." ) );

    m_url = urlPhoto;
    The::networkAccessManager()->getData( urlPhoto, this,
         SLOT(setImageFromInternet(KUrl,QByteArray,NetworkAccessManagerProxy::Error)) );
}

void
ArtistWidget::setImageFromInternet( const KUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e )
{
    if( m_url != url )
        return;

    m_url.clear();
    if( e.code != QNetworkReply::NoError )
    {
        m_image->clear();
        m_image->setText( i18n( "Unable to fetch the picture: %1", e.description ) );
        return;
    }

    QPixmap image;
    image.loadFromData( data );

    if( image.width() > 100 )
    {
        image = image.scaledToWidth( 100, Qt::SmoothTransformation );
    }

    if( image.height() > 100 )
    {
        image = image.scaledToHeight( 100, Qt::SmoothTransformation );
    }
    m_image->clear();
    m_image->setPixmap( The::svgHandler()->addBordersToPixmap( image, 5, QString(), true ) );
    //the height of the widget depends on the height of the artist picture
    //setMaximumHeight(image.height());
}

void
ArtistWidget::setArtist( const QString &nom, const KUrl &url )
{
    m_name = nom;
    m_nameLabel->setText( "<a href='" + url.url() + "'>" + nom + "</a>" );

    //Figure out of this applet is present in the local collection, and show the "show in collection" button if so
    m_navigateButton->hide();

    Collections::QueryMaker *qm = CollectionManager::instance()->queryMaker();
    
    qm->setQueryType( Collections::QueryMaker::Artist );
    qm->addFilter( Meta::valArtist, m_name );
    qm->limitMaxResultSize( 1 );
    qm->setAutoDelete( true );

    connect( qm, SIGNAL( newResultReady( QString, Meta::ArtistList ) ),
            SLOT( resultReady( QString, Meta::ArtistList ) ), Qt::QueuedConnection );

    qm->run();
    
}

void
ArtistWidget::setMatch( const int match )
{
    m_genre->setText( i18n( "Match" ) + " : " + QString::number( match ) + "%" );
}

void
ArtistWidget::clear()
{
    m_image->clear();
    m_nameLabel->clear();
    m_genre->clear();
    m_topTrackLabel->clear();
}

void
ArtistWidget::openUrl( const QString &url )
{
    QDesktopServices::openUrl( KUrl( "http://" + url ) );
}

void
ArtistWidget::setDescription(const QString &description)
{
    if(description.isEmpty())
    {
        m_desc->setText(i18n("No description available in your language"));
        m_descString.clear(); //we delete the precedent artist description
    } else {
        QTextDocument descriptionText;
        descriptionText.setHtml(description);
        QString descriptionString = descriptionText.toPlainText();
        m_descString=descriptionString;

        elideArtistDescription();
    }
}

void
ArtistWidget::setTopTrack(const QString &topTrack)
{
    m_topTrackButton->hide();
    
    if(topTrack.isEmpty())
    {
        m_topTrackLabel->setText(i18n("Top track not found"));
    }
    else
    {
        m_topTrackTitle = topTrack;
        m_topTrackLabel->setText( i18n( "Top track" ) + ": " +  topTrack );

        Collections::QueryMaker *qm = CollectionManager::instance()->queryMaker();

        qm->setQueryType( Collections::QueryMaker::Track );
        qm->beginAnd();
        qm->addFilter( Meta::valArtist, m_name );
        qm->addFilter( Meta::valTitle, m_topTrackTitle );
        qm->endAndOr();
        qm->limitMaxResultSize( 1 );
        qm->setAutoDelete( true );

        connect( qm, SIGNAL( newResultReady( QString, Meta::TrackList ) ),
                 SLOT( resultReady( QString, Meta::TrackList ) ) );
         
        qm->run();
    }
}

void
ArtistWidget::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)
    elideArtistDescription();
}

void
ArtistWidget::elideArtistDescription()
{
    DEBUG_BLOCK
    if(!m_descString.isEmpty())
    {
        QFontMetrics fontMetric(fontMetrics());
        int space = fontMetric.lineSpacing();
        int lineSpace = fontMetric.leading();
        int heightChar = fontMetric.height();
        int nbWidth = m_desc->width() ;
        float nbHeight = m_desc->height() / (heightChar+lineSpace) ;
        int widthTot = nbWidth * nbHeight - (space*(nbHeight-1));

        QString stringTmp=fontMetric.elidedText(m_descString,Qt::ElideRight,widthTot);

        //we delete nbHeigth words because of the wordWrap action
        for( int i = 0; i < nbHeight; ++i)
        {
            stringTmp = stringTmp.left( stringTmp.lastIndexOf(' ') );
        }

        stringTmp.append("...");
        m_desc->setText(stringTmp);
    }
}

void
ArtistWidget::addTopTrackToPlaylist()
{
    The::playlistController()->insertOptioned( m_topTrack, Playlist::AppendAndPlay );
}


void
ArtistWidget::navigateToArtist()
{
    AmarokUrl url;
    url.setCommand( "navigate" );
    url.setPath( "collections" );
    url.appendArg( "filter", "artist:\"" + m_name + "\"" );
    url.run();
}

void
ArtistWidget::addLastfmArtistStation()
{
    const QString url = "lastfm://artist/" + m_name + "/similarartists";
    Meta::TrackPtr lastfmtrack = CollectionManager::instance()->trackForUrl( KUrl( url ) );
    The::playlistController()->insertOptioned( lastfmtrack, Playlist::AppendAndPlay );
}

void
ArtistWidget::resultReady( const QString &collectionId, const Meta::ArtistList &artists )
{
    Q_UNUSED( collectionId )
    if( artists.length() > 0 )
        m_navigateButton->show();

}

void
ArtistWidget::resultReady( const QString &collectionId, const Meta::TrackList &tracks )
{
    Q_UNUSED( collectionId )
    if( !tracks.isEmpty() )
    {
        m_topTrack = tracks.first();
        m_navigateButton->show();
        m_topTrackButton->show();
    }
}

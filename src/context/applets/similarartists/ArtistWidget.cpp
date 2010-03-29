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

#include "ArtistWidget.h"

//Amarok
#include "amarokurls/AmarokUrl.h"
#include "core/collections/Collection.h"
#include "core-implementations/collections/support/CollectionManager.h"
#include "core/collections/QueryMaker.h"
#include "core/support/Debug.h"
#include "playlist/PlaylistModelStack.h"

//KDE
#include <KHBox>
#include <KIcon>
#include <KLocale>

//Qt
#include <QGridLayout>
#include <QGraphicsScene>
#include <QGraphicsProxyWidget>
#include <QPushButton>
#include <QLabel>
#include <QDesktopServices>
#include <QTextDocument>

/**
 * ArtistWidget constructor
 * @param parent The widget parent
 */
ArtistWidget::ArtistWidget( QWidget *parent )
    : QWidget( parent )
    , m_qm( 0 )
{

    // set a fixed size for all widget, for harmonize the similar artists applet display
    this->setMinimumHeight( 105 );
    this->setMaximumHeight( 105 );

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

    m_topTrack = new QLabel( this );
    m_topTrack->setWordWrap( true );
    // The background of the QLabel is transparent
    m_topTrack->setAttribute( Qt::WA_TranslucentBackground, true );
    m_topTrack->setAlignment( Qt::AlignLeft );


    KHBox * spacer = new KHBox( this );
    spacer->setFixedHeight( 20 );

    //make sure the buttons are pushed all the way to the right.
    new QWidget( spacer );

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
    m_layout->addWidget( m_topTrack, 1, 1, 1, 2 );
    m_layout->addWidget( spacer, 1, 2, 1, 1 );
    m_layout->addWidget( m_desc, 2, 1, 1, 2 );

    // open the url of the similar artist when his name is clicked
    connect( m_nameLabel, SIGNAL( linkActivated( const QString & ) ), this
             , SLOT( openUrl( const QString  & ) ) );
}


/**
 * ArtistWidget destructor
 */
ArtistWidget::~ArtistWidget()
{
    delete m_layout;
    delete m_image;
    delete m_nameLabel;
    delete m_genre;
    delete m_topTrack;
    delete m_imageJob;
    delete m_desc;
}


/**
 * Change the photo of the artist
 * @param photo The new artist photo
 */
void
ArtistWidget::setPhoto( const QPixmap &photo )
{
    m_image->setPixmap( photo );
}

/**
 * Change the photo of the artist with a photo load from an Url
 * @param photo The url of the new artist photo
 */
void
ArtistWidget::setPhoto( const KUrl& urlPhoto )
{
    // display a message for the user while the fetch of the picture
    m_image->clear();
    m_image->setText( i18n( "Loading the picture..." ) );

    m_imageJob = KIO::storedGet( urlPhoto, KIO::NoReload, KIO::HideProgressInfo );
    connect( m_imageJob, SIGNAL( result( KJob* ) )
             , SLOT( setImageFromInternet( KJob* ) ) );
}


/**
 * Put the image of the artist in the QPixMap
 * @param job, pointer to the job which get the pixmap from the web
 */
void
ArtistWidget::setImageFromInternet( KJob *job )
{
    if ( !m_imageJob ) return; //track changed while we were fetching

    // It's the correct job but it errored out
    if ( job->error() != KJob::NoError && job == m_imageJob )
    {
        m_image->clear();
        m_image->setText( i18n( "Unable to fetch the picture" ) );
        m_imageJob = 0; // clear job
        return;
    }

    // not the right job, so let's ignore it
    if ( job != m_imageJob )
        return;

    if ( job )
    {
        KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( job );
        QPixmap image;
        image.loadFromData( storedJob->data() );
        if ( image.width() > 100 )
        {
            image = image.scaledToWidth( 100, Qt::SmoothTransformation );
        }

        if ( image.height() > 100 )
        {
            image = image.scaledToHeight( 100, Qt::SmoothTransformation );
        }

        m_image->clear();
        m_image->setPixmap( image );
        //the height of the widget depends on the height of the artist picture
        //setMaximumHeight(image.height());
    }
    else
    {
        m_image->clear();
        m_image->setText( i18n( "No picture" ) );
    }

    m_imageJob = 0;
}

 /**
 * Change the artist name and the url which allows to display a page
 * which contains informations about this artist
 * @param nom The name of this artist
 * @param url The url of the artist about page
 */
void
ArtistWidget::setArtist( const QString &nom, const KUrl &url )
{
    m_name = nom;
    m_nameLabel->setText( "<a href='" + url.url() + "'>" + nom + "</a>" );

    //Figure out of this applet is present in the local collection, and show the "show in collection" button if so
    m_navigateButton->hide();

    if( m_qm )
        m_qm->reset();
    else
    {
        Collections::Collection *coll = CollectionManager::instance()->primaryCollection();
        m_qm = coll->queryMaker();
    }
    
    m_qm->setQueryType( Collections::QueryMaker::Artist );
    m_qm->addFilter( Collections::QueryMaker::ArtistFilter, m_name );
    m_qm->limitMaxResultSize( 1 );

    connect( m_qm, SIGNAL( newResultReady( QString, Meta::ArtistList ) ),
            SLOT( resultReady( QString, Meta::ArtistList ) ), Qt::QueuedConnection );

    m_qm->run();
    
}

/**
 * Change the match pourcentage of the artist
 * @param match The match of this artist
 */
void
ArtistWidget::setMatch( const int match )
{
    m_genre->setText( i18n( "Match" ) + " : " + QString::number( match ) + "%" );
}

/**
 * Clean the widget => the content of the QLabel are empty
 */
void
ArtistWidget::clear()
{
    m_image->clear();
    m_nameLabel->clear();
    m_genre->clear();
    m_topTrack->clear();
}

/**
 * Open an URL
 * @param url The URL of the artist
 */
void
ArtistWidget::openUrl( const QString &url )
{
    QDesktopServices::openUrl( KUrl( "http://" + url ) );
}


/**
 * Change the artist description which contains informations about this artist
 * @param desc The description of this artist
 */
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

/**
 * Change the most known track of this artist
 * @param topTrack the top track of this artist
 */
void
ArtistWidget::setTopTrack(const QString &topTrack)
{
    if(topTrack.isEmpty())
    {
        m_topTrack->setText(i18n("Top track not found"));
    } else {
        m_topTrack->setText( i18n( "Top track" ) + " : " +  topTrack);
    }
}



void
ArtistWidget::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)
    elideArtistDescription();
}

/**
 * Elide the artist description depending on the widget size
 */
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
        for(int i; i<nbHeight; i++) {
            stringTmp=stringTmp.left(stringTmp.lastIndexOf(' '));
        }

        stringTmp.append("...");
        m_desc->setText(stringTmp);
    }
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
    DEBUG_BLOCK
    Q_UNUSED( collectionId )
    if( artists.length() > 0 )
        m_navigateButton->show();

}

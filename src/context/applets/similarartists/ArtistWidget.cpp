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
#include "Debug.h"

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
ArtistWidget::ArtistWidget( QWidget *parent ) : QWidget( parent )
{

    // set a fixed size for all widget, for harmonize the similar artists applet display
    this->setMinimumHeight( 105 );
    this->setMaximumHeight( 105 );

    // The background og this widget is transparent
    m_layout = new QGridLayout( this );
    this->setAttribute( Qt::WA_TranslucentBackground, true );

    m_image = new QLabel( this );
    // The background of the QLabel is transparent
    m_image->setAttribute( Qt::WA_TranslucentBackground, true );
    m_image->setAlignment( Qt::AlignCenter );

    m_name = new QLabel( this );
    // The background of the QLabel is transparent
    m_name->setAttribute( Qt::WA_TranslucentBackground, true );
    m_name->setAlignment( Qt::AlignCenter );

    m_genre = new QLabel( this );
    // The background of the QLabel is transparent
    m_genre->setAttribute( Qt::WA_TranslucentBackground, true );
    m_genre->setAlignment( Qt::AlignCenter );

    m_topTrack = new QLabel( this );
    m_topTrack->setWordWrap( true );
    // The background of the QLabel is transparent
    m_topTrack->setAttribute( Qt::WA_TranslucentBackground, true );
    m_topTrack->setAlignment( Qt::AlignLeft );

    m_desc= new QLabel( this );
    m_desc->setWordWrap( true );
    // The background of the QLabel is transparent
    m_desc->setAttribute( Qt::WA_TranslucentBackground, true );
    m_desc->setAlignment( Qt::AlignLeft );

    

    // the image display is extended on two row
    m_layout->addWidget( m_image, 0, 0, 3, 1 );
    m_layout->addWidget( m_name, 0, 1 );
    m_layout->addWidget( m_genre, 0, 2 );
    m_layout->addWidget( m_topTrack, 1, 1, 1, 2 );
    m_layout->addWidget( m_desc, 2, 1, 1, 2 );

    // open the url of the similar artist when his name is clicked
    connect( m_name, SIGNAL( linkActivated( QString ) ), this
             , SLOT( openUrl( QString ) ) );
}


/**
 * ArtistWidget destructor
 */
ArtistWidget::~ArtistWidget()
{
    delete m_layout;
    delete m_image;
    delete m_name;
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
    m_name->setText( "<a href='" + url.url() + "'>" + nom + "</a>" );
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
    m_name->clear();
    m_genre->clear();
    m_topTrack->clear();
}

/**
 * Open an URL
 * @param url The URL of the artist
 */
void
ArtistWidget::openUrl( QString &url )
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
    } else {
        QTextDocument descriptionText;
        descriptionText.setHtml(description);
        QString descriptionString = descriptionText.toPlainText();
        //resize the description
        if (description.length() > 150)
        {
            //resizing the qstring
            descriptionString.resize(150);
            //looking for the last space
            int last_space = descriptionString.lastIndexOf(" ");
            //adding ... following this space
            descriptionString = descriptionString.mid(0, last_space).append("...");
        }
        m_desc->setText(descriptionString);
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


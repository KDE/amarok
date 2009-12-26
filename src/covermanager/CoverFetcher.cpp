/****************************************************************************************
 * Copyright (c) 2004 Mark Kretschmann <kretschmann@kde.org>                            *
 * Copyright (c) 2004 Stefan Bogner <bochi@online.ms>                                   *
 * Copyright (c) 2004 Max Howell <max.howell@methylblue.com>                            *
 * Copyright (c) 2007 Dan Meltzer <parallelgrapefruit@gmail.com>                        *
 * Copyright (c) 2009 Martin Sandsmark <sandsmark@samfundet.no>                         *
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

#include "CoverFetcher.h"

#include "Amarok.h"
#include "amarokconfig.h"
#include "CoverManager.h"
#include "Debug.h"
#include "statusbar/StatusBar.h"
#include "ui_EditCoverSearchDialog.h"

#include <KIO/Job>
#include <KLocale>
#include <KUrl>

#include <QDomDocument>
#include <QDomElement>
#include <QDomNode>
#include <QImageReader>
#include <QLabel>
#include <QRegExp>


CoverLabel::CoverLabel ( QWidget * parent, Qt::WindowFlags f )
        : QLabel( parent, f)
{}

void CoverLabel::mouseReleaseEvent(QMouseEvent *pEvent) {
    if (pEvent->button() == Qt::LeftButton || pEvent->button() == Qt::RightButton)
    {
//         Amarok::coverContextMenu( this, pEvent->globalPos(), m_albumPtr, false );
    }
}

CoverFetcher* CoverFetcher::s_instance = 0;

CoverFetcher*
CoverFetcher::instance()
{
    return s_instance ? s_instance : new CoverFetcher();
}

void CoverFetcher::destroy() {
    if (s_instance) {
        delete s_instance;
        s_instance = 0;
    }
}

CoverFetcher::CoverFetcher()
        : QObject()
        , m_size( 2 )
        , m_success( true )
        , m_isFetching( false )
{
    DEBUG_FUNC_INFO
    setObjectName( "CoverFetcher" );

    s_instance = this;
}

CoverFetcher::~CoverFetcher()
{
    DEBUG_FUNC_INFO
}

void
CoverFetcher::manualFetch( Meta::AlbumPtr album )
{
    m_interactive = true;
    m_albums << album;
    startFetch( album );
}

void
CoverFetcher::queueAlbum( Meta::AlbumPtr album )
{
    if( m_albumPtr == album || m_albums.contains( album ) )
        return;
    m_interactive = false;
    m_albumsMutex.lock();
    m_albums << album;
    m_albumsMutex.unlock();
    m_fetchMutex.lock();
    if( !m_isFetching )
    {
        m_fetchMutex.unlock();

        m_albumsMutex.lock();
        if( !m_albums.isEmpty() )
        {
            m_isFetching = true;
            Meta::AlbumPtr firstAlbum = m_albums.takeFirst();
            m_albumsMutex.unlock();
            startFetch( album );
        }
        else
        {
            m_albumsMutex.unlock();
        }
    }
    else
    {
        m_fetchMutex.unlock();
    }
}
void
CoverFetcher::queueAlbums( Meta::AlbumList albums )
{
    m_interactive = false;
    m_albumsMutex.lock();
    foreach( Meta::AlbumPtr album, albums )
    {
        if( m_albumPtr == album || m_albums.contains( album ) )
            continue;
        m_albums << album;
    }
    m_albumsMutex.unlock();
    m_fetchMutex.lock();
    if( !m_isFetching )
    {
        m_fetchMutex.unlock();
        m_albumsMutex.lock();

        if( !m_albums.isEmpty() )
        {
            Meta::AlbumPtr album = m_albums.takeFirst();
            m_albumsMutex.unlock();
            startFetch( album );
        }
        else
        {
            m_albumsMutex.unlock();
        }

    }
    else
    {
        m_fetchMutex.unlock();
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE SLOTS
//////////////////////////////////////////////////////////////////////////////////////////

void
CoverFetcher::startFetch( Meta::AlbumPtr album )
{
    m_fetchMutex.lock();
    m_isFetching = true;
    m_fetchMutex.unlock();
    m_albumPtr = album;

    // reset all values
    m_xml.clear();
    m_size = 3;

    QUrl url;
    url.setScheme( "http" );
    url.setHost( "ws.audioscrobbler.com" );
    url.setPath( "/2.0/" );
    url.addQueryItem( "method", "album.search" );
    url.addQueryItem( "api_key", "402d3ca8e9bc9d3cf9b85e1202944ca5" );
    url.addQueryItem( "album", album->name().toLocal8Bit() );

    debug() << url;

    KJob* job = KIO::storedGet( url, KIO::NoReload, KIO::HideProgressInfo );
    connect( job, SIGNAL(result( KJob* )), SLOT(finishedXmlFetch( KJob* )) );

    if( m_interactive )
        The::statusBar()->newProgressOperation( job, i18n( "Fetching Cover" ) );
}

void
CoverFetcher::finishedXmlFetch( KJob *job ) //SLOT
{
    // NOTE: job can become 0 when this method is called from attemptAnotherFetch()
    if( job && job->error() )
    {
        finishWithError( i18n( "There was an error communicating with last.fm." ), job );
        return;
    }

    if( job )
    {
        KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( job );
        m_xml = QString::fromUtf8( storedJob->data().data(), storedJob->data().size() );
    }

    QDomDocument doc;
    if( !doc.setContent( m_xml ) )
    {
        m_errors += i18n( "The XML obtained from Last.fm is invalid." );
        if( m_albums.size() > 0 )
            startFetch( m_albums.takeFirst() );

        return;
    }

    m_pixmaps.clear();
    m_processedCovers = 0;
    m_numURLS = 0;

    const QDomNodeList foundAlbums = doc.documentElement().namedItem( "results" ).namedItem( "albummatches" ).childNodes();

    for( uint x = 0; x < foundAlbums.length(); x++ )
    {
        const QDomNodeList list = foundAlbums.item( x ).childNodes();

        QString size;
        switch( m_size )
        {
            case 0:  size = "small";  break;
            case 1:  size = "medium"; break;
            case 2:  size = "large"; break;
            default: size = "extralarge";  break;
        }
        QString coverUrl;
        for( int i = 0; i < list.count(); i++ )
        {
            QDomNode n = list.item( i );
            if( n.nodeName() == "image" )
            {
                const QDomNode node = list.item( i );

                if( node.hasAttributes() )
                {
                    const QString imageSize = node.attributes().namedItem( "size" ).nodeValue();
                    if( imageSize == size && node.isElement() )
                    {
                        coverUrl = node.toElement().text();
                    }
                }
            }
        }

        if( coverUrl.isEmpty() )
        {
            continue;
        }

        m_numURLS++;

        //FIXME: Asyncronous behaviour without informing the user is bad in this case
        KJob* getJob = KIO::storedGet( KUrl(coverUrl), KIO::NoReload, KIO::HideProgressInfo );
        connect( getJob, SIGNAL( result( KJob* ) ), SLOT( finishedImageFetch( KJob* ) ) );
    }

    if ( m_numURLS == 0 )
        finishNotFound();
}

void
CoverFetcher::finishedImageFetch( KJob *job ) //SLOT
{
    QPixmap pixmap;
    KIO::StoredTransferJob* storedJob = static_cast<KIO::StoredTransferJob*>( job );
    QImageReader imageReader( storedJob->data() );

    // NOTE: Using QImageReader here is a workaround for a bug in Qt 4.6.0,
    // in QPixmap::loadFromData(), which crashes.
    // @see: https://bugs.kde.org/show_bug.cgi?id=215392
    if( job->error() || !imageReader.canRead() )
    {
        debug() << "finishedImageFetch(): KIO::error(): " << storedJob->error();
        m_errors += i18n( "The cover could not be retrieved." );
        finishWithError( i18n( "The cover could not be retrieved." ), job );
        return;
    }
    else
    {
        pixmap.fromImage( imageReader.read() );
        m_pixmaps.append( pixmap );
    }

    m_processedCovers++;
    
    if( m_processedCovers == m_numURLS )
    {
        if( m_pixmaps.length() > 0 )
        {
            if( m_interactive )
            {
                //yay! images found :)
                //lets see if the user wants one of it
                m_processedCovers = 9999; //prevents to popup a 2nd window
                showCover();
            }
            else
            {
                m_selPixmap = m_pixmaps.takeFirst();
                //image loaded successfully yay!
                finish();
            }
        }
        else if( m_processedCovers == m_numURLS )
        {
            debug() << "No cover could be retrieved!";
            finishNotFound();
        }
    }

    The::statusBar()->endProgressOperation( job ); //just to be safe...
}

void
CoverFetcher::showCover()
{
    CoverFoundDialog dialog( static_cast<QWidget*>( parent() ), m_pixmaps, m_currentCoverName );

    switch( dialog.exec() )
    {
    case KDialog::Accepted:
        m_selPixmap = QPixmap( dialog.image() );
        finish();
        break;
    case KDialog::Rejected: //make sure we do not show any more dialogs
        debug() << "cover rejected";
        break;
    default:
        finishWithError( i18n( "Aborted." ) );
        break;
    }
}


void
CoverFetcher::finish()
{
    The::statusBar()->shortMessage( i18n( "Retrieved cover successfully" ) );
    m_albumPtr->setImage( image() );
    m_isFetching = false;
    if( !m_interactive /*manual fetch*/ && !m_albums.isEmpty() )
        startFetch( m_albums.takeFirst() );

}

void
CoverFetcher::finishWithError( const QString &message, KJob *job )
{
    if( job )
        warning() << message << " KIO::error(): " << job->errorText();

    m_errors += message;
    m_success = false;

    debug() << "Album name" << m_albumPtr->name();

    m_isFetching = false;

    // Time to move on.
    if( !m_albums.isEmpty() )
    {
        debug() << "next album" << m_albums[0]->name();
        startFetch( m_albums.takeFirst() );
    }

}

void 
CoverFetcher::finishNotFound()
{
    if( m_interactive )
        //FIXME: Not visible behind cover manager
        The::statusBar()->longMessage( i18n( "Unable to find a cover for the specified song." ), StatusBar::Sorry );
    else
        //FIXME: Not visible behind cover manager
        The::statusBar()->shortMessage( i18n( "Unable to find a cover for %1.", m_albumPtr->name() ) );
    
    m_isFetching = false;

    if( !m_interactive /*manual fetch*/ && !m_albums.isEmpty() )
    {
				debug() << "album not found, size of m_albums: " << m_albums.size();
        startFetch( m_albums.takeFirst() );
    }
}

CoverFoundDialog::CoverFoundDialog( QWidget *parent, const QList<QPixmap> &covers, const QString &productname ) : KDialog( parent )
{
    m_curCover = 0;
    m_covers.clear();
    m_covers = covers;
    this->setButtons( None );
    this->showButtonSeparator( false );
    KVBox *box = new KVBox( this );
    this->setMainWidget(box);

    m_labelPix  = new QLabel( box );
    m_labelName = new QLabel( box );
    m_buttons   = new KHBox( box );
    m_prev      = new KPushButton( KStandardGuiItem::back(), m_buttons );
    m_save      = new KPushButton( KStandardGuiItem::save(), m_buttons );
    m_cancel    = new KPushButton( KStandardGuiItem::cancel(), m_buttons );
    m_next      = new KPushButton( KStandardGuiItem::forward(), m_buttons );

    if( m_curCover == m_covers.length() )
        m_next->setEnabled( false );
    else
        m_next->setEnabled( true );

    m_prev->setEnabled( false );

    m_labelPix ->setMinimumHeight( 300 );
    m_labelPix ->setMinimumWidth( 300 );
    m_labelPix ->setAlignment( Qt::AlignHCenter );
    m_labelName->setAlignment( Qt::AlignHCenter );
    m_labelPix ->setPixmap( m_covers.at( m_curCover ) );
    m_labelName->setText( productname );

    m_save->setDefault( true );
    this->setFixedSize( sizeHint() );
    this->setCaption( i18n( "Cover Found" ) );

    connect( m_prev, SIGNAL(clicked()), SLOT(prevPix()) );
    connect( m_save,   SIGNAL(clicked()), SLOT(accept()) );
    connect( m_cancel, SIGNAL(clicked()), SLOT(reject()) );
    connect( m_next, SIGNAL(clicked()), SLOT(nextPix()) );
}

//SLOT
void CoverFoundDialog::nextPix()
{
    if( m_curCover < m_covers.length()-1 )
    {
        m_curCover++;
        m_labelPix ->setPixmap( m_covers.at( m_curCover ) );
        m_prev->setEnabled( true );
    }
        
    if( m_curCover >= m_covers.length()-1 )
        m_next->setEnabled( false );
    else
        m_next->setEnabled( true );
}

//SLOT
void CoverFoundDialog::prevPix()
{
    if( m_curCover > 0 )
    {
        m_curCover--;
        m_labelPix ->setPixmap( m_covers.at( m_curCover ) );
        m_next->setEnabled( true );
    }
        
    if( m_curCover == 0 )
        m_prev->setEnabled( false );
    else
        m_prev->setEnabled( true );
}


#include "CoverFetcher.moc"


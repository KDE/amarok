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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
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

#include <KDialog>
#include <KHBox>
#include <KIO/Job>
#include <KLocale>
#include <KPushButton>
#include <KUrl>
#include <KVBox>

#include <QDomDocument>
#include <QDomElement>
#include <QDomNode>
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
    url.addQueryItem( "method", "album.getinfo" );
    url.addQueryItem( "api_key", "402d3ca8e9bc9d3cf9b85e1202944ca5" );
    url.addQueryItem( "album", album->name().toLocal8Bit() );

    if ( album->hasAlbumArtist() )
        url.addQueryItem( "artist", album->albumArtist()->name().toLocal8Bit() );

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

    const QDomNodeList list = doc.documentElement().namedItem( "album" ).childNodes();

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

            if ( node.hasAttributes() ) {
                const QString imageSize = node.attributes().namedItem( "size" ).nodeValue();
                if ( imageSize == size && node.isElement() ) {
                    coverUrl = node.toElement().text();
                }
            }
        }
    }

    if ( coverUrl.isEmpty() ) {
        if (m_interactive)
            The::statusBar()->longMessage( "Unable to find a cover for the specified song.", StatusBar::Sorry );
        else
            The::statusBar()->shortMessage( "Unable to find a cover for the specified song." );

        return;
    }

    KJob* getJob = KIO::storedGet( KUrl(coverUrl), KIO::NoReload, KIO::HideProgressInfo );
    connect( getJob, SIGNAL( result( KJob* ) ), SLOT( finishedImageFetch( KJob* ) ) );
}

void
CoverFetcher::finishedImageFetch( KJob *job ) //SLOT
{
    if( job->error() || !m_pixmap.loadFromData( static_cast<KIO::StoredTransferJob*>( job )->data() ) )
    {
        debug() << "finishedImageFetch(): KIO::error(): " << job->error();
        m_errors += i18n( "The cover could not be retrieved." );
        return;
    }

    else if( m_interactive )
    {
        //yay! image found :)
        //lets see if the user wants it
        showCover();
    }
    else
        //image loaded successfully yay!
        finish();

    The::statusBar()->endProgressOperation( job ); //just to be safe...
}

    class CoverFoundDialog : public KDialog
    {
    public:
        CoverFoundDialog( QWidget *parent, const QPixmap &cover, const QString &productname )
                : KDialog( parent )
        {
            setButtons( None );
            showButtonSeparator( false );
            KVBox *box = new KVBox( this );
            setMainWidget(box);

            QLabel      *labelPix  = new QLabel( box );
            QLabel      *labelName = new QLabel( box );
            KHBox       *buttons   = new KHBox( box );
            KPushButton *save      = new KPushButton( KStandardGuiItem::save(), buttons );
            KPushButton *cancel    = new KPushButton( KStandardGuiItem::cancel(), buttons );

            labelPix ->setAlignment( Qt::AlignHCenter );
            labelName->setAlignment( Qt::AlignHCenter );
            labelPix ->setPixmap( cover );
            labelName->setText( productname );

            save->setDefault( true );
            this->setFixedSize( sizeHint() );
            this->setCaption( i18n("Cover Found") );

            connect( save,   SIGNAL(clicked()), SLOT(accept()) );
            connect( cancel, SIGNAL(clicked()), SLOT(reject()) );
        }

        virtual void accept()
        {
            if( qstrcmp( sender()->objectName().toAscii(), "NewSearch" ) == 0 )
                done( 1000 );
            else if( qstrcmp( sender()->objectName().toAscii(), "NextCover" ) == 0 )
                done( 1001 );
            else
                KDialog::accept();
        }
    };


void
CoverFetcher::showCover()
{
    CoverFoundDialog dialog( static_cast<QWidget*>( parent() ), m_pixmap, m_currentCoverName );

    switch( dialog.exec() )
    {
    case KDialog::Accepted:
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

#include "CoverFetcher.moc"


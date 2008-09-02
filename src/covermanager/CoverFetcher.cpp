/******************************************************************************
 * Copyright (c) 2004 Mark Kretschmann <kretschmann@kde.org>                  *
 *           (c) 2004 Stefan Bogner <bochi@online.ms>                         *
 *           (c) 2004 Max Howell <max.howell@methylblue.com>                  *
 *           (c) 2007 Dan Meltzer <hydrogen@notyetimplemented.com>            *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License as             *
 * published by the Free Software Foundation; either version 2 of             *
 * the License, or (at your option) any later version.                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.      *
 ******************************************************************************/

#include "CoverFetcher.h"

#include "Amarok.h"
#include "amarokconfig.h"
#include "CoverManager.h"
#include "Debug.h"
#include "StatusBar.h"
#include "ui_EditCoverSearchDialog.h"

#include <KDialog>
#include <KHBox>
#include <KIO/Job>
#include <KLineEdit>
#include <KLocale>
#include <KPushButton>
#include <KUrl>
#include <KVBox>

#include <QDomDocument>
#include <QDomElement>
#include <QDomNode>
#include <QLabel>
#include <QMutexLocker>
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

class CoverFetcherSingleton
{
    public:
        CoverFetcher instance;
};

K_GLOBAL_STATIC( CoverFetcherSingleton, s_privateInstance )

CoverFetcher*
CoverFetcher::instance()
{
    return &s_privateInstance->instance;
}

CoverFetcher::CoverFetcher()
        : QObject()
        , m_size( 2 )
        , m_success( true )
        , m_isFetching( false )
{
    DEBUG_FUNC_INFO
    setObjectName( "CoverFetcher" );

    qAddPostRoutine( s_privateInstance.destroy ); //Ensures that the dtor gets called when QApplication destructs
}

CoverFetcher::~CoverFetcher()
{
    DEBUG_FUNC_INFO
}

void
CoverFetcher::manualFetch( Meta::AlbumPtr album )
{
    m_userCanEditQuery = true;
    m_albums << album;
    buildQueries( album );
}

void
CoverFetcher::queueAlbum( Meta::AlbumPtr album )
{
    if( m_albumPtr == album || m_albums.contains( album ) )
        return;
    m_userCanEditQuery = false;
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
            Meta::AlbumPtr album = m_albums.takeFirst();
            m_albumsMutex.unlock();
            buildQueries( album );
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
    m_userCanEditQuery = false;
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
            buildQueries( album );
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


void CoverFetcher::buildQueries( Meta::AlbumPtr album )
{
    DEBUG_BLOCK
    m_fetchMutex.lock();
    m_isFetching = true;
    m_fetchMutex.unlock();
    m_albumPtr = album;

    // Amazon doesn't like accents, so we use cleanPath() to remove them
    QString albumName = Amarok::cleanPath( album->name() );
    QString artistName = Amarok::cleanPath( album->hasAlbumArtist() ? album->albumArtist()->name() : QString() );

    QStringList extensions;
    extensions << i18n("disc") << i18n("disk") << i18n("remaster") << i18n("cd") << i18n("single") << i18n("soundtrack") << i18n("part")
            << "disc" << "disk" << "remaster" << "cd" << "single" << "soundtrack" << "part" << "cds" /*cd single*/;

    m_queries.clear();
    m_userQuery.clear();

    //we do several queries, one raw ie, without the following modifications
    //the others have the above strings removed with the following regex, as this can increase hit-rate
    const QString template1 = " ?-? ?[(^{]* ?%1 ?\\d*[)^}\\]]* *$"; //eg album - [disk 1] -> album
    foreach( const QString &extension, extensions ) {
        QRegExp regexp( template1.arg( extension ) );
        regexp.setCaseSensitivity( Qt::CaseInsensitive );
        albumName.remove( regexp );
    }

    //TODO try queries that remove anything in album after a " - " eg Les Mis. - Excerpts

    /**
     * We search for artist - album, and just album, using the exact album text and the
     * manipulated album text.
     */

    //search on our modified term, then the original
    if ( !artistName.isEmpty() )
        m_userQuery = artistName + " - ";
    m_userQuery += albumName;

    m_queries += m_userQuery;
    m_queries += artistName + " - " + albumName;
    m_queries += albumName;

    //don't do the same searches twice in a row
    m_queries.pop_front();
    m_queries.pop_back();

    /**
     * Finally we do a search for just the artist, just in case as this often
     * turns up a cover, and it might just be the right one! Also it would be
     * the only valid search if m_album.isEmpty()
     */
    m_queries += artistName;
    debug() << m_queries;
    startFetch( album );
}
void
CoverFetcher::startFetch( Meta::AlbumPtr album )
{
    DEBUG_BLOCK
    m_fetchMutex.lock();
    m_isFetching = true;
    m_fetchMutex.unlock();
    m_albumPtr = album;

    // Amarok's Amazon license Key.
    const QString LICENSE( "0N04TFAWZR6YVWWS3CG2" );

    // reset all values
    m_coverAmazonUrls.clear();
    m_coverAsins.clear();
    m_coverUrls.clear();
    m_coverNames.clear();
    m_xml.clear();
    m_size = 2;

    if( m_queries.isEmpty() )
    {
        debug() << "m_queries is empty";
        finishWithError( i18n("No cover found") );
        return;
    }
    QString query = m_queries.front();
    m_queries.pop_front();

    // '&' breaks searching
    query.remove('&');

    // Bug 97901: Import cover from amazon france doesn't work properly
    // (we have to set "mode=music-fr" instead of "mode=music")
    QString locale = AmarokConfig::amazonLocale();
    //Amazon Japan isn't on xml.amazon.com
    QString tld = "com";

    if( locale == "us" )
        tld = "com";
    else if( locale =="uk" )
        tld = "co.uk";
    else if( locale == "jp" )
        tld = "co.jp";
    else
        tld = locale;

    QString url;
    url = "http://ecs.amazonaws." + tld
            + "/onca/xml?Service=AWSECommerceService&Version=2007-10-29&Operation=ItemSearch&AssociateTag=webservices-20&AWSAccessKeyId=" + LICENSE
            + "&Keywords=" + QUrl::toPercentEncoding( query, "/" )
            + "&SearchIndex=Music&ResponseGroup=Small,Images";
    debug() << url;

    KJob* job = KIO::storedGet( url, KIO::NoReload, KIO::HideProgressInfo );
    connect( job, SIGNAL(result( KJob* )), SLOT(finishedXmlFetch( KJob* )) );
    
    if( m_userCanEditQuery )
        The::statusBar()->newProgressOperation( job ).setDescription( i18n( "Fetching Cover" ) );

}

void
CoverFetcher::finishedXmlFetch( KJob *job ) //SLOT
{
    DEBUG_BLOCK

    // NOTE: job can become 0 when this method is called from attemptAnotherFetch()
    if( job && job->error() )
    {
        finishWithError( i18n("There was an error communicating with Amazon."), job );
        return;
    }

    /*
    if( m_albums.isEmpty() )
    {
        finishWithError( i18n("Internal error, no albums in queue"), job );
        return;
    }
    */

    if( job )
    {
        KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( job );
        m_xml = QString::fromUtf8( storedJob->data().data(), storedJob->data().size() );
    }

    QDomDocument doc;
    if( !doc.setContent( m_xml ) )
    {
        m_errors += i18n("The XML obtained from Amazon is invalid.");
        buildQueries( m_albums.takeFirst() );
        return;
    }

    const QDomNode details = doc.documentElement().namedItem( "Details" );

    // the url for the Amazon product info page
    const QDomNodeList list = doc.documentElement().namedItem( "Items" ).childNodes();

    for(int i = 0; i < list.count(); i++ )
    {
        QDomNode n = list.item( i );
        if( n.isElement() && n.nodeName() == "IsValid" )
        {
            if( n.toElement().text() == "False" )
            {
                warning() << "The XML Is Invalid!";
                return;
            }
        }
        else if( list.item( i ).nodeName() == "Item" )
        {
            const QDomNode node = list.item( i );
            parseItemNode( node );
        }
    }
    attemptAnotherFetch();
}


void CoverFetcher::parseItemNode( const QDomNode &node )
{
    DEBUG_BLOCK
    QDomNode it = node.firstChild();

    QString size;
    switch( m_size )
    {
        case 0:  size = "Small";  break;
        case 1:  size = "Medium"; break;
        default: size = "Large";  break;
    }
    size += "Image";

    while( !it.isNull() )
    {
        if( it.isElement() )
        {
            QDomElement e = it.toElement();
            if( e.tagName() == "ASIN" )
            {
                m_asin = e.text();
                m_coverAsins += m_asin;
            }
            else if( e.tagName() == "DetailPageURL" )
            {
                m_amazonURL = e.text();
                m_coverAmazonUrls += m_amazonURL;
            }
            else if( e.tagName() == size )
            {
                QDomNode subIt = e.firstChild();
                while( !subIt.isNull() )
                {
                    if( subIt.isElement() )
                    {
                        QDomElement subE = subIt.toElement();
                        if( subE.tagName() == "URL" )
                        {
                            const QString coverUrl = subE.text();
                            m_coverUrls += coverUrl;
                            break;
                        }
                    }
                    subIt = subIt.nextSibling();
                }
            }
            else if( e.tagName() == "ItemAttributes" )
            {
                QDomNodeList nodes = e.childNodes();
                QDomNode iter;
                QString artist;
                QString album;
                for( int i = 0; i < nodes.count(); i++ )
                {
                    iter = nodes.item( i );

                    if( iter.isElement() )
                    {
                        if( iter.nodeName() == "Artist" )
                        {
                            artist = iter.toElement().text();
                        }
                        else if( iter.nodeName() == "Title" )
                        {
                            album = iter.toElement().text();
                        }
                    }
                }
                m_coverNames += QString( artist + " - " + album );
            }
        }
        it = it.nextSibling();
    }
}

void
CoverFetcher::finishedImageFetch( KJob *job ) //SLOT
{
    DEBUG_BLOCK
    if( job->error() )
    {
        debug() << "finishedImageFetch(): KIO::error(): " << job->error();
        m_errors += i18n("The cover could not be retrieved.");
        attemptAnotherFetch();
        return;
    }

    if( !m_image.loadFromData( static_cast<KIO::StoredTransferJob*>( job )->data() ) || m_image.width() <= 1 )
    {
        //Amazon seems to offer images of size 1x1 sometimes
        //Amazon has nothing to offer us for the requested image size
        m_errors += i18n("The cover-data produced an invalid image.");
        attemptAnotherFetch();
    }

    else if( m_userCanEditQuery )
    {
        //yay! image found :)
        //lets see if the user wants it
        showCover();
    }
    else
        //image loaded successfully yay!
        finish();
}


void
CoverFetcher::attemptAnotherFetch()
{
    DEBUG_BLOCK

    if( !m_coverUrls.isEmpty() )
    {
        // Amazon suggested some more cover URLs to try before we
        // try a different query
        KJob* job = KIO::storedGet( KUrl(m_coverUrls.front()), KIO::NoReload, KIO::HideProgressInfo );
        connect( job, SIGNAL(result( KJob* )), SLOT(finishedImageFetch( KJob* )) );

        The::statusBar()->newProgressOperation( job ).setDescription( i18n( "Fetching Cover" ) );

        m_coverUrls.pop_front();

        m_currentCoverName = m_coverNames.front();
        m_coverNames.pop_front();

        m_amazonURL = m_coverAmazonUrls.front();
        m_coverAmazonUrls.pop_front();

        m_asin = m_coverAsins.front();
        m_coverAsins.pop_front();
    }

    else if( !m_xml.isEmpty() && m_size > 0 )
    {
        // we need to try smaller sizes, this often is
        // fruitless, but does work out sometimes.
        m_size--;

        finishedXmlFetch( 0 );
    }

    else if( !m_queries.isEmpty() )
    {
        // we have some queries left in the pot
        startFetch( m_albumPtr );
    }
    else if( m_userCanEditQuery )
    {
        // we have exhausted all the predetermined queries
        // so lets let the user give it a try
        getUserQuery( i18n("You have seen all the covers Amazon returned using the query below. Perhaps you can refine it:") );
        m_coverAmazonUrls.clear();
        m_coverAsins.clear();
        m_coverUrls.clear();
        m_coverNames.clear();
    }
    else
    {
        m_isFetching = false;
        finishWithError( i18n("No cover found") );
    }
}


// Moved outside the only function that uses it because
// gcc 2.95 doesn't like class declarations there.
    class EditSearchDialog : public QDialog, public Ui::EditCoverSearchDialog
    {
    public:
        EditSearchDialog( QWidget* parent, const QString &text, const QString &keyword, CoverFetcher *fetcher )
                : QDialog( parent )
        {
            setupUi( this );
            setWindowTitle( i18n( "Amazon Query Editor" ) );
            textLabel->setText( text );
            SearchLineEdit->setText( keyword );

            if( CoverManager::instance() )
                connect( amazonLocale, SIGNAL( activated(int) ),
                        CoverManager::instance(), SLOT( changeLocale(int) ) );
            else
                connect( amazonLocale, SIGNAL( activated(int) ),
                        fetcher, SLOT( changeLocale(int) ) );

            int currentLocale = CoverFetcher::localeStringToID( AmarokConfig::amazonLocale() );
            amazonLocale->setCurrentIndex( currentLocale );

            adjustSize();
            setFixedHeight( height() );
        }

        QString query() { return SearchLineEdit->text(); }
    };

QString
CoverFetcher::localeIDToString( int id )//static
{
    switch ( id )
    {
    case International:
        return "us";
    case Canada:
        return "ca";
    case France:
        return "fr";
    case Germany:
        return "de";
    case Japan:
        return "jp";
    case UK:
        return "uk";
    }

    return "us";
}

int
CoverFetcher::localeStringToID( const QString &s )
{
    int id = International;
    if( s == "fr" ) id = France;
    else if( s == "de" ) id = Germany;
    else if( s == "jp" ) id = Japan;
    else if( s == "uk" ) id = UK;
    else if( s == "ca" ) id = Canada;

    return id;
}

void
CoverFetcher::changeLocale( int id )//SLOT
{
    QString locale = localeIDToString( id );
    AmarokConfig::setAmazonLocale( locale );
}

void
CoverFetcher::getUserQuery( QString explanation )
{
    DEBUG_BLOCK
    if( explanation.isEmpty() )
        explanation = i18n("Ask Amazon for covers using this query:");

    EditSearchDialog dialog(
            static_cast<QWidget*>( parent() ),
            explanation,
            m_userQuery,
            this );


    switch( dialog.exec() )
    {
        case QDialog::Accepted:
            m_userQuery = dialog.query();
            debug() << m_userQuery;
            m_queries.clear();
            m_queries << m_userQuery;
            startFetch( m_albumPtr );
            break;
        case QDialog::Rejected:
            break;
        default:
            finishWithError( i18n( "Aborted." ) );
            break;
    }
}

    class CoverFoundDialog : public KDialog
    {
    public:
        CoverFoundDialog( QWidget *parent, const QImage &cover, const QString &productname )
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
            KPushButton *newsearch = new KPushButton( i18n( "Ne&w Search..." ), buttons );
            newsearch->setObjectName( "NewSearch" );
            KPushButton *nextcover = new KPushButton( i18n( "&Next Cover" ), buttons );
            nextcover->setObjectName( "NextCover" );
            KPushButton *cancel    = new KPushButton( KStandardGuiItem::cancel(), buttons );

            labelPix ->setAlignment( Qt::AlignHCenter );
            labelName->setAlignment( Qt::AlignHCenter );
            labelPix ->setPixmap( QPixmap::fromImage( cover ) );
            labelName->setText( productname );

            save->setDefault( true );
            this->setFixedSize( sizeHint() );
            this->setCaption( i18n("Cover Found") );

            connect( save,      SIGNAL(clicked()), SLOT(accept()) );
            connect( newsearch, SIGNAL(clicked()), SLOT(accept()) );
            connect( nextcover, SIGNAL(clicked()), SLOT(accept()) );
            connect( cancel,    SIGNAL(clicked()), SLOT(reject()) );
        }

        virtual void accept()
        {
            DEBUG_BLOCK
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
    DEBUG_BLOCK
    CoverFoundDialog dialog( static_cast<QWidget*>( parent() ), m_image, m_currentCoverName );

    switch( dialog.exec() )
    {
    case KDialog::Accepted:
        finish();
        break;
    case KDialog::Rejected: //make sure we do not show any more dialogs
        debug() << "cover rejected";
        break;
    case 1000: //showQueryEditor()
        getUserQuery();
        m_coverAmazonUrls.clear();
        m_coverAsins.clear();
        m_coverUrls.clear();
        m_coverNames.clear();
        break;
    case 1001: //nextCover()
        attemptAnotherFetch();
        break;
    default:
        finishWithError( i18n( "Aborted." ) );
        break;
    }
}


void
CoverFetcher::finish()
{
    DEBUG_BLOCK
    The::statusBar()->shortMessage( i18n( "Retrieved cover successfully" ) );
    m_albumPtr->setImage( image() );
    m_isFetching = false;
    if( !m_userCanEditQuery /*manual fetch*/ && !m_albums.isEmpty() )
        buildQueries( m_albums.takeFirst() );
}

void
CoverFetcher::finishWithError( const QString &message, KJob *job )
{
    DEBUG_BLOCK
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
        buildQueries( m_albums.takeFirst() );
    }
}

#include "CoverFetcher.moc"

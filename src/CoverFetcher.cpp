// (C) 2004 Mark Kretschmann <markey@web.de>
// (C) 2004 Stefan Bogner <bochi@online.ms>
// (C) 2004 Max Howell
// See COPYING file for licensing information.

#include "CoverFetcher.h"

#include "amarok.h"
#include "amarokconfig.h"
#include "CoverManager.h"
#include "debug.h"
#include "AmarokStatusBar.h"

#include <KApplication>
#include <KComboBox>
#include <KCursor> //waiting cursor
#include <KDialog>
#include <KHBox>
#include <KIconLoader>
#include <KFileDialog>
#include <KIO/Job>
#include <KLineEdit>
#include <KLocale>
#include <KMessageBox>
#include <KMenu>
#include <KPushButton>
#include <KUrl>
#include <KVBox>
#include <KWindowSystem>

#include <QDomDocument>
#include <QDomElement>
#include <QDomNode>
#include <QLabel>
#include <QLayout>
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

K_GLOBAL_STATIC( CoverFetcherSingleton, privateInstance )

CoverFetcher*
CoverFetcher::instance()
{
    return &privateInstance->instance;
}

CoverFetcher::CoverFetcher()
        : QObject()
        , m_size( 2 )
        , m_success( true )
        , m_isFetching( false )
{
    DEBUG_FUNC_INFO
    setObjectName( "CoverFetcher" );
}

CoverFetcher::~CoverFetcher()
{
    DEBUG_FUNC_INFO
}

void
CoverFetcher::manualFetch( Meta::AlbumPtr album )
{
    m_userCanEditQuery = true;
    buildQueries( album );
}

void
CoverFetcher::queueAlbum( Meta::AlbumPtr album )
{
    m_albumsMutex.lock();
    m_albums << album;
    m_albumsMutex.unlock();
    m_fetchMutex.lock();
    if( !m_isFetching )
    {
        m_fetchMutex.unlock();
        startFetchLoop();
    }
    else
    {
        m_fetchMutex.unlock();
    }
}
void
CoverFetcher::queueAlbums( Meta::AlbumList albums )
{
    m_albumsMutex.lock();
    m_albums << albums;
    m_albumsMutex.unlock();
    m_fetchMutex.lock();
    if( !m_isFetching )
    {
        m_fetchMutex.unlock();
        startFetchLoop();
    }
    else
    {
        m_fetchMutex.unlock();
    }
}
void
CoverFetcher::startFetchLoop()
{
    m_userCanEditQuery = false;
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

//////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE SLOTS
//////////////////////////////////////////////////////////////////////////////////////////


void CoverFetcher::buildQueries( Meta::AlbumPtr album )
{
    m_fetchMutex.lock();
    m_isFetching = true;
    m_fetchMutex.unlock();
    m_albumPtr = album;
    QString albumName = album->name();
    QString artistName = album->hasAlbumArtist() ? album->albumArtist()->name() : QString();

    QStringList extensions;
    extensions << i18n("disc") << i18n("disk") << i18n("remaster") << i18n("cd") << i18n("single") << i18n("soundtrack") << i18n("part")
            << "disc" << "disk" << "remaster" << "cd" << "single" << "soundtrack" << "part" << "cds" /*cd single*/;


    m_queries.clear();
    m_userQueries.clear();

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

    m_queries += artistName + " - " + albumName;
    m_queries += m_userQuery;
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
    m_fetchMutex.lock();
    m_isFetching = true;
    m_fetchMutex.unlock();
    m_albumPtr = album;

    // Static license Key. Thanks muesli ;-)
    const QString LICENSE( "D1URM11J3F2CEH" );

    // reset all values
    m_coverAmazonUrls.clear();
    m_coverAsins.clear();
    m_coverUrls.clear();
    m_coverNames.clear();
    m_xml.clear();
    m_size = 2;

    if ( m_queries.isEmpty() ) {
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
    QString musicMode = "music";
    //Amazon Japan isn't on xml.amazon.com
    QString tld = "com";
    int mibenum = 4;  // latin1
    if( AmarokConfig::amazonLocale() == "jp" ) {
        musicMode = "music-jp";
        tld = "co.jp";
        mibenum = 106;  // utf-8
    }
    else if( AmarokConfig::amazonLocale() == "ca" )
        musicMode = "music-ca";
    else if( AmarokConfig::amazonLocale() == "fr" )
        musicMode = "music-fr";

    QString url;
    // changed to type=lite because it makes less traffic
    url = "http://xml.amazon." + tld
            + "/onca/xml3?t=webservices-20&dev-t=" + LICENSE
            + "&KeywordSearch=" + KUrl::toPercentEncoding( query, "/" ) // FIXME: we will have to find something else
            + "&mode=" + musicMode
            + "&type=lite&locale=" + AmarokConfig::amazonLocale()
            + "&page=1&f=xml";
    debug() << url;

    KJob* job = KIO::storedGet( url, KIO::NoReload, KIO::HideProgressInfo );
    connect( job, SIGNAL(result( KJob* )), SLOT(finishedXmlFetch( KJob* )) );

    The::amarokStatusBar()->setProgressText( i18n( "Fetching Cover" ) );
    The::amarokStatusBar()->setProgress( 0 );
}

void
CoverFetcher::finishedXmlFetch( KJob *job ) //SLOT
{
    DEBUG_BLOCK

    // NOTE: job can become 0 when this method is called from attemptAnotherFetch()

    if( job && job->error() ) {
        finishWithError( i18n("There was an error communicating with Amazon."), job );
        return;
    }
    if ( job ) {
        KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( job );
        m_xml = QString::fromUtf8( storedJob->data().data(), storedJob->data().size() );
    }

    QDomDocument doc;
    if( !doc.setContent( m_xml ) ) {
        m_errors += i18n("The XML obtained from Amazon is invalid.");
        startFetchLoop();
        return;
    }

    const QDomNode details = doc.documentElement().namedItem( "Details" );

    // the url for the Amazon product info page
    m_amazonURL = details.attributes().namedItem( "url" ).toAttr().value();
    QDomNode it = details.firstChild();
    while ( !it.isNull() ) {
        if ( it.isElement() ) {
            QDomElement e = it.toElement();
            if(e.tagName()=="Asin")
            {
                m_asin = e.firstChild().toText().data();
                debug() << "setting the ASIN as" << m_asin;
                break;
            }
        }
        it = it.nextSibling();
    }

    QString size = "ImageUrl";
    switch( m_size ) {
        case 0:  size += "Small";  break;
        case 1:  size += "Medium"; break;
        default: size += "Large";  break;
    }

    debug() << "Fetching size: " << size;

    m_coverAsins.clear();
    m_coverAmazonUrls.clear();
    m_coverUrls.clear();
    m_coverNames.clear();
    for( QDomNode node = details; !node.isNull(); node = node.nextSibling() ) {
        QString amazonUrl = node.attributes().namedItem( "url" ).toAttr().value();
        QString coverUrl = node.namedItem( size ).firstChild().toText().nodeValue();
        QString asin = node.namedItem( "Asin" ).firstChild().toText().nodeValue();
        QString name = node.namedItem( "ProductName" ).firstChild().toText().nodeValue();

        const QDomNode  artists = node.namedItem("Artists");
        // in most cases, Amazon only sends one Artist in Artists
        QString artist = "";
        if (!artists.isNull())
            artist = artists.namedItem( "Artist" ).firstChild().toText().nodeValue();

        debug() << "name:" << name << " artist:" << artist << " url:" << coverUrl;

        if( !coverUrl.isEmpty() )
        {
            m_coverAmazonUrls += amazonUrl;
            m_coverAsins += asin;
            m_coverUrls += coverUrl;
            m_coverNames += artist + " - " + name;
        }
    }

    attemptAnotherFetch();
}


void
CoverFetcher::finishedImageFetch( KJob *job ) //SLOT
{
    if( job->error() ) {
        debug() << "finishedImageFetch(): KIO::error(): " << job->error();

        m_errors += i18n("The cover could not be retrieved.");

        attemptAnotherFetch();
        return;
    }

    m_image.loadFromData( static_cast<KIO::StoredTransferJob*>( job )->data() );

    if( m_image.width() <= 1 ) {
        //Amazon seems to offer images of size 1x1 sometimes
        //Amazon has nothing to offer us for the requested image size
        m_errors += i18n("The cover-data produced an invalid image.");
        attemptAnotherFetch();
    }

    else if( m_userCanEditQuery )
        //yay! image found :)
        //lets see if the user wants it
        showCover();

    else
        //image loaded successfully yay!
        finish();
}


void
CoverFetcher::attemptAnotherFetch()
{
    DEBUG_BLOCK

    if( !m_coverUrls.isEmpty() ) {
        // Amazon suggested some more cover URLs to try before we
        // try a different query
        KJob* job = KIO::storedGet( KUrl(m_coverUrls.front()), KIO::NoReload, KIO::HideProgressInfo );
        connect( job, SIGNAL(result( KJob* )), SLOT(finishedImageFetch( KJob* )) );

        The::amarokStatusBar()->setProgressText( i18n( "Fetching Cover" ) );
        The::amarokStatusBar()->setProgress( 0 );

        m_coverUrls.pop_front();

        m_currentCoverName = m_coverNames.front();
        m_coverNames.pop_front();

        m_amazonURL = m_coverAmazonUrls.front();
        m_coverAmazonUrls.pop_front();

        m_asin = m_coverAsins.front();
        m_coverAsins.pop_front();
    }

    else if( !m_xml.isEmpty() && m_size > 0 ) {
        // we need to try smaller sizes, this often is
        // fruitless, but does work out sometimes.
        m_size--;

        finishedXmlFetch( 0 );
    }

    else if( !m_queries.isEmpty() ) {
        // we have some queries left in the pot
        startFetch( m_albumPtr );
    }

    else if( m_userCanEditQuery ) {
        // we have exhausted all the predetermined queries
        // so lets let the user give it a try
        getUserQuery( i18n("You have seen all the covers Amazon returned using the query below. Perhaps you can refine it:") );
        m_coverAmazonUrls.clear();
        m_coverAsins.clear();
        m_coverUrls.clear();
        m_coverNames.clear();
    }
    else
        finishWithError( i18n("No cover found") );
}


// Moved outside the only function that uses it because
// gcc 2.95 doesn't like class declarations there.
    class EditSearchDialog : public KDialog
    {
    public:
        EditSearchDialog( QWidget* parent, const QString &text, const QString &keyword, CoverFetcher *fetcher )
                : KDialog( parent )
        {
            setCaption( i18n( "Amazon Query Editor" ) );

            // amazon combo box
            KComboBox* amazonLocale = new KComboBox( this );
            amazonLocale->addItem( i18n("International"), CoverFetcher::International );
            amazonLocale->addItem( i18n("Canada"), CoverFetcher::Canada );
            amazonLocale->addItem( i18n("France"), CoverFetcher::France );
            amazonLocale->addItem( i18n("Germany"), CoverFetcher::Germany );
            amazonLocale->addItem( i18n("Japan"), CoverFetcher::Japan);
            amazonLocale->addItem( i18n("United Kingdom"), CoverFetcher::UK );
            if( CoverManager::instance() )
                connect( amazonLocale, SIGNAL( activated(int) ),
                        CoverManager::instance(), SLOT( changeLocale(int) ) );
            else
                connect( amazonLocale, SIGNAL( activated(int) ),
                        fetcher, SLOT( changeLocale(int) ) );
            QHBoxLayout *hbox1 = new QHBoxLayout();
            hbox1->setSpacing( 8 );
            hbox1->addWidget( new QLabel( i18n( "Amazon Locale: " ), this ) );
            hbox1->addWidget( amazonLocale );

            int currentLocale = CoverFetcher::localeStringToID( AmarokConfig::amazonLocale() );
            amazonLocale->setCurrentIndex( currentLocale );

            KPushButton* cancelButton = new KPushButton( KStandardGuiItem::cancel(), this );
            KPushButton* searchButton = new KPushButton( i18n("&Search"), this );

            QHBoxLayout *hbox2 = new QHBoxLayout();
            hbox2->setSpacing( 8 );
            hbox2->addItem( new QSpacerItem( 160, 8, QSizePolicy::Expanding, QSizePolicy::Minimum ) );
            hbox2->addWidget( searchButton );
            hbox2->addWidget( cancelButton );

            QVBoxLayout *vbox = new QVBoxLayout();
            vbox->setMargin( 8 );
            vbox->setSpacing( 8 );
            vbox->addLayout( hbox1 );
            vbox->addWidget( new QLabel( "<qt>" + text, this ) );
            vbox->addWidget( new KLineEdit( keyword, this ) );
            vbox->addLayout( hbox2 );

            searchButton->setDefault( true );

            adjustSize();
            setFixedHeight( height() );

            connect( searchButton, SIGNAL(clicked()), SLOT(accept()) );
            connect( cancelButton, SIGNAL(clicked()), SLOT(reject()) );
        }

        QString query() { return findChild<KLineEdit*>( "Query" )->text(); }
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
        m_queries.clear();
        m_queries << m_userQuery;
        startFetchLoop();
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
            // Gives the window a small title bar, and skips a taskbar entry
            //KWindowSystem::setType( winId(), NET::Utility );
            //KWindowSystem::setState( winId(), NET::SkipTaskbar );
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
    CoverFoundDialog dialog( static_cast<QWidget*>( parent() ), m_image, m_currentCoverName );

    switch( dialog.exec() )
    {
    case KDialog::Accepted:
        finish();
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
    The::amarokStatusBar()->setMessage( "Retrieved cover successfully", KDE::StatusBar::None );
    m_albumPtr->setImage( image() );
    m_isFetching = false;
    if( !m_albums.isEmpty() )
        buildQueries( m_albums.takeFirst() );
}

void
CoverFetcher::finishWithError( const QString &message, KJob *job )
{
    if( job )
        warning() << message << " KIO::error(): " << job->errorText();

    m_errors += message;
    m_success = false;
}

#include "CoverFetcher.moc"

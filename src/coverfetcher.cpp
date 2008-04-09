// (C) 2004 Mark Kretschmann <markey@web.de>
// (C) 2004 Stefan Bogner <bochi@online.ms>
// (C) 2004 Max Howell
// See COPYING file for licensing information.

#include "amarok.h"
#include "amarokconfig.h"
#include "collectiondb.h"
#include "config.h"        //for AMAZON_SUPPORT
#include "covermanager.h"
#include "coverfetcher.h"
#include "debug.h"
#include "statusbar.h"

#include <qdom.h>
#include <qhbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qregexp.h>

#include <kapplication.h>
#include <kcombobox.h>
#include <kcursor.h> //waiting cursor
#include <kdialog.h>
#include <kiconloader.h>
#include <kfiledialog.h>
#include <kio/job.h>
#include <kio/jobclasses.h>
#include <klineedit.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpopupmenu.h>
#include <kpushbutton.h>
#include <kwin.h>


void
Amarok::coverContextMenu( QWidget *parent, QPoint point, const QString &artist, const QString &album, bool showCoverManager )
{
        KPopupMenu menu;
        enum { SHOW, FETCH, CUSTOM, DELETE, MANAGER };

        menu.insertTitle( i18n( "Cover Image" ) );

        menu.insertItem( SmallIconSet( Amarok::icon( "zoom" ) ), i18n( "&Show Fullsize" ), SHOW );
        menu.insertItem( SmallIconSet( Amarok::icon( "download" ) ), i18n( "&Fetch From amazon.%1" ).arg( CoverManager::amazonTld() ), FETCH );
        menu.insertItem( SmallIconSet( Amarok::icon( "files" ) ), i18n( "Set &Custom Cover" ), CUSTOM );
        bool disable = !album.isEmpty(); // disable setting covers for unknown albums
        menu.setItemEnabled( FETCH, disable );
        menu.setItemEnabled( CUSTOM, disable );
        menu.insertSeparator();

        menu.insertItem( SmallIconSet( Amarok::icon( "remove" ) ), i18n( "&Unset Cover" ), DELETE );
        if ( showCoverManager ) {
            menu.insertSeparator();
            menu.insertItem( SmallIconSet( Amarok::icon( "covermanager" ) ), i18n( "Cover &Manager" ), MANAGER );
        }
        #ifndef AMAZON_SUPPORT
        menu.setItemEnabled( FETCH, false );
        #endif
        disable = !CollectionDB::instance()->albumImage( artist, album, 0 ).contains( "nocover" );
        menu.setItemEnabled( SHOW, disable );
        menu.setItemEnabled( DELETE, disable );

        switch( menu.exec( point ) )
        {
        case SHOW:
            CoverManager::viewCover( artist, album, parent );
            break;

        case DELETE:
        {
            const int button = KMessageBox::warningContinueCancel( parent,
                i18n( "Are you sure you want to remove this cover from the Collection?" ),
                QString::null,
                KStdGuiItem::del() );

            if ( button == KMessageBox::Continue )
                CollectionDB::instance()->removeAlbumImage( artist, album );
            break;
        }

        case FETCH:
        #ifdef AMAZON_SUPPORT
            CollectionDB::instance()->fetchCover( parent, artist, album, false );
            break;
        #endif

        case CUSTOM:
        {
            QString artist_id; artist_id.setNum( CollectionDB::instance()->artistID( artist ) );
            QString album_id; album_id.setNum( CollectionDB::instance()->albumID( album ) );
            QStringList values = CollectionDB::instance()->albumTracks( artist_id, album_id );
            QString startPath = ":homedir";

            if ( !values.isEmpty() ) {
                KURL url;
                url.setPath( values.first() );
                startPath = url.directory();
            }

            KURL file = KFileDialog::getImageOpenURL( startPath, parent, i18n("Select Cover Image File") );
            if ( !file.isEmpty() )
                CollectionDB::instance()->setAlbumImage( artist, album, file );
            break;
        }

        case MANAGER:
            CoverManager::showOnce( album );
            break;
        }
}



CoverLabel::CoverLabel ( QWidget * parent, const char * name, WFlags f )
        : QLabel( parent, name, f)
{}


void CoverLabel::mouseReleaseEvent(QMouseEvent *pEvent) {
    if (pEvent->button() == LeftButton || pEvent->button() == RightButton)
    {
        Amarok::coverContextMenu( this, pEvent->globalPos(), m_artist, m_album, false );
    }
}


CoverFetcher::CoverFetcher( QWidget *parent, const QString &artist, QString album )
        : QObject( parent, "CoverFetcher" )
        , m_artist( artist )
        , m_album( album )
        , m_size( 2 )
        , m_success( true )
{
    DEBUG_FUNC_INFO

    QStringList extensions;
    extensions << i18n("disc") << i18n("disk") << i18n("remaster") << i18n("cd") << i18n("single") << i18n("soundtrack") << i18n("part")
               << "disc" << "disk" << "remaster" << "cd" << "single" << "soundtrack" << "part" << "cds" /*cd single*/;

    //we do several queries, one raw ie, without the following modifications
    //the others have the above strings removed with the following regex, as this can increase hit-rate
    const QString template1 = " ?-? ?[(^{]* ?%1 ?\\d*[)^}\\]]* *$"; //eg album - [disk 1] -> album
    foreach( extensions ) {
        QRegExp regexp( template1.arg( *it ) );
        regexp.setCaseSensitive( false );
        album.remove( regexp );
    }

    //TODO try queries that remove anything in album after a " - " eg Les Mis. - Excerpts

    /**
     * We search for artist - album, and just album, using the exact album text and the
     * manipulated album text.
     */

    //search on our modified term, then the original
    if ( !m_artist.isEmpty() )
        m_userQuery = m_artist + " - ";
    m_userQuery += m_album;

    m_queries += m_artist + " - " + album;
    m_queries += m_userQuery;
    m_queries += album;
    m_queries += m_album;

    //don't do the same searches twice in a row
    if( m_album == album )  {
        m_queries.pop_front();
        m_queries.pop_back();
    }

    /**
     * Finally we do a search for just the artist, just in case as this often
     * turns up a cover, and it might just be the right one! Also it would be
     * the only valid search if m_album.isEmpty()
     */
    m_queries += m_artist;

    QApplication::setOverrideCursor( KCursor::workingCursor() );
}

CoverFetcher::~CoverFetcher()
{
    DEBUG_FUNC_INFO

    QApplication::restoreOverrideCursor();
}

void
CoverFetcher::startFetch()
{
    DEBUG_FUNC_INFO

    // Static license Key. Thanks hydrogen ;-)
    const QString LICENSE( "11ZKJS8X1ETSTJ6MT802" );

    // reset all values
    m_coverAmazonUrls.clear();
    m_coverAsins.clear();
    m_coverUrls.clear();
    m_coverNames.clear();
    m_xml = QString::null;
    m_size = 2;

    if ( m_queries.isEmpty() ) {
        debug() << "m_queries is empty" << endl;
        finishWithError( i18n("No cover found") );
        return;
    }
    QString query = m_queries.front();
    m_queries.pop_front();

    // '&' breaks searching
    query.remove('&');
    
    QString locale = AmarokConfig::amazonLocale();
    QString tld;

    if( locale == "us" )
        tld = "com";
    else if( locale =="uk" )
        tld = "co.uk";
    else
        tld = locale;

    int mibenum = 106; // utf-8

    QString url;
    url = "http://ecs.amazonaws." + tld
        + "/onca/xml?Service=AWSECommerceService&Version=2007-10-29&Operation=ItemSearch&AssociateTag=webservices-20&AWSAccessKeyId=" + LICENSE
        + "&Keywords=" + KURL::encode_string_no_slash( query, mibenum )
        + "&SearchIndex=Music&ResponseGroup=Small,Images";
    debug() << url << endl;

    KIO::TransferJob* job = KIO::storedGet( url, false, false );
    connect( job, SIGNAL(result( KIO::Job* )), SLOT(finishedXmlFetch( KIO::Job* )) );

    Amarok::StatusBar::instance()->newProgressOperation( job );
}


//////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE SLOTS
//////////////////////////////////////////////////////////////////////////////////////////

void
CoverFetcher::finishedXmlFetch( KIO::Job *job ) //SLOT
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
        startFetch();
        return;
    }

    m_coverAsins.clear();
    m_coverAmazonUrls.clear();
    m_coverUrls.clear();
    m_coverNames.clear();

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
    QDomNode it = node.firstChild();

    QString size;
    switch( m_size )
    {
        case 0:  size = "Small";  break;
        case 1:  size = "Medium"; break;
        default: size = "Large";  break;
    }
    size += "Image";

    while ( !it.isNull() ) {
        if ( it.isElement() ) {
            QDomElement e = it.toElement();
            if(e.tagName()=="ASIN")
            {
                m_asin = e.text();
                m_coverAsins += m_asin;
            }
            else if(e.tagName() == "DetailPageURL" )
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
CoverFetcher::finishedImageFetch( KIO::Job *job ) //SLOT
{
    if( job->error() ) {
        debug() << "finishedImageFetch(): KIO::error(): " << job->error() << endl;

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

        KIO::TransferJob* job = KIO::storedGet( KURL(m_coverUrls.front()), false, false );
        connect( job, SIGNAL(result( KIO::Job* )), SLOT(finishedImageFetch( KIO::Job* )) );

        Amarok::StatusBar::instance()->newProgressOperation( job );

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
        startFetch();
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
            amazonLocale->insertItem( i18n("International"), CoverFetcher::International );
            amazonLocale->insertItem( i18n("Canada"), CoverFetcher::Canada );
            amazonLocale->insertItem( i18n("France"), CoverFetcher::France );
            amazonLocale->insertItem( i18n("Germany"), CoverFetcher::Germany );
            amazonLocale->insertItem( i18n("Japan"), CoverFetcher::Japan);
            amazonLocale->insertItem( i18n("United Kingdom"), CoverFetcher::UK );
            if( CoverManager::instance() )
                connect( amazonLocale, SIGNAL( activated(int) ),
                        CoverManager::instance(), SLOT( changeLocale(int) ) );
            else
                connect( amazonLocale, SIGNAL( activated(int) ),
                        fetcher, SLOT( changeLocale(int) ) );
            QHBoxLayout *hbox1 = new QHBoxLayout( 8 );
            hbox1->addWidget( new QLabel( i18n( "Amazon Locale: " ), this ) );
            hbox1->addWidget( amazonLocale );

            int currentLocale = CoverFetcher::localeStringToID( AmarokConfig::amazonLocale() );
            amazonLocale->setCurrentItem( currentLocale );

            KPushButton* cancelButton = new KPushButton( KStdGuiItem::cancel(), this );
            KPushButton* searchButton = new KPushButton( i18n("&Search"), this );

            QHBoxLayout *hbox2 = new QHBoxLayout( 8 );
            hbox2->addItem( new QSpacerItem( 160, 8, QSizePolicy::Expanding, QSizePolicy::Minimum ) );
            hbox2->addWidget( searchButton );
            hbox2->addWidget( cancelButton );

            QVBoxLayout *vbox = new QVBoxLayout( this, 8, 8 );
            vbox->addLayout( hbox1 );
            vbox->addWidget( new QLabel( "<qt>" + text, this ) );
            vbox->addWidget( new KLineEdit( keyword, this, "Query" ) );
            vbox->addLayout( hbox2 );

            searchButton->setDefault( true );

            adjustSize();
            setFixedHeight( height() );

            connect( searchButton, SIGNAL(clicked()), SLOT(accept()) );
            connect( cancelButton, SIGNAL(clicked()), SLOT(reject()) );
        }

        QString query() { return static_cast<KLineEdit*>(child( "Query" ))->text(); }
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
        m_queries = m_userQuery;
        startFetch();
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
            // Gives the window a small title bar, and skips a taskbar entry
            KWin::setType( winId(), NET::Utility );
            KWin::setState( winId(), NET::SkipTaskbar );

            (new QVBoxLayout( this ))->setAutoAdd( true );

            QLabel      *labelPix  = new QLabel( this );
            QLabel      *labelName = new QLabel( this );
            QHBox       *buttons   = new QHBox( this );
            KPushButton *save      = new KPushButton( KStdGuiItem::save(), buttons );
            KPushButton *newsearch = new KPushButton( i18n( "Ne&w Search..." ), buttons, "NewSearch" );
            KPushButton *nextcover = new KPushButton( i18n( "&Next Cover" ), buttons, "NextCover" );
            KPushButton *cancel    = new KPushButton( KStdGuiItem::cancel(), buttons );

            labelPix ->setAlignment( Qt::AlignHCenter );
            labelName->setAlignment( Qt::AlignHCenter );
            labelPix ->setPixmap( cover );
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
            if( qstrcmp( sender()->name(), "NewSearch" ) == 0 )
                done( 1000 );
            else if( qstrcmp( sender()->name(), "NextCover" ) == 0 )
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
    emit result( this );

    deleteLater();
}

void
CoverFetcher::finishWithError( const QString &message, KIO::Job *job )
{
    if( job )
        warning() << message << " KIO::error(): " << job->errorText() << endl;

    m_errors += message;
    m_success = false;

    emit result( this );

    deleteLater();
}

#include "coverfetcher.moc"

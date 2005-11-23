// (C) 2004 Mark Kretschmann <markey@web.de>
// (C) 2004 Stefan Bogner <bochi@online.ms>
// (C) 2004 Max Howell
// See COPYING file for licensing information.

#include "amarok.h"
#include "amarokconfig.h"
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
#include <kio/job.h>
#include <kio/jobclasses.h>
#include <klineedit.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpushbutton.h>
#include <kwin.h>


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

    // Static license Key. Thanks muesli ;-)
    const QString LICENSE( "D1URM11J3F2CEH" );

    // reset all values
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

    // Bug 97901: Import cover from amazon france doesn't work properly
    // (we have to set "mode=music-fr" instead of "mode=music")
    QString musicMode = AmarokConfig::amazonLocale() == "fr" ? "music-fr" : "music";
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

    QString url;
    // changed to type=lite because it makes less traffic
    url = "http://xml.amazon." + tld
        + "/onca/xml3?t=webservices-20&dev-t=" + LICENSE
        + "&KeywordSearch=" + KURL::encode_string_no_slash( query, mibenum )
        + "&mode=" + musicMode
        + "&type=lite&locale=" + AmarokConfig::amazonLocale()
        + "&page=1&f=xml";
    debug() << url << endl;

    KIO::TransferJob* job = KIO::storedGet( url, false, false );
    connect( job, SIGNAL(result( KIO::Job* )), SLOT(finishedXmlFetch( KIO::Job* )) );

    amaroK::StatusBar::instance()->newProgressOperation( job );
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
                debug() << "setting the ASIN as" << m_asin << endl;
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

    debug() << "Fetching size: " << size << endl;

    m_coverUrls.clear();
    m_coverNames.clear();
    for( QDomNode node = details; !node.isNull(); node = node.nextSibling() ) {
        QString url = node.namedItem( size ).firstChild().toText().nodeValue();
        QString name = node.namedItem( "ProductName" ).firstChild().toText().nodeValue();

    const QDomNode  artists = node.namedItem("Artists");
    // in most cases Amazon only sends one Artit in Artits
    QString artist="";
    if (!artists.isNull()) artist = artists.namedItem( "Artist" ).firstChild().toText().nodeValue();

        debug() << "name:" << name << " artist:" << artist << " url:" << url << endl;

        if( !url.isEmpty() )
        {
            m_coverUrls += url;
            m_coverNames += artist + " - " + name;
        }
    }

    attemptAnotherFetch();
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

        amaroK::StatusBar::instance()->newProgressOperation( job );

        m_coverUrls.pop_front();

        m_currentCoverName = m_coverNames.front();
        m_coverNames.pop_front();
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
    }
    else
        finishWithError( i18n("No cover found") );
}


// Moved outside the only function that uses it because
// gcc 2.95 doesn't like class declarations there.
    class EditSearchDialog : public KDialog
    {
    public:
        EditSearchDialog( QWidget* parent, const QString &text, const QString &keyword )
                : KDialog( parent )
        {
            setCaption( i18n( "Amazon Query Editor" ) );

            QVBoxLayout *vbox = new QVBoxLayout( this, 8, 8 );
            QHBoxLayout *hbox = new QHBoxLayout( 8 );

            KPushButton* cancelButton = new KPushButton( KStdGuiItem::cancel(), this );
            KPushButton* searchButton = new KPushButton( i18n("&Search"), this );

            hbox->addItem( new QSpacerItem( 80, 8, QSizePolicy::Expanding, QSizePolicy::Minimum ) );
            hbox->addWidget( searchButton );
            hbox->addWidget( cancelButton );

            vbox->addWidget( new QLabel( "<qt>" + text, this ) );
            vbox->addWidget( new KLineEdit( keyword, this, "Query" ) );
            vbox->addLayout( hbox );

            searchButton->setDefault( true );

            adjustSize();
            setFixedHeight( height() );

            connect( searchButton, SIGNAL(clicked()), SLOT(accept()) );
            connect( cancelButton, SIGNAL(clicked()), SLOT(reject()) );
        }

        QString query() { return static_cast<KLineEdit*>(child( "Query" ))->text(); }
    };


void
CoverFetcher::getUserQuery( QString explanation )
{
    if( explanation.isEmpty() )
        explanation = i18n("Ask Amazon for covers using this query:");

    EditSearchDialog dialog(
            (QWidget*)parent(),
            explanation,
            m_userQuery );

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
            KPushButton *newsearch = new KPushButton( i18n( "New &Search..." ), buttons, "NewSearch" );
            KPushButton *nextcover = new KPushButton( i18n( "Next &Cover" ), buttons, "NextCover" );
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
    CoverFoundDialog dialog( (QWidget*)parent(), m_image, m_currentCoverName );

    switch( dialog.exec() )
    {
    case KDialog::Accepted:
        finish();
        break;
    case 1000: //showQueryEditor()
        getUserQuery();
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

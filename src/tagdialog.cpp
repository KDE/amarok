// (c) 2004 Mark Kretschmann <markey@web.de>
// (c) 2004 Pierpaolo Di Panfilo <pippo_dp@libero.it>
// (c) 2005 Alexandre Pereira de Oliveira <aleprj@gmail.com>
// See COPYING file for licensing information.

#include "amarok.h"
#include "debug.h"
#include "contextbrowser.h"
#include "collectionbrowser.h"
#include "collectiondb.h"
#include "metabundle.h"
#include "playlist.h"
#include "playlistitem.h"
#include "statusbar.h"       //for status messages
#include "tagdialog.h"
#include "trackpickerdialog.h"

#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/tfile.h>
#include <taglib/tstring.h>
#include <taglib/id3v2framefactory.h>

#include <qfile.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qtooltip.h>

#include <kapplication.h>
#include <kcombobox.h>
#include <kcursor.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <ktabwidget.h>
#include <ktextedit.h>
#include <klineedit.h>
#include <kmessagebox.h>
#include <knuminput.h>
#include <krun.h>
#include <kstandarddirs.h>


TagDialog::TagDialog( const KURL& url, QWidget* parent )
    : TagDialogBase( parent )
    , m_bundle( MetaBundle( url ) )
    , m_score ( CollectionDB::instance()->getSongPercentage( url.path() ) )
    , m_playcount( CollectionDB::instance()->getPlayCount( url.path() ) )
    , m_firstPlay ( CollectionDB::instance()->getFirstPlay( url.path() ) )
    , m_lastPlay ( CollectionDB::instance()->getLastPlay( url.path() ) )
    , m_playlistItem( 0 )
    , m_currentCover( 0 )
{
    init();
}


TagDialog::TagDialog( const KURL::List list, QWidget* parent )
    : TagDialogBase( parent )
    , m_bundle( MetaBundle() )
    , m_score ( 0 )
    , m_firstPlay ( QDateTime() )
    , m_lastPlay ( QDateTime()  )
    , m_playlistItem( 0 )
    , m_urlList( list )
    , m_currentCover( 0 )
{
    setCaption( kapp->makeStdCaption( i18n("1 Track", "Information for %n Tracks", list.count()) ) );
    init();
}


TagDialog::TagDialog( const MetaBundle& mb, PlaylistItem* item, QWidget* parent )
    : TagDialogBase( parent )
    , m_bundle( mb )
    , m_score ( CollectionDB::instance()->getSongPercentage( mb.url().path() ) )
    , m_playcount( CollectionDB::instance()->getPlayCount( mb.url().path() ) )
    , m_firstPlay ( CollectionDB::instance()->getFirstPlay( mb.url().path() ) )
    , m_lastPlay ( CollectionDB::instance()->getLastPlay( mb.url().path() ) )
    , m_playlistItem( item )
    , m_currentCover( 0 )
{
    init();
}


TagDialog::~TagDialog()
{
    KConfig *config = amaroK::config( "TagDialog" );
    config->writeEntry( "CurrentTab", kTabWidget->currentPageIndex() );
}


////////////////////////////////////////////////////////////////////////////////
// PRIVATE SLOTS
////////////////////////////////////////////////////////////////////////////////

void
TagDialog::cancelPressed() //SLOT
{
    QApplication::restoreOverrideCursor(); // restore the cursor before closing the dialog
    reject();
}


void
TagDialog::accept() //SLOT
{
    pushButton_ok->setEnabled( false ); //visual feedback
    saveTags();

    QDialog::accept();
}


inline void
TagDialog::openPressed() //SLOT
{
    // run konqueror with the track's directory
    const QString cmd = "kfmclient openURL \"%1\"";
    KRun::runCommand( cmd.arg( m_path ), "kfmclient", "konqueror" );
}


inline void
TagDialog::previousTrack()
{
    if( !m_playlistItem->itemAbove() ) return;

    storeTags();

    m_playlistItem = (PlaylistItem *)m_playlistItem->itemAbove();

    QMap<QString, MetaBundle>::ConstIterator itTags;
    itTags = storedTags.find( m_playlistItem->url().path() );
    m_bundle = itTags != storedTags.end() ? itTags.data() : MetaBundle( m_playlistItem );

    QMap<QString, int>::ConstIterator itScores;
    itScores = storedScores.find( m_playlistItem->url().path() );
    m_score = itScores != storedScores.end()
        ? itScores.data()
        : CollectionDB::instance()->getSongPercentage( m_playlistItem->url().path() );

    m_playcount = CollectionDB::instance()->getPlayCount( m_playlistItem->url().path() );
    m_firstPlay = CollectionDB::instance()->getFirstPlay( m_playlistItem->url().path() );
    m_lastPlay = CollectionDB::instance()->getLastPlay( m_playlistItem->url().path() );
    readTags();
}


inline void
TagDialog::nextTrack()
{
    if( !m_playlistItem->itemBelow() ) return;

    storeTags();

    m_playlistItem = (PlaylistItem *)m_playlistItem->itemBelow();

    QMap<QString, MetaBundle>::ConstIterator itTags;
    itTags = storedTags.find( m_playlistItem->url().path() );
    m_bundle = itTags != storedTags.end() ? itTags.data() : MetaBundle( m_playlistItem );

    QMap<QString, int>::ConstIterator itScores;
    itScores = storedScores.find( m_playlistItem->url().path() );
    m_score = itScores != storedScores.end()
        ? itScores.data()
        : CollectionDB::instance()->getSongPercentage( m_playlistItem->url().path() );
    m_playcount = CollectionDB::instance()->getPlayCount( m_playlistItem->url().path() );
    m_firstPlay = CollectionDB::instance()->getFirstPlay( m_playlistItem->url().path() );
    m_lastPlay = CollectionDB::instance()->getLastPlay( m_playlistItem->url().path() );
    readTags();
}


inline void
TagDialog::checkModified() //SLOT
{
    pushButton_ok->setEnabled( hasChanged() || storedTags.count() > 0 || storedScores.count() > 0|| storedLyrics.count() > 0 );
}


void
TagDialog::musicbrainzQuery() //SLOT
{
#if HAVE_TUNEPIMP
    kdDebug() << k_funcinfo << endl;

    m_mbTrack = m_bundle.url().path();
    KTRMLookup* ktrm = new KTRMLookup( m_mbTrack, true );
    connect( ktrm, SIGNAL( sigResult( KTRMResultList ) ), SLOT( queryDone( KTRMResultList ) ) );

    pushButton_musicbrainz->setEnabled( false );
    pushButton_musicbrainz->setText( i18n( "Generating audio fingerprint..." ) );
    QApplication::setOverrideCursor( KCursor::workingCursor() );
#endif
}

void
TagDialog::queryDone( KTRMResultList results ) //SLOT
{
#if HAVE_TUNEPIMP

    if ( !results.isEmpty() )
    {
        TrackPickerDialog* t = new TrackPickerDialog(m_bundle.url().filename(),results,this);
        t->show();
    }
    else
        KMessageBox::sorry( this, i18n( "The track was not found in the MusicBrainz database." ) );

    QApplication::restoreOverrideCursor();
    pushButton_musicbrainz->setEnabled( true );
    pushButton_musicbrainz->setText( m_buttonMbText );

#endif
}

void
TagDialog::fillSelected( KTRMResult selected ) //SLOT
{
#if HAVE_TUNEPIMP
    kdDebug() << k_funcinfo << endl;


    if ( m_bundle.url().path() == m_mbTrack ) {
        if ( !selected.title().isEmpty() )    kLineEdit_title->setText( selected.title() );
        if ( !selected.artist().isEmpty() )   kComboBox_artist->setCurrentText( selected.artist() );
        if ( !selected.album().isEmpty() )    kComboBox_album->setCurrentText( selected.album() );
        if ( selected.track() != 0 )          kIntSpinBox_track->setValue( selected.track() );
        if ( selected.year() != 0 )           kIntSpinBox_year->setValue( selected.year() );
    } else {
        MetaBundle mb;
        mb.setPath( m_mbTrack );
        if ( !selected.title().isEmpty() )    mb.setTitle( selected.title() );
        if ( !selected.artist().isEmpty() )   mb.setArtist( selected.artist() );
        if ( !selected.album().isEmpty() )    mb.setAlbum( selected.album() );
        if ( selected.track() != 0 )          mb.setTrack( selected.track() );
        if ( selected.year() != 0 )           mb.setYear( selected.year() );

        storedTags.insert( m_mbTrack, mb );
    }



#endif
}


////////////////////////////////////////////////////////////////////////////////
// PRIVATE
////////////////////////////////////////////////////////////////////////////////

void TagDialog::init()
{
    //NOTE We allocate on the stack in Playlist
    if( parent() != (void*)Playlist::instance() )
        setWFlags( getWFlags() | Qt::WDestructiveClose );

    KConfig *config = amaroK::config( "TagDialog" );

    kTabWidget->addTab( summaryTab, i18n( "Summary" ) );
    kTabWidget->addTab( tagsTab, i18n( "Tags" ) );
    kTabWidget->addTab( lyricsTab, i18n( "Lyrics" ) );
    kTabWidget->addTab( statisticsTab, i18n( "Statistics" ) );
    kTabWidget->setCurrentPage( config->readNumEntry( "CurrentTab", 0 ) );

    const QStringList artists = CollectionDB::instance()->artistList();
    kComboBox_artist->insertStringList( artists );
    kComboBox_artist->completionObject()->insertItems( artists );
    kComboBox_artist->completionObject()->setIgnoreCase( true );
    kComboBox_artist->setCompletionMode( KGlobalSettings::CompletionPopup );

    const QStringList albums = CollectionDB::instance()->albumList();
    kComboBox_album->insertStringList( albums );
    kComboBox_album->completionObject()->insertItems( albums );
    kComboBox_album->completionObject()->setIgnoreCase( true );
    kComboBox_album->setCompletionMode( KGlobalSettings::CompletionPopup );

//    const QStringList genres = MetaBundle::genreList();
    const QStringList genres = CollectionDB::instance()->genreList();
    kComboBox_genre->insertStringList( genres );
    kComboBox_genre->completionObject()->insertItems( genres );
    kComboBox_genre->completionObject()->setIgnoreCase( true );

    // looks better to have a blank label than 0, we can't do this in
    // the UI file due to bug in Designer
    kIntSpinBox_track->setSpecialValueText( " " );
    kIntSpinBox_year->setSpecialValueText( " " );
    kIntSpinBox_score->setSpecialValueText( " " );

    //HACK due to deficiency in Qt that will be addressed in version 4
    // QSpinBox doesn't emit valueChanged if you edit the value with
    // the lineEdit until you change the keyboard focus
    connect( kIntSpinBox_year->child( "qt_spinbox_edit" ),  SIGNAL(textChanged( const QString& )), SLOT(checkModified()) );
    connect( kIntSpinBox_track->child( "qt_spinbox_edit" ), SIGNAL(textChanged( const QString& )), SLOT(checkModified()) );
    connect( kIntSpinBox_score->child( "qt_spinbox_edit" ), SIGNAL(textChanged( const QString& )), SLOT(checkModified()) );


    // Connects for modification check
    connect( kLineEdit_title,  SIGNAL(textChanged( const QString& )), SLOT(checkModified()) );
    connect( kComboBox_artist, SIGNAL(activated( int )),              SLOT(checkModified()) );
    connect( kComboBox_artist, SIGNAL(textChanged( const QString& )), SLOT(checkModified()) );
    connect( kComboBox_album,  SIGNAL(activated( int )),              SLOT(checkModified()) );
    connect( kComboBox_album,  SIGNAL(textChanged( const QString& )), SLOT(checkModified()) );
    connect( kComboBox_genre,  SIGNAL(activated( int )),              SLOT(checkModified()) );
    connect( kComboBox_genre,  SIGNAL(textChanged( const QString& )), SLOT(checkModified()) );
    connect( kIntSpinBox_track,SIGNAL(valueChanged( int )),           SLOT(checkModified()) );
    connect( kIntSpinBox_year, SIGNAL(valueChanged( int )),           SLOT(checkModified()) );
    connect( kIntSpinBox_score,SIGNAL(valueChanged( int )),           SLOT(checkModified()) );
    connect( kTextEdit_comment,SIGNAL(textChanged()), SLOT(checkModified()) );
    connect( kTextEdit_lyrics, SIGNAL(textChanged()), SLOT(checkModified()) );

    // Remember original button text
    m_buttonMbText = pushButton_musicbrainz->text();

    connect( pushButton_cancel,   SIGNAL(clicked()), SLOT(cancelPressed()) );
    connect( pushButton_ok,       SIGNAL(clicked()), SLOT(accept()) );
    connect( pushButton_open,     SIGNAL(clicked()), SLOT(openPressed()) );
    connect( pushButton_previous, SIGNAL(clicked()), SLOT(previousTrack()) );
    connect( pushButton_next,     SIGNAL(clicked()), SLOT(nextTrack()) );

    // draw an icon onto the open-in-konqui button
     pushButton_open->setIconSet( kapp->iconLoader()->loadIconSet( "fileopen", KIcon::Small ) );

    if( !m_playlistItem ) {
        pushButton_previous->hide();
        pushButton_next->hide();
    }

    //Update lyrics on Context Browser

    connect( this, SIGNAL(lyricsChanged( const QString& )), ContextBrowser::instance(), SLOT( lyricsChanged( const QString& ) ) );

#if HAVE_TUNEPIMP
    connect( pushButton_musicbrainz, SIGNAL(clicked()), SLOT(musicbrainzQuery()) );
#else
    QToolTip::add( pushButton_musicbrainz, i18n("Please install MusicBrainz to enable this functionality") );
#endif

    if( m_urlList.count() ) {   //editing multiple tracks
        setMultipleTracksMode();
        readMultipleTracks();
    }
    else
        readTags();

    // make it as small as possible
    resize( minimumSize() );

}


inline const QString TagDialog::unknownSafe( QString s ) {
    return ( s.isNull() || s.isEmpty() || s == "?" || s == "-" )
           ? i18n ( "Unknown" )
           : s;
}

const QStringList TagDialog::statisticsData() {

    QStringList data, values;
    const uint artist_id = CollectionDB::instance()->artistID( m_bundle.artist() );
    const uint album_id  = CollectionDB::instance()->albumID ( m_bundle.album() );

    QueryBuilder qb;

    if ( !m_bundle.artist().isEmpty() ) {
        // tracks by this artist
        qb.clear();
        qb.addReturnFunctionValue( QueryBuilder::funcCount, QueryBuilder::tabSong, QueryBuilder::valTitle );
        qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valArtistID, QString::number( artist_id ) );
        values = qb.run();
        data += i18n( "Tracks by this Artist" );
        data += values[0];


        // albums by this artist
        qb.clear();
        qb.addReturnFunctionValue( QueryBuilder::funcCount, QueryBuilder::tabAlbum, QueryBuilder::valID );
        qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valArtistID, QString::number( artist_id ) );
        qb.groupBy( QueryBuilder::tabSong, QueryBuilder::valAlbumID );
        qb.excludeMatch( QueryBuilder::tabAlbum, i18n( "Unknown" ) );
        qb.setOptions( QueryBuilder::optNoCompilations );
        values = qb.run();
        data += i18n( "Albums by this Artist" );
        data += QString::number( values.count() );


        // Favorite track by this artist
        qb.clear();
        qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTitle );
        qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valScore );
        qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valArtistID, QString::number( artist_id ) );
        qb.sortBy( QueryBuilder::tabStats, QueryBuilder::valPercentage, true );
        qb.setLimit( 0, 1 );
        values = qb.run();
        data += i18n( "Favorite by this Artist" );
        data += values[0];

        if ( !m_bundle.album().isEmpty() ) {
            // Favorite track on this album
            qb.clear();
            qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTitle );
            qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valScore );
            qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valAlbumID, QString::number( album_id ) );
            qb.sortBy( QueryBuilder::tabStats, QueryBuilder::valPercentage, true );
            qb.setLimit( 0, 1 );
            values = qb.run();
            data += i18n( "Favorite on this Album" );
            data += values[0];
        }

        // Related Artists
        const QString sArtists = CollectionDB::instance()->similarArtists( m_bundle.artist(), 4 ).join(", ");
        if ( !sArtists.isEmpty() ) {
            data += i18n( "Related Artists" );
            data += sArtists;
        }
    }
    return data;
}

void TagDialog::readTags()
{
    bool local = m_bundle.url().isLocalFile();

    setCaption( kapp->makeStdCaption( i18n("Track Information: %1 by %2").arg( m_bundle.title(),  m_bundle.artist() ) ) );

    QString niceTitle;
    if ( m_bundle.album().isEmpty() ) {
        if( !m_bundle.title().isEmpty() ) {
            if( !m_bundle.artist().isEmpty() )
                niceTitle = i18n( "<b>%1</b> by <b>%2</b>" ).arg( m_bundle.title(),  m_bundle.artist() );
            else
                niceTitle = QString( "<b>%s</b>" ).arg( m_bundle.title() );
        }
        else niceTitle = m_bundle.prettyTitle();
    }
    else {
        niceTitle = i18n( "<b>%1</b> by <b>%2</b> on <b>%3</b>" )
            .arg( m_bundle.title(), m_bundle.artist(), m_bundle.album() );
    }
    trackArtistAlbumLabel->setText( niceTitle );
    trackArtistAlbumLabel2->setText( niceTitle );

    kLineEdit_title    ->setText( m_bundle.title() );
    kComboBox_artist   ->setCurrentText( m_bundle.artist() );
    kComboBox_album    ->setCurrentText( m_bundle.album() );
    kComboBox_genre    ->setCurrentText( m_bundle.genre() );
    kIntSpinBox_track  ->setValue( m_bundle.track() );
    kIntSpinBox_year   ->setValue( m_bundle.year() );
    kIntSpinBox_score  ->setValue( m_score );
    kTextEdit_comment  ->setText( m_bundle.comment() );

    QString summaryText, statisticsText;
    const QString body2cols = i18n( "<tr><td>Label:</td><td><b>Value</b></td></tr>", "<tr><td><nobr>%1:</nobr></td><td><b>%2</b></td></tr>" );
    const QString body1col = "<tr><td colspan=2>%1</td></td></tr>";
    const QString emptyLine = "<tr><td colspan=2></td></tr>";

    summaryText = "<table width=100%><tr><td width=50%><table>";
    summaryText += body2cols.arg( i18n("Length"), unknownSafe( m_bundle.prettyLength() ) );
    summaryText += body2cols.arg( i18n("Bitrate"), unknownSafe( m_bundle.prettyBitrate() ) );
    summaryText += body2cols.arg( i18n("Samplerate"), unknownSafe( m_bundle.prettySampleRate() ) );

    summaryText += "</table></td><td width=50%><table>";
    summaryText += body2cols.arg( i18n("Score"), QString::number( m_score ) );

    summaryText += body2cols.arg( i18n("Playcount"), QString::number( m_playcount ) );
    summaryText += body2cols.arg( i18n("First Played"), m_playcount ? KGlobal::locale()->formatDate( m_firstPlay.date() , true ) : i18n("Never") );
    summaryText += body2cols.arg( i18n("Last Played"), m_playcount ? KGlobal::locale()->formatDate( m_firstPlay.date() , true ) : i18n("Never") );

    summaryText += "</table></td></tr></table>";
    summaryLabel->setText( summaryText );

    statisticsText = "<table>";

    QStringList sData = statisticsData();
    for ( uint i = 0; i<sData.count(); i+=2 ) {
        statisticsText += body2cols.arg( sData[i], sData[i+1] );
    }

    statisticsText += "</table>";

    statisticsLabel->setText( statisticsText );

    kLineEdit_location->setText( local ? m_bundle.url().path() : m_bundle.url().url() );
    // draw the album cover on the dialog
    QString cover = CollectionDB::instance()->albumImage( m_bundle, 1 );

    //lyrics
    kTextEdit_lyrics->setText( CollectionDB::instance()->getLyrics( m_bundle.url().path() ) );

    if( m_currentCover != cover ) {
        pixmap_cover->setPixmap( QPixmap( cover, "PNG" ) );
        m_currentCover = cover;
    }

    // enable only for local files
    kLineEdit_title->setReadOnly( !local );
    kComboBox_artist->setEnabled( local );
    kComboBox_album->setEnabled( local );
    kComboBox_genre->setEnabled( local );
    kIntSpinBox_track->setEnabled( local );
    kIntSpinBox_year->setEnabled( local );
    kIntSpinBox_score->setEnabled( local );
    kTextEdit_comment->setEnabled( local );

    // If it's a local file, write the directory to m_path, else disable the "open in konqui" button
    if ( local )
        m_path = m_bundle.url().directory();
    else
        pushButton_open->setEnabled( false );

    pushButton_ok->setEnabled( storedTags.count() > 0 || storedScores.count() > 0 || storedLyrics.count() > 0 );

#if HAVE_TUNEPIMP
    pushButton_musicbrainz->setEnabled( m_bundle.url().isLocalFile() );
#else
    pushButton_musicbrainz->setEnabled( false );
#endif

    if( m_playlistItem ) {
        pushButton_previous->setEnabled( m_playlistItem->itemAbove() );
        pushButton_next->setEnabled( m_playlistItem->itemBelow() );
    }
}


void
TagDialog::setMultipleTracksMode()
{

    kTabWidget->setTabEnabled( summaryTab, false );
    kTabWidget->setTabEnabled( lyricsTab, false );
    kTabWidget->setTabEnabled( statisticsTab, false );
    kTabWidget->setCurrentPage( 1 );

    kComboBox_artist->setCurrentText( "" );
    kComboBox_album->setCurrentText( "" );
    kComboBox_genre->setCurrentText( "" );

    kLineEdit_title->setEnabled( false );
    kIntSpinBox_track->setEnabled( false );

    pushButton_musicbrainz->hide();

    locationLabel->hide();
    kLineEdit_location->hide();
    pushButton_open->hide();
    pixmap_cover->hide();
}


void
TagDialog::readMultipleTracks()
{
    //Check which fields are the same for all selected tracks
    const KURL::List::ConstIterator end = m_urlList.end();
    KURL::List::ConstIterator it = m_urlList.begin();
    MetaBundle first( *it );
    bool artist=true, album=true, genre=true, comment=true, year=true, score=true;
    uint scoreFirst = CollectionDB::instance()->getSongPercentage( first.url().path() );

    for ( ; it != end; ++it ) {
        if( !(*it).isLocalFile() ) {
            // If we have a non local file, don't even lose more time comparing, just leave
            artist=false; album=false; genre=false; comment=false, year=false, score=false;
            break;
        }
        MetaBundle mb( *it );
        if ( artist && mb.artist()!=first.artist() ) { artist=false; };
        if ( album && mb.album()!=first.album() ) { album=false; };
        if ( genre && mb.genre()!=first.genre() ) { genre=false; };
        if ( comment && mb.comment()!=first.comment() ) { comment=false; };
        if ( year && mb.year()!=first.year() ) { year=false; };

        uint scoreCurrent = CollectionDB::instance()->getSongPercentage( mb.url().path() );
        if ( score && scoreFirst != scoreCurrent )
            score = false;

        if (!artist && !album && !genre && !comment && !year && !score)
            break;
    }
    // Set them in the dialog and in m_bundle ( so we don't break hasChanged() )
    if (artist) {
        m_bundle.setArtist( first.artist() );
        kComboBox_artist->setCurrentText( first.artist() );
    }
    if (album) {
        m_bundle.setAlbum( first.album() );
        kComboBox_album->setCurrentText( first.album() );
    }
    if (genre) {
        m_bundle.setGenre( first.genre() );
        kComboBox_genre->setCurrentText( first.genre() );
    }
    if (comment) {
        m_bundle.setComment( first.comment() );
        kTextEdit_comment->setText( first.comment() );
    }
    if (year) {
        m_bundle.setYear( first.year() );
        kIntSpinBox_year->setValue( first.year() );
    }
    if (score) {
        m_score = scoreFirst;
        kIntSpinBox_score->setValue( scoreFirst );
    }
}

inline bool
equalString( const QString &a, const QString &b )
{
    return (a.isEmpty() && b.isEmpty()) ? true : a == b;
}

bool
TagDialog::hasChanged()
{
    return changes();
}

int
TagDialog::changes()
{
    int result=TagDialog::NOCHANGE;
    bool modified = false;
    modified |= !equalString( kComboBox_artist->lineEdit()->text(), m_bundle.artist() );
    modified |= !equalString( kComboBox_album->lineEdit()->text(), m_bundle.album() );
    modified |= !equalString( kComboBox_genre->lineEdit()->text(), m_bundle.genre() );
    modified |= kIntSpinBox_year->value()  != m_bundle.year();

    modified |= !equalString( kTextEdit_comment->text(), m_bundle.comment() );

    if (!m_urlList.count()) { //ignore these on MultipleTracksMode
        modified |= !equalString( kLineEdit_title->text(), m_bundle.title() );
        modified |= kIntSpinBox_track->value() != m_bundle.track();
    }
    if (modified)
        result |= TagDialog::TAGSCHANGED;

    if (kIntSpinBox_score->value() != m_score)
        result |= TagDialog::SCORECHANGED;
    if ( !equalString( kTextEdit_lyrics->text(), CollectionDB::instance()->getLyrics( m_bundle.url().path() ) ) )
        result |= TagDialog::LYRICSCHANGED;

    return result;
}

void
TagDialog::storeTags()
{
    int result = changes();
    QString url = m_bundle.url().path();
    if( result & TagDialog::TAGSCHANGED ) {
        MetaBundle mb;

        mb.setPath( url );
        mb.setTitle( kLineEdit_title->text() );
        mb.setArtist( kComboBox_artist->currentText() );
        mb.setAlbum( kComboBox_album->currentText() );
        mb.setComment( kTextEdit_comment->text() );
        mb.setGenre( kComboBox_genre->currentText() );
        mb.setTrack( kIntSpinBox_track->value() );
        mb.setYear( kIntSpinBox_year->value() );
        mb.setLength( m_bundle.length() );
        mb.setBitrate( m_bundle.bitrate() );
        mb.setSampleRate( m_bundle.sampleRate() );

        storedTags.insert( url, mb );
    }
    if( result & TagDialog::SCORECHANGED )
        storedScores.insert( url, kIntSpinBox_score->value() );
    if( result & TagDialog::LYRICSCHANGED )
        storedLyrics.insert( url, kTextEdit_lyrics->text() );
}

void
TagDialog::saveTags()
{
    if( m_urlList.count() )    //editing multiple tracks
        saveMultipleTracks();
    else
    {

        storeTags();
        QMap<QString, MetaBundle>::ConstIterator endStore( storedTags.end() );
        for(QMap<QString, MetaBundle>::ConstIterator it = storedTags.begin(); it != endStore; ++it ) {
            if( writeTag( it.data(), it == --storedTags.end() ) )    //update the collection browser if it's the last track
                Playlist::instance()->updateMetaData( it.data() );
            else
                amaroK::StatusBar::instance()->longMessage( i18n(
                    "Sorry, the tag for %1 could not be changed." ).arg( it.data().prettyURL() ), KDE::StatusBar::Error );
        }
        QMap<QString, int>::ConstIterator endScore( storedScores.end() );
        for(QMap<QString, int>::ConstIterator it = storedScores.begin(); it != endScore; ++it ) {
            CollectionDB::instance()->setSongPercentage( it.key(), it.data() );
        }
        QMap<QString, QString>::ConstIterator endLyrics( storedLyrics.end() );
        for(QMap<QString, QString>::ConstIterator it = storedLyrics.begin(); it != endLyrics; ++it ) {
            CollectionDB::instance()->setLyrics( it.key(), it.data() );
            emit lyricsChanged( it.key() );
        }
    }
}


void
TagDialog::saveMultipleTracks()
{
    bool tagsChanged = false, anyTrack = false;
    const KURL::List::ConstIterator end = m_urlList.end();
    for ( KURL::List::ConstIterator it = m_urlList.begin(); it != end; ++it ) {
        if( !(*it).isLocalFile() ) continue;

        MetaBundle mb( *it );

   /* we have to update the values if they changed, so:
   1) !kLineEdit_field->text().isEmpty() && kLineEdit_field->text() != mb.field
          i.e.: The user wrote something on the field, and it's different from
          what we have in the tag.
   2) !m_bundle.field().isEmpty() && kLineEdit_field->text().isEmpty()
          i.e.: The user was shown some value for the field (it was the same
          for all selected tracks), and he deliberately emptied it.
  TODO: All this mess is because the dialog uses "" to represent what the user
        doesn't want to change, maybe we can think of something better?
   */
        if( !kComboBox_artist->currentText().isEmpty() && kComboBox_artist->currentText() != mb.artist() ||
             kComboBox_artist->currentText().isEmpty() && !m_bundle.artist().isEmpty() ) {
            mb.setArtist( kComboBox_artist->currentText() );
            tagsChanged = true;
        }

        if( !kComboBox_album->currentText().isEmpty() && kComboBox_album->currentText() != mb.album() ||
             kComboBox_album->currentText().isEmpty() && !m_bundle.album().isEmpty() ) {
            mb.setAlbum( kComboBox_album->currentText() );
            tagsChanged = true;
        }
        if( !kComboBox_genre->currentText().isEmpty() && kComboBox_genre->currentText() != mb.genre() ||
             kComboBox_genre->currentText().isEmpty() && !m_bundle.genre().isEmpty() ) {
            mb.setGenre( kComboBox_genre->currentText() );
            tagsChanged = true;
        }
        if( !kTextEdit_comment->text().isEmpty() && kTextEdit_comment->text() != mb.comment() ||
             kTextEdit_comment->text().isEmpty() && !m_bundle.comment().isEmpty() ) {
            mb.setComment( kTextEdit_comment->text() );
            tagsChanged = true;
        }
        if( kIntSpinBox_year->value() && kIntSpinBox_year->value() != mb.year() ||
            !kIntSpinBox_year->value() && m_bundle.year() ) {
            mb.setYear( kIntSpinBox_year->value() );
            tagsChanged = true;
        }
        if( kIntSpinBox_score->value() && kIntSpinBox_score->value() != m_score ||
            !kIntSpinBox_score->value() && m_score )
            CollectionDB::instance()->setSongPercentage( mb.url().path(), kIntSpinBox_score->value() );

        if ( tagsChanged ) { // We only try to update the file if one of the tags changed
            if( writeTag( mb, false ) ) //leave the Collection Browser update to be made after all changes
                Playlist::instance()->updateMetaData( mb );
            else
                amaroK::StatusBar::instance()->longMessage( i18n(
                    "Sorry, the tag for %1 could not be changed." ).arg( mb.prettyURL() ), KDE::StatusBar::Error );
        }
        anyTrack |= tagsChanged;
    }
    // if any change was really made for any of the tracks, update Collection Browser.
    if (anyTrack)
        CollectionView::instance()->renderView();
}


bool
TagDialog::writeTag( MetaBundle mb, bool updateCB )
{
    //Set default codec to UTF-8 (see bugs 111246 and 111232)
    TagLib::ID3v2::FrameFactory::instance()->setDefaultTextEncoding(TagLib::String::UTF8);

    QCString path = QFile::encodeName( mb.url().path() );
    if ( !TagLib::File::isWritable( path ) ) {
        amaroK::StatusBar::instance()->longMessage( i18n(
           "TagLib claims %1 file is not writable." ).arg( path ), KDE::StatusBar::Error );

        return false;
    }

    TagLib::FileRef f( path, false );

    if ( !f.isNull() )
    {
        //visual feedback
        QApplication::setOverrideCursor( KCursor::waitCursor() );

        TagLib::Tag * t = f.tag();
        t->setTitle( QStringToTString( mb.title() ) );
        t->setArtist( QStringToTString( mb.artist() ) );
        t->setAlbum( QStringToTString( mb.album() ) );
        t->setTrack( mb.track() );
        t->setYear( mb.year() );
        t->setComment( QStringToTString( mb.comment() ) );
        t->setGenre( QStringToTString( mb.genre() ) );

        bool result = f.save();
        if( result )
            //update the collection db
            CollectionDB::instance()->updateTags( mb.url().path(), mb, updateCB );

        QApplication::restoreOverrideCursor();

        return result;
    }
    else return false;
}


#include "tagdialog.moc"

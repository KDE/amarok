// (c) 2004 Mark Kretschmann <markey@web.de>
// (c) 2004 Pierpaolo Di Panfilo <pippo_dp@libero.it>
// See COPYING file for licensing information.

#include "collectiondb.h"
#include "metabundle.h"
#include "playlist.h"
#include "playlistitem.h"
#include "tagdialog.h"

#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/tfile.h>
#include <taglib/tstring.h>

#include <qfile.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qtooltip.h>

#include <kapplication.h>
#include <kcombobox.h>
#include <kcursor.h>
#include <kdebug.h>
#include <kglobal.h>
#include <klineedit.h>
#include <kmessagebox.h>
#include <knuminput.h>
#include <krun.h>
#include <kstandarddirs.h>


TagDialog::TagDialog( const KURL& url, QWidget* parent )
    : TagDialogBase( parent )
    , m_bundle( MetaBundle( url ) )
    , m_playlistItem( 0 )
    , m_currentCover( 0 )
{
    init();
}


TagDialog::TagDialog( const KURL::List list, QWidget* parent )
    : TagDialogBase( parent )
    , m_bundle( MetaBundle() )
    , m_playlistItem( 0 )
    , m_urlList( list )
    , m_currentCover( 0 )
{
    setCaption( kapp->makeStdCaption( i18n( "Meta Information (Multiple Files)" ) ) );
    init();
}


TagDialog::TagDialog( const MetaBundle& mb, PlaylistItem* item, QWidget* parent )
    : TagDialogBase( parent )
    , m_bundle( mb )
    , m_playlistItem( item )
    , m_currentCover( 0 )
{
    init();
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

    deleteLater();
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

    if( hasChanged() )
        storeTags();

    m_playlistItem = (PlaylistItem *)m_playlistItem->itemAbove();
    QMap<QString, MetaBundle>::ConstIterator it;
    it = storedTags.find( m_playlistItem->url().path() );
    m_bundle = it != storedTags.end() ? it.data() : MetaBundle( m_playlistItem );

    readTags();
}


inline void
TagDialog::nextTrack()
{
    if( !m_playlistItem->itemBelow() ) return;

    if( hasChanged() )
        storeTags();

    m_playlistItem = (PlaylistItem *)m_playlistItem->itemBelow();
    QMap<QString, MetaBundle>::ConstIterator it;
    it = storedTags.find( m_playlistItem->url().path() );
    m_bundle = it != storedTags.end() ? it.data() : MetaBundle( m_playlistItem );

    readTags();
}


inline void
TagDialog::checkModified() //SLOT
{
    pushButton_ok->setEnabled( hasChanged() );
}


void
TagDialog::musicbrainzQuery() //SLOT
{
#ifdef HAVE_MUSICBRAINZ
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
#ifdef HAVE_MUSICBRAINZ
    kdDebug() << k_funcinfo << endl;
    QApplication::restoreOverrideCursor();

    pushButton_musicbrainz->setEnabled( true );
    pushButton_musicbrainz->setText( m_buttonMbText );

    if ( !results.isEmpty() )
    {
        if ( m_bundle.url().path() == m_mbTrack ) {
            if ( !results[0].title().isEmpty() )    kLineEdit_title->setText( results[0].title() );
            if ( !results[0].artist().isEmpty() )   kComboBox_artist->setCurrentText( results[0].artist() );
            if ( !results[0].album().isEmpty() )    kComboBox_album->setCurrentText( results[0].album() );
            if ( results[0].track() != 0 )          kIntSpinBox_track->setValue( results[0].track() );
            if ( results[0].year() != 0 )           kIntSpinBox_year->setValue( results[0].year() );
        }
        else {
            MetaBundle mb;
            mb.setPath( m_mbTrack );
            if ( !results[0].title().isEmpty() )    mb.setTitle( results[0].title() );
            if ( !results[0].artist().isEmpty() )   mb.setArtist( results[0].artist() );
            if ( !results[0].album().isEmpty() )    mb.setAlbum( results[0].album() );
            if ( results[0].track() != 0 )          mb.setTrack( QString::number( results[0].track() ) );
            if ( results[0].year() != 0 )           mb.setYear( QString::number( results[0].year() ) );

            storedTags.insert( m_mbTrack, mb );
        }

    }
    else
        KMessageBox::sorry( this, i18n( "The track was not found in the MusicBrainz database." ) );
#endif
}


////////////////////////////////////////////////////////////////////////////////
// PRIVATE
////////////////////////////////////////////////////////////////////////////////

void TagDialog::init()
{
//     NOTE: WDestructiveClose must not be used when the widget is created on the stack
//     setWFlags( getWFlags() | Qt::WDestructiveClose );

    //get artist and album list from collection db
    QStringList artistList, albumList;
    {
        artistList = CollectionDB::instance()->artistList();
        albumList  = CollectionDB::instance()->albumList();
    }

    //enable auto-completion for artist, album and genre
    kComboBox_artist->insertStringList( artistList );
    kComboBox_artist->completionObject()->insertItems( artistList );
    kComboBox_artist->completionObject()->setIgnoreCase( true );

    kComboBox_album->insertStringList( albumList );
    kComboBox_album->completionObject()->insertItems( albumList );
    kComboBox_album->completionObject()->setIgnoreCase( true );

    QStringList genreList = MetaBundle::genreList();
    kComboBox_genre->insertStringList( genreList );
    kComboBox_genre->completionObject()->insertItems( genreList );
    kComboBox_genre->completionObject()->setIgnoreCase( true );

    //looks better to have a blank label than 0
    kIntSpinBox_track->setSpecialValueText( " " );
    kIntSpinBox_year->setSpecialValueText( " " );

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
    connect( kLineEdit_comment,SIGNAL(textChanged( const QString& )), SLOT(checkModified()) );

    // Remember original button text
    m_buttonMbText = pushButton_musicbrainz->text();

    connect( pushButton_cancel,   SIGNAL(clicked()), SLOT(cancelPressed()) );
    connect( pushButton_ok,       SIGNAL(clicked()), SLOT(accept()) );
    connect( pushButton_open,     SIGNAL(clicked()), SLOT(openPressed()) );
    connect( pushButton_previous, SIGNAL(clicked()), SLOT(previousTrack()) );
    connect( pushButton_next,     SIGNAL(clicked()), SLOT(nextTrack()) );

    // draw an icon onto the open-in-konqui button
    pushButton_open->setPixmap( QPixmap( locate( "data", QString( "amarok/images/folder_crystal.png" ) ), "PNG" ) );

    if( !m_playlistItem ) {
        pushButton_previous->hide();
        pushButton_next->hide();
    }

#ifdef HAVE_MUSICBRAINZ
    connect( pushButton_musicbrainz, SIGNAL(clicked()), SLOT(musicbrainzQuery()) );
#else
    QToolTip::add( pushButton_musicbrainz, i18n("Please install MusicBrainz to enable this functionality") );
#endif

    if( m_urlList.count() )    //editing multiple tracks
        setMultipleTracksMode();
    else
        readTags();

    adjustSize();
}


void TagDialog::readTags()
{
    kLineEdit_title->setText( m_bundle.title() );
    kComboBox_artist->setCurrentText( m_bundle.artist() );
    kComboBox_album->setCurrentText( m_bundle.album() );
    kComboBox_genre->setCurrentText( m_bundle.genre() );
    kIntSpinBox_track->setValue( m_bundle.track().toInt() );
    kIntSpinBox_year->setValue( m_bundle.year().toInt() );
    kLineEdit_comment->setText( m_bundle.comment() );
    kLineEdit_length->setText( m_bundle.prettyLength() );
    kLineEdit_bitrate->setText( m_bundle.prettyBitrate() );
    kLineEdit_samplerate->setText( m_bundle.prettySampleRate() );
    kLineEdit_location->setText( m_bundle.url().isLocalFile() ? m_bundle.url().path() : m_bundle.url().url() );
    // draw the album cover on the dialog
    QString cover = CollectionDB::instance()->albumImage( m_bundle );

    if( m_currentCover != cover ) {
        pixmap_cover->setPixmap( QPixmap( cover, "PNG" ) );
        m_currentCover = cover;
    }

    // Disable the tag editor for streams
    if ( !m_bundle.url().isLocalFile() )
    {
          kLineEdit_title->setReadOnly( true );
          kComboBox_artist->setEnabled( false );
          kComboBox_album->setEnabled( false );
          kComboBox_genre->setEnabled( false );
          kIntSpinBox_track->setEnabled( false );
          kIntSpinBox_year->setEnabled( false );
          kLineEdit_comment->setEnabled( false );
    }

    // If it's a local file, write the directory to m_path, else disable the "open in konqui" button
    if ( m_bundle.url().isLocalFile() )
        m_path = m_bundle.url().directory();
    else
        pushButton_open->setEnabled( false );

    pushButton_ok->setEnabled( storedTags.count() > 0 );

#ifdef HAVE_MUSICBRAINZ
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
    kComboBox_artist->setCurrentText( "" );
    kComboBox_album->setCurrentText( "" );
    kComboBox_genre->setCurrentText( "" );

    kLineEdit_title->setEnabled( false );
    kIntSpinBox_track->setEnabled( false );

    pushButton_musicbrainz->hide();

    line1->hide();
    lengthLabel->hide();
    kLineEdit_length->hide();
    bitrateLabel->hide();
    kLineEdit_bitrate->hide();
    samplerateLabel->hide();
    kLineEdit_samplerate->hide();
    locationLabel->hide();
    kLineEdit_location->hide();
    pushButton_open->hide();
    pixmap_cover->hide();
}


bool
TagDialog::hasChanged()
{
    bool modified = false;
    modified |= !equalString( kLineEdit_title->text(), m_bundle.title() );
    modified |= !equalString( kComboBox_artist->lineEdit()->text(), m_bundle.artist() );
    modified |= !equalString( kComboBox_album->lineEdit()->text(), m_bundle.album() );
    modified |= !equalString( kComboBox_genre->lineEdit()->text(), m_bundle.genre() );
    modified |= kIntSpinBox_track->value() != m_bundle.track().toInt();
    modified |= kIntSpinBox_year->value()  != m_bundle.year().toInt();
    modified |= !equalString( kLineEdit_comment->text(), m_bundle.comment() );

    return modified;
}


bool
TagDialog::equalString( const QString &a, const QString &b )
{
    if( a.isEmpty() && b.isEmpty() )
        return true;
    else
        return a == b;
}


void
TagDialog::storeTags()
{
    MetaBundle mb;
    QString url = m_bundle.url().path();

    mb.setPath( url );
    mb.setTitle( kLineEdit_title->text() );
    mb.setArtist( kComboBox_artist->currentText() );
    mb.setAlbum( kComboBox_album->currentText() );
    mb.setComment( kLineEdit_comment->text() );
    mb.setGenre( kComboBox_genre->currentText() );
    mb.setTrack( QString::number( kIntSpinBox_track->value() ) );
    mb.setYear( QString::number( kIntSpinBox_year->value() ) );
    mb.setLength( m_bundle.length() );
    mb.setBitrate( m_bundle.bitrate() );
    mb.setSampleRate( m_bundle.sampleRate() );

    storedTags.insert( url, mb );
}


void
TagDialog::saveTags()
{
    if( m_urlList.count() )    //editing multiple tracks
        saveMultipleTracks();
    else
    {
        if( hasChanged() )
            storeTags();

        QMap<QString, MetaBundle>::ConstIterator it;
        for( it = storedTags.begin(); it != storedTags.end(); ++it ) {
            if( writeTag( it.data(), it == --storedTags.end() ) )    //update the collection browser if it's the last track
                Playlist::instance()->updateMetaData( it.data() );
        }
    }
}


void
TagDialog::saveMultipleTracks()
{
    const KURL::List::ConstIterator end = m_urlList.end();
    for ( KURL::List::ConstIterator it = m_urlList.begin(); it != end; ++it ) {
        if( !(*it).isLocalFile() ) continue;

        MetaBundle mb( *it );

        if( !kComboBox_artist->currentText().isEmpty() )
            mb.setArtist( kComboBox_artist->currentText() );

        if( !kComboBox_album->currentText().isEmpty() )
            mb.setAlbum( kComboBox_album->currentText() );

        if( !kComboBox_genre->currentText().isEmpty() )
            mb.setGenre( kComboBox_genre->currentText() );

        if( !kLineEdit_comment->text().isEmpty() )
            mb.setComment( kLineEdit_comment->text() );

        if( kIntSpinBox_year->value() )
            mb.setYear( QString::number( kIntSpinBox_year->value() ) );

        if( writeTag( mb, it == --m_urlList.end() ) )    //update the collection browser if it's the last track
            Playlist::instance()->updateMetaData( mb );
    }
}


bool
TagDialog::writeTag( MetaBundle mb, bool updateCB )
{
    QCString path = QFile::encodeName( mb.url().path() );

    if ( !TagLib::File::isWritable( path ) ) {
        KMessageBox::sorry( this, i18n( "TagLib claims this file is not writable." ) );
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
        t->setTrack( mb.track().toInt() );
        t->setYear( mb.year().toInt() );
        t->setComment( QStringToTString( mb.comment() ) );
        t->setGenre( QStringToTString( mb.genre() ) );

        bool result = f.save();
        if( result )
            //update the collection db
            CollectionDB::instance()->updateTags( path, mb, updateCB );

        QApplication::restoreOverrideCursor();

        return result;
    }
    else return false;
}


#include "tagdialog.moc"

// (c) 2004 Mark Kretschmann <markey@web.de>
// (c) 2004 Pierpaolo Di Panfilo <pippo_dp@libero.it>
// See COPYING file for licensing information.

#include "debug.h"
#include "collectiondb.h"
#include "metabundle.h"
#include "playlist.h"
#include "playlistitem.h"
#include "tagdialog.h"
#include "trackpickerdialog.h"

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
#include <kglobal.h>
#include <kiconloader.h>
#include <klineedit.h>
#include <kmessagebox.h>
#include <knuminput.h>
#include <krun.h>
#include <kstandarddirs.h>


TagDialog::TagDialog( const KURL& url, QWidget* parent )
    : TagDialogBase( parent )
    , m_bundle( MetaBundle( url ) )
    , m_score ( CollectionDB::instance()->getSongPercentage( url.path() ) )
    , m_playlistItem( 0 )
    , m_currentCover( 0 )
{
    init();
}


TagDialog::TagDialog( const KURL::List list, QWidget* parent )
    : TagDialogBase( parent )
    , m_bundle( MetaBundle() )
    , m_score ( 0 )
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
    , m_score ( CollectionDB::instance()->getSongPercentage( mb.url().path() ) )
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
        if ( selected.track() != 0 )          mb.setTrack( QString::number( selected.track() ) );
        if ( selected.year() != 0 )           mb.setYear( QString::number( selected.year() ) );

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
    connect( kLineEdit_comment,SIGNAL(textChanged( const QString& )), SLOT(checkModified()) );

    // Remember original button text
    m_buttonMbText = pushButton_musicbrainz->text();

    connect( pushButton_cancel,   SIGNAL(clicked()), SLOT(cancelPressed()) );
    connect( pushButton_ok,       SIGNAL(clicked()), SLOT(accept()) );
    connect( pushButton_open,     SIGNAL(clicked()), SLOT(openPressed()) );
    connect( pushButton_previous, SIGNAL(clicked()), SLOT(previousTrack()) );
    connect( pushButton_next,     SIGNAL(clicked()), SLOT(nextTrack()) );

    // draw an icon onto the open-in-konqui button
     pushButton_open->setIconSet( kapp->iconLoader()->loadIconSet( locate( "data", QString( "amarok/images/folder_crystal.png" ) ), KIcon::Small ) );

    if( !m_playlistItem ) {
        pushButton_previous->hide();
        pushButton_next->hide();
    }

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
    kIntSpinBox_score->setValue( m_score );
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
          kIntSpinBox_score->setEnabled( false );
          kLineEdit_comment->setEnabled( false );
    }

    // If it's a local file, write the directory to m_path, else disable the "open in konqui" button
    if ( m_bundle.url().isLocalFile() )
        m_path = m_bundle.url().directory();
    else
        pushButton_open->setEnabled( false );

    pushButton_ok->setEnabled( storedTags.count() > 0 );

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
        kLineEdit_comment->setText( first.comment() );
    }
    if (year) {
        m_bundle.setYear( first.year() );
        kIntSpinBox_year->setValue( first.year().toInt() );
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
    bool modified = false;
    modified |= !equalString( kComboBox_artist->lineEdit()->text(), m_bundle.artist() );
    modified |= !equalString( kComboBox_album->lineEdit()->text(), m_bundle.album() );
    modified |= !equalString( kComboBox_genre->lineEdit()->text(), m_bundle.genre() );
    modified |= kIntSpinBox_year->value()  != m_bundle.year().toInt();
    modified |= kIntSpinBox_score->value() != m_score;
    modified |= !equalString( kLineEdit_comment->text(), m_bundle.comment() );
    if (!m_urlList.count()) { //ignore these on MultipleTracksMode
        modified |= !equalString( kLineEdit_title->text(), m_bundle.title() );
        modified |= kIntSpinBox_track->value() != m_bundle.track().toInt();
    }

    return modified;
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

    CollectionDB::instance()->setSongPercentage( url, kIntSpinBox_score->value() );

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
             kComboBox_artist->currentText().isEmpty() && !m_bundle.artist().isEmpty() )
            mb.setArtist( kComboBox_artist->currentText() );

        if( !kComboBox_album->currentText().isEmpty() && kComboBox_album->currentText() != mb.album() ||
             kComboBox_album->currentText().isEmpty() && !m_bundle.album().isEmpty() )
            mb.setAlbum( kComboBox_album->currentText() );

        if( !kComboBox_genre->currentText().isEmpty() && kComboBox_genre->currentText() != mb.genre() ||
             kComboBox_genre->currentText().isEmpty() && !m_bundle.genre().isEmpty() )
            mb.setGenre( kComboBox_genre->currentText() );

        if( !kLineEdit_comment->text().isEmpty() && kLineEdit_comment->text() != mb.comment() ||
             kLineEdit_comment->text().isEmpty() && !m_bundle.comment().isEmpty() )
            mb.setComment( kLineEdit_comment->text() );

        if( kIntSpinBox_year->value() && kIntSpinBox_year->value() != mb.year().toInt() ||
            !kIntSpinBox_year->value() && m_bundle.year().toInt() )
            mb.setYear( QString::number( kIntSpinBox_year->value() ) );

        if( kIntSpinBox_score->value() && kIntSpinBox_score->value() != m_score ||
            !kIntSpinBox_score->value() && m_score )
            CollectionDB::instance()->setSongPercentage( mb.url().path(), kIntSpinBox_score->value() );

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

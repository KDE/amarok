// (c) 2004 Mark Kretschmann <markey@web.de>
// See COPYING file for licensing information.

#include "collectiondb.h"
#include "playlistitem.h"
#include "tagdialog.h"

#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/tfile.h>
#include <taglib/tstring.h>

#include <qapplication.h>
#include <qfile.h>
#include <qpushbutton.h>
#include <qtooltip.h>

#include <kcombobox.h>
#include <kcursor.h>
#include <kdebug.h>
#include <kglobal.h>
#include <klineedit.h>
#include <kmessagebox.h>
#include <knuminput.h>
#include <krun.h>


TagDialog::TagDialog( const MetaBundle& mb, QWidget* parent )
    : TagDialogBase( parent )
    , m_bundle( mb )
{
    kLineEdit_title->setText( m_bundle.title() );

    //get artist and album list from collection db
    QStringList artistList, albumList;
    {
        CollectionDB db;
        artistList = db.artistList();
        albumList  = db.albumList();
    }

    //enable auto-completion for artist, album and genre
    kComboBox_artist->insertStringList( artistList );
    kComboBox_artist->completionObject()->insertItems( artistList );
    kComboBox_artist->setCurrentText( m_bundle.artist() );
    kComboBox_album->insertStringList( albumList );
    kComboBox_album->completionObject()->insertItems( albumList );
    kComboBox_album->setCurrentText( m_bundle.album() );

    QStringList genreList = MetaBundle::genreList();
    kComboBox_genre->insertStringList( genreList );
    kComboBox_genre->completionObject()->insertItems( genreList );
    kComboBox_genre->setCurrentText( m_bundle.genre() );

    //looks better to have a blank label than 0
    kIntSpinBox_track->setSpecialValueText( " " );
    kIntSpinBox_year->setSpecialValueText( " " );

    kIntSpinBox_track->setValue( m_bundle.track().toInt() );
    kIntSpinBox_year->setValue( m_bundle.year().toInt() );
    kLineEdit_comment->setText( m_bundle.comment() );
    kLineEdit_length->setText( m_bundle.prettyLength() );
    kLineEdit_bitrate->setText( m_bundle.prettyBitrate() );
    kLineEdit_samplerate->setText( m_bundle.prettySampleRate() );
    kLineEdit_location->setText( m_bundle.url().isLocalFile() ? m_bundle.url().path() : m_bundle.url().url() );

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

    // If it's a local file, write the directory to m_path, else disable the "open in konqui" button
    if ( m_bundle.url().isLocalFile() )
        m_path = m_bundle.url().directory();
    else
        pushButton_open->setEnabled( false );

    connect( pushButton_cancel, SIGNAL(clicked()), SLOT(reject()) );
    connect( pushButton_ok,     SIGNAL(clicked()), SLOT(accept()) );
    connect( pushButton_open,   SIGNAL(clicked()), SLOT(openPressed()) );
    pushButton_ok->setEnabled( false );

#ifdef HAVE_MUSICBRAINZ
    connect( pushButton_musicbrainz, SIGNAL(clicked()), SLOT(musicbrainzQuery()) );
    pushButton_musicbrainz->setEnabled( m_bundle.url().isLocalFile() );
#else
    pushButton_musicbrainz->setEnabled( false );
    QToolTip::add( pushButton_musicbrainz, i18n("Please install MusicBrainz to enable this functionality") );
#endif

    adjustSize();
}


void
TagDialog::exec( PlaylistItem *item )
{
    if( exec() == QDialog::Accepted )
    {
        //Playlist uses this function and guarentees playlistItem will
        //still exist

        item->setText( PlaylistItem::Title,   kLineEdit_title->text() );
        item->setText( PlaylistItem::Artist,  kComboBox_artist->currentText() );
        item->setText( PlaylistItem::Album,   kComboBox_album->currentText() );
        item->setText( PlaylistItem::Genre,   kComboBox_genre->currentText() );
        item->setText( PlaylistItem::Track,   kIntSpinBox_track->text() );
        item->setText( PlaylistItem::Year,    kIntSpinBox_year->text() );
        item->setText( PlaylistItem::Comment, kLineEdit_comment->text() );
    }
}



////////////////////////////////////////////////////////////////////////////////
// PRIVATE SLOTS
////////////////////////////////////////////////////////////////////////////////

void
TagDialog::accept() //SLOT
{
    if ( hasChanged() ) {
        pushButton_ok->setEnabled( false ); //visual feedback
        if( !writeTag() )
            QDialog::reject();
    }

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
TagDialog::checkModified() //SLOT
{
    pushButton_ok->setEnabled( hasChanged() );
}


void
TagDialog::musicbrainzQuery() //SLOT
{
#ifdef HAVE_MUSICBRAINZ
    kdDebug() << k_funcinfo << endl;

    MusicBrainzQuery* query = new MusicBrainzQuery( MusicBrainzQuery::File, m_bundle.url().path() );
    connect( query, SIGNAL( signalDone( const MusicBrainzQuery::TrackList& ) ),
              this,   SLOT( queryDone( const MusicBrainzQuery::TrackList& ) ) );

    if ( query->start() )
    {
        pushButton_musicbrainz->setEnabled( false );
        pushButton_musicbrainz->setText( i18n( "Generating TRM..." ) );
        QApplication::setOverrideCursor( KCursor::workingCursor() );
    }
#endif
}


void
TagDialog::queryDone( const MusicBrainzQuery::TrackList& tracklist ) //SLOT
{
#ifdef HAVE_MUSICBRAINZ
    kdDebug() << k_funcinfo << endl;
    QApplication::restoreOverrideCursor();

    pushButton_musicbrainz->setEnabled( true );
    pushButton_musicbrainz->setText( m_buttonMbText );

    if ( !tracklist.isEmpty() )
    {
        if ( !tracklist[0].name.isEmpty() )     kLineEdit_title->setText( tracklist[0].name );
        if ( !tracklist[0].artist.isEmpty() )   kComboBox_artist->setCurrentText( tracklist[0].artist );
        if ( !tracklist[0].album.isEmpty() )    kComboBox_album->setCurrentText( tracklist[0].album );
    }
    else
        KMessageBox::sorry( this, i18n( "The track was not found in the MusicBrainz database." ) );
#endif
}


////////////////////////////////////////////////////////////////////////////////
// PRIVATE
////////////////////////////////////////////////////////////////////////////////


bool
TagDialog::hasChanged()
{
    bool modified = false;

    modified |= kLineEdit_title->text()              != m_bundle.title();
    modified |= kComboBox_artist->lineEdit()->text() != m_bundle.artist();
    modified |= kComboBox_album->lineEdit()->text()  != m_bundle.album();
    modified |= kComboBox_genre->lineEdit()->text()  != m_bundle.genre();
    modified |= kIntSpinBox_track->value()           != m_bundle.track().toInt();
    modified |= kIntSpinBox_year->value()            != m_bundle.year().toInt();
    modified |= kLineEdit_comment->text()            != m_bundle.comment();

    return modified;
}


bool
TagDialog::writeTag()
{
    QCString path = QFile::encodeName( m_bundle.url().path() );

    if ( !TagLib::File::isWritable( path ) ) {
        KMessageBox::error( this, i18n( "The file is not writable." ) );
        return false;
    }

    TagLib::FileRef f( path, false );

    if ( !f.isNull() )
    {
        //visual feedback
        QApplication::setOverrideCursor( KCursor::waitCursor() );

        TagLib::Tag * t = f.tag();
        t->setTitle( QStringToTString( kLineEdit_title->text() ) );
        t->setArtist( QStringToTString( kComboBox_artist->currentText() ) );
        t->setAlbum( QStringToTString( kComboBox_album->currentText() ) );
        t->setTrack( kIntSpinBox_track->value() );
        t->setYear( kIntSpinBox_year->value() );
        t->setComment( QStringToTString( kLineEdit_comment->text() ) );
        t->setGenre( QStringToTString( kComboBox_genre->currentText() ) );

        f.save();

         //update the collection db
        CollectionDB().updateTags( path, MetaBundle( m_bundle.url(), t ) );

        QApplication::restoreOverrideCursor();

        return true;
    }
    else return false;
}

#include "tagdialog.moc"

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
#include <qlabel.h>
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
#include <kstandarddirs.h>


TagDialog::TagDialog( const KURL& url, QWidget* parent )
    : TagDialogBase( parent )
    , m_bundle( MetaBundle( url ) )
    , m_playlistItem( 0 )
{
    init();
}


TagDialog::TagDialog( const MetaBundle& mb, PlaylistItem* item, QWidget* parent )
    : TagDialogBase( parent )
    , m_bundle( mb )
    , m_playlistItem( item )
{
    init();
}


////////////////////////////////////////////////////////////////////////////////
// PRIVATE SLOTS
////////////////////////////////////////////////////////////////////////////////

void
TagDialog::accept() //SLOT
{
    if ( hasChanged() ) {
        pushButton_ok->setEnabled( false ); //visual feedback
        if ( writeTag() )
            if ( m_playlistItem )
                syncItemText();
    }                

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
TagDialog::checkModified() //SLOT
{
    pushButton_ok->setEnabled( hasChanged() );
}


void
TagDialog::musicbrainzQuery() //SLOT
{
#ifdef HAVE_MUSICBRAINZ
    kdDebug() << k_funcinfo << endl;

    KTRMLookup* ktrm = new KTRMLookup( m_bundle.url().path(), true );
    connect( ktrm, SIGNAL( sigResult( KTRMResultList ) ), SLOT( queryDone( KTRMResultList ) ) );

    pushButton_musicbrainz->setEnabled( false );
    pushButton_musicbrainz->setText( i18n( "Generating TRM..." ) );
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
        if ( !results[0].title().isEmpty() )    kLineEdit_title->setText( results[0].title() );
        if ( !results[0].artist().isEmpty() )   kComboBox_artist->setCurrentText( results[0].artist() );
        if ( !results[0].album().isEmpty() )    kComboBox_album->setCurrentText( results[0].album() );
                                                kIntSpinBox_track->setValue( results[0].track() );
                                                kIntSpinBox_year->setValue( results[0].year() );
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
    setWFlags( getWFlags() | Qt::WDestructiveClose );    
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
    
    // draw an icon onto the open-in-konqui button
    pushButton_open->setPixmap( QPixmap( locate( "data", QString( "amarok/images/folder_crystal.png" ) ), "PNG" ) );
    // draw the fancy amaroK logo on the dialog ;-)
    pixmap_cover->setPixmap( QPixmap( locate( "data", QString( "amarok/images/amarok_cut.png" ) ), "PNG" ) );

#ifdef HAVE_MUSICBRAINZ
    connect( pushButton_musicbrainz, SIGNAL(clicked()), SLOT(musicbrainzQuery()) );
    pushButton_musicbrainz->setEnabled( m_bundle.url().isLocalFile() );
#else
    pushButton_musicbrainz->setEnabled( false );
    QToolTip::add( pushButton_musicbrainz, i18n("Please install MusicBrainz to enable this functionality") );
#endif

    adjustSize();
}


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


void
TagDialog::syncItemText()
{    
    QListViewItem* item = m_playlistItem->listView()->firstChild();
    
    // Find out if item still exists in the listView
    do {
        if ( item == m_playlistItem ) break;
        item = item->nextSibling();
    }
    while( item );
    
    if ( item ) {
        // Reflect changes in PlaylistItem text
        m_playlistItem->setText( PlaylistItem::Title,   kLineEdit_title->text() );
        m_playlistItem->setText( PlaylistItem::Artist,  kComboBox_artist->currentText() );
        m_playlistItem->setText( PlaylistItem::Album,   kComboBox_album->currentText() );
        m_playlistItem->setText( PlaylistItem::Genre,   kComboBox_genre->currentText() );
        m_playlistItem->setText( PlaylistItem::Track,   kIntSpinBox_track->text() );
        m_playlistItem->setText( PlaylistItem::Year,    kIntSpinBox_year->text() );
        m_playlistItem->setText( PlaylistItem::Comment, kLineEdit_comment->text() );
    }
}


#include "tagdialog.moc"

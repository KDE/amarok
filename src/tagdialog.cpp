// (c) 2004 Mark Kretschmann <markey@web.de>
// See COPYING file for licensing information.

#include "metabundle.h"
#include "tagdialog.h"

#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/tstring.h>

#include <qfile.h>
#include <qpushbutton.h>

#include <kcombobox.h>
#include <kdebug.h>
#include <kglobal.h>
#include <klineedit.h>
#include <kmessagebox.h>
#include <knuminput.h>


TagDialog::TagDialog( const MetaBundle& mb, QWidget* parent )
    : TagDialogBase( parent )
    , m_metaBundle( mb )
{
    kLineEdit_title->setText( mb.title() );
    kLineEdit_artist->setText( mb.artist() );
    kLineEdit_album->setText( mb.album() );
    kComboBox_genre->insertStringList( MetaBundle::genreList() );
    kComboBox_genre->setCurrentText( mb.genre() );
    kIntSpinBox_track->setValue( mb.track().toInt() );
    kIntSpinBox_year->setValue( mb.year().toInt() );
    kLineEdit_comment->setText( mb.comment() );
    kLineEdit_length->setText( mb.prettyLength() );
    kLineEdit_bitrate->setText( mb.prettyBitrate() );
    kLineEdit_samplerate->setText( mb.prettySampleRate() );
    kLineEdit_location->setText( mb.url().isLocalFile() ? mb.url().path() : mb.url().url() );

    // Connects for modification check
    connect( kLineEdit_title, SIGNAL( textChanged( const QString& ) ), this, SLOT( checkModified() ) );
    connect( kLineEdit_artist, SIGNAL( textChanged( const QString& ) ), this, SLOT( checkModified() ) );
    connect( kLineEdit_album, SIGNAL( textChanged( const QString& ) ), this, SLOT( checkModified() ) );
    connect( kComboBox_genre, SIGNAL( activated( int ) ), this, SLOT( checkModified() ) );
    connect( kComboBox_genre, SIGNAL( textChanged( const QString& ) ), this, SLOT( checkModified() ) );
    connect( kIntSpinBox_track, SIGNAL( valueChanged( int ) ), this, SLOT( checkModified() ) );
    connect( kIntSpinBox_year, SIGNAL( valueChanged( int ) ), this, SLOT( checkModified() ) );
    connect( kLineEdit_comment, SIGNAL( textChanged( const QString& ) ), this, SLOT( checkModified() ) );
    
    // Remember original button text
    m_buttonMbText = pushButton_musicbrainz->text();
    
    connect( pushButton_cancel, SIGNAL( clicked() ), this, SLOT( deleteLater() ) );
    connect( pushButton_ok, SIGNAL( clicked() ), this, SLOT( okPressed() ) );
    pushButton_ok->setEnabled( false );
    
#ifdef HAVE_MUSICBRAINZ
    connect( pushButton_musicbrainz, SIGNAL( clicked() ), this, SLOT( musicbrainzQuery() ) );
#endif
#ifndef HAVE_MUSICBRAINZ
    pushButton_musicbrainz->setEnabled( false );
#endif
    
    adjustSize();
}


////////////////////////////////////////////////////////////////////////////////
// PRIVATE SLOTS
////////////////////////////////////////////////////////////////////////////////

void
TagDialog::okPressed() //SLOT
{
    kdDebug() << k_funcinfo << endl;
    
    TagLib::FileRef f( QFile::encodeName( m_metaBundle.url().path() ), false );

    if ( !f.isNull() ) {
        TagLib::Tag * t = f.tag();
        
        t->setTitle( QStringToTString( kLineEdit_title->text() ) );
        t->setArtist( QStringToTString( kLineEdit_artist->text() ) );
        t->setAlbum( QStringToTString( kLineEdit_album->text() ) );
        t->setTrack( kIntSpinBox_track->value() );
        t->setYear( kIntSpinBox_year->value() );
        t->setComment( QStringToTString( kLineEdit_comment->text() ) );
        t->setGenre( QStringToTString( kComboBox_genre->currentText() ) );
        
        f.save();
    }
            
    deleteLater();
}


void
TagDialog::checkModified() //SLOT
{
    bool modified = false;
    
    modified |= kLineEdit_title->text()        != m_metaBundle.title();
    modified |= kLineEdit_artist->text()       != m_metaBundle.artist();
    modified |= kLineEdit_album->text()        != m_metaBundle.album();
    modified |= kComboBox_genre->currentText() != m_metaBundle.genre();
    modified |= kIntSpinBox_track->value()     != m_metaBundle.track().toInt();
    modified |= kIntSpinBox_year->value()      != m_metaBundle.year().toInt();
    modified |= kLineEdit_comment->text()      != m_metaBundle.comment();
    
    pushButton_ok->setEnabled( modified );
}


void
TagDialog::musicbrainzQuery() //SLOT
{
#ifdef HAVE_MUSICBRAINZ
    kdDebug() << k_funcinfo << endl;
    
    MusicBrainzQuery* query = new MusicBrainzQuery( MusicBrainzQuery::File, m_metaBundle.url().path() );
    connect( query, SIGNAL( signalDone( const MusicBrainzQuery::TrackList& ) ),
              this,   SLOT( queryDone( const MusicBrainzQuery::TrackList& ) ) );
    
    if ( !query->start() ) return;

    pushButton_musicbrainz->setEnabled( false );
    pushButton_musicbrainz->setText( i18n( "Working.." ) );
#endif
}


void
TagDialog::queryDone( const MusicBrainzQuery::TrackList& tracklist ) //SLOT
{
#ifdef HAVE_MUSICBRAINZ
    kdDebug() << k_funcinfo << endl;
    
    pushButton_musicbrainz->setEnabled( true );
    pushButton_musicbrainz->setText( m_buttonMbText );

    if ( tracklist.isEmpty() ) {
        KMessageBox::sorry( this, i18n( "Track was not found in MusicBrainz database." ) );
        return;
    }
                
    if ( !tracklist[0].name.isEmpty() )     kLineEdit_title->setText( tracklist[0].name );
    if ( !tracklist[0].artist.isEmpty() )   kLineEdit_artist->setText( tracklist[0].artist );
    if ( !tracklist[0].album.isEmpty() )    kLineEdit_album->setText( tracklist[0].album );
#endif
}


////////////////////////////////////////////////////////////////////////////////
// PRIVATE
////////////////////////////////////////////////////////////////////////////////


#include "tagdialog.moc"

// (c) 2004 Mark Kretschmann <markey@web.de>
// See COPYING file for licensing information.

#include "metabundle.h"
#include "tagdialog.h"

#include <qpushbutton.h>

#include <kdebug.h>
#include <kglobal.h>
#include <klineedit.h>


TagDialog::TagDialog( const MetaBundle& mb, QWidget* parent )
    : TagDialogBase( parent )
    , m_metaBundle( mb )
{
    kLineEdit_title->setText( mb.title() );
    kLineEdit_artist->setText( mb.artist() );
    kLineEdit_album->setText( mb.album() );
    kLineEdit_genre->setText( mb.genre() );
    kLineEdit_year->setText( mb.year() );
    kLineEdit_comment->setText( mb.comment() );
    kLineEdit_length->setText( mb.prettyLength() );
    kLineEdit_bitrate->setText( mb.prettyBitrate() );
    kLineEdit_samplerate->setText( mb.prettySampleRate() );
    kLineEdit_location->setText( mb.url().isLocalFile() ? mb.url().path() : mb.url().url() );

    connect( pushButton_cancel, SIGNAL( clicked() ), this, SLOT( deleteLater() ) );
    connect( pushButton_ok, SIGNAL( clicked() ), this, SLOT( okPressed() ) );
    
#ifdef HAVE_MUSICBRAINZ
    connect( pushButton_musicbrainz, SIGNAL( clicked() ), this, SLOT( musicbrainzQuery() ) );
#endif
#ifndef HAVE_MUSICBRAINZ
    pushButton_musicbrainz->setEnabled( false );
#endif
}


////////////////////////////////////////////////////////////////////////////////
// PRIVATE SLOTS
////////////////////////////////////////////////////////////////////////////////

void
TagDialog::okPressed() //SLOT
{
    kdDebug() << k_funcinfo << endl;
    
    deleteLater();
}


#ifdef HAVE_MUSICBRAINZ
void
TagDialog::musicbrainzQuery() //SLOT
{
    kdDebug() << k_funcinfo << endl;
    
    pushButton_musicbrainz->setEnabled( false );
    pushButton_musicbrainz->setText( i18n( "Working.." ) );
    
    MusicBrainzQuery* query = new MusicBrainzQuery( MusicBrainzQuery::File, m_metaBundle.url().path() );
    connect( query, SIGNAL( signalDone( const MusicBrainzQuery::TrackList& ) ),
              this,   SLOT( queryDone( const MusicBrainzQuery::TrackList& ) ) );
    
    query->start();
}


void
TagDialog::queryDone( const MusicBrainzQuery::TrackList& tracklist ) //SLOT
{
    kdDebug() << k_funcinfo << endl;
    
    pushButton_musicbrainz->setEnabled( true );
    pushButton_musicbrainz->setText( i18n( "MusicBrainz" ) );
    
    kLineEdit_title->setText( tracklist[0].name );
    kLineEdit_artist->setText( tracklist[0].artist );
    kLineEdit_album->setText( tracklist[0].album );
    kLineEdit_length->setText( tracklist[0].duration );
}
#endif


#include "tagdialog.moc"

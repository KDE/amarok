//
// Author: Max Howell (C) Copyright 2004
//
// Copyright: See COPYING file that comes with this distribution
//

#include "metabundle.h"
#include "playlistitem.h"
#include "threadweaver.h"

#include <kdebug.h>
#include <kapplication.h>
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/tstring.h>


ThreadWeaver::ThreadWeaver( QWidget *w )
  : m_parent( w )
  , m_bool( true )
  , m_currentJob( 0 )
{}

void
ThreadWeaver::append( Job* const job, bool priorityJob )
{
   //intended to be used by GUI thread, but is thread-safe

    mutex.lock();
    if( priorityJob )
        m_Q.prepend( job );
    else
        m_Q.append( job );
    mutex.unlock();

    if( !running() )
    {
        start( QThread::LowestPriority );
    }
}

bool
ThreadWeaver::remove( Job* const job )
{
    //TODO you need to be able to only remove jobs of a certain type frankly.
    //TODO this is no good when you say need to remove a job that will act on a playlistitem etc.
    //maybe above is void* and you make operator== pure virtual?

    bool b;

    mutex.lock();
    //TODO we delete or user deletes is yet undecided
    b = m_Q.remove( job );
    mutex.unlock();

    //TODO inform users of thread that you have to postpone deletion of stuff that may be in event loop
    return b;
}

void
ThreadWeaver::cancel()
{
    m_Q.setAutoDelete( true );
    mutex.lock();
    m_currentJob = 0; //FIXME will never be deleted!
    m_Q.clear();
    mutex.unlock();
    m_Q.setAutoDelete( false );

    //TODO inform users of thread that you have to postpone deletion of stuff that may be in event loop
}

void
ThreadWeaver::run()
{
    msleep( 200 ); //this is an attempt to encourage the queue to be filled with more than 1 item before we
                   //start processing, and thus prevent unecessary stopping and starting of the thread

    kdDebug() << "[weaver] Started..\n";
    QApplication::postEvent( m_parent, new QCustomEvent( ThreadWeaver::Started ) );

    while( m_bool )
    {
        mutex.lock();
        if( m_Q.isEmpty() ) { mutex.unlock(); break; } //we exit the loop here
        m_currentJob = m_Q.getFirst();
        m_Q.removeFirst();
        mutex.unlock();

        bool b = m_currentJob->doJob();

        mutex.lock();
        if( m_currentJob )
        {
            if( b ) m_currentJob->postJob(); //Qt will delete the job for us
            else    delete m_currentJob;     //we need to delete the job
        }
        mutex.unlock();
    }

    kdDebug() << "[weaver] Done!\n";
    QApplication::postEvent( m_parent, new QCustomEvent( ThreadWeaver::Done ) );
}



ThreadWeaver::Job::Job( QObject *obj, JobType type )
  : QCustomEvent( type )
  , m_target( obj )
{}

inline void
ThreadWeaver::Job::postJob()
{
    QApplication::postEvent( m_target, this );
}



TagReader::TagReader( QObject *o, PlaylistItem *pi )
   : Job( o, Job::TagReader )
   , m_item( pi )
   , m_url( pi->url() )
   , m_tags( 0 )
{}

TagReader::~TagReader()
{
    delete m_tags;
}

bool
TagReader::doJob()
{
    if( m_url.protocol() == "file" )
    {
        m_tags = readTags( m_url );
        return true;
    }

    return false;
}

MetaBundle*
TagReader::readTags( const KURL &url, bool readAudioProps ) //STATIC
{
   //audioproperties are read on demand
   TagLib::FileRef f( url.path().local8Bit(), readAudioProps ); //this is the slow step

   return f.isNull()? 0 : new MetaBundle( url, f.tag(), f.audioProperties() );
}

void
TagReader::bindTags()
{
   //for GUI access only
   //we're a friend of PlaylistItem
   if( m_tags )
       m_item->setText( *m_tags );
}

void
TagReader::addSearchTokens( QStringList &tokens, QPtrList<QListViewItem> &ptrs )
{
    //for GUI access only
    //we need to add this item to the search-index

    QString s = m_item->trackName();

    if( m_tags )
    {
        s += ' ';
        s += m_tags->m_artist;
        s += ' ';
        s += m_tags->m_title;
    }

    tokens.append( s );
    ptrs.append( m_item );
}



AudioPropertiesReader::AudioPropertiesReader( QObject *o, PlaylistItem *pi )
   : Job( o )
   , m_item( pi )
   , m_listView( static_cast<QListViewItem *>(pi)->listView() )
   , m_url( pi->url() )
{
    //TODO derive this from TagReader?
}

bool
AudioPropertiesReader::doJob()
{
    //TODO it is probably safer to record the itemPos in the ctor and check here with listview->itemAt()
    //TODO it is probably more efficient to do this with less specifity, i.e. when view scrolls put a job
    //     in to read tags for visible items, and then get this function to get the visible items for that moment

    int y  = m_item->itemPos(); //const //FIXME slow function!
    int h  = m_item->height();  //const
    int y2 = m_listView->contentsY(); //TODO find out the performance of this function
    if( (y + h) >= y2 && y <= (y2 + m_listView->visibleHeight()) )
    {
        //This is a quick scan
        //A more accurate scan is done when the track is played, and those properties are recorded with the track
        TagLib::FileRef f( m_url.path().local8Bit(), true, TagLib::AudioProperties::Fast );
        int length  = MetaBundle::Unavailable;
        int bitrate = MetaBundle::Unavailable;

        if( !f.isNull() )
        {
            //FIXME do we need to check the ap pointer? Seems to have not crashed so far..
            TagLib::AudioProperties *ap = f.audioProperties();
            length  = ap->length();
            bitrate = ap->bitrate();
        }

        m_length  = MetaBundle::prettyLength( length );
        m_bitrate = MetaBundle::prettyBitrate( bitrate );

        return true;
    }

    return false;
}

void
AudioPropertiesReader::completeJob()
{
    //TODO do in playlistItem class or at least enum these numbers!
    m_item->setText(  9, m_length );
    m_item->setText( 10, m_bitrate );
}



TagWriter::TagWriter( QObject *o, PlaylistItem *pi, const QString &s, const int col )
  : Job( o )
  , m_item( pi )
  , m_tagString( s )
  , m_tagType( col )
{
    //TODO deepcopy?
    //TODO leave a temp message in the listview item until this completes
    //TODO use a enum for TagType
    pi->setText( col, i18n( "Writing tag..." ) );
}

bool
TagWriter::doJob()
{
    const KURL url = m_item->url(); //FIXME safe?
    TagLib::FileRef f( url.path().local8Bit(), false );

    if( !f.isNull() )
    {
        TagLib::Tag *t = f.tag();
        TagLib::String s = QStringToTString( m_tagString );

        switch( m_tagType ) {
        case 1:
            t->setTitle( s );
            break;
        case 2:
            t->setArtist( s );
            break;
        case 3:
            t->setAlbum( s );
            break;
        case 4:
            t->setYear( m_tagString.toInt() );
            break;
        case 5:
            //FIXME how does this work for vorbis files?
            //Are we likely to overwrite some other comments?
            //Vorbis can have multiple comment fields..
            t->setComment( s );
            break;
        case 6:
            t->setGenre( s );
            break;
        case 7:
            t->setTrack( m_tagString.toInt() );
            break;
        default:
            return false;
        }

        f.save();
    }

    //FIXME can TagLib::Tag::save() not fail?
    return true;
}

void
TagWriter::completeJob()
{
    //FIXME see PlaylistItem::setText() for an explanation for this hack
    m_item->setText( m_tagType, m_tagString.isEmpty() ? " " : m_tagString );
}



PLStats::PLStats( QObject *o, const KURL &u, const KURL::List &ul )
  : Job( o, Job::PLStats )
  , m_url( u )
  , m_contents( ul )
  , m_length( 0 )
{}

bool
PLStats::doJob()
{
    //TODO update playlistItems too
    //TODO find out if tags are read on creation of the file ref and if so, update them too
    //TODO currently this is horrendously inefficient, tags are read so many times!

    for( KURL::List::const_iterator it = m_contents.begin(); it != m_contents.end(); ++it )
    {
        TagLib::FileRef f( (*it).path().local8Bit(), true, TagLib::AudioProperties::Accurate );

        if( !f.isNull() ) m_length += f.audioProperties()->length();
    }

    return true;
}

//yes! no moc! There are no Q_OBJECTS

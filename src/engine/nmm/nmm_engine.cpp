// Copyright (c) Max Howell 2004
// Licensed under GPL v2
// "lack of pussy makes you brave!"

#include "nmm_engine.h"
#ifdef HAVE_NMM

//#include "playerapp.h" //FIXME bah!
#include "plugin/plugin.h"

#include <nmm/base/connect.hpp>
#include <nmm/base/EDObject.hpp> //for the badass TEDObject template
#include <nmm/plugins/file/GenericReadNode.hpp>
#include <nmm/plugins/file/mpeg/MP3ReadNode.hpp>
#include <nmm/plugins/audio/mpegdec/MPEGAudioDecodeNode.hpp>
#include <nmm/plugins/audio/oggvorbis/OggVorbisDecodeNode.hpp>
#include <nmm/plugins/audio/PlaybackNode.hpp>

#include <nmm/interfaces/general/progress/IProgressListener.hpp>
#include <nmm/interfaces/audio/oggvorbis/IOggVorbis.hpp> //dunno why we need this

#include <iostream>
#include <kurl.h>



//TODO make members
NMM::GenericSourceNode    *readfile = 0; //mp3readnode and genericreadnode derive from this
NMM::GenericProcessorNode *decode   = 0;
NMM::PlaybackNode         *playback = 0;



AMAROK_EXPORT_PLUGIN( NmmEngine )


NmmEngine::NmmEngine()
  : Engine::Base()
  , m_firstTime( TRUE )
  , m_state( Engine::Empty )
  , m_lastKnownPosition( 0 )
{
    //NamedObject::getGlobalInstance().setErrorStream(NULL, NamedObject::ALL_LEVELS);
    NMM::NamedObject::getGlobalInstance().setDebugStream(NULL, NMM::NamedObject::ALL_LEVELS);
    NMM::NamedObject::getGlobalInstance().setWarningStream(NULL, NMM::NamedObject::ALL_LEVELS);
    //NamedObject::getGlobalInstance().setMessageStream(NULL, NamedObject::ALL_LEVELS);

//TODO dynamically load nmm libraries, makes life simpler (well simpler afterwards)
//    pm.loadLibrary("libnmmoggvorbis.so");
}

NmmEngine::~NmmEngine()
{
    stop();

    delete readfile;
    delete decode;
    delete playback;
}



Engine::State
NmmEngine::state() const
{
    //return readfile->isStarted() ? EngineBase::Playing : EngineBase::Paused;
    //return !playback->isStarted() ? EngineBase::Idle : EngineBase::Playing;
    return m_state;
}



bool
NmmEngine::load( const KURL& url, bool stream )
{
    m_isStream = stream;
    if( !url.isLocalFile() ) return 0; //FIXME

    stop(); //NOTE essential!!

    delete playback;
    delete decode;
    delete readfile;

    /* Some detail:
     * There are three nodes, a file reader, a decoder and a playback node
     * There are several states, init, initOutput, activate and start
     * You have to do everything in the right order or it crashes
     * You can only really manipulate the nodes by getting interfaces from them
     * Use exceptions! They are quite handy
     */

    try
    {
        //FIXME it should _not_ be necessary to do all this everytime! but I can't stop the crashes when I try :(

        QString filename = url.path();

        if( filename.endsWith( "mp3" ) )
        {
            readfile = new NMM::MP3ReadNode(); //what advantages do we get with this node over generic?
            decode   = new NMM::MPEGAudioDecodeNode("MP3DECODE", NMM::StreamQueue::MODE_SUSPEND, 1);
        }
        else if( filename.endsWith( "ogg" ) )
        {
            readfile = new NMM::GenericReadNode();
            decode   = new NMM::OggVorbisDecodeNode();
        }
        else
        {
            readfile = 0; decode = 0;
            return 0;
        }

        //TODO you don't have to create a new plyaback object but you tried to do it with deinitOutput() etc.
        //     but it just crashed, and crashed and crashed and crashed and you gave up.
        playback = new NMM::PlaybackNode("PLAYBACK", NMM::StreamQueue::MODE_SUSPEND, 1); //FIXME what do these parameters mean?!

        readfile->init();
        decode->init();
        playback->init();

        //grab the progress information interface from the readfile node
        NMM::IProgress *progress = readfile->getInterface<NMM::IProgress>();
        //get it to send us progress events periodically
        if( progress ) progress->sendProgressInformation( true );
        //register an event listener on the readfile interface, the listener is setProgress //FIXME rename
        playback->getEventDispatcher().
                registerEventListener( NMM::IProgressListener::setProgress_event,
                                        new NMM::TEDObject2<NmmEngine, u_int64_t, u_int64_t>( this, &NmmEngine::setProgress ) );
        playback->getEventDispatcher().
                registerEventListener( NMM::ITrack::endTrack_event,
                                        new NMM::TEDObject0<NmmEngine>( this, &NmmEngine::endTrack ) );

        NMM::IFileHandler_var ifile( readfile->getCheckedInterface<NMM::IFileHandler>() );
        ifile->setFilename( filename.ascii() );

        //get the INode interfaces, we need to connect these together
        NMM::INode_var iread( readfile->getInterface<NMM::INode>() );
        NMM::INode_var idecode( decode->getCheckedInterface<NMM::INode>() );
        NMM::INode_var iplay( playback->getInterface<NMM::INode>() );

        //init output parts of nodes and connect nodes into output chain
        readfile->initOutput();
        NMM::connect(iread, idecode);
        readfile->activate();

        decode->initOutput();
        NMM::connect(idecode, iplay);
        decode->activate();

        playback->initOutput();
        playback->activate();

    }
    catch( NMM::Exception e ) { std::cerr << e << endl; return false; }

    return true;
}

bool
NmmEngine::play( uint offset )
{
    if (offset != 0) seek(offset);
    try {
        readfile->start();
        decode->start();
        playback->start();
    }
    catch( NMM::Exception e ) { std::cerr << e << endl; return false; }

    //FIXME test for playback first
    m_state = Engine::Playing;
    return true;
}

void
NmmEngine::stop()
{
    try
    {
        if( readfile && readfile->isStarted() )
        {
            readfile->stop();
            readfile->flush();
        }
        if( decode   && decode->isStarted() )
        {
            decode->stop();
            decode->flush();
        }
        if( playback && playback->isStarted() )
        {
            playback->stop();
            playback->flush();
        }

        m_state = Engine::Idle;
    }
    catch( NMM::Exception e )
    {
        std::cerr << e << endl;

        m_state = Engine::Empty;
    }

    m_lastKnownPosition = 0;
}

void
NmmEngine::pause()
{
    stop();

    m_state = Engine::Paused;
}

void
NmmEngine::seek( uint ms )
{
    NMM::ISeekable *iseek = readfile->getCheckedInterface<NMM::ISeekable>();

    //if( iseek ) iseek->seekPercentTo( NMM::Rational(ms, pApp->trackLength()) );

}



uint
NmmEngine::position() const
{
    return m_lastKnownPosition;
}


bool
NmmEngine::initMixer( bool hardware )
{ return true; }

bool
NmmEngine::canDecode( const KURL &url ) const
{
    QString filename = url.path();

    if( filename.endsWith( "mp3" ) )
        return true;
    else if( filename.endsWith( "ogg" ) )
        return true;
    else
        return false;
}


void
NmmEngine::setVolumeSW( uint percent )
{
    m_volume = percent;
    if( playback ) playback->setLineInVolume( 0x5050 * percent / 100 );
}



NMM::Result
NmmEngine::setProgress( u_int64_t& numerator, u_int64_t& denominator )
{
    std::cout << numerator << ", " << denominator << endl;

    //m_lastKnownPosition = pApp->trackLength() * (double)numerator / denominator;

    return NMM::SUCCESS;
}

NMM::Result
NmmEngine::endTrack()
{
    std::cout << "Track ended!\n";

    //NOTE calling stop() here cause amaroK to crash. Why? Who knows..
    //NMM docs certainly wouldn't be able to tell you!

    m_state = Engine::Idle;

    return NMM::SUCCESS;
}

#include "nmm_engine.moc"

#endif

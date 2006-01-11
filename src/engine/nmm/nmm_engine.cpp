/* NMM - Network-Integrated Multimedia Middleware
 *
 * Copyright (C) 2002-2004
 *                    NMM work group,
 *                    Computer Graphics Lab,
 *                    Saarland University, Germany
 *                    http://www.networkmultimedia.org
 *
 * Maintainer:        Wolfram von Funck <wolfram@graphics.cs.uni-sb.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307
 * USA
 */

#include "nmm_engine.h"

#ifdef HAVE_NMM

#include "debug.h"
#include "plugin/plugin.h"

#include <nmm/base/graph/GraphBuilder2.hpp>
#include <nmm/base/registry/NodeDescription.hpp>
#include <nmm/base/ProxyApplication.hpp>
#include <nmm/interfaces/base/sync/ISynchronizedSink.hpp>
#include <nmm/interfaces/file/ISeekable.hpp>
#include <nmm/interfaces/file/ITrack.hpp>
#include <nmm/interfaces/file/IBufferSize.hpp>
#include <nmm/interfaces/general/progress/IProgressListener.hpp>
#include <nmm/interfaces/general/progress/IProgress.hpp>
#include <nmm/interfaces/general/ITrackDuration.hpp>
#include <nmm/interfaces/device/audio/IAudioDevice.hpp>
#include <nmm/base/ProxyObject.hpp>

#include <qapplication.h>

#include <kfileitem.h>
#include <kmimetype.h>
#include <iostream>
#include <kurl.h>

AMAROK_EXPORT_PLUGIN( NmmEngine )

NmmEngine::NmmEngine()
  : Engine::Base(),
    __position(0),
    __track_length(0),
    __state(Engine::Empty),
    __app(0),
    __endTrack_listener(this, &NmmEngine::endTrack),
    __syncReset_listener(this, &NmmEngine::syncReset),
    __setProgress_listener(this, &NmmEngine::setProgress),
    __trackDuration_listener(this, &NmmEngine::trackDuration),
    __composite(0),
    __playback(0),
    __seeking(false),
    __running(true),
    __cond(__mutex)
{
  // start the secondary thread
  activateThread();
}

bool NmmEngine::init()
{
  // disable debug and warning streams
  NamedObject::getGlobalInstance().setDebugStream(NULL, NamedObject::ALL_LEVELS);
  NamedObject::getGlobalInstance().setWarningStream(NULL, NamedObject::ALL_LEVELS);

  // create new NMM application object
  __app = ProxyApplication::getApplication(0, 0);

  return true;
}

NmmEngine::~NmmEngine()
{
  // stop the secondary thread
  __mutex.lock();
  __running = false;
  __cond.notify();
  __mutex.unlock();

  // stop all nodes
  stop();

  // delete application object
  if (__app) {
    delete __app;
  }
}

Engine::State NmmEngine::state() const
{
  return __state;
}

bool NmmEngine::load(const KURL& url, bool stream)
{
  DEBUG_BLOCK

  // play only local files
  if (!url.isLocalFile()) return false;

  Engine::Base::load(url, stream);

  cleanup();

  // make the GraphBuilder construct an appropriate graph for the given URL
  try {
    // these nodes will be used for audio and video playback
    NodeDescription playback_nd("PlaybackNode");
    NodeDescription display_nd("XDisplayNode");

    GraphBuilder2 gb;

    // convert the URL to a valid NMM url
    if(!gb.setURL("file://" + string(url.path().ascii()))) {
      throw Exception("Invalid URL given");
    }

    // get a playback node interface from the registry
    ClientRegistry& registry = __app->getRegistry();
    {
      RegistryLock lock(registry);
      list<Response> playback_response = registry.initRequest(playback_nd);

      if (playback_response.empty()) {
	throw Exception("PlaybackNode is not available");
      }
      
      __playback = registry.requestNode(playback_response.front());
    }

    // initialize the GraphBuilder
    gb.setAudioSink(__playback);
    gb.setVideoSink(display_nd);
    gb.setDemuxAudioJackTag("audio");
    gb.setDemuxVideoJackTag("video");

    // create the graph represented by a composite node
    __composite = gb.createGraph(*__app);

    // register the needed event listeners at the playback node
   
    __playback->getParentObject()->registerEventListener(IProgressListener::setProgress_event, 
							 &__setProgress_listener);

    __playback->getParentObject()->registerEventListener(ITrack::endTrack_event, 
							 &__endTrack_listener);

    __playback->getParentObject()->registerEventListener(ISyncReset::syncReset_event, 
							 &__syncReset_listener);

    __playback->getParentObject()->registerEventListener(ITrackDuration::trackDuration_event, &__trackDuration_listener);

    // Tell the node that implements the IProgress interface to send progress events frequently.
    IProgress_var progress(__composite->getInterface<IProgress>());
    if (progress.get()) {
      progress->sendProgressInformation(true);
      progress->setProgressInterval(1);
    }

    // minimize the buffer size to increase the frequency of progress events
    IBufferSize_var buffer_size(__composite->getInterface<IBufferSize>());
    if (buffer_size.get()) {
      buffer_size->setBufferSize(1000);
    }

    // we don't know the track length yet - we have to wait for the trackDuration event
    __track_length = 0;

    // finally start the graph
    if(__playback->isActivated()) {
      __playback->reachStarted();
    }
      
    __composite->reachStarted();
      
    __seeking = false;
    __state = Engine::Playing;
  }
  catch (const Exception& e) {
    cerr << e << endl;
  }
  catch(...) {
    cerr << "something went wrong..." << endl;
  }

  return true;
}

bool NmmEngine::play(uint)
{
  DEBUG_BLOCK

  if (!__composite) {
    return false;
  }

  // wake up if paused
  ISynchronizedSink_var sync_sink(__playback->getParentObject()->getCheckedInterface<ISynchronizedSink>());
  if (sync_sink.get()) {
    sync_sink->getController()->wakeup();
  }

  __state = Engine::Playing;
  emit stateChanged(Engine::Playing);

  return true;
}

void NmmEngine::cleanup()
{
  DEBUG_BLOCK

  if (!__composite) {
    return;
  }

  // remove all event listeners
  __playback->getParentObject()->removeEventListener(&__setProgress_listener);
  __playback->getParentObject()->removeEventListener(&__endTrack_listener);
  __playback->getParentObject()->removeEventListener(&__syncReset_listener);
  __playback->getParentObject()->removeEventListener(&__trackDuration_listener);

  // stop the graph
  __composite->reachActivated();

  if(__playback->isStarted()) {
    __playback->reachActivated();
  }

  __composite->flush();
  __composite->reachConstructed();

  __playback->flush();
  __playback->reachConstructed();

  // release the playback node
  ClientRegistry& registry = __app->getRegistry();
  {
    RegistryLock lock(registry);
    registry.releaseNode(*__playback);
  }

  delete __composite;
  __composite = 0;
  __playback = 0;

  __position = 0;

  __state = Engine::Idle;
}

void NmmEngine::stop()
{
  DEBUG_BLOCK

  cleanup();

  __state = Engine::Empty;
  emit stateChanged(Engine::Empty);
}

void NmmEngine::pause()
{
  if (!__composite) {
    return;
  }

  // pause or play...
  if (__state == Engine::Playing) {
    ISynchronizedSink_var sync_sink(__playback->getParentObject()->getCheckedInterface<ISynchronizedSink>());
    if (sync_sink.get()) {
      sync_sink->getController()->pause();
    }
    __state = Engine::Paused;
    emit stateChanged(Engine::Paused);
  }
  else {
    play();
  }
}

void NmmEngine::seek(uint ms)
{
  if (!__track_length) {
    return;
  }

  __seeking = true;
  __position = ms;

  ISeekable_var seek(__composite->getCheckedInterface<ISeekable>());
  if (seek.get()) {
    seek->seekPercentTo(Rational(ms, __track_length));
  }
}

uint NmmEngine::position() const
{
  return __position;
}

bool NmmEngine::canDecode(const KURL& url) const
{
    static QStringList types;

    if (url.protocol() == "http" ) return false; 

    // the following MIME types can be decoded
    types += QString("audio/x-mp3");
    types += QString("audio/x-wav");
    types += QString("audio/ac3");
    types += QString("audio/vorbis");
    types += QString("video/mpeg");
    types += QString("video/x-msvideo");
    types += QString("video/x-ogm");

    KFileItem fileItem( KFileItem::Unknown, KFileItem::Unknown, url, false ); //false = determineMimeType straight away
    KMimeType::Ptr mimetype = fileItem.determineMimeType();

    return types.contains(mimetype->name());
}


void NmmEngine::setVolumeSW(uint percent)
{
  strstream str;
  str << "/usr/bin/aumix -v " << percent;
  char s[50];
  str.getline(s, 50);
  system(s);
}



Result NmmEngine::setProgress(u_int64_t& numerator, u_int64_t& denominator)
{
  // compute the track position in milliseconds
  u_int64_t position = numerator * __track_length / denominator;

  if (__seeking) {
    return SUCCESS;
  }
  
  __position = position;

  return SUCCESS;
}

void NmmEngine::run()
{
  // this is the secondary thread that is used to emit the trackEnded signal
  MutexGuard mg(__mutex);
  while (__running) {
    __cond.wait();
    if (__running) {
      QApplication::postEvent(this, new QCustomEvent(3000) );
    }
  }
}

Result NmmEngine::endTrack()
{
  __state = Engine::Idle;
  __position = 0;

  // make the secondary thread emit the trackEnded signal
  __mutex.lock();
  __cond.notify();
  __mutex.unlock();

  return SUCCESS;
}

void NmmEngine::customEvent( QCustomEvent *e )
{
    switch( e->type() )
    {
        case 3000:
            // emit signal from GUI thread
            emit trackEnded();
            break;

        default:
            ;
    }
}

Result NmmEngine::syncReset()
{
  __seeking = false;
  return SUCCESS;
}

Result NmmEngine::trackDuration(Interval& duration)
{
  // we got the duration of the track, so let's convert it to milliseconds
  __track_length = duration.sec * 1000 + duration.nsec / 1000;
  return SUCCESS;
}

#include "nmm_engine.moc"

#endif

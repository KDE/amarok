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


#ifndef NMM_ENGINE_H
#define NMM_ENGINE_H

#include <config.h>

#ifdef HAVE_NMM
#include "engine/enginebase.h"
#include <nmm/base/graph/CompositeNode.hpp>
#include <nmm/base/NMMApplication.hpp>
#include <nmm/base/EDObject.hpp>

using namespace NMM;

class NmmEngine : public Engine::Base, public ThreadedObject
{
Q_OBJECT
public:
    NmmEngine();
    ~NmmEngine();

    bool init();
    bool canDecode(const KURL&) const;
    uint position() const;

    Engine::State state() const;

public slots:
    bool  load(const KURL&, bool stream = false);
    bool  play(unsigned int offset = 0);
    void  stop();
    void  pause();

    void  seek(uint);
    void  setVolumeSW(uint = 0);

private:
    void cleanup();

    /**
     * This method is called when a setProgress event is received. The two parameters represent a rational number
     * (numerator and denominator) containing the amount of progress as a value between 0 and 1.
     */
    Result setProgress(u_int64_t&, u_int64_t&);

    /**
     * This method is called when an endTrack event is received.
     */
    Result endTrack();

    /**
     * This method is called when a syncReset event is received. When an NMM source node has finished seeking, such an event is
     * sent. Here, it is used to prevent the engine from updating the track position while receiving setProgress events before seeking
     * is done, since these setProgress events may contain old progress information. Otherwise, the progress slider would jump
     * back and forth...
     */
    Result syncReset();

    /**
     * This method is called when a trackDuration event is received. The duration of a track is represented by
     * an Interval that contains the time in seconds and nanoseconds.
     */
    Result trackDuration(Interval& duration);

    /**
     * Overwritten run method from ThreadedObject. A secondary thread is needed to avoid a deadlock that would be caused
     * by emitting a trackEnded signal when an endTrack event is received: The emission of a trackEnded signal makes
     * all NMM nodes stop, which is not possible before the endTrack event has been processed. Thus, the trackEnded signal
     * is emitted from a secondary thread, allowing the endTrack method to return.
     */
    void run();

    void customEvent( QCustomEvent* );

    /**
     * The current track position in milliseconds.
     */
    u_int64_t __position;

    /**
     * The length of the track in milliseconds.
     */
    u_int64_t __track_length;

    /**
     * The current engine state
     */
    Engine::State __state;

    /**
     * The NMM application object.
     */
    NMMApplication* __app;

    /**
     * Event listeners for various NMM events.
     */
    TEDObject0<NmmEngine> __endTrack_listener;
    TEDObject0<NmmEngine> __syncReset_listener;
    TEDObject2<NmmEngine, u_int64_t, u_int64_t> __setProgress_listener;
    TEDObject1<NmmEngine, Interval> __trackDuration_listener;

    /**
     * The composite node that contains the graph created by the GraphBuilder.
     */
    CompositeNode* __composite;

    /**
     * The playback node where the various events like endTrack, setProgress etc. are caught.
     */
    INode* __playback;

    /**
     * This flag is set during seeking.
     */
    bool __seeking;

    /**
     * This flag is set while the secondary thread is running.
     */
    bool __running;

    /**
     * Mutex used for thread synchronization.
     */    
    ThreadMutex __mutex;

    /**
     * Condition variable used for thread synchronization.
     */
    ThreadCondition __cond;
};

#endif
#endif

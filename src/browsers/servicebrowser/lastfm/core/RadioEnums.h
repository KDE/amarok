/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Erik Jalevik, Last.fm Ltd <erik@last.fm>                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef RADIOENUMS_H
#define RADIOENUMS_H

#include "WebService/fwd.h"

/*************************************************************************/ /**
    State enum used by all radio classes and ultimately emitted from the
    Radio class to the outside world.
    
    As they all use a subset of these states, it was easier to separate it
    out into its own header rather than declare a special State enum inside
    each class.
    
    These are probably in sequential order --mxcl
******************************************************************************/
enum RadioState
{
    State_Uninitialised = 0,
    State_Handshaking,
    State_Handshaken,
    State_ChangingStation,
    State_FetchingPlaylist,
    State_FetchingStream, /** it's like requesting the start of the streaming */
    State_StreamFetched,  /** server responded ok, start buffering */
    State_Buffering,
    State_Streaming,
    State_Skipping,
    State_Stopping,
    State_Stopped
};

/*********************************************************************/ /**
    Translate RadioStates to strings for debugging purposes.
**************************************************************************/
inline QString radioState2String( RadioState state )
{
    switch ( state )
    {
        case State_Uninitialised: return "State_Uninitialised";
        case State_Handshaking: return "State_Handshaking";
        case State_Handshaken: return "State_Handshaken";
        case State_ChangingStation: return "State_ChangingStation";
        case State_FetchingPlaylist: return "State_FetchingPlaylist";
        case State_FetchingStream: return "State_FetchingStream";
        case State_StreamFetched: return "State_StreamFetched";
        case State_Buffering: return "State_Buffering";
        case State_Streaming: return "State_Streaming";
        case State_Skipping: return "State_Skipping";
        case State_Stopping: return "State_Stopping";
        case State_Stopped: return "State_Stopped";
        default: Q_ASSERT( !"Unhandled state" ); return "";
    }
}

// This enum extends the WebRequestResult enum so that we can reuse
// the error codes in our error signal.
enum RadioError
{
    Radio_BadPlaylist = WebRequestResult_Custom + 1,
    Radio_InvalidUrl,
    Radio_InvalidAuth,
    Radio_TooManyRetries,
    Radio_TrackNotFound,
    Radio_SkipLimitExceeded,
    Radio_IllegalResume,
    Radio_OutOfPlaylist,
    Radio_PluginLoadFailed,
    Radio_NoSoundcard,
    Radio_PlaybackError,
    Radio_ConnectionRefused,
    Radio_UnknownError
};
    
#endif // RADIOENUMS_H

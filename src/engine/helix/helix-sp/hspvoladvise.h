/*
 *
 * This software is released under the provisions of the GPL version 2.
 * see file "COPYING".  If that file is not available, the full statement 
 * of the license can be found at
 *
 * http://www.fsf.org/licensing/licenses/gpl.txt
 *
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved.
 * Portions (c) Paul Cifarelli 2005
 *
 */

#ifndef _HSPVOLADVISE_INCLUDED_
#define _HSPVOLADVISE_INCLUDED_

class HelixSimplePlayerVolumeAdvice : public IHXVolumeAdviseSink
{
public:
   HelixSimplePlayerVolumeAdvice(HelixSimplePlayer *player, int playerIndex) : m_Player(player),m_index(playerIndex),m_lRefCount(0) {}
   virtual ~HelixSimplePlayerVolumeAdvice() {}

   /*
    *  IUnknown methods
    */
   STDMETHOD(QueryInterface)   (THIS_
                               REFIID riid,
                               void** ppvObj);
    
   STDMETHOD_(ULONG32,AddRef)  (THIS);

   STDMETHOD_(ULONG32,Release) (THIS);

   /*
    * IHXVolumeAdviceSink methods
    */
   STDMETHOD(OnVolumeChange)   (THIS_ 
                                const UINT16 uVolume
                               );
   STDMETHOD(OnMuteChange)     (THIS_
				const BOOL bMute
			       );
   
private:
   HelixSimplePlayerVolumeAdvice();
   HelixSimplePlayer *m_Player;
   int                m_index;
   LONG32             m_lRefCount;
   LONG32             m_lClientIndex;
};

#endif

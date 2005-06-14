/* **********
 *
 * This software is released under the provisions of the GPL version 2.
 * see file "COPYING".  If that file is not available, the full statement 
 * of the license can be found at
 *
 * http://www.fsf.org/licensing/licenses/gpl.txt
 *
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved.
 * Portions Copyright (c) Paul Cifarelli 2005
 *
 * ********** */
#ifndef _HSPHOOK_H_INCLUDED_
#define _HSPHOOK_H_INCLUDED_


class HSPPostMixAudioHook : public IHXAudioHook
{
public:
   HSPPostMixAudioHook(HelixSimplePlayer *player, int playerIndex);
   virtual ~HSPPostMixAudioHook();
   /*
    *  IUnknown methods
    */
   STDMETHOD(QueryInterface)   (THIS_
                               REFIID riid,
                               void** ppvObj);
   STDMETHOD_(ULONG32,AddRef)  (THIS);
   STDMETHOD_(ULONG32,Release) (THIS);
   /*
    * IHXAudioHook methods
    */
   STDMETHOD(OnBuffer) (THIS_
                        HXAudioData *pAudioInData,
                        HXAudioData *pAudioOutData);
   STDMETHOD(OnInit) (THIS_
                      HXAudioFormat *pFormat);

private:
   HSPPostMixAudioHook();
   HelixSimplePlayer *m_Player;
   LONG32             m_lRefCount;
   int                m_index;
   HXAudioFormat      m_format;
   int                m_count;

#ifdef TEST_APP
   int                buf[MAX_SCOPE_SAMPLES];
#endif
};


#endif

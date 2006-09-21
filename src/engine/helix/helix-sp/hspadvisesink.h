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

#ifndef _HSPADVISESINK_
#define _HSPADVISESINK_

struct IHXClientAdviseSink;
struct IUnknown;
struct IHXRegistry;
struct IHXScheduler;
class HelixSimplePlayer;

class HSPClientAdviceSink : public IHXClientAdviseSink
{
  private:
    HelixSimplePlayer *m_splayer;
    LONG32          m_lRefCount;
    LONG32          m_lClientIndex;
    
    IUnknown*       m_pUnknown;
    IHXRegistry*    m_pRegistry;
    IHXScheduler*   m_pScheduler;
    
    UINT32          m_ulStartTime;
    UINT32          m_ulStopTime;

    UINT32          m_position;
    UINT32          m_duration;
    
    UINT32    m_lCurrentBandwidth;
    UINT32    m_lAverageBandwidth;
    BOOL      m_bOnStop;
    
    HX_RESULT DumpRegTree(const char* pszTreeName, UINT16 index );

    //PRIVATE_DESTRUCTORS_ARE_NOT_A_CRIME

    void GetStatistics (char* pszRegistryKey);
    void GetAllStatistics (void);
 
  public:

    HSPClientAdviceSink(IUnknown* pUnknown, LONG32 lClientIndex, HelixSimplePlayer *pSplay);
    virtual ~HSPClientAdviceSink();

    UINT32 position() { return m_position; }
    UINT32 duration() { return m_duration; }
    UINT32 currentBW() { return m_lCurrentBandwidth; }
    UINT32 averageBW() { return m_lAverageBandwidth; }

    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface) (THIS_
                               REFIID riid,
                               void** ppvObj);
    
    STDMETHOD_(ULONG32,AddRef) (THIS);

    STDMETHOD_(ULONG32,Release) (THIS);

    /*
     *  IHXClientAdviseSink methods
     */

    /************************************************************************
     *  Method:
     *      IHXClientAdviseSink::OnPosLength
     *  Purpose:
     *      Called to advise the client that the position or length of the
     *      current playback context has changed.
     */
    STDMETHOD(OnPosLength) (THIS_
                            UINT32    ulPosition,
                            UINT32    ulLength);
    
    /************************************************************************
     *  Method:
     *      IHXClientAdviseSink::OnPresentationOpened
     *  Purpose:
     *      Called to advise the client a presentation has been opened.
     */
    STDMETHOD(OnPresentationOpened) (THIS);

    /************************************************************************
     *  Method:
     *      IHXClientAdviseSink::OnPresentationClosed
     *  Purpose:
     *      Called to advise the client a presentation has been closed.
     */
    STDMETHOD(OnPresentationClosed) (THIS);

    /************************************************************************
     *  Method:
     *      IHXClientAdviseSink::OnStatisticsChanged
     *  Purpose:
     *      Called to advise the client that the presentation statistics
     *      have changed. 
     */
    STDMETHOD(OnStatisticsChanged) (THIS);

    /************************************************************************
     *  Method:
     *      IHXClientAdviseSink::OnPreSeek
     *  Purpose:
     *      Called by client engine to inform the client that a seek is
     *      about to occur. The render is informed the last time for the 
     *      stream's time line before the seek, as well as the first new
     *      time for the stream's time line after the seek will be completed.
     *
     */
    STDMETHOD (OnPreSeek) (THIS_
                           ULONG32 ulOldTime,
                           ULONG32  ulNewTime);

    /************************************************************************
     *  Method:
     *      IHXClientAdviseSink::OnPostSeek
     *  Purpose:
     *      Called by client engine to inform the client that a seek has
     *      just occurred. The render is informed the last time for the 
     *      stream's time line before the seek, as well as the first new
     *      time for the stream's time line after the seek.
     *
     */
    STDMETHOD (OnPostSeek) (THIS_
                            ULONG32 ulOldTime,
                            ULONG32 ulNewTime);

    /************************************************************************
     *  Method:
     *      IHXClientAdviseSink::OnStop
     *  Purpose:
     *      Called by client engine to inform the client that a stop has
     *      just occurred. 
     *
     */
    STDMETHOD (OnStop) (THIS);

    /************************************************************************
     *  Method:
     *      IHXClientAdviseSink::OnPause
     *  Purpose:
     *      Called by client engine to inform the client that a pause has
     *      just occurred. The render is informed the last time for the 
     *      stream's time line before the pause.
     *
     */
    STDMETHOD (OnPause) (THIS_
                         ULONG32 ulTime);

    /************************************************************************
     *  Method:
     *      IHXClientAdviseSink::OnBegin
     *  Purpose:
     *      Called by client engine to inform the client that a begin or
     *      resume has just occurred. The render is informed the first time 
     *      for the stream's time line after the resume.
     *
     */
    STDMETHOD (OnBegin) (THIS_
                         ULONG32 ulTime);

    /************************************************************************
     *  Method:
     *      IHXClientAdviseSink::OnBuffering
     *  Purpose:
     *      Called by client engine to inform the client that buffering
     *      of data is occurring. The render is informed of the reason for
     *      the buffering (start-up of stream, seek has occurred, network
     *      congestion, etc.), as well as percentage complete of the 
     *      buffering process.
     *
     */
    STDMETHOD (OnBuffering) (THIS_
                             ULONG32 ulFlags,
                             UINT16 unPercentComplete);


    /************************************************************************
     *  Method:
     *      IHXClientAdviseSink::OnContacting
     *  Purpose:
     *      Called by client engine to inform the client is contacting
     *      hosts(s).
     *
     */
    STDMETHOD (OnContacting) (THIS_
                              const char* pHostName);

};

#endif /* _EXAMPLECLSNK_ */

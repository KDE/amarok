/* **********
 *
 * This software is released under the provisions of the GPL version 2.
 * see file "COPYING".  If that file is not available, the full statement 
 * of the license can be found at
 *
 * http://www.fsf.org/licensing/licenses/gpl.txt
 *
 * Copyright (c) Paul Cifarelli 2005
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved.
 * PCM time-domain equalizer:
 *    (c) 2002 Felipe Rivera <liebremx at users sourceforge net>
 *    (c) 2004 Mark Kretschmann <markey@web.de>
 *
 * ********** */
#ifndef _HSPHOOK_H_INCLUDED_
#define _HSPHOOK_H_INCLUDED_

struct GAIN_STATE;

#define FADE_MIN_dB -120

class HSPPreMixAudioHook : public IHXAudioHook
{
public:
   HSPPreMixAudioHook(HelixSimplePlayer *player, int playerIndex, IHXAudioStream *pAudioStream,
                      bool fadein = false, unsigned long fadelength = 0);
   virtual ~HSPPreMixAudioHook();
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

   void setFadeout(bool fadeout);
   void setFadelength(unsigned long fadelength) { m_fadelength = fadelength; }

private:
   HSPPreMixAudioHook();

   HelixSimplePlayer *m_Player;
   LONG32             m_lRefCount;
   int                m_index;
   IHXAudioStream    *m_stream;
   HXAudioFormat      m_format;
   int                m_count;

   GAIN_STATE        *m_gaintool;
   float              m_gaindb;
   bool               m_fadein;
   bool               m_fadeout;
   unsigned long      m_fadelength;

   int volumeize(unsigned char *data, unsigned char *outbuf, size_t len);
};



#define BAND_NUM 10
#define EQ_MAX_BANDS 10
#define EQ_CHANNELS 2 // Helix DNA currently only supports stereo

// Floating point
typedef struct
{
   float beta;
   float alpha;
   float gamma;
} sIIRCoefficients;

/* Coefficient history for the IIR filter */
typedef struct
{
   float x[3]; /* x[n], x[n-1], x[n-2] */
   float y[3]; /* y[n], y[n-1], y[n-2] */
} sXYData;


struct DelayQueue;

class HSPPostProcessor : public IHXAudioHook
{
public:
   HSPPostProcessor(HelixSimplePlayer *player, int playerIndex);
   virtual ~HSPPostProcessor();
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

   void updateEQgains(int preamp, vector<int> &equalizerGains);

   void scopeify(unsigned long time, unsigned char *data, size_t len);

#ifndef HELIX_SW_VOLUME_INTERFACE
   void setGain(int volume);
#endif

   void setIndex(int playerIndex) { m_index = playerIndex; }

private:
   HSPPostProcessor();

   void equalize(unsigned char *datain, unsigned char *dataout, size_t len);
#ifndef HELIX_SW_VOLUME_INTERFACE
   int volumeize(unsigned char *data, size_t len);
   int volumeize(unsigned char *data, unsigned char *outbuf, size_t len);
   // returns samples (not bytes)
   int unpack(unsigned char *data, size_t len, /*out*/ INT32 *signal);
   // returns bytes (siglen is in samples)
   int pack(INT32 *signal, size_t siglen, /*out*/ unsigned char *data);
#endif

   HelixSimplePlayer *m_Player;
   LONG32             m_lRefCount;
   int                m_index;
   HXAudioFormat      m_format;
   int                m_count;

   // scope
   struct DelayQueue *m_item;
   int                m_current;
   unsigned long      m_prevtime;

   // equalizer

   // Gain for each band
   // values should be between -0.2 and 1.0
   float gain[EQ_MAX_BANDS][EQ_CHANNELS] __attribute__((aligned));
   // Volume gain
   // values should be between 0.0 and 1.0
   float preamp[EQ_CHANNELS] __attribute__((aligned));
   // Coefficients
   sIIRCoefficients* iir_cf;
   sXYData data_history[EQ_MAX_BANDS][EQ_CHANNELS] __attribute__((aligned));
   // history indices
   int                m_i;
   int                m_j;
   int                m_k;

#ifndef HELIX_SW_VOLUME_INTERFACE
   // volume stuff
   GAIN_STATE        *m_gaintool;
   float              m_gaindB;
#endif
};


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

   void updateEQgains(int preamp, vector<int> &equalizerGains);

#ifndef HELIX_SW_VOLUME_INTERFACE
   void setGain(int volume);
#endif

private:
   HSPPostMixAudioHook();

   HelixSimplePlayer *m_Player;
   int                m_index;
   LONG32             m_lRefCount;

   HSPPostProcessor  *m_processor;
};


class HSPFinalAudioHook : public IHXAudioHook
{
public:
   HSPFinalAudioHook(HelixSimplePlayer *player);
   virtual ~HSPFinalAudioHook();
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


   void updateEQgains(int preamp, vector<int> &equalizerGains);

#ifndef HELIX_SW_VOLUME_INTERFACE
   void setGain(int volume);
#endif

private:
   HSPFinalAudioHook();

   HelixSimplePlayer *m_Player;
   LONG32             m_lRefCount;

   HSPPostProcessor  *m_processor;
};


#endif

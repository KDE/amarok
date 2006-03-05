/* ***** BEGIN LICENSE BLOCK *****
 *
 * This software is released under the provisions of the GPL version 2.
 * see file "COPYING".  If that file is not available, the full statement 
 * of the license can be found at
 *
 * http://www.fsf.org/licensing/licenses/gpl.txt
 *
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.
 * Copyright (c) 2005 Paul Cifarelli All Rights Reserved.
 * 
 * 
 * This file is part of the Helix DNA Technology. RealNetworks is the
 * developer of the Original Code and owns the copyrights in the
 * portions it created.
 * 
 * This file, and the files included with this file, is distributed
 * and made available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS
 * ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET
 * ENJOYMENT OR NON-INFRINGEMENT.
 * 
 * Technology Compatibility Kit Test Suite(s) Location:
 *    http://www.helixcommunity.org/content/tck
 * 
 * ***** END LICENSE BLOCK ***** */

#ifndef _GAIN_H_
#define _GAIN_H_

#include "hxtypes.h"

#define GAIN_MAX_dB  0.0
#define GAIN_MIN_dB -27.0 

struct GAIN_STATE;
typedef struct GAIN_STATE GAIN_STATE;

class GainTool
{
public:
   GainTool(int sampleRate, int nChannels, int bytePerSample);
   ~GainTool();

   float gainSetImmediatedB(float dB);
   float gainSetSmoothdB(float dB);
   float gainSetImmediate(float dB);
   float gainSetSmooth(float dB);
   void gainSetMute();
   int gainSetTimeConstant(float millis);
   void gainFeed(unsigned char* signal, unsigned char *outsignal, int len) ;

private:
   GAIN_STATE* gainInit(int sampleRate, int nChannels, int bytePerSample);
   void gainFree(GAIN_STATE*) ;
   void gainFeedMono(unsigned char* signal, unsigned char *outsignal, int len);
   void gainFeedStereo(unsigned char* signal, unsigned char *outsignal, int len);
   void gainFeedMulti(unsigned char* signal, unsigned char *outsignal, int len);

   GAIN_STATE *m_g;
};

#endif /* _GAIN_H_ */

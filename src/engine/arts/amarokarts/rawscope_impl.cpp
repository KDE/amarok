/*
Copyright (C) 2000 Stefan Westerfeld <stefan@space.twc.de>
              2000 Charles Samuels   <charles@kde.org>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.
  
This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.
   
You should have received a copy of the GNU Library General Public License
along with this library; see the file COPYING.LIB.  If not, write to
the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
Boston, MA 02110-1301, USA.

*/

#include "amarokarts.h"

#include <vector>
#include <stdsynthmodule.h>

using namespace std;
using namespace Arts;

namespace Amarok
{

class RawScope_impl : public RawScope_skel, public StdSynthModule
{
    protected:
        float *mScope;

        int mScopeLength;
        float *mScopeEnd;
        float *mCurrent;

    public:
        vector<float> *scope()
        {
            vector<float> *buf = new vector<float>;
            buf->resize(mScopeLength);
            char *front = (char *)(&buf->front());
            memcpy(front, mCurrent, (mScopeEnd - mCurrent) * sizeof(float));
            memcpy(front + (mScopeEnd - mCurrent)*sizeof(float), mScope,
                   (mCurrent - mScope) * sizeof(float));
            return buf;
        }

        void buffer(long len)
        {
            delete [] mScope;

            mScopeLength=len;
            mScope=new float[len];
            mScopeEnd=mScope+mScopeLength;
            mCurrent=mScope;

            memset(mScope, 0, mScopeLength);
        }

        long buffer()
        {
            return mScopeLength;
        }

        void calculateBlock(unsigned long samples)
        {
            for (unsigned long i=0; i<samples; ++i)
            {
                for (; mCurrent<mScopeEnd && i<samples; ++mCurrent, ++i)
                {
                    *mCurrent = inleft[i] + inright[i];
                }
                if (mCurrent>=mScopeEnd)
                    mCurrent=mScope;
            }

            memcpy(outleft, inleft, sizeof(float)*samples);
            memcpy(outright, inright, sizeof(float)*samples);

        }

        RawScope_impl()
        {
            mScope=0;
            buffer(512);

        }

        ~RawScope_impl()
        {
            delete [] mScope;
        }
};


REGISTER_IMPLEMENTATION( RawScope_impl );

} // </namespace Amarok>

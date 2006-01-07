/*  aKode: aKodeScope

    Copyright (C) 2005 Allan Sandfeld Jensen <kde@carewolf.com>

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

#include <config.h>

#include <akode/audioframe.h>
#include <akode/converter.h>
#include "akode-scope.h"

#include <vector>
//#include <iostream>
using aKode::AudioFrame;

// Copies fromFrame to toFrame fast (Actually a swap happens to save malloc and frees).
static inline void takeover(AudioFrame* toFrame, AudioFrame* fromFrame)
{
    AudioFrame tmpFrame;

    tmpFrame = *toFrame;
    *toFrame = *fromFrame;
    *fromFrame = tmpFrame;
    tmpFrame.data = 0;
}

struct aKodeScope::private_data
{
    private_data() : length(512), convert(16) {};
    int length;
    aKode::AudioFrame frame;
    aKode::AudioFrame frame1;
    aKode::Converter convert;
    std::vector<int16_t> scope;
};

aKodeScope::aKodeScope()
{
    d = new private_data;
}

aKodeScope::~aKodeScope()
{
    delete d;
}

void aKodeScope::writeFrame(AudioFrame* frame)
{
    takeover(&d->frame, frame);
}
/*
void aKodeScope::setLength(length)
{
    d->length = length;
}

int aKodeScope::length() const
{
    return d->length;
}*/

const Engine::Scope& aKodeScope::scope()
{
    d->convert.doFrame(&d->frame, &d->frame1);

    int length = d->frame1.length;
    int channels = d->frame1.channels;
    if (length > 512) length = 512;
    d->scope.resize(length*channels);
    int16_t **data = (int16_t**)d->frame1.data;
    for(int j=0 ; j < length; j++)
        for (int i = 0; i < channels; i++)
            d->scope[j*channels + i] = data[i][j];

    return d->scope;
}

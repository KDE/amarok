/*  aKode: scope

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

#ifndef _AKODE_SCOPE_H
#define _AKODE_SCOPE_H

#include <akode/player.h>
#include "enginebase.h"

namespace aKode {
class AudioFrame;
}

class aKodeScope : public aKode::Player::Monitor
{
public:
    aKodeScope();
    ~aKodeScope();

    void writeFrame(aKode::AudioFrame *frame);

    const Engine::Scope& scope();
    struct private_data;
private:
    private_data *d;
};

#endif

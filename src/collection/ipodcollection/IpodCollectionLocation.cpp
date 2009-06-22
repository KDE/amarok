/*
 *  Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "IpodCollectionLocation.h"

#include "Debug.h"
#include "Meta.h"
#include "IpodCollection.h"
#include "IpodHandler.h"
#include "IpodMeta.h"
#include "../../statusbar/StatusBar.h"
#include "MediaDeviceCache.h" // for collection refresh hack


#include <kjob.h>
#include <KLocale>
#include <kio/job.h>
#include <kio/jobclasses.h>


using namespace Meta;

IpodCollectionLocation::IpodCollectionLocation( IpodCollection const *collection )
    : MediaDeviceCollectionLocation( collection )
{
    //nothing to do
}

IpodCollectionLocation::~IpodCollectionLocation()
{
    DEBUG_BLOCK
    //nothing to do
}

bool
IpodCollectionLocation::isWritable() const
{
    return true;
}

#include "IpodCollectionLocation.moc"


/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/
 
#ifndef AUDIOCDCOLLECTIONCAPABILITY_H
#define AUDIOCDCOLLECTIONCAPABILITY_H

#include "core/meta/Meta.h"

#include "core/capabilities/CollectionCapability.h"

class AudioCdCollection;

namespace Capabilities
{
    class AudioCdCollectionCapability : public CollectionCapability
    {
        Q_OBJECT

        public:
            AudioCdCollectionCapability( AudioCdCollection *coll );
            virtual QList<QAction*> collectionActions();

        private:
            AudioCdCollection *m_collection;
    };
}

#endif

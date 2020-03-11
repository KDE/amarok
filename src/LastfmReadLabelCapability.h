/***************************************************************************************
* Copyright (c) 2009 Dan Meltzer <parallelgrapefruit@gmail.com>                        *
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


#ifndef LASTFMREADLABELCAPABILITY_H
#define LASTFMREADLABELCAPABILITY_H

#include "core/meta/forward_declarations.h"
#include "core/capabilities/Capability.h"
#include "core/capabilities/ReadLabelCapability.h"

#include <QStringList>

class QNetworkReply;

namespace Capabilities
{

class LastfmReadLabelCapability : public Capabilities::ReadLabelCapability
{
    Q_OBJECT
    public:
        explicit LastfmReadLabelCapability( Meta::Track *track );
        void fetchLabels() override;
        void fetchGlobalLabels() override;
        QStringList labels() override;

    private:
        QStringList m_labels;
        Meta::TrackPtr m_track;
        QNetworkReply *m_job;

    private Q_SLOTS:
        void onTagsFetched();
};
}

#endif // LASTFMREADLABELCAPABILITY_H

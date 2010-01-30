/*
    Copyright (C) 2009 Dan Meltzer <parallelgrapefruit@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef SQLREADLABELCAPABILITY_H
#define SQLREADLABELCAPABILITY_H

#include "meta/capabilities/ReadLabelCapability.h"
#include "SqlMeta.h"

class SqlStorage;

namespace Meta
{

class SqlReadLabelCapability : public Meta::ReadLabelCapability
{
    Q_OBJECT
    public:
        SqlReadLabelCapability( Meta::SqlTrack *track, SqlStorage *storage );

        /**
        *   fetches a list of labels assigned to this track
        */
        virtual void fetchLabels();

        /**
        *   fetches a list of all labels in the database
        */
        virtual void fetchGlobalLabels();   //TODO: This shouldnt be in a Track capability

        /**
        *   @returns all labels assigned to this track
        */
        virtual QStringList labels();

    private:
        QStringList m_labels;
        Meta::TrackPtr m_track;
        SqlStorage *m_storage;
        void fetch( QString uniqueURL );
};

}

#endif // SQLREADLABELCAPABILITY_H

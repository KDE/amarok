/* This file is part of the KDE project
   Copyright (C) 2007 Bart Cerneels <bart.cerneels@gmail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#include "PodcastCollection.h"

#include "debug.h"
#include "PodcastReader.h"
#include "support/memoryquerymaker.h"
#include "TheInstances.h"

#include <QFile>

#include <klocale.h>

PodcastCollection * PodcastCollection::s_instance = 0;

PodcastCollection*
The::podcastCollection()
{
    return PodcastCollection::instance();
}

PodcastCollection::PodcastCollection()
 : Collection()
{
}


PodcastCollection::~PodcastCollection()
{
}

QueryMaker*
PodcastCollection::queryMaker()
{
    return new MemoryQueryMaker( this, collectionId() );
}

QString
PodcastCollection::prettyName() const
{
    return i18n("Podcasts");
}

QString
PodcastCollection::collectionId() const
{
    return "Podcasts";
}


bool
PodcastCollection::possiblyContainsTrack(const KUrl & url) const
{
    return false;
}

Meta::TrackPtr
PodcastCollection::trackForUrl(const KUrl & url)
{
    return TrackPtr();
}

CollectionLocation *
PodcastCollection::location() const
{
    return 0;
}

void
PodcastCollection::slotUpdateAll()
{
    //TODO: just calling it for the first one now
    slotUpdate( QString("/home/kde-devel/tmp/testcast.xml") );
}

void
PodcastCollection::slotUpdate( QString url )
{
    DEBUG_BLOCK

    PodcastReader podcastReader( this );
    QFile file( url );

    if (!file.open(QFile::ReadOnly | QFile::Text))
    {
        debug() << "error opening file " << file.fileName() << endl;
    }

    if (!podcastReader.read(&file)) {
        debug() << "Parse error in file " << file.fileName() << " at line "
            << podcastReader.lineNumber() << " column "
            << podcastReader.columnNumber() << " : "
            << podcastReader.errorString() << endl;
    }
    else
    {
        debug() << "Finished updating: " << url << endl;
    }

    emit( updated() );
}

PodcastCollection *
PodcastCollection::instance()
{
    if( s_instance == 0 )
        s_instance = new PodcastCollection();

    return s_instance;
}

#include "PodcastCollection.moc"

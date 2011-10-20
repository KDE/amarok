/****************************************************************************************
 * Copyright (c) 2011 Sven Krohlas <sven@getamarok.com>                                 *
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

#ifndef AMAZONPARSER_H
#define AMAZONPARSER_H

#include "AmazonCollection.h"
#include "AmazonMeta.h"

#include <QString>
#include <QDomDocument>

#include <kio/job.h>
#include <kio/jobclasses.h>

#include <threadweaver/Job.h>


class AmazonParser : public ThreadWeaver::Job
{
public:
    AmazonParser( QString tempFileName, Collections::AmazonCollection* collection, AmazonMetaFactory* factory );
    ~AmazonParser();

protected:
    virtual void run();

private:
    int addArtistToCollection( QString artist, QString description );
    int addAlbumToCollection( QString albumTitle, QString description, QString artistID, QString price, QString imgUrl, QString albumAsin );
    Collections::AmazonCollection* m_collection;
    QString m_tempFileName;
    QDomDocument *m_responseDocument;
    AmazonMetaFactory *m_factory;
};


#endif // AMAZONPARSER_H

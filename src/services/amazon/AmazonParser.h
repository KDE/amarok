/****************************************************************************************
 * Copyright (c) 2011 Sven Krohlas <sven@asbest-online.de>                              *
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

#include <ThreadWeaver/Job>


class AmazonParser : public QObject, public ThreadWeaver::Job
{
    Q_OBJECT
public:
    AmazonParser( QString tempFileName, Collections::AmazonCollection* collection, AmazonMetaFactory* factory );
    ~AmazonParser();

    // Reimplemented from ThreadWeaver::Job.
    // The parser can fail e.g. for invalid replies, network failures, etc.
    virtual bool success() const;

protected:
    void defaultBegin(const ThreadWeaver::JobPointer& job, ThreadWeaver::Thread *thread) Q_DECL_OVERRIDE;
    void defaultEnd(const ThreadWeaver::JobPointer& job, ThreadWeaver::Thread *thread) Q_DECL_OVERRIDE;
    void run(ThreadWeaver::JobPointer self = QSharedPointer<ThreadWeaver::Job>(), ThreadWeaver::Thread *thread = 0) Q_DECL_OVERRIDE;

    Collections::AmazonCollection* m_collection;
    QString m_tempFileName;
    QDomDocument *m_responseDocument;
    AmazonMetaFactory *m_factory;
    bool m_success;

Q_SIGNALS:
    /** This signal is emitted when this job is being processed by a thread. */
    void started(ThreadWeaver::JobPointer);
    /** This signal is emitted when the job has been finished (no matter if it succeeded or not). */
    void done(ThreadWeaver::JobPointer);
    /** This job has failed.
     * This signal is emitted when success() returns false after the job is executed. */
    void failed(ThreadWeaver::JobPointer);

private:
    /**
    * Adds an artist to the collection if it does not yet exist. In any case it returns the ID of the artist.
    * @param artist name of the artist to add.
    * @param description description of the artist to add.
    */
    int addArtistToCollection( const QString &artist, const QString &description );

    /**
    * Adds an album to the collection if it does not yet exist. In any case it returns the ID of the album.
    * @param albumTitle name of the album to add.
    * @param descritpion description of the album to add.
    * @param artistID ID of the artist this album belongs to.
    * @param price price of the album.
    * @param imgUrl url of a cover image.
    * @param albumAsin the ASIN for this album in the Amazon store.
    */
    int addAlbumToCollection( const QString &albumTitle, const QString &description, const QString &artistID, const QString &price, const QString &imgUrl, const QString &albumAsin, const bool isCompilation );
};

#endif // AMAZONPARSER_H

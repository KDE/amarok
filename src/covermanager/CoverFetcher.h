/****************************************************************************************
 * Copyright (c) 2004 Mark Kretschmann <kretschmann@kde.org>                            *
 * Copyright (c) 2004 Stefan Bogner <bochi@online.ms>                                   *
 * Copyright (c) 2007 Dan Meltzer <parallelgrapefruit@gmail.com>                        *
 * Copyright (c) 2009 Martin Sandsmark <sandsmark@samfundet.no>                         *
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

#ifndef AMAROK_COVERFETCHER_H
#define AMAROK_COVERFETCHER_H

#include "meta/Meta.h"
#include "CoverFetchUnit.h"

#include <QHash>
#include <QObject>      //baseclass
#include <QStringList>  //stack allocated

class CoverFetchQueue;
class CoverFoundDialog;
class KJob;

namespace KIO { class Job; }

class CoverFetcher : public QObject
{
    Q_OBJECT

public:
    AMAROK_EXPORT static CoverFetcher* instance();
    AMAROK_EXPORT static void destroy();

    /// Main fetch methods
    AMAROK_EXPORT void manualFetch( Meta::AlbumPtr album );
    AMAROK_EXPORT void queueAlbum( Meta::AlbumPtr album );
    AMAROK_EXPORT void queueAlbums( Meta::AlbumList albums );

    QStringList errors() const { return m_errors; }

    enum FinishState { Success, Error, NotFound, Cancelled };

public slots:
    AMAROK_EXPORT void queueQuery( const QString &query );

signals:
    void finishedSingle( int state );

private slots:

    /// Fetch a cover
    void slotFetch( const CoverFetchUnit::Ptr unit );
    void slotResult( KJob *job );

private:
    static CoverFetcher* s_instance;
    CoverFetcher();
    ~CoverFetcher();

    const int        m_limit;      /// maximum number of concurrent fetches
    CoverFetchQueue *m_queue;      /// current fetch queue
    Meta::AlbumList  m_queueLater; /// put here if m_queue exceeds m_limit

    QHash< const KJob*, CoverFetchUnit::Ptr > m_jobs;
    QHash< const CoverFetchUnit::Ptr, QList< QPixmap > > m_pixmaps;
    QHash< const CoverFetchUnit::Ptr, QPixmap > m_selectedPixmaps;

    QStringList m_errors;

    CoverFoundDialog *m_dialog;

    /// cleanup depending on the fetch result
    void finish( const CoverFetchUnit::Ptr unit,
                 FinishState state = Success,
                 const QString &message = QString() );

    /// Show the cover that has been found
    void showCover( const CoverFetchUnit::Ptr unit );
};

namespace The
{
    inline CoverFetcher *coverFetcher() { return CoverFetcher::instance(); }
}

#endif /* AMAROK_COVERFETCHER_H */

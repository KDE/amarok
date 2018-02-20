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

#include "core/meta/forward_declarations.h"
#include "CoverFetchUnit.h"
#include "network/NetworkAccessManagerProxy.h"

#include <QHash>
#include <QObject>      //baseclass
#include <QPointer>
#include <QStringList>  //stack allocated

class CoverFetchQueue;
class CoverFoundDialog;

namespace KIO { class Job; }

class CoverFetcher : public QObject
{
    Q_OBJECT

public:
    AMAROK_EXPORT static CoverFetcher* instance();
    AMAROK_EXPORT static void destroy();

    AMAROK_EXPORT void manualFetch( Meta::AlbumPtr album );
    AMAROK_EXPORT void queueAlbum( Meta::AlbumPtr album );
    AMAROK_EXPORT void queueAlbums( Meta::AlbumList albums );

    QStringList errors() const { return m_errors; }

    enum FinishState { Success, Error, NotFound, Cancelled };

public Q_SLOTS:
    AMAROK_EXPORT void queueQuery( Meta::AlbumPtr album, const QString &query, int page = 0 );

Q_SIGNALS:
    void finishedSingle( int state );

private Q_SLOTS:

    /// Fetch a cover
    void slotFetch( CoverFetchUnit::Ptr unit );

    /// Handle result of a fetch job
    void slotResult( const QUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e );

    /// Cover found dialog is closed by the user
    void slotDialogFinished();

    /// The fetch request was redirected.
    void fetchRequestRedirected( QNetworkReply *oldReply, QNetworkReply *newReply );

private:
    static CoverFetcher* s_instance;
    CoverFetcher();
    ~CoverFetcher();

    /// Remove a fetch unit from the queue, and clean up any running jobs
    void abortFetch( CoverFetchUnit::Ptr unit );

    void queueQueryForAlbum( Meta::AlbumPtr album );

    const int m_limit;            //!< maximum number of concurrent fetches
    CoverFetchQueue *m_queue;     //!< current fetch queue
    Meta::AlbumList m_queueLater; //!< put here if m_queue exceeds m_limit

    QHash< QUrl, CoverFetchUnit::Ptr > m_urls;
    QHash< const CoverFetchUnit::Ptr, QImage > m_selectedImages;

    QStringList m_errors;

    QPointer<CoverFoundDialog> m_dialog;

    /// cleanup depending on the fetch result
    void finish( const CoverFetchUnit::Ptr unit,
                 FinishState state = Success,
                 const QString &message = QString() );

    /// Show the cover that has been found
    void showCover( const CoverFetchUnit::Ptr &unit,
                    const QImage &cover = QImage(),
                    const CoverFetch::Metadata &data = CoverFetch::Metadata() );

    void handleCoverPayload( const CoverFetchUnit::Ptr &unit,
                             const QByteArray &data,
                             const QUrl &url );

    CoverFetch::Source fetchSource() const;
};

namespace The
{
    inline CoverFetcher *coverFetcher() { return CoverFetcher::instance(); }
}

#endif /* AMAROK_COVERFETCHER_H */

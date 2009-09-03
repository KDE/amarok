/****************************************************************************************
 * Copyright (c) 2008 Ian Monroe <ian@monroe.nu>                                        *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_DIRECTORYLOADER_H
#define AMAROK_DIRECTORYLOADER_H

#include <QObject>

#include "Meta.h"

#include <KFileItem>

class KJob;
namespace KIO {
    class Job;
    class UDSEntry;
    typedef QList< UDSEntry >  UDSEntryList;
    typedef QList< KFileItem > KFileItemList;
}



class DirectoryLoader : public QObject
{
    Q_OBJECT
    public:
        DirectoryLoader();
        ~DirectoryLoader();

        void insertAtRow( int row ); // call before init to tell the loader the row to start inserting tracks
        void init( const QList<KUrl>& urls ); //!list all 
        void init( const QList<QUrl>& urls ); //!convience 

    signals:
        void finished( const Meta::TrackList& );

    private slots:
        void directoryListResults( KIO::Job *job, const KIO::UDSEntryList &list );
        void listJobFinished( KJob* );
        void doInsertAtRow();

    private:
        void finishUrlList();
        static bool directorySensitiveLessThan( const KFileItem& item1, const KFileItem& item2 );
        /**
         * the number of directory list operations. this is used so that
         * the last directory operations knows its the last */
        int m_listOperations; 
        bool m_localConnection; //!was insertAtRow called? otherwise finishUrlList should deleteLater
        int m_row; //! for insertAtRow
        KIO::KFileItemList m_expanded;
        Meta::TrackList m_tracks; //! the tracks found. they get all sorted at the end.
};
#endif

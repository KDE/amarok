/***************************************************************************
                         directorylist.h  -  description
                            -------------------
   begin                : Tue Feb 4 2003
   copyright            : (C) 2003 by Scott Wheeler
   email                : wheeler@kde.org
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_DIRECTORYLIST_H
#define AMAROK_DIRECTORYLIST_H

#include <kdialogbase.h>    //baseclass


class DirectoryListBase;

class DirectoryList : public KDialogBase {
        Q_OBJECT

    public:
        struct Result {
            QStringList dirs;
            QStringList addedDirs;
            QStringList removedDirs;
            DialogCode status;
            bool addPlaylists;
        };

        DirectoryList( const QStringList &directories, bool importPlaylists,
                       QWidget *parent = 0, const char *name = 0 );
        virtual ~DirectoryList();

    public slots:
        Result exec();

    signals:
        void signalDirectoryAdded( const QString &directory );
        void signalDirectoryRemoved( const QString &directory );

    private slots:
        void slotAddDirectory();
        void slotRemoveDirectory();

    private:
        QStringList m_dirList;
        bool m_importPlaylists;
        DirectoryListBase *m_base;
        Result m_result;
};


#endif /* AMAROK_DIRECTORYLIST_H */




/***************************************************************************
 * copyright            : (C) 2007 Seb Ruiz <ruiz@kde.org>                 *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_QUEUEMANAGER_H
#define AMAROK_QUEUEMANAGER_H

#include <KDialog>

#include <QList>

namespace QueueManagerNS
{

class QueueManager : public KDialog
{
    Q_OBJECT

    public:
        explicit QueueManager( QWidget *parent = 0, const char *name = 0 );
        ~QueueManager();

//         QList<PlaylistItem* > newQueue();

        static QueueManager *instance() { return s_instance; }

    public slots:
//         void    applyNow();
//         void    addItems( QListWidgetItem *after = 0 ); // For the add button (uses selected playlist tracks)
//         void    changeQueuedItems( const QList<PlaylistItem*> &in, const QList<PlaylistItem*> &out );  // For keeping queue/dequeue in sync
//         void    updateButtons();

    private slots:
//         void    removeSelected();
//         void    changed();

    private:
//         void    insertItems();
//         void    addQueuedItem( PlaylistItem *item );
//         void    removeQueuedItem( PlaylistItem *item );

        static QueueManager *s_instance;
};

}

#endif /* AMAROK_QUEUEMANAGER_H */

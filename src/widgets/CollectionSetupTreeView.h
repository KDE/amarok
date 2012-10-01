#ifndef COLLECTIONSETUPTREEVIEW_H
#define COLLECTIONSETUPTREEVIEW_H

#include <QTreeView>

class CollectionSetupTreeView : public QTreeView
{
    Q_OBJECT

    public:
        CollectionSetupTreeView( QWidget* );
        ~CollectionSetupTreeView();

    protected slots:
        /** Shows a context menu if the right mouse button is pressed over a directory. */
        void slotPressed( const QModelIndex &index );
        void slotRescanDirTriggered();

    private:
        QAction *m_rescanDirAction;
        QString m_currDir;

};

#endif // COLLECTIONSETUPTREEVIEW_H

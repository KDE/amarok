/****************************************************************************************
 * Copyright (c) 2003 Scott Wheeler <wheeler@kde.org>                                   *
 * Copyright (c) 2004 Max Howell <max.howell@methylblue.com>                            *
 * Copyright (c) 2004-2008 Mark Kretschmann <kretschmann@kde.org>                       *
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
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

#ifndef AMAROK_COLLECTIONSETUP_H
#define AMAROK_COLLECTIONSETUP_H

#include <KVBox>      //baseclass

#include <QCheckBox>
#include <QFileSystemModel>
#include <QTreeWidgetItem>

#include "core/support/Debug.h"

class QTreeView;

namespace CollectionFolder { class Model; }

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

class CollectionSetup : public KVBox
{
    Q_OBJECT

    friend class CollectionFolder::Model;

    public:
        static CollectionSetup* instance() { return s_instance; }

        CollectionSetup( QWidget* );
        
        void writeConfig();
        bool hasChanged() const;
         
        QStringList dirs() const { return m_dirs; }
        bool recursive() const { return m_recursive && m_recursive->isChecked(); }
        bool monitor() const { return m_monitor && m_monitor->isChecked(); }
        bool writeBackStatistics() const { return m_writeBackStatistics && m_writeBackStatistics->isChecked(); }
        bool writeBackCover() const { return m_writeBackCover && m_writeBackCover->isChecked(); }
        bool charset() const { return m_charset && m_charset->isChecked(); }

        const QString modelFilePath( const QModelIndex &index ) const;

    signals:
        void changed();

    private slots:
        void importCollection();

    private:
        static CollectionSetup* s_instance;

        CollectionSetupTreeView *m_view;
        CollectionFolder::Model *m_model;
        QStringList m_dirs;
        QCheckBox *m_recursive;
        QCheckBox *m_monitor;
        QCheckBox *m_writeBackStatistics;
        QCheckBox *m_writeBackCover;
        QCheckBox *m_charset;
};


namespace CollectionFolder //just to keep it out of the global namespace
{
    class Model : public QFileSystemModel
    {
        public:
            Model();
        
            virtual Qt::ItemFlags flags( const QModelIndex &index ) const;
            QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const;
            bool setData( const QModelIndex& index, const QVariant& value, int role = Qt::EditRole );

            void setDirectories( QStringList &dirs ); // will clear m_checked before inserting new directories
            QStringList directories() const;

            virtual int columnCount( const QModelIndex& ) const { return 1; }

        private:
            bool ancestorChecked( const QString &path ) const;
            QStringList allCheckedAncestors( const QString &path ) const;
            bool descendantChecked( const QString &path ) const;
            bool isForbiddenPath( const QString &path ) const;
            void checkRecursiveSubfolders( const QString &root, const QString &excludePath );
            inline bool recursive() const
            {
                // Simply for convenience
                return CollectionSetup::instance() && CollectionSetup::instance()->recursive();
            }

            static inline QString normalPath( const QString &path )
            {
                return path.endsWith( '/' ) ? path : path + '/';
            }

            QSet<QString> m_checked;
    };

} // end namespace CollectionFolder

#endif


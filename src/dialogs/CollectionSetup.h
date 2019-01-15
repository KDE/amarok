/****************************************************************************************
 * Copyright (c) 2003 Scott Wheeler <wheeler@kde.org>                                   *
 * Copyright (c) 2004 Max Howell <max.howell@methylblue.com>                            *
 * Copyright (c) 2004-2008 Mark Kretschmann <kretschmann@kde.org>                       *
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
 * Copyright (c) 2013 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#include <QFileSystemModel>

#include "core/support/Debug.h"

#include "ui_CollectionConfig.h"

class QAction;
class QCheckBox;
class TranscodingConfig;

namespace CollectionFolder { class Model; }

class CollectionSetup : public QWidget
{
    Q_OBJECT

    friend class CollectionFolder::Model;

    public:
        static CollectionSetup* instance() { return s_instance; }

        explicit CollectionSetup( QWidget* );
        virtual ~CollectionSetup() {}

        void writeConfig();
        bool hasChanged() const;

        bool recursive() const;
        bool monitor() const;

        const QString modelFilePath( const QModelIndex &index ) const;
        Transcoding::SelectConfigWidget *transcodingConfig() const { return m_ui.transcodingConfig; }

    Q_SIGNALS:
        void changed();

    private Q_SLOTS:
        void importCollection();

        /** Shows a context menu if the right mouse button is pressed over a directory. */
        void slotPressed( const QModelIndex &index );
        void slotRescanDirTriggered();

    private:
        /** Returns true if the given path is contained in the primary collection
         *  @author Peter Zhou
         */
        bool isDirInCollection( const QString& path ) const;

        static CollectionSetup* s_instance;

        Ui::CollectionConfig m_ui;
        CollectionFolder::Model *m_model;

        QAction *m_rescanDirAction;
        /** This is the directory where the rescanDirAction was triggered */
        QString m_currDir;

        QCheckBox *m_recursive;
        QCheckBox *m_monitor;
};


namespace CollectionFolder //just to keep it out of the global namespace
{
    class Model : public QFileSystemModel
    {
        public:
            explicit Model( QObject *parent );

            Qt::ItemFlags flags( const QModelIndex &index ) const override;
            QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const override;
            bool setData( const QModelIndex& index, const QVariant& value, int role = Qt::EditRole ) override;
            int columnCount( const QModelIndex& ) const override { return 1; }

            /** Set the currently checked directories according to dirs */
            void setDirectories( QStringList &dirs );

            /** Returns the currently checked directories in this model. */
            QStringList directories() const;

        private:
            bool isForbiddenPath( const QString &path ) const;

            /** Returns true if one of the parent paths is checked. */
            bool ancestorChecked( const QString &path ) const;

            /**
             * Get a list of all checked paths that are an ancestor of
             * the given path.
             */
            QStringList allCheckedAncestors( const QString &path ) const;

            /** Returns true if at least one of the subpaths are checked */
            bool descendantChecked( const QString &path ) const;

            /**
             * Check the logical recursive difference of root and excludePath and
             * updates m_checked.
             * For example, if excludePath is a grandchild of root, then this method
             * will check all of the children of root except the one that is the
             * parent of excludePath, as well as excludePath's siblings.
             */
            void checkRecursiveSubfolders( const QString &root, const QString &excludePath );

            inline bool recursive() const
            {
                // Simply for convenience
                return CollectionSetup::instance() && CollectionSetup::instance()->recursive();
            }

            static inline QString normalPath( const QString &path )
            {
                return path.endsWith( QLatin1Char('/') ) ? path : path + '/';
            }

            QSet<QString> m_checked;
    };

} // end namespace CollectionFolder

#endif


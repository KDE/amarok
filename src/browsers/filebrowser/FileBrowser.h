/****************************************************************************************
 * Copyright (c) 2010 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#ifndef FILEBROWSERMKII_H
#define FILEBROWSERMKII_H

#include "browsers/BrowserCategory.h"

#include <QUrl>

class QAbstractItemView;
class QModelIndex;

class FileBrowser : public BrowserCategory
{
    Q_OBJECT

public:
    FileBrowser( const char *name, QWidget *parent );
    ~FileBrowser();

    void setupAddItems() override;

    /**
    * Navigate to a specific directory
    */
    void setDir( const QUrl &dir );

    /**
     * Return the path of the currently shown dir.
     */
    QString currentDir() const;

public Q_SLOTS:
    void addItemActivated( const QString &callback );

protected Q_SLOTS:
    void slotNavigateToDirectory( const QModelIndex &index );

    void reActivate() override;

    /**
     * Shows/hides the columns as selected in the context menu of the header of the
     * file view.
     * @param toggled the visibility state of a column in the context menu.
     */
    void toggleColumn( bool toggled );

    /**
     * Go backward in history
     */
    void back();

    /**
     * Go forward in history
     */
    void forward();

    /**
     * Navigates up one level in the path shown
     */
    void up();

    /**
     * Navigates to home directory
     */
    void home();

    /*
     * Refreshes current directory
     */
    void refresh();

    /**
     * Handle results of trying to setup an item in "places" that needed mounting or other
     * special setup.
     * @param index the index that we tried to setup
     * @param success did the setup succeed?
     */
    void setupDone( const QModelIndex &index, bool success );

private Q_SLOTS:
    void initView();
    void updateHeaderState();

private:
    class Private;
    Private *const d;
};

/**
 * Helper class that calls setCurrentIndex on a model view as soon as the model
 * adds a row and then it auto-deletes itself.
 */
class DelayedActivator : public QObject
{
    Q_OBJECT

    public:
        explicit DelayedActivator( QAbstractItemView *view );

    private Q_SLOTS:
        void slotRowsInserted( const QModelIndex &parent, int start );

    private:
        QAbstractItemView *m_view;
};

#endif // FILEBROWSERMKII_H

/* This file is part of the KDE project
   Copyright (C) 2004 Max Howell
   Copyright (C) 2004 Mark Kretschmann <markey@web.de>
   Copyright (C) 2003 Roberto Raggi <roberto@kdevelop.org>
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef FILESELECTOR_WIDGET_H
#define FILESELECTOR_WIDGET_H

#include <qvbox.h>        //baseclass
#include <kdiroperator.h> //some inline functions
#include <ktoolbar.h>     //baseclass
#include <kurl.h>         //stack allocated

class ClickLineEdit;
class QTimer;
class KActionCollection;
class KFileItem;
class KFileView;
class KURLComboBox;
class Medium;

//Hi! I think we ripped this from Kate, since then it's been modified somewhat

/*
    The KDev file selector presents a directory view, in which the default action is
    to open the activated file.
    Additinally, a toolbar for managing the kdiroperator widget + sync that to
    the directory of the current file is available, as well as a filter widget
    allowing to filter the displayed files using a name filter.
*/


class FileBrowser : public QVBox
{
    Q_OBJECT

    enum MenuId { MakePlaylist, SavePlaylist, MediaDevice, AppendToPlaylist, QueueTrack, QueueTracks, SelectAllFiles, BurnCd, MoveToCollection, CopyToCollection, OrganizeFiles, EditTags };

public:
    FileBrowser( const char *name = 0, Medium *medium = 0 );
   ~FileBrowser();

    KURL url() const { return m_dir->url(); }

public slots:
    void setUrl( const KURL &url );
    void setUrl( const QString &url );
    void setFilter( const QString& );
    void dropped( const KFileItem*, QDropEvent*, const KURL::List& );

private slots:
    void activate( const KFileItem* );
    void contextMenuActivated( int );
    void gotoCurrentFolder();
    void prepareContextMenu();
    void selectAll();
    void slotViewChanged( KFileView* );
    void urlChanged( const KURL& );

private:
    KURL::List selectedItems();
    void playlistFromURLs( const KURL::List &urls );

    KURLComboBox  *m_combo;
    KDirOperator  *m_dir;
    ClickLineEdit *m_filter;
    Medium        *m_medium;
};



#include <kfileitem.h> //KFileItemList
#include <qregexp.h>

class KDirLister;
class KURLView;
class QLineEdit;
class QListViewItem;

///@author Max Howell
///@short Widget for recursive searching of current FileBrowser location

class SearchPane : public QVBox
{
    Q_OBJECT

public:
    SearchPane( FileBrowser *parent );

private slots:
    void toggle( bool );
    void urlChanged( const KURL& );
    void searchTextChanged( const QString &text );
    void searchMatches( const KFileItemList& );
    void searchComplete();
    void _searchComplete();
    void activate( QListViewItem* );

private:
    KURL searchURL() const { return static_cast<FileBrowser*>(parentWidget())->url(); }

    QLineEdit  *m_lineEdit;
    KURLView   *m_listView;
    KDirLister *m_lister;
    QRegExp     m_filter;
    KURL::List  m_dirs;
};

#endif

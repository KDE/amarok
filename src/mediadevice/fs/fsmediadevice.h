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

#include <mediabrowser.h>

class ClickLineEdit;
class QTimer;
class KActionCollection;
class KDirOperator;
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

class FSMediaDevice : public MediaDevice
{
    Q_OBJECT

    public:
        FSMediaDevice();
        void              init( MediaBrowser* parent );
        virtual           ~FSMediaDevice();
        virtual bool      autoConnect() { return true; }
        virtual bool      asynchronousTransfer() { return true; }

        bool              isConnected() { return true; }

#if 0
        virtual void      addConfigElements( QWidget *parent );
        virtual void      removeConfigElements( QWidget *parent );
        virtual void      applyConfig();
        virtual void      loadConfig();
#endif

    protected:
        MediaItem        *trackExists( const MetaBundle& bundle ) { return 0; }

        bool              openDevice( bool silent=false ) { return true; }
        bool              closeDevice() { return true; }
        bool              lockDevice( bool ) { return true; }
        void              unlockDevice() {}
        void              synchronizeDevice() {}
        MediaItem        *copyTrackToDevice( const MetaBundle &bundle, const PodcastInfo *pinfo ) { return 0; }
        int               deleteItemFromDevice( MediaItem *item, bool onlyPlayed ) { return 0; }

};


class FSBrowser : public MediaView
{
    Q_OBJECT

    enum MenuId { MakePlaylist, SavePlaylist, CopyMediaDevice, AppendToPlaylist, SelectAllFiles, BurnCd, MoveToCollection, CopyToCollection, EditTags };

public:
    FSBrowser( QWidget *parent, MediaDevice *device, const char *name = 0, Medium *medium = 0 );
   ~FSBrowser();

    KURL url() const { return m_dir->url(); }

public slots:
    void setUrl( const KURL &url );
    void setUrl( const QString &url );
    void setFilter( const QString& );
    void dropped( const KFileItem*, QDropEvent*, const KURL::List& );

private slots:
    void urlChanged( const KURL& );
    void activate( const KFileItem* );
    void contextMenuActivated( int );
    void prepareContextMenu();
    void slotViewChanged( KFileView* );
    void selectAll();



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
///@short Widget for recursive searching of current FSBrowser location

class SearchPane : public QVBox
{
    Q_OBJECT

public:
    SearchPane( FSBrowser *parent );

private slots:
    void toggle( bool );
    void urlChanged( const KURL& );
    void searchTextChanged( const QString &text );
    void searchMatches( const KFileItemList& );
    void searchComplete();
    void _searchComplete();
    void activate( QListViewItem* );

private:
    KURL searchURL() const { return static_cast<FSBrowser*>(parentWidget())->url(); }

    QLineEdit  *m_lineEdit;
    KURLView   *m_listView;
    KDirLister *m_lister;
    QRegExp     m_filter;
    KURL::List  m_dirs;
};

#endif

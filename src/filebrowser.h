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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef FILESELECTOR_WIDGET_H
#define FILESELECTOR_WIDGET_H

#include <kdiroperator.h>
#include <ktoolbar.h>
#include <kurl.h>
#include <qvbox.h>

class KActionCollection;
class KDirOperator;
class KFileItem;
class KFileView;
class KURLComboBox;
class KLineEdit;
class QColor;
class QTimer;


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

public:
    FileBrowser( const char * name = 0 );
    ~FileBrowser();

    KDirOperator *dirOperator() { return m_dir; }
    KActionCollection *actionCollection() { return m_dir->actionCollection(); };
    QString location() const;

    static QColor altBgColor;

public slots:
    void slotSetFilter();
    void setDir( const KURL& );
    void setDir( const QString& url ) { setDir( KURL( url ) ); }

private slots:
    void cmbPathActivated( const KURL& u ) { cmbPathReturnPressed( u.url() ); }
    void cmbPathReturnPressed( const QString& u );
    void dirUrlEntered( const KURL& u );
    void slotSetFilterTimeout();
    void slotViewChanged( KFileView* );
    void activateThis( const KFileItem* );
    void makePlaylist();
    void addToPlaylist();
    void selectAllFiles();
    void burnDataCd();
    void burnAudioCd();

private:
    void setupToolbar();
    KURL::List selectedItems();
    bool eventFilter( QObject*, QEvent* );

    class ToolBar: public KToolBar
    {
    public:
        ToolBar( QWidget *parent )
            : KToolBar( parent, 0, true )
        {}

        virtual void setMovingEnabled( bool )
        {
            KToolBar::setMovingEnabled( false ); //TODO find out why we need this!
        }
    };

    ToolBar           *m_toolbar;
    KURLComboBox      *m_cmbPath;
    KDirOperator      *m_dir;
    KLineEdit         *m_filterEdit;
    QTimer            *m_timer;
};

#endif

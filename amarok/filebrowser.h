/* This file is part of the KDE project
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
class KActionSelector;
class KDirOperator;
class KURLComboBox;
class KHistoryCombo;
class QColor;


//Hi! I think we ripped this from Kate, since then it's been modified somewhat

/*
    The KDev file selector presents a directory view, in which the default action is
    to open the activated file.
    Additinally, a toolbar for managing the kdiroperator widget + sync that to
    the directory of the current file is available, as well as a filter widget
    allowing to filter the displayed files using a name filter.
*/


class KDevFileSelectorToolBar: public KToolBar
{
public:
    KDevFileSelectorToolBar( QWidget *parent )
      : KToolBar( parent, 0, true )
    {
        setMinimumWidth(10);
    }

    virtual void KDevFileSelectorToolBar::setMovingEnabled( bool )
    {
        KToolBar::setMovingEnabled( false );
    }
};


class KDevDirOperator: public KDirOperator
{
public:
    KDevDirOperator( const KURL &urlName, QWidget *parent=0, const char *name=0 )
      : KDirOperator(urlName, parent, name)
    {}

protected:
    virtual KFileView* createView( QWidget*, KFile::FileView );
};


class FileBrowser : public QVBox
{
    Q_OBJECT

public:
    FileBrowser( const char * name = 0 );
    ~FileBrowser();

    void readConfig();
    void setupToolbar();
    KDevDirOperator *dirOperator(){ return dir; }
    KActionCollection *actionCollection() { return m_actionCollection; };
    QString location() const;
    QSize sizeHint() const { return QSize( 220, 50 ); } //default embedded sidebar width

    static QColor altBgColor;

public slots:
    void slotFilterChange(const QString&);
    void setDir(KURL);
    void setDir( const QString& url ) { setDir( KURL( url ) ); };

private slots:
    void cmbPathActivated( const KURL& u );
    void cmbPathReturnPressed( const QString& u );
    void dirUrlEntered( const KURL& u );
    void btnFilterClick();

private:
    class KDevFileSelectorToolBar *toolbar;
    KActionCollection *m_actionCollection;
    class KBookmarkHandler *bookmarkHandler;
    KURLComboBox *cmbPath;
    KDevDirOperator *dir;
    KHistoryCombo * filter;
    class QToolButton *btnFilter;

    QString lastFilter;
};

/*  @todo anders
    KFSFilterHelper
    A popup widget presenting a listbox with checkable items
    representing the mime types available in the current directory, and
    providing a name filter based on those.
*/

#endif

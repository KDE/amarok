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

#include <qwidget.h>
#include <kfile.h>
#include <kurl.h>
#include <ktoolbar.h>
#include <kdiroperator.h>

class QColor;

class KActionCollection;
class KActionSelector;
class KDirOperator;
class KURLComboBox;
class KHistoryCombo;

class PlayerApp;
extern PlayerApp *pApp;

/*
    The KDev file selector presents a directory view, in which the default action is
    to open the activated file.
    Additinally, a toolbar for managing the kdiroperator widget + sync that to
    the directory of the current file is available, as well as a filter widget
    allowing to filter the displayed files using a name filter.
*/

/* I think this fix for not moving toolbars is better */
class KDevFileSelectorToolBar: public KToolBar
{
    Q_OBJECT
public:
    KDevFileSelectorToolBar(QWidget *parent);
    virtual ~KDevFileSelectorToolBar();

    virtual void setMovingEnabled( bool b );
};

class KDevFileSelectorToolBarParent: public QFrame
{
    Q_OBJECT
public:
    KDevFileSelectorToolBarParent(QWidget *parent);
    ~KDevFileSelectorToolBarParent();
    void setToolBar(KDevFileSelectorToolBar *tb);

private:
    KDevFileSelectorToolBar *m_tb;

protected:
    virtual void resizeEvent ( QResizeEvent * );
};

class KDevDirOperator: public KDirOperator
{
    Q_OBJECT
public:
    KDevDirOperator( const KURL &urlName=KURL(), QWidget *parent=0, const char *name=0 )
        :KDirOperator(urlName, parent, name)
    {
    }

protected:
    KFileView* createView( QWidget*, KFile::FileView ); 

protected slots:
    virtual void activatedMenu (const KFileItem *fi, const QPoint &pos);

private:
};


class KDevFileSelector : public QWidget
{
    Q_OBJECT

public:
    /* When to sync to current document directory */
    enum AutoSyncEvent { DocumentChanged=1, DocumentOpened=2, GotVisible=4 };

    KDevFileSelector( QWidget * parent = 0, const char * name = 0 );
    ~KDevFileSelector();

    void readConfig();
    void writeConfig();
    void setupToolbar();
    KDevDirOperator *dirOperator(){ return dir; }
    KActionCollection *actionCollection() { return mActionCollection; };
    static QColor altBgColor;
    
public slots:
    void slotFilterChange(const QString&);
    void setDir(KURL);
    void setDir( const QString& url ) { setDir( KURL( url ) ); };

private slots:
    void cmbPathActivated( const KURL& u );
    void cmbPathReturnPressed( const QString& u );
    void dirUrlEntered( const KURL& u );
    void dirFinishedLoading();
    void setActiveDocumentDir();
    void viewChanged();
    void btnFilterClick();
    void autoSync();

protected:
    void focusInEvent( QFocusEvent * );
    void showEvent( QShowEvent * );
    bool eventFilter( QObject *, QEvent * );
    KURL activeDocumentUrl();

private:
    class KDevFileSelectorToolBar *toolbar;
    KActionCollection *mActionCollection;
    class KBookmarkHandler *bookmarkHandler;
    KURLComboBox *cmbPath;
    KDevDirOperator * dir;
    class KAction *acSyncDir;
    KHistoryCombo * filter;
    class QToolButton *btnFilter;

    QString lastFilter;
    int autoSyncEvents; // enabled autosync events
    QString waitingUrl; // maybe display when we gets visible
    QString waitingDir;
};

/*  @todo anders
    KFSFilterHelper
    A popup widget presenting a listbox with checkable items
    representing the mime types available in the current directory, and
    providing a name filter based on those.
*/

#endif

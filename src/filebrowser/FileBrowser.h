/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>
   Copyright (C) 2007 Mirko Stocker <me@misto.ch>
   Copyright (C) 2007 Ian Monroe <ian@monroe.nu>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __KATE_FILESELECTOR_H__
#define __KATE_FILESELECTOR_H__


#include <KVBox>
#include <KFile>
#include <KFileItem>
#include <KUrl>
#include <KToolBar>

class QAbstractItemView;
class KActionCollection;
class KActionSelector;
class KConfigBase;
class KBookmarkHandler;
class KDirOperator;
class KHistoryComboBox;
class KUrlComboBox;
class QAction;
class QToolButton;
class QCheckBox;
class QEvent;
class QSpinBox;
class QShowEvent;
class QFocusEvent;


/*
    The kate file selector presents a directory view, in which the default action is
    to open the activated file.
    Additionally, a toolbar for managing the kdiroperator widget + sync that to
    the directory of the current file is available, as well as a filter widget
    allowing to filter the displayed files using a name filter.
*/

namespace FileBrowser {

  class ToolBar: public KToolBar
  {
      Q_OBJECT
    public:
      ToolBar(QWidget *parent);
      virtual ~ToolBar();
  };

  class Widget : public KVBox
  {
      Q_OBJECT

    public:
      explicit Widget( const char * name = 0 );
      ~Widget();

      virtual void readSessionConfig( KConfigBase *, const QString & );
      virtual void writeSessionConfig( KConfigBase *, const QString & );
      void readConfig();
      void writeConfig();
      void setupToolbar( QStringList actions );
      void setView( KFile::FileView );
      KDirOperator *dirOperator()
      {
        return m_dir;
      }
      KActionCollection *actionCollection()
      {
        return m_actionCollection;
      }

    public Q_SLOTS:
      void slotFilterChange(const QString&);
      void setDir(KUrl);
      void setDir( const QString& url )
      {
        setDir( KUrl( url ) );
      }
      void selectorViewChanged( QAbstractItemView * );

    private Q_SLOTS:
      void fileSelected(const KFileItem & /*file*/);
      void cmbPathActivated( const KUrl& u );
      void cmbPathReturnPressed( const QString& u );
      void dirUrlEntered( const KUrl& u );
      void dirFinishedLoading();
      void btnFilterClick();

    protected:
      void focusInEvent( QFocusEvent * );
      bool eventFilter( QObject *, QEvent * );
      void initialDirChangeHack();

    private:
      ToolBar *m_toolbar;
      KActionCollection *m_actionCollection;
      KBookmarkHandler *m_bookmarkHandler;
      KUrlComboBox *m_cmbPath;
      KDirOperator * m_dir;
      QAction *m_acSyncDir;
      KHistoryComboBox * m_filter;
      QToolButton *m_btnFilter;

      QString lastFilter;
      QString waitingUrl; // maybe display when we gets visible
      QString waitingDir;
  };

}

#endif //__KATE_FILESELECTOR_H__

// kate: space-indent on; indent-width 2; replace-tabs on;

/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>
   Copyright (C) 2007 Mirko Stocker <me@misto.ch>

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

//BEGIN Includes
#include "FileBrowser.h"
#include "FileBrowser.moc"

#include "collection/CollectionManager.h"
#include "playlist/PlaylistModel.h"
#include "TheInstances.h"
#include "filebrowser/MyDirOperator.h"
#include "filebrowser/kbookmarkhandler.h"

#include <QLayout>
#include <QToolButton>
#include <khbox.h>
#include <kvbox.h>
#include <QLabel>

#include <QListWidget>
#include <QScrollBar>
#include <QSpinBox>
#include <QGroupBox>
#include <QCheckBox>
#include <QRegExp>
#include <QTimer>
#include <QDir>
//Added by qt3to4:
#include <QPixmap>
#include <QFocusEvent>
#include <QEvent>
#include <QFrame>
#include <QShowEvent>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <QLineEdit>

#include <KActionCollection>
#include <KActionMenu>
#include <KActionSelector>
#include <KApplication>
#include <KConfig>
#include <KComboBox>
#include <KDebug>
#include <KDialog>
#include <KFileItem>
#include <kfileplacesmodel.h>
#include <KGenericFactory>
#include <KGlobal>
#include <KIconLoader>
#include <KLocale>
#include <KMenu>
#include <KMessageBox>
#include <KProtocolInfo>
// #include <KUrlComboBox>
#include <KUrlNavigator>
#include <KUrlCompletion>
#include <KConfigGroup>
#include <KHistoryComboBox>
#include <kdeversion.h>
//END Includes

//BEGIN Toolbar
// from kfiledialog.cpp - avoid qt warning in STDERR (~/.xsessionerrors)
static void silenceQToolBar(QtMsgType, const char *)
{}

// helper classes to be able to have a toolbar without move handle
FileBrowser::ToolBar::ToolBar(QWidget *parent)
    : KToolBar( parent, "Kate FileSelector Toolbar", true )
{
  setMinimumWidth(10);
}

FileBrowser::ToolBar::~ToolBar()
{}
//END

//BEGIN Constructor/destructor

FileBrowser::Widget::Widget( const char * name )
    : KVBox ( 0 ),
      m_toolbar( 0 ),
      m_actionCollection( 0 ),
      m_bookmarkHandler( 0 ),
      m_filePlacesModel( 0 ),
//       m_cmbPath( 0 ),
      m_urlNav( 0 ),
      m_dir( 0 ),
      m_acSyncDir( 0 ),
      m_filter( 0 ),
      m_btnFilter( 0 )
{
  setObjectName(name);
  m_actionCollection = new KActionCollection( this );

  QtMsgHandler oldHandler = qInstallMsgHandler( silenceQToolBar );

  m_toolbar = new FileBrowser::ToolBar(this);
  m_toolbar->setMovable(false);
  qInstallMsgHandler( oldHandler );

//   m_cmbPath = new KUrlComboBox( KUrlComboBox::Directories, true, this);
//   m_cmbPath->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ));
//   KUrlCompletion* cmpl = new KUrlCompletion(KUrlCompletion::DirCompletion);
//   m_cmbPath->setCompletionObject( cmpl );
//   m_cmbPath->setAutoDeleteCompletionObject( true );
  m_filePlacesModel = new KFilePlacesModel( this );
  m_urlNav = new KUrlNavigator( m_filePlacesModel, KUrl(QDir::home().path()), this );

// FIXME
//  m_cmbPath->listBox()->installEventFilter( this );

  m_dir = new MyDirOperator(KUrl(QDir::home().path()), this);
//   m_cmbPath->setUrl( KUrl(QDir::home().path()) );
  m_dir->setView( KFile::Simple );
  m_dir->view()->setSelectionMode(QAbstractItemView::ExtendedSelection);
  connect ( m_dir, SIGNAL( viewChanged(QAbstractItemView *) ),
           this, SLOT( selectorViewChanged(QAbstractItemView *) ) );
  setStretchFactor(m_dir, 2);
  m_dir->setSizePolicy (QSizePolicy (QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));

  KActionCollection *coll = m_dir->actionCollection();
  // some shortcuts of diroperator that clashes with Kate
  coll->action( "delete" )->setShortcut( Qt::ALT + Qt::Key_Delete );
  //coll->action( "reload" )->setShortcut( Qt::ALT + Qt::Key_F5 );
  //coll->action( "back" )->setShortcut( Qt::ALT + Qt::SHIFT + Qt::Key_Left );
  //coll->action( "forward" )->setShortcut( Qt::ALT + Qt::SHIFT + Qt::Key_Right );
  // some consistency - reset up for dir too
  //coll->action( "up" )->setShortcut( Qt::ALT + Qt::SHIFT + Qt::Key_Up );
  //coll->action( "home" )->setShortcut( Qt::CTRL + Qt::ALT + Qt::Key_Home );

  // bookmarks action!
  KActionMenu *acmBookmarks = new KActionMenu( KIcon("bookmarks"), i18n("Bookmarks"), this );
  m_actionCollection->addAction( "bookmarks", acmBookmarks );
  acmBookmarks->setDelayed( false );
  m_bookmarkHandler = new KBookmarkHandler( this, acmBookmarks->menu() );
  KHBox* filterBox = new KHBox(this);

  m_btnFilter = new QToolButton( filterBox );
  m_btnFilter->setIcon( KIcon("view-filter" ) );
  m_btnFilter->setCheckable( true );
  m_filter = new KHistoryComboBox( true, filterBox);
  m_filter->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ));
  filterBox->setStretchFactor(m_filter, 2);
  connect( m_btnFilter, SIGNAL( clicked() ), this, SLOT( btnFilterClick() ) );

  connect( m_filter, SIGNAL( activated(const QString&) ),
           SLOT( slotFilterChange(const QString&) ) );
  connect( m_filter, SIGNAL( returnPressed(const QString&) ),
           m_filter, SLOT( addToHistory(const QString&) ) );

  // kaction for the dir sync method
  m_acSyncDir = m_actionCollection->addAction( "sync_dir" );
  m_acSyncDir->setIcon( KIcon("curfiledir") );
  m_acSyncDir->setText( i18n("Current Document Folder") );
  connect( m_acSyncDir, SIGNAL( triggered() ), this, SLOT( setActiveDocumentDir() ) );
  m_toolbar->setToolButtonStyle( Qt::ToolButtonIconOnly );
  m_toolbar->setIconDimensions( 16 );
  m_toolbar->setContextMenuPolicy( Qt::NoContextMenu );

//   connect( m_cmbPath, SIGNAL( urlActivated( const KUrl&  )),
//            this, SLOT( cmbPathActivated( const KUrl& ) ));
  connect( m_urlNav, SIGNAL( urlChanged( const KUrl&  )),
           this, SLOT( cmbPathActivated( const KUrl& ) ));
//   connect( m_cmbPath, SIGNAL( returnPressed( const QString&  )),
//            this, SLOT( cmbPathReturnPressed( const QString& ) ));
  connect(m_dir, SIGNAL(urlEntered(const KUrl&)),
          this, SLOT(dirUrlEntered(const KUrl&)) );

  connect(m_dir, SIGNAL(finishedLoading()),
          this, SLOT(dirFinishedLoading()) );

  // Connect the bookmark handler
  connect( m_bookmarkHandler, SIGNAL( openUrl( const QString& )),
           this, SLOT( setDir( const QString& ) ) );

  waitingUrl.clear();

  // whatsthis help
//   m_cmbPath->setWhatsThis(       i18n("<p>Here you can enter a path for a folder to display.</p>"
//                                     "<p>To go to a folder previously entered, press the arrow on "
//                                     "the right and choose one.</p><p>The entry has folder "
//                                     "completion. Right-click to choose how completion should behave.</p>") );
  m_urlNav->setWhatsThis(       i18n("<p>Here you can enter a path for a folder to display.</p>"
                                    "<p>To go to a folder previously entered, press the arrow on "
                                    "the right and choose one.</p><p>The entry has folder "
                                    "completion. Right-click to choose how completion should behave.</p>") );
  m_filter->setWhatsThis(        i18n("<p>Here you can enter a name filter to limit which files are displayed.</p>"
                                    "<p>To clear the filter, toggle off the filter button to the left.</p>"
                                    "<p>To reapply the last filter used, toggle on the filter button.</p>" ) );
  m_btnFilter->setWhatsThis(        i18n("<p>This button clears the name filter when toggled off, or "
                                       "reapplies the last filter used when toggled on.</p>") );

  connect(m_dir, SIGNAL(fileSelected(const KFileItem&)), this, SLOT(fileSelected(const KFileItem&)));

  readConfig();

  m_actionCollection->addAssociatedWidget(this);
  foreach (QAction* action, m_actionCollection->actions())
#if QT_VERSION < KDE_MAKE_VERSION(4,4,0)
    action->setShortcutContext(Qt::WidgetShortcut); // remove after Qt4.4 becomes mandatory
#else
    action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
#endif
}

FileBrowser::Widget::~Widget()
{}
//END Constroctor/Destrctor

//BEGIN Public Methods

void FileBrowser::Widget::readConfig()
{

//   dir->setView( KFile::Default );
//   dir->view()->setSelectionMode(QAbstractItemView::ExtendedSelection);

  // set up the toolbar
  KConfigGroup fileselectorConfigGroup(KGlobal::config(), "fileselector");
  setupToolbar( fileselectorConfigGroup.readEntry( "toolbar actions", QStringList() ) );

//   m_cmbPath->setMaxItems( fileselectorConfigGroup.readEntry( "pathcombo history len", 9 ) );
  // if we restore history

  m_filter->setMaxCount( fileselectorConfigGroup.readEntry( "filter history len", 9 ) );

}

void FileBrowser::Widget::readSessionConfig(KConfigBase *config, const QString & name)
{

  KConfigGroup cgView(config, name + ":view");
  m_dir->setViewConfig(cgView );

  KConfigGroup cgDir(config, name + ":dir");
  m_dir->readConfig(cgDir);

  KConfigGroup cg (config, name );
//   m_cmbPath->setUrls( cg.readPathEntry( "dir history", QStringList() ) );

  KConfigGroup globalConfig( KGlobal::config(), "fileselector" );

  if ( globalConfig.readEntry( "restore location", true) || kapp->isSessionRestored() )
  {
    QString loc( cg.readPathEntry( "location", QString() ) );
    if ( ! loc.isEmpty() )
      setDir( loc );
  }

  m_filter->setHistoryItems( cg.readEntry("filter history", QStringList()), true );
  lastFilter = cg.readEntry( "last filter" );
  QString flt("");
  if ( globalConfig.readEntry( "restore last filter", true ) || kapp->isSessionRestored() )
    flt = cg.readEntry("current filter");
  m_filter->lineEdit()->setText( flt );
  slotFilterChange( flt );
}

void FileBrowser::Widget::initialDirChangeHack()
{
  setDir( waitingDir );
}

void FileBrowser::Widget::setupToolbar( QStringList actions )
{
  m_toolbar->clear();
  if ( actions.isEmpty() )
  {
    // reasonable collection for default toolbar
    actions << "up" << "back" << "forward" << "home" <<
    "short view" << "detailed view" <<
    "bookmarks" << "sync_dir";
  }
  QAction *ac;
  for ( QStringList::Iterator it = actions.begin(); it != actions.end(); ++it )
  {
    if ( *it == "bookmarks" || *it == "sync_dir" )
      ac = m_actionCollection->action( (*it).toLatin1().constData() );
    else
      ac = m_dir->actionCollection()->action( (*it).toLatin1().constData() );
    if ( ac )
      m_toolbar->addAction( ac );
  }
}

void FileBrowser::Widget::writeConfig()
{
  KConfigGroup cg = KConfigGroup( KGlobal::config(), "fileselector" );

//   cg.writeEntry( "pathcombo history len", m_cmbPath->maxItems() );
  cg.writeEntry( "filter history len", m_filter->maxCount() );
  cg.writeEntry( "filter history", m_filter->historyItems() );
}

void FileBrowser::Widget::writeSessionConfig(KConfigBase *config, const QString & name)
{
  KConfigGroup cgDir(config, name + ":dir");
  m_dir->writeConfig(cgDir);

  KConfigGroup cg = KConfigGroup( config, name );
  QStringList l;
//   for (int i = 0; i < m_cmbPath->count(); i++)
//   {
//     l.append( m_cmbPath->itemText( i ) );
//   }
//   cg.writePathEntry( "dir history", l );
//   cg.writePathEntry( "location", m_cmbPath->currentText() );
  cg.writeEntry( "current filter", m_filter->currentText() );
  cg.writeEntry( "last filter", lastFilter );
}

void FileBrowser::Widget::setView(KFile::FileView view)
{
  m_dir->setView(view);
  m_dir->view()->setSelectionMode(QAbstractItemView::ExtendedSelection);
}

//END Public Methods

//BEGIN Public Slots

void FileBrowser::Widget::slotFilterChange( const QString & nf )
{
  QString f = nf.trimmed();
  bool empty = f.isEmpty() || f == "*";
  if ( empty )
  {
    m_dir->clearFilter();
    m_filter->lineEdit()->setText( QString() );
    m_btnFilter->setToolTip(
      i18n("Apply last filter (\"%1\")", lastFilter )) ;
  }
  else
  {
    m_dir->setNameFilter( f );
    lastFilter = f;
    m_btnFilter->setToolTip( i18n("Clear filter") );
  }
  m_btnFilter->setChecked( !empty );
  m_dir->updateDir();
  // this will be never true after the filter has been used;)
  m_btnFilter->setEnabled( !( empty && lastFilter.isEmpty() ) );

}

namespace FileBrowser {

    bool isReadable ( const KUrl& url )
    {
    if ( !url.isLocalFile() )
        return true; // what else can we say?

    QDir dir (url.path());
    return dir.exists ();
    }

}
void FileBrowser::Widget::setDir( KUrl u )
{
  KUrl newurl;

  if ( !u.isValid() )
    newurl.setPath( QDir::homePath() );
  else
    newurl = u;

  QString pathstr = newurl.path( KUrl::AddTrailingSlash );
  newurl.setPath(pathstr);

  if ( !FileBrowser::isReadable ( newurl ) )
    newurl.cd(QString::fromLatin1(".."));

  if ( !FileBrowser::isReadable (newurl) )
    newurl.setPath( QDir::homePath() );

  m_dir->setUrl(newurl, true);
}

//END Public Slots

//BEGIN Private Slots

void FileBrowser::Widget::fileSelected(const KFileItem & /*file*/)
{
  const KFileItemList list = m_dir->selectedItems();

  KUrl::List urlList;
  foreach (const KFileItem& item, list)
  {
      urlList << item.url();
  }

  Meta::TrackList trackList = CollectionManager::instance()->tracksForUrls( urlList );
  The::playlistModel()->insertOptioned( trackList, Playlist::Append );
  m_dir->view()->selectionModel()->clear();
}




void FileBrowser::Widget::cmbPathActivated( const KUrl& u )
{
  cmbPathReturnPressed( u.url() );
}

void FileBrowser::Widget::cmbPathReturnPressed( const QString& u )
{
//   KUrl typedURL( u );
//   if ( typedURL.hasPass() )
//     typedURL.setPass( QString() );
// 
//   QStringList urls = m_cmbPath->urls();
//   urls.removeAll( typedURL.url() );
//   urls.prepend( typedURL.url() );
//   m_cmbPath->setUrls( urls, KUrlComboBox::RemoveBottom );
  m_dir->setFocus();
  m_dir->setUrl( KUrl(u), true );
}

void FileBrowser::Widget::dirUrlEntered( const KUrl& u )
{
  m_urlNav->setUrl( u );
//   m_cmbPath->setUrl( u );
}

void FileBrowser::Widget::dirFinishedLoading()
{}


/*
   When the button in the filter box toggles:
   If off:
   If the name filer is anything but "" or "*", reset it.
   If on:
   Set last filter.
*/
void FileBrowser::Widget::btnFilterClick()
{
  if ( !m_btnFilter->isChecked() )
  {
    slotFilterChange( QString() );
  }
  else
  {
    m_filter->lineEdit()->setText( lastFilter );
    slotFilterChange( lastFilter );
  }
}

void FileBrowser::Widget::selectorViewChanged( QAbstractItemView * newView )
{
  newView->setSelectionMode(QAbstractItemView::ExtendedSelection);
}

//END Private Slots

//BEGIN Protected

void FileBrowser::Widget::focusInEvent( QFocusEvent * )
{
  m_dir->setFocus();
}

bool FileBrowser::Widget::eventFilter( QObject* o, QEvent *e )
{
  /*
      This is rather unfortunate, but:
      QComboBox does not support setting the size of the listbox to something
      reasonable. Even using listbox->setVariableWidth() does not yield a
      satisfying result, something is wrong with the handling of the sizehint.
      And the popup is rather useless, if the paths are only partly visible.
  */
  /* FIXME QListWidget *lb = m_cmbPath->listBox();
    if ( o == lb && e->type() == QEvent::Show ) {
      int add = lb->height() < lb->contentsHeight() ? lb->verticalScrollBar()->width() : 0;
      int w = qMin( mainwin->width(), lb->contentsWidth() + add );
      lb->resize( w, lb->height() );
      // TODO - move the listbox to a suitable place if necessary
      // TODO - decide if it is worth caching the size while untill the contents
      //        are changed.
    }
    */// TODO - same thing for the completion popup?
  return QWidget::eventFilter( o, e );
}

//END Protected

//BEGIN ACtionLBItem
/*
   QListboxItem that can store and return a string,
   used for the toolbar action selector.
*/
class ActionLBItem : public QListWidgetItem
{
  public:
    ActionLBItem( QListWidget *lb = 0,
                  const QIcon &pm = QIcon(),
                  const QString &text = QString(),
                  const QString &str = QString() ) :
        QListWidgetItem(pm, text, lb, 0 ),
        _str(str)
    {}
    QString idstring()
    {
      return _str;
    }
  private:
    QString _str;
};
//END ActionLBItem

// kate: space-indent on; indent-width 4; replace-tabs on;

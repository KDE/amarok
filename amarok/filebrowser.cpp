/* This file is part of the KDE project
   Copyright (C) 2004 Mark Kretschmann <markey@web.de>
   Copyright (C) 2003 Alexander Dymo <cloudtemple@mksat.net>
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

//BEGIN Includes
#include "amarokconfig.h"
#include "filebrowser.h"
#include "kactionselector.h"
#include "kbookmarkhandler.h"

#include <qapplication.h>
#include <qcheckbox.h>
#include <qdockarea.h>
#include <qgroupbox.h>
#include <qhbox.h>
#include <qlabel.h>
#include <qlistbox.h>
#include <qregexp.h>
#include <qscrollbar.h>
#include <qspinbox.h>
#include <qstrlist.h>
#include <qtimer.h>
#include <qtoolbutton.h>
#include <qtooltip.h>
#include <qvbox.h>
#include <qwhatsthis.h>

#include <kaction.h>
#include <kapplication.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <kio/netaccess.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpopupmenu.h>
#include <kprotocolinfo.h>
#include <ktoolbarbutton.h>
#include <kurlcombobox.h>
#include <kurlcompletion.h>
#include <qtoolbar.h>
//END Includes

// from kfiledialog.cpp - avoid qt warning in STDERR (~/.xsessionerrors)
static void silenceQToolBar(QtMsgType, const char *)
{}


KDevFileSelectorToolBar::KDevFileSelectorToolBar(QWidget *parent)
        : KToolBar( parent, "KDev FileSelector Toolbar", true )
{
    setMinimumWidth(10);
}

KDevFileSelectorToolBar::~KDevFileSelectorToolBar()
{}

void KDevFileSelectorToolBar::setMovingEnabled( bool)
{
    KToolBar::setMovingEnabled(false);
}


KDevFileSelectorToolBarParent::KDevFileSelectorToolBarParent(QWidget *parent)
        :QFrame(parent),m_tb(0)
{}

KDevFileSelectorToolBarParent::~KDevFileSelectorToolBarParent()
{}

void KDevFileSelectorToolBarParent::setToolBar(KDevFileSelectorToolBar *tb)
{
    m_tb=tb;
}

void KDevFileSelectorToolBarParent::resizeEvent ( QResizeEvent * )
{
    if (m_tb)
    {
        setMinimumHeight(m_tb->sizeHint().height());
        m_tb->resize(width(),height());
    }
}


//BEGIN Constructor/destructor

QColor KDevFileSelector::altBgColor;


KDevFileSelector::KDevFileSelector( QWidget * parent, const char * name )
        : QWidget(parent, name)
{
    mActionCollection = new KActionCollection( this );
    QVBoxLayout* lo = new QVBoxLayout(this);
    QtMsgHandler oldHandler = qInstallMsgHandler( silenceQToolBar );

    KDevFileSelectorToolBarParent *tbp=new KDevFileSelectorToolBarParent(this);
    toolbar = new KDevFileSelectorToolBar(tbp);
    tbp->setToolBar(toolbar);
    lo->addWidget(tbp);
    toolbar->setMovingEnabled(false);
    toolbar->setFlat(true);
    qInstallMsgHandler( oldHandler );

    cmbPath = new KURLComboBox( KURLComboBox::Directories, true, this, "path combo" );
    cmbPath->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ));
    KURLCompletion* cmpl = new KURLCompletion(KURLCompletion::DirCompletion);
    cmbPath->setCompletionObject( cmpl );
    lo->addWidget(cmbPath);
    cmbPath->listBox()->installEventFilter( this );

    dir = new KDevDirOperator( QString::null, this, "operator" );
    dir->setView( KFile::Detail );
    dir->setMode( KFile::Files );
    
    KActionCollection *coll = dir->actionCollection();
    // some shortcuts of diroperator that clashes with KDev
    coll->action( "delete" )->setShortcut( KShortcut( ALT + Key_Delete ) );
    coll->action( "reload" )->setShortcut( KShortcut( ALT + Key_F5 ) );
    coll->action( "back" )->setShortcut( KShortcut( ALT + SHIFT + Key_Left ) );
    coll->action( "forward" )->setShortcut( KShortcut( ALT + SHIFT + Key_Right ) );
    // some consistency - reset up for dir too
    coll->action( "up" )->setShortcut( KShortcut( ALT + SHIFT + Key_Up ) );
    coll->action( "home" )->setShortcut( KShortcut( CTRL + ALT + Key_Home ) );

    lo->addWidget(dir);
    lo->setStretchFactor(dir, 2);

    // bookmarks action!
    KActionMenu *acmBookmarks = new KActionMenu( i18n("Bookmarks"), "bookmark",
                                mActionCollection, "bookmarks" );
    acmBookmarks->setDelayed( false );

    bookmarkHandler = new KBookmarkHandler( this, acmBookmarks->popupMenu() );

    QHBox* filterBox = new QHBox(this);

    btnFilter = new QToolButton( filterBox );
    btnFilter->setIconSet( SmallIconSet("filter" ) );
    btnFilter->setToggleButton( true );
    filter = new KHistoryCombo( true, filterBox, "filter");
    filter->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ));
    filterBox->setStretchFactor(filter, 2);
    connect( btnFilter, SIGNAL( clicked() ), this, SLOT( btnFilterClick() ) );
    lo->addWidget(filterBox);

    connect( filter, SIGNAL( activated(const QString&) ),
             SLOT( slotFilterChange(const QString&) ) );
    connect( filter, SIGNAL( returnPressed(const QString&) ),
             filter, SLOT( addToHistory(const QString&) ) );

    // kaction for the dir sync method
    acSyncDir = new KAction( i18n("Current Document Directory"), "dirsynch", 0,
                             this, SLOT( setActiveDocumentDir() ), mActionCollection, "sync_dir" );
    toolbar->setIconText( KToolBar::IconOnly );
    toolbar->setIconSize( 16 );
    toolbar->setEnableContextMenu( false );

    connect( cmbPath, SIGNAL( urlActivated( const KURL&  )),
             this,  SLOT( cmbPathActivated( const KURL& ) ));
    connect( cmbPath, SIGNAL( returnPressed( const QString&  )),
             this,  SLOT( cmbPathReturnPressed( const QString& ) ));
    connect(dir, SIGNAL(urlEntered(const KURL&)),
            this, SLOT(dirUrlEntered(const KURL&)) );

    connect(dir, SIGNAL(finishedLoading()),
            this, SLOT(dirFinishedLoading()) );

    // Connect the bookmark handler
    connect( bookmarkHandler, SIGNAL( openURL( const QString& )),
             this, SLOT( setDir( const QString& ) ) );

    waitingUrl = QString::null;

    // whatsthis help
    QWhatsThis::add
        ( cmbPath,
                i18n("<p>Here you can enter a path for a directory to display."
                     "<p>To go to a directory previously entered, press the arrow on "
                     "the right and choose one. <p>The entry has directory "
                     "completion. Right-click to choose how completion should behave.") );
    QWhatsThis::add
        ( filter,
                i18n("<p>Here you can enter a name filter to limit which files are displayed."
                     "<p>To clear the filter, toggle off the filter button to the left."
                     "<p>To reapply the last filter used, toggle on the filter button." ) );
    QWhatsThis::add
        ( btnFilter,
                i18n("<p>This button clears the name filter when toggled off, or "
                     "reapplies the last filter used when toggled on.") );

    readConfig();
}

KDevFileSelector::~KDevFileSelector()
{}
//END Constructor/Destructor

//BEGIN Public Methods

void KDevFileSelector::readConfig()
{
    // set up the toolbar
    setupToolbar();

    cmbPath->setMaxItems( AmarokConfig::pathcomboHistoryLen() );
    cmbPath->setURLs( AmarokConfig::dirHistory() );

    // if we restore history
    if ( AmarokConfig::restoreLocation() || kapp->isRestored() )
    {    
        if ( ! AmarokConfig::location().isEmpty() )
            setDir( AmarokConfig::location() );
    }
            
    // else is automatic, as cmpPath->setURL is called when a location is entered.

    filter->setMaxCount( AmarokConfig::filterHistoryLen() );
    filter->setHistoryItems( AmarokConfig::filterHistory(), true );
    lastFilter = AmarokConfig::lastFilter();

    QString flt("");
    if ( AmarokConfig::restoreLastFilter() || kapp->isRestored() )
        flt = AmarokConfig::currentFilter();
    filter->lineEdit()->setText( flt );
    slotFilterChange( flt );

    autoSyncEvents = ( AmarokConfig::autoSyncEvents() );
}


void KDevFileSelector::setupToolbar()
{
    toolbar->clear();

    QStringList tbactions;
    // resonable collection for default toolbar
    tbactions << "up" << "back" << "forward" << "home"
              << "short view" << "detailed view" << "bookmarks";

    KAction *ac;
    for ( QStringList::Iterator it=tbactions.begin(); it != tbactions.end(); ++it )
    {
        if ( *it == "bookmarks" || *it == "sync_dir" )
            ac = mActionCollection->action( (*it).latin1() );
        else
            ac = dir->actionCollection()->action( (*it).latin1() );
        if ( ac )
            ac->plug( toolbar );
    }
}

void KDevFileSelector::writeConfig()
{
    AmarokConfig::setPathcomboHistoryLen( cmbPath->maxItems() );

    QStringList l;
    for (int i = 0; i < cmbPath->count(); i++)
        l.append( cmbPath->text( i ) );
    AmarokConfig::setDirHistory( l );

    AmarokConfig::setLocation( cmbPath->currentText() );
    AmarokConfig::setFilterHistoryLen( filter->maxCount() );
    AmarokConfig::setFilterHistory( filter->historyItems() );
    AmarokConfig::setCurrentFilter( filter->currentText() );
    AmarokConfig::setLastFilter( lastFilter );
    AmarokConfig::setAutoSyncEvents( autoSyncEvents );
}


//END Public Methods

//BEGIN Public Slots

void KDevFileSelector::slotFilterChange( const QString & nf )
{
    QString f = nf.stripWhiteSpace();
    bool empty = f.isEmpty() || f == "*";
    if ( empty )
    {
        dir->clearFilter();
        filter->lineEdit()->setText( QString::null );
        QToolTip::add
            ( btnFilter,
                    QString( i18n("Apply last filter (\"%1\")") ).arg( lastFilter ) );
    }
    else
    {
        dir->setNameFilter( f );
        lastFilter = f;
        QToolTip::add
            ( btnFilter, i18n("Clear filter") );
    }
    btnFilter->setOn( !empty );
    dir->updateDir();
    // this will be never true after the filter has been used;)
    btnFilter->setEnabled( !( empty && lastFilter.isEmpty() ) );

}
void KDevFileSelector::setDir( KURL u )
{
    dir->setURL(u, true);
}

//END Public Slots

//BEGIN Private Slots

void KDevFileSelector::cmbPathActivated( const KURL& u )
{
    cmbPathReturnPressed( u.url() );
}

void KDevFileSelector::cmbPathReturnPressed( const QString& u )
{
    QStringList urls = cmbPath->urls();
    urls.remove( u );
    urls.prepend( u );
    cmbPath->setURLs( urls, KURLComboBox::RemoveBottom );
    dir->setFocus();
    dir->setURL( KURL(u), true );
}

void KDevFileSelector::dirUrlEntered( const KURL& u )
{
    cmbPath->setURL( u );
}

void KDevFileSelector::dirFinishedLoading()
{}


/*
   When the button in the filter box toggles:
   If off:
   If the name filer is anything but "" or "*", reset it.
   If on:
   Set last filter.
*/
void KDevFileSelector::btnFilterClick()
{
    if ( !btnFilter->isOn() )
    {
        slotFilterChange( QString::null );
    }
    else
    {
        filter->lineEdit()->setText( lastFilter );
        slotFilterChange( lastFilter );
    }
}


void KDevFileSelector::autoSync()
{
    kdDebug()<<"KDevFileSelector::autoSync()"<<endl;
    // if visible, sync
    if ( isVisible() )
    {
        setActiveDocumentDir();
        waitingUrl = QString::null;
    }
    // else set waiting url
    else
    {
        KURL u = activeDocumentUrl();
        if (!u.isEmpty())
            waitingUrl = u.directory();
    }
}


/// \FIXME crash on shutdown
void KDevFileSelector::setActiveDocumentDir()
{
    //kdDebug()<<"KDevFileSelector::setActiveDocumentDir()"<<endl;
    KURL u = activeDocumentUrl();
    if (!u.isEmpty())
        setDir( u.upURL() );
}

void KDevFileSelector::viewChanged()
{
    /// @todo make sure the button is disabled if the directory is unreadable, eg
    ///       the document URL has protocol http
    acSyncDir->setEnabled( ! activeDocumentUrl().directory().isEmpty() );
}

//END Private Slots

//BEGIN Protected

void KDevFileSelector::focusInEvent( QFocusEvent * )
{
    dir->setFocus();
}

void KDevFileSelector::showEvent( QShowEvent * )
{
    // sync if we should
    if ( autoSyncEvents & GotVisible )
    {
        kdDebug()<<"syncing fs on show"<<endl;
        setActiveDocumentDir();
        waitingUrl = QString::null;
    }
    // else, if we have a waiting URL set it
    else if ( ! waitingUrl.isEmpty() )
    {
        setDir( waitingUrl );
        waitingUrl = QString::null;
    }
}

bool KDevFileSelector::eventFilter( QObject* o, QEvent *e )
{
    /*
        This is rather unfortunate, but:
        QComboBox does not support setting the size of the listbox to something
        resonable. Even using listbox->setVariableWidth() does not yeld a
        satisfying result, something is wrong with the handling of the sizehint.
        And the popup is rather useless, if the paths are only partly visible.
    */
    QListBox *lb = cmbPath->listBox();
    if ( o == lb && e->type() == QEvent::Show )
    {
        int add
            = lb->height() < lb->contentsHeight() ? lb->verticalScrollBar()->width() : 0;
        //FIXME
        int w = QMIN( 200, lb->contentsWidth() + add );
        //         int w = QMIN( mainwin->main()->width(), lb->contentsWidth() + add );
        lb->resize( w, lb->height() );
        /// @todo - move the listbox to a suitable place if nessecary
        /// @todo - decide if it is worth caching the size while untill the contents
        ///        are changed.
    }
    /// @todo - same thing for the completion popup?
    return QWidget::eventFilter( o, e );
}

//END Protected

//BEGIN ACtionLBItem
/*
   QListboxItem that can store and return a string,
   used for the toolbar action selector.
*/
class ActionLBItem : public QListBoxPixmap
{
    public:
        ActionLBItem( QListBox *lb=0,
                      const QPixmap &pm = QPixmap(),
                      const QString &text=QString::null,
                      const QString &str=QString::null ) :
                QListBoxPixmap( lb, pm, text ),
                _str(str)
        {}
        ;
        QString idstring()
        {
            return _str;
        };
    private:
        QString _str;
};

KURL KDevFileSelector::activeDocumentUrl( )
{
    /*    KTextEditor::Document* doc = dynamic_cast<KTextEditor::Document*>( partController->activePart() );
        if( doc )
    	return doc->url();*/

    return KURL();
}
//END ActionLBItem


//BEGIN KDevDirOperator

void KDevDirOperator::activatedMenu( const KFileItem *fi, const QPoint & pos )
{
    setupMenu();
    updateSelectionDependentActions();

    KActionMenu * am = dynamic_cast<KActionMenu*>(actionCollection()->action("popupMenu"));
    if (!am)
        return;
    KPopupMenu *popup = am->popupMenu();

    if (fi)
    {
        /*        FileContext context( KURL::List(fi->url()));
                if ( (m_part) && (m_part->core()))
                    m_part->core()->fillContextMenu(popup, &context);*/
    }

    popup->popup(pos);
}

//we override this method, so that we can set the alternateBackgroundColor
#include <klistview.h>
#include <kfileview.h>
KFileView* KDevDirOperator::createView( QWidget *parent, KFile::FileView view )
{
    kdDebug() << "[KDevDirOperator::createView()]" << endl;
    
    KFileView *pView = KDirOperator::createView( parent, view );
    
    if ( dynamic_cast<QObject*>( pView )->inherits( "KListView" ) )
        dynamic_cast<KListView*>( pView )->setAlternateBackground( KDevFileSelector::altBgColor );
    
    return pView;
}

//END KDevDirOperator


#include "filebrowser.moc"

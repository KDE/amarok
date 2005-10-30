#include "debug.h"
#include "newdynamic.h"
#include "party.h"
#include "playlistbrowser.h"
#include "playlistselection.h"

#include <qcheckbox.h>
#include <qlistview.h>
#include <qstringlist.h>

#include <kapplication.h>
#include <knuminput.h>
#include <klistview.h>
#include <klocale.h>

PlaylistSelection::PlaylistSelection(QWidget* parent, char* name)
: KListView(parent, name)
{ 
    addColumn("Select Playlists");
    setRootIsDecorated(true);
    PlaylistBrowserView* browserTree = PlaylistBrowser::instance()->getListView();
    QListViewItem* browserItem = browserTree->firstChild();
    //load into the tree the first two items, which is the smart playlist and the playlist
    for(int i = 0; i<2; i++)
    {
        QListViewItem* newItem = new QListViewItem(this, browserItem->text(0));
        newItem->setPixmap(0,*browserItem->pixmap(0));
        loadChildren(browserItem, newItem);
        newItem->setOpen(true);
        browserItem= browserItem->nextSibling();
    }

}

void PlaylistSelection::loadChildren(QListViewItem* browserParent, QListViewItem* selectionParent)
{
    QListViewItem* browserChild = browserParent->firstChild();
    while(browserChild) 
    {
        SelectionListItem* selectionChild = new SelectionListItem(selectionParent, browserChild->text(0), browserChild);
        selectionChild->setPixmap(0,*browserChild->pixmap(0));
        if(browserChild->childCount() > 0)
            loadChildren(browserChild, selectionChild);
        browserChild = browserChild->nextSibling();
    }
}
////////////////////////////////
/// ConfigDynamic
////////////////////////////////
void ConfigDynamic::dynamicDialog(QWidget* parent)
{
    KDialogBase dialog(parent, "new dynamic", true, i18n("Create Dynamic Playlist"), KDialogBase::Ok | KDialogBase::Cancel);
    kapp->setTopWidget( &dialog );
    NewDynamic* nd = new NewDynamic( &dialog, "new dynamic");
    dialog.setMainWidget(nd);
    if(dialog.exec() == QDialog::Accepted)
        addDynamic(nd);
}
void ConfigDynamic::addDynamic(NewDynamic* dialog)
{
    QListViewItem* parent =PlaylistBrowser::instance()->getDynamicCategory();

    PartyEntry *saveMe = new PartyEntry( parent, 0, dialog->m_name->text() );

    saveMe->setCycled( dialog->m_cycleTracks->isChecked() );
    saveMe->setMarked( dialog->m_markHistory->isChecked() );
    saveMe->setUpcoming( dialog->m_upcomingIntSpinBox->value() );
    saveMe->setPrevious( dialog->m_previousIntSpinBox->value() );
    saveMe->setAppendCount( dialog->m_appendCountIntSpinBox->value() );
    saveMe->setAppendType( Party::CUSTOM );

    QStringList list;
    debug() << "Saving custom list..." << endl;
    QListViewItemIterator it(dialog->selectPlaylist, QListViewItemIterator::Checked);
    while(it.current()) {
        list.append( it.current()->text(0) );
        ++it;
    }
    saveMe->setItems( list );
    parent->sortChildItems( 0, true );
    parent->setOpen( true );

    PlaylistBrowser::instance()->saveDynamics();
}
////////////////////////////////
/// SelectionListItem
////////////////////////////////
SelectionListItem::SelectionListItem(QCheckListItem * parent, const QString& text, QListViewItem* browserEquivalent)
: QCheckListItem(parent, text, QCheckListItem::CheckBox)
, m_browserEquivalent(browserEquivalent)
{ }

SelectionListItem::SelectionListItem(QListViewItem * parent, const QString& text, QListViewItem* browserEquivalent)
: QCheckListItem(parent, text, QCheckListItem::CheckBox)
, m_browserEquivalent(browserEquivalent)
{ }

void SelectionListItem::stateChange(bool b)
{
    QListViewItem* it = firstChild();
    while (it) {
        static_cast<SelectionListItem*>(it)->setOn(b); //calls stateChange, so is recursive
        it=it->nextSibling();
    }
}
#include "playlistselection.moc"

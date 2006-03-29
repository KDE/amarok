/***************************************************************************
 *   Copyright (C) 2005 Ian Monroe <ian@monroe.nu>                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#define DEBUG_PREFIX "PlaylistSelection"

#include "amarok.h" //foreach
#include "debug.h"
#include "newdynamic.h"
#include "dynamicmode.h"
#include "playlistbrowser.h"
#include "playlistselection.h"

#include <qcheckbox.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlistview.h>
#include <qsizepolicy.h>
#include <qstringlist.h>

#include <kapplication.h>
#include <kiconloader.h>
#include <knuminput.h>
#include <klistview.h>
#include <klocale.h>

PlaylistSelection::PlaylistSelection( QWidget* parent, char* name )
    : KListView( parent, name )
{
    addColumn( i18n("Select Playlists") );
    setRootIsDecorated( true );
    PlaylistBrowserView* browserTree = PlaylistBrowser::instance()->getListView();
    QListViewItem*       browserItem = browserTree->firstChild();
    //load into the tree the first two items, which is the smart playlist and the playlist
    for( int i = 0; i<2; i++ )
    {
        QListViewItem* newItem = new QListViewItem( this, browserItem->text(0) );
        newItem->setPixmap( 0, *browserItem->pixmap(0) );
        loadChildren( browserItem, newItem );
        newItem->setOpen( true );
        browserItem = browserItem->nextSibling();
    }
}

void PlaylistSelection::loadChildren( QListViewItem* browserParent, QListViewItem* selectionParent )
{
    QListViewItem* browserChild = browserParent->firstChild();

    while( browserChild )
    {
        SelectionListItem* selectionChild = new SelectionListItem( selectionParent, browserChild->text(0), browserChild );
        if ( browserChild->pixmap(0) )
            selectionChild->setPixmap( 0, *browserChild->pixmap(0) );

        if( browserChild->childCount() > 0 )
            loadChildren( browserChild, selectionChild );

        browserChild = browserChild->nextSibling();
    }
}

////////////////////////////////
/// ConfigDynamic
////////////////////////////////
namespace ConfigDynamic
{
    KDialogBase* basicDialog( QWidget* parent )
    {
        KDialogBase* dialog = new KDialogBase( parent, "new dynamic", true,
                              i18n("Create Dynamic Playlist"),
                              KDialogBase::Ok | KDialogBase::Cancel,
                              KDialogBase::Ok, true );
        kapp->setTopWidget( dialog );
        dialog->setCaption( i18n("Dynamic Mode") );
        NewDynamic* nd = new NewDynamic( dialog, "new dynamic");
        //QSizePolicy policy;
        //policy.setHorData(QSizePolicy::Maximum);
        //dialog->setSizePolicy(policy);
        dialog->setMainWidget( nd );
        return dialog;
    }

    void dynamicDialog( QWidget* parent )
    {
        KDialogBase* dialog = basicDialog( parent );
        NewDynamic*  nd     = static_cast<NewDynamic*>(dialog->mainWidget());
        nd->m_mixLabel->setText( i18n("Add Dynamic Playlist") );

        if( dialog->exec() == QDialog::Accepted )
            addDynamic( nd );
    }

    void editDynamicPlaylist( QWidget* parent, DynamicMode* mode )
    {
        DEBUG_BLOCK
        KDialogBase* dialog = basicDialog( parent );
        NewDynamic*  nd     = static_cast<NewDynamic*>(dialog->mainWidget());

        nd->m_name->setText( mode->title() );
        nd->m_cycleTracks->setChecked( mode->cycleTracks() );
        nd->m_markHistory->setChecked( mode->markHistory() );
        nd->m_upcomingIntSpinBox->setValue( mode->upcomingCount() );
        nd->m_previousIntSpinBox->setValue( mode->previousCount() );
        nd->m_appendCountIntSpinBox->setValue( mode->appendCount() );

        if( mode->appendType() == DynamicMode::CUSTOM )
        {
            //check items in the custom playlist
            nd->m_mixLabel->setText( i18n("Edit Dynamic Playlist") );
            QStringList items = mode->items();
            foreach( items )
            {
                QCheckListItem* current = static_cast<QCheckListItem*>( nd->selectPlaylist->findItem((*it),0) );
                if( current )
                    current->setOn(true);
            }
        }
        else //if its a suggested song or a random mix...
        {
           nd->selectPlaylist->hide();
           nd->layout()->remove( nd->selectPlaylist );
           if( mode->appendType() == DynamicMode::RANDOM )
           {
              nd->m_mixLabel->setText( i18n("Random Mix") );
           }
           else
           {
              nd->m_mixLabel->setText( i18n("Suggested Songs") );
           }
        }

        nd->updateGeometry();
        dialog->resize( nd->minimumSizeHint() );

        if( dialog->exec() == QDialog::Accepted )
        {
            loadDynamicMode( mode, nd );
            PlaylistBrowser::instance()->getDynamicCategory()->sortChildItems( 0, true );
            PlaylistBrowser::instance()->saveDynamics();
        }

    }

    void loadDynamicMode( DynamicMode* saveMe, NewDynamic* dialog )
    {
        saveMe->setTitle( dialog->m_name->text() );
        saveMe->setCycleTracks( dialog->m_cycleTracks->isChecked() );
        saveMe->setMarkHistory( dialog->m_markHistory->isChecked() );
        saveMe->setUpcomingCount( dialog->m_upcomingIntSpinBox->value() );
        saveMe->setPreviousCount( dialog->m_previousIntSpinBox->value() );
        saveMe->setAppendCount( dialog->m_appendCountIntSpinBox->value() );

        QStringList list;
        debug() << "Saving custom list..." << endl;
        QListViewItemIterator it( dialog->selectPlaylist, QListViewItemIterator::Checked );

        while( it.current() )
        {
            list.append( it.current()->text(0) );
            ++it;
        }
        saveMe->setItems( list );
    }

    void addDynamic( NewDynamic* dialog )
    {
        QListViewItem *parent = PlaylistBrowser::instance()->getDynamicCategory();
        DynamicEntry    *saveMe = new DynamicEntry( parent, 0, dialog->m_name->text() );
        saveMe->setAppendType( DynamicMode::CUSTOM );

        loadDynamicMode( saveMe, dialog );

        parent->sortChildItems( 0, true );
        parent->setOpen( true );

        PlaylistBrowser::instance()->saveDynamics();
    }

}
////////////////////////////////
/// SelectionListItem
////////////////////////////////
SelectionListItem::SelectionListItem( QCheckListItem * parent, const QString& text, QListViewItem* browserEquivalent )
    : QCheckListItem( parent, text, QCheckListItem::CheckBox )
    , m_browserEquivalent( browserEquivalent )
{ }

SelectionListItem::SelectionListItem( QListViewItem * parent, const QString& text, QListViewItem* browserEquivalent )
    : QCheckListItem( parent, text, QCheckListItem::CheckBox )
    , m_browserEquivalent( browserEquivalent )
{ }

void SelectionListItem::stateChange( bool b )
{
    QListViewItem* it = firstChild();
    while( it )
    {
        static_cast<SelectionListItem*>(it)->setOn( b ); //calls stateChange, so is recursive
        it = it->nextSibling();
    }
}
#include "playlistselection.moc"

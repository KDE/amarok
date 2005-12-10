/*
  (c) 2005 Alexandre Oliveira <aleprj@gmail.com>
  (c) 2003 Frerich Raabe <raabe@kde.org>

 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

#include "tagguesser.h"

#include <qevent.h>

#include <kapplication.h>
#include <klineedit.h>
#include <kiconloader.h>


void TagGuesserConfigDialog::init()
{
    setCaption( i18n( "Guess By Filename Configuration" ) );
    lvSchemes->setItemsRenameable( true );
    lvSchemes->setSorting( -1 );
    lvSchemes->setDefaultRenameAction( QListView::Accept );
    bMoveUp->setIconSet( SmallIconSet( "1uparrow" ) );
    bMoveDown->setIconSet( SmallIconSet( "1downarrow" ) );

    const QStringList schemes = TagGuesser::schemeStrings();
    QStringList::ConstIterator it = schemes.begin();
    QStringList::ConstIterator end = schemes.end();
    for ( ; it != end; ++it ) {
        KListViewItem *item = new KListViewItem( lvSchemes, *it );
        item->moveItem( lvSchemes->lastItem() );
    }

    connect( lvSchemes, SIGNAL( currentChanged( QListViewItem * ) ),
            this, SLOT( slotCurrentChanged( QListViewItem * ) ) );
    connect( lvSchemes, SIGNAL( doubleClicked( QListViewItem *, const QPoint &, int ) ),
            this, SLOT( slotRenameItem( QListViewItem *, const QPoint &, int ) ) );
    connect( bMoveUp, SIGNAL( clicked() ), this, SLOT( slotMoveUpClicked() ) );
    connect( bMoveDown, SIGNAL( clicked() ), this, SLOT( slotMoveDownClicked() ) );
    connect( bAdd, SIGNAL( clicked() ), this, SLOT( slotAddClicked() ) );
    connect( bModify, SIGNAL( clicked() ), this, SLOT( slotModifyClicked() ) );
    connect( bRemove, SIGNAL( clicked() ), this, SLOT( slotRemoveClicked() ) );
    connect( bOk, SIGNAL( clicked() ), this, SLOT( accept() ) );
    connect( bCancel, SIGNAL( clicked() ), this, SLOT( reject() ) );

    lvSchemes->setSelected( lvSchemes->firstChild(), true );
    slotCurrentChanged( lvSchemes->currentItem() );
    resize( 400, 300 );
}

void TagGuesserConfigDialog::slotCurrentChanged(QListViewItem *item)
{
    bMoveUp->setEnabled( item != 0 && item->itemAbove() != 0 );
    bMoveDown->setEnabled( item != 0 && item->itemBelow() != 0 );
    bModify->setEnabled( item != 0 );
    bRemove->setEnabled( item != 0 );
}

void TagGuesserConfigDialog::accept()
{
    if(lvSchemes->renameLineEdit()) {
        QKeyEvent returnKeyPress(QEvent::KeyPress, Key_Return, 0, 0);
        KApplication::sendEvent( lvSchemes->renameLineEdit(), &returnKeyPress );
    }

    QStringList schemes;
    for ( QListViewItem *it = lvSchemes->firstChild(); it; it = it->nextSibling() )
        schemes += it->text(0);
    TagGuesser::setSchemeStrings( schemes );
    KDialog::accept();
}


void TagGuesserConfigDialog::reject()
{
    KDialog::reject();
}


void TagGuesserConfigDialog::slotRenameItem(QListViewItem *item, const QPoint &, int c)
{
    lvSchemes->rename(item, c);
}

void TagGuesserConfigDialog::slotMoveUpClicked()
{
    QListViewItem *item = lvSchemes->currentItem();
    if( item->itemAbove() == lvSchemes->firstChild() )
        item->itemAbove()->moveItem(item);
    else
      item->moveItem(item->itemAbove()->itemAbove());
    lvSchemes->ensureItemVisible(item);
    slotCurrentChanged(item);
}

void TagGuesserConfigDialog::slotMoveDownClicked()
{
    QListViewItem *item = lvSchemes->currentItem();
    item->moveItem( item->itemBelow() );
    lvSchemes->ensureItemVisible(item);
    slotCurrentChanged(item);
}

void TagGuesserConfigDialog::slotAddClicked()
{
    KListViewItem *item = new KListViewItem( lvSchemes );
    lvSchemes->rename(item, 0);
}

void TagGuesserConfigDialog::slotModifyClicked()
{
    lvSchemes->rename(lvSchemes->currentItem(), 0);
}

void TagGuesserConfigDialog::slotRemoveClicked()
{
    delete lvSchemes->currentItem();
}

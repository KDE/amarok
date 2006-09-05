/***************************************************************************
    begin                : Tue Aug 31 21:59:58 EST 2004
    copyright            : (C) 2004 by Michael Pyne <michael.pyne@kdemail.net>
                           (C) 2006 by Ian Monroe <ian@monroe.nu>
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <kconfig.h>
#include <kdeversion.h>
#include <kdialogbase.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <kio/job.h>
#include <klocale.h>
#include <kstdguiitem.h>
#include <kurl.h>

#include <qstringlist.h>
#include <qcheckbox.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qtimer.h>
#include <qvbox.h>
#include <qhbox.h>

#include "amarok.h"
#include "playlist.h"
#include "collectiondb.h"
#include "deletedialog.h"
#include "statusbar.h"

//////////////////////////////////////////////////////////////////////////////
// DeleteWidget implementation
//////////////////////////////////////////////////////////////////////////////

DeleteWidget::DeleteWidget(QWidget *parent, const char *name)
    : DeleteDialogBase(parent, name)
{
    KConfigGroup messageGroup(KGlobal::config(), "FileRemover");

    bool deleteInstead = messageGroup.readBoolEntry("deleteInsteadOfTrash", false);
    slotShouldDelete(deleteInstead);
    ddShouldDelete->setChecked(deleteInstead);
}

void DeleteWidget::setFiles(const KURL::List &files)
{
    ddFileList->clear();
//    ddFileList->insertStringList(files);
    for( KURL::List::ConstIterator it = files.begin(); it != files.end(); it++)
    {
        if( (*it).isLocalFile() ) //path is nil for non-local
            ddFileList->insertItem( (*it).path() );
        else
            ddFileList->insertItem( (*it).url() );
    }
    ddNumFiles->setText(i18n("<b>1</b> file selected.", "<b>%n</b> files selected.", files.count()));
}

void DeleteWidget::slotShouldDelete(bool shouldDelete)
{
    if(shouldDelete) {
        ddDeleteText->setText(i18n("<qt>These items will be <b>permanently "
            "deleted</b> from your hard disk.</qt>"));
        ddWarningIcon->setPixmap(KGlobal::iconLoader()->loadIcon("messagebox_warning",
            KIcon::Desktop, KIcon::SizeLarge));
    }
    else {
        ddDeleteText->setText(i18n("<qt>These items will be moved to the Trash Bin.</qt>"));
        ddWarningIcon->setPixmap(KGlobal::iconLoader()->loadIcon("trashcan_full",
            KIcon::Desktop, KIcon::SizeLarge));
    }
}

//////////////////////////////////////////////////////////////////////////////
// DeleteDialog implementation
//////////////////////////////////////////////////////////////////////////////

DeleteDialog::DeleteDialog(QWidget *parent, const char *name) :
    KDialogBase(Swallow, WStyle_DialogBorder, parent, name,
        true /* modal */, i18n("About to delete selected files"),
        Ok | Cancel, Cancel /* Default */, true /* separator */),
    m_trashGuiItem(i18n("&Send to Trash"), "trashcan_full")
{
    m_widget = new DeleteWidget(this, "delete_dialog_widget");
    setMainWidget(m_widget);

    m_widget->setMinimumSize(400, 300);
    setMinimumSize(410, 326);
    adjustSize();

    slotShouldDelete(shouldDelete());
    connect(m_widget->ddShouldDelete, SIGNAL(toggled(bool)), SLOT(slotShouldDelete(bool)));

}

bool DeleteDialog::confirmDeleteList(const KURL::List& condemnedFiles)
{
    m_widget->setFiles(condemnedFiles);

    return exec() == QDialog::Accepted;
}

void DeleteDialog::setFiles(const KURL::List &files)
{
    m_widget->setFiles(files);
}

void DeleteDialog::accept()
{
    KConfigGroup messageGroup(KGlobal::config(), "FileRemover");

    // Save user's preference

    messageGroup.writeEntry("deleteInsteadOfTrash", shouldDelete());
    messageGroup.sync();

    KDialogBase::accept();
}

void DeleteDialog::slotShouldDelete(bool shouldDelete)
{
    setButtonGuiItem(Ok, shouldDelete ? KStdGuiItem::del() : m_trashGuiItem);
}

bool DeleteDialog::showTrashDialog(QWidget* parent, const KURL::List& files)
{
    DeleteDialog dialog(parent);
    bool doDelete = dialog.confirmDeleteList(files);

    if( doDelete )
    {
        KIO::Job* job = 0;
        bool shouldDelete = dialog.shouldDelete();
        if ( ( shouldDelete && (job = KIO::del( files )) ) ||
             ( job = Amarok::trashFiles( files )   ) )
        {
            if(shouldDelete) //amarok::trashFiles already does the progress operation
                Amarok::StatusBar::instance()->newProgressOperation( job )
                    .setDescription( i18n("Deleting files") );
        }

    }

    return doDelete;
}
#include "deletedialog.moc"

// vim: set et ts=4 sw=4:

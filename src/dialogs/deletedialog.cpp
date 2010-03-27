/****************************************************************************************
 * Copyright (c) 2004 Michael Pyne <michael.pyne@kdemail.net>                           *
 * Copyright (c) 2006 Ian Monroe <ian@monroe.nu>                                        *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "deletedialog.h"

#include "core/support/Amarok.h"

#include "App.h"
#include "statusbar/StatusBar.h"

#include <KConfigGroup>
#include <KDialog>
#include <KGlobal>
#include <KIconLoader>
#include <KIO/DeleteJob>
#include <KLocale>
#include <KStandardGuiItem>
#include <KUrl>



//////////////////////////////////////////////////////////////////////////////
// DeleteWidget implementation
//////////////////////////////////////////////////////////////////////////////

DeleteWidget::DeleteWidget(QWidget *parent)
    : DeleteDialogBase(parent)
{
    KConfigGroup messageGroup(KGlobal::config(), "FileRemover");

    bool deleteInstead = messageGroup.readEntry("deleteInsteadOfTrash", false);
    slotShouldDelete(deleteInstead);
    ddShouldDelete->setChecked(deleteInstead);
}

void DeleteWidget::setFiles(const KUrl::List &files)
{
    ddFileList->clear();
//    ddFileList->insertStringList(files);
    for( KUrl::List::ConstIterator it = files.constBegin(), end = files.constEnd(); it != end; ++it)
    {
        if( (*it).isLocalFile() ) //path is nil for non-local
            ddFileList->insertItem( (*it).path() );
        else
            ddFileList->insertItem( (*it).url() );
    }
    ddNumFiles->setText(i18np("<b>1</b> file selected.", "<b>%1</b> files selected.", files.count()));
}

void DeleteWidget::slotShouldDelete(bool shouldDelete)
{
    if(shouldDelete) {
        ddDeleteText->setText(i18n("<qt>These items will be <b>permanently "
            "deleted</b> from your hard disk.</qt>"));
        ddWarningIcon->setPixmap(KIconLoader::global()->loadIcon("dialog-warning",
            KIconLoader::Desktop, KIconLoader::SizeLarge));
    }
    else {
        ddDeleteText->setText(i18n("<qt>These items will be moved to the Trash Bin.</qt>"));
        ddWarningIcon->setPixmap(KIconLoader::global()->loadIcon("user-trash-full",
            KIconLoader::Desktop, KIconLoader::SizeLarge));
    }
}

//////////////////////////////////////////////////////////////////////////////
// DeleteDialog implementation
//////////////////////////////////////////////////////////////////////////////

DeleteDialog::DeleteDialog( QWidget *parent, const char *name )
    : KDialog( parent ),
    m_trashGuiItem(i18n("&Send to Trash"), "user-trash-full")
{
//Swallow, Qt::WStyle_DialogBorder, parent, name,
        //true /* modal */, i18n("About to delete selected files"),
       // Ok | Cancel, Cancel /* Default */, true /* separator */
    setObjectName( name );
    setCaption( i18n("About to delete selected files") );
    setModal( true );
    setButtons( Ok | Cancel );
    setDefaultButton( Cancel );
    showButtonSeparator( true );

    m_widget = new DeleteWidget(this);
    m_widget->setObjectName("delete_dialog_widget");
    setMainWidget(m_widget);

    m_widget->setMinimumSize(400, 300);
    setMinimumSize(410, 326);
    adjustSize();

    slotShouldDelete(shouldDelete());
    connect(m_widget->ddShouldDelete, SIGNAL(toggled(bool)), SLOT(slotShouldDelete(bool)));

}

bool DeleteDialog::confirmDeleteList(const KUrl::List& condemnedFiles)
{
    m_widget->setFiles(condemnedFiles);

    return exec() == QDialog::Accepted;
}

void DeleteDialog::setFiles(const KUrl::List &files)
{
    m_widget->setFiles(files);
}

void DeleteDialog::accept()
{
    KConfigGroup messageGroup(KGlobal::config(), "FileRemover");

    // Save user's preference

    messageGroup.writeEntry("deleteInsteadOfTrash", shouldDelete());
    messageGroup.sync();

    KDialog::accept();
}

void DeleteDialog::slotShouldDelete(bool shouldDelete)
{
    setButtonGuiItem(Ok, shouldDelete ? KStandardGuiItem::del() : m_trashGuiItem);
}

bool DeleteDialog::showTrashDialog(QWidget* parent, const KUrl::List& files)
{
    DeleteDialog dialog(parent);
    bool doDelete = dialog.confirmDeleteList(files);

    if( doDelete )
    {
        KIO::Job* job = 0;
        bool shouldDelete = dialog.shouldDelete();
        if ( ( shouldDelete && (job = KIO::del( files )) ) ||
             ( job = App::instance()->trashFiles( files )   ) )
        {
            if(shouldDelete) //amarok::trashFiles already does the progress operation
                The::statusBar()->newProgressOperation( job, i18n("Deleting files") );

        }

    }

    return doDelete;
}
#include "deletedialog.moc"

// vim: set et ts=4 sw=4:


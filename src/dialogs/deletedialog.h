/****************************************************************************************
 * Copyright (c) 2004 Michael Pyne <michael.pyne@kdemail.net>                           *
 * Copyright (c) 2006 Ian Monroe <ian@monroe.nu>                                        *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef _DELETEDIALOG_H
#define _DELETEDIALOG_H

#include "ui_deletedialogbase.h"

#include <KDialog>
#include <KUrl>

#include <QCheckBox>
#include <QLabel>

class KGuiItem;
class QLabel;

class DeleteDialogBase : public QWidget, public Ui::DeleteDialogBase
{
public:
  DeleteDialogBase( QWidget *parent ) : QWidget( parent ) {
    setupUi( this );
  }
};


class DeleteWidget : public DeleteDialogBase
{
    Q_OBJECT

public:
    DeleteWidget(QWidget *parent = 0);

    void setFiles(const KUrl::List &files);

protected slots:
    virtual void slotShouldDelete(bool shouldDelete);
};

class DeleteDialog : public KDialog
{
    Q_OBJECT

public:
    explicit DeleteDialog(QWidget *parent, const char *name = "delete_dialog");
    static bool showTrashDialog(QWidget* parent, const KUrl::List &files);

    bool confirmDeleteList(const KUrl::List &condemnedFiles);
    void setFiles(const KUrl::List &files);
    bool shouldDelete() const { return m_widget->ddShouldDelete->isChecked(); }

protected slots:
    virtual void accept();
    void slotShouldDelete(bool shouldDelete);

private:
    DeleteWidget *m_widget;
    KGuiItem m_trashGuiItem;
};

#endif

// vim: set et ts=4 sw=4:

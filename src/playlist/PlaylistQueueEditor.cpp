/****************************************************************************************
 * Copyright (c) 2010 Andreas Hartmetz <ahartmetz@gmail.com>                            *
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

#include "PlaylistQueueEditor.h"

#include "PlaylistActions.h"
#include "PlaylistModelStack.h"

static const int s_idRole = Qt::UserRole;
static const int s_myType = QListWidgetItem::UserType;

//### due to playlists typically having no more than 10k items and no more than
//    100 queued items we can get away with using simple and slow algorithms.

PlaylistQueueEditor::PlaylistQueueEditor()
    : QDialog(),
      m_blockViewUpdates(false)
{
    m_ui.setupUi(this);
    updateView();
    connect(The::playlist()->qaim(), SIGNAL(queueChanged()), SLOT(queueChanged()));
    m_ui.upButton->setIcon(KIcon("go-up"));
    m_ui.downButton->setIcon(KIcon("go-down"));
    m_ui.clearButton->setIcon(KIcon("edit-clear-list"));
    connect(m_ui.upButton, SIGNAL(clicked()), SLOT(moveUp()));
    connect(m_ui.downButton, SIGNAL(clicked()), SLOT(moveDown()));
    connect(m_ui.clearButton, SIGNAL(clicked()), SLOT(clear()));
    connect(m_ui.buttonBox->buttons().first(), SIGNAL(clicked()), SLOT(accept()));
}

void PlaylistQueueEditor::updateView()
{
    if (m_blockViewUpdates) {
        return;
    }
    m_ui.listWidget->clear();
    foreach (quint64 id, The::playlistActions()->queue()) {
        QListWidgetItem *item = new QListWidgetItem(m_ui.listWidget, s_myType);
        item->setData(s_idRole, id);
        item->setText(The::playlist()->trackForId(id)->fixedName());
    }
}

void PlaylistQueueEditor::queueChanged()
{
    updateView();
}

quint64 PlaylistQueueEditor::currentId()
{
    if (QListWidgetItem *item = m_ui.listWidget->currentItem()) {
        bool ok;
        quint64 id = item->data(s_idRole).toULongLong(&ok);
        if (ok) {
            return id;
        }
    }
    return 0;
}

void PlaylistQueueEditor::moveUp()
{
    quint64 id = currentId();
    if (!id) {
        return;
    }
    //The::playlistActions()->queueMoveUp(id);
}

void PlaylistQueueEditor::moveDown()
{
    quint64 id = currentId();
    if (!id) {
        return;
    }
    //The::playlistActions()->queueMoveDown(id);
}

void PlaylistQueueEditor::clear()
{
    m_blockViewUpdates = true;
    QList<int> rowsToDequeue;
    foreach (quint64 id, The::playlistActions()->queue()) {
        Meta::TrackPtr track = The::playlist()->trackForId(id);
        foreach (int row, The::playlist()->allRowsForTrack(track)) {
            rowsToDequeue += row;
        }
    }
    The::playlistActions()->dequeue(rowsToDequeue);
    m_blockViewUpdates = false;
    updateView();
}
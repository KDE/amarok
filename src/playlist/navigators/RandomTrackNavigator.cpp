/***************************************************************************
 * copyright            : (C) 2008 Seb Ruiz <ruiz@kde.org>    
 *                      : (C) 2008 Soren Harward <stharward@gmail.com>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy 
 * defined in Section 14 of version 3 of the license.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **************************************************************************/

#define DEBUG_PREFIX "Playlist::RandomTrackNavigator"

#include "RandomTrackNavigator.h"

#include "Debug.h"
#include "playlist/PlaylistItem.h"
#include "playlist/PlaylistModel.h"

#include <KRandom>

#include <algorithm> // STL

Playlist::RandomTrackNavigator::RandomTrackNavigator() : SimpleTrackNavigator() {
    Model* model = Model::instance();
    connect(model, SIGNAL(insertedIds(const QList<quint64>&)), this, SLOT(recvInsertedIds(const QList<quint64>&)));
    connect(model, SIGNAL(removedIds(const QList<quint64>&)), this, SLOT(recvRemovedIds(const QList<quint64>&)));

    const int max = model->rowCount();
    for (int i=0; i<max; i++) {
        if ((model->stateOfRow(i) == Item::Unplayed) || (model->stateOfRow(i) == Item::NewlyAdded)) {
            m_unplayedRows.append(model->idAt(i));
        } else {
            m_playedRows.append(model->idAt(i));
        }
    }

    std::random_shuffle(m_unplayedRows.begin(), m_unplayedRows.end());
    std::random_shuffle(m_playedRows.begin(), m_playedRows.end());
}

void
Playlist::RandomTrackNavigator::recvInsertedIds(const QList<quint64>& list) {
    Model* model = Model::instance();
    foreach (quint64 t, list) {
        if ((model->stateOfId(t) == Item::Unplayed) || (model->stateOfId(t) == Item::NewlyAdded)) {
            m_unplayedRows.append(t);
        } else {
            int pos = KRandom::random() % m_playedRows.size();
            m_playedRows.insert(pos, t);
        }
    }

    std::random_shuffle(m_unplayedRows.begin(), m_unplayedRows.end());
}

void
Playlist::RandomTrackNavigator::recvRemovedIds(const QList<quint64>& list) {
    foreach (quint64 t, list) {
        m_unplayedRows.removeAll(t);
        m_playedRows.removeAll(t);
    }
}

void
Playlist::RandomTrackNavigator::recvActiveTrackChanged(const quint64 id) {
    if (m_unplayedRows.contains(id)) {
        m_playedRows.prepend(m_unplayedRows.takeAt(m_unplayedRows.indexOf(id)));
    }
}

int
Playlist::RandomTrackNavigator::nextRow() {
    if (m_unplayedRows.isEmpty()) {
        return -1;
    } else {
        quint64 t = m_unplayedRows.takeFirst();
        m_playedRows.prepend(t);
        return Model::instance()->rowForId(t);
    }
}

int
Playlist::RandomTrackNavigator::lastRow() {
    if (m_playedRows.isEmpty()) {
        return -1;
    } else {
        quint64 t = m_playedRows.takeFirst();
        m_unplayedRows.prepend(t);
        return Model::instance()->rowForId(t);
    }
}

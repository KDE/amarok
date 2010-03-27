/****************************************************************************************
 * Copyright (c) 2009 Daniel Dewald <Daniel.Dewald@time-shift.de>                       *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_LABELLISTMODEL_H
#define AMAROK_LABELLISTMODEL_H

#include "core/support/Amarok.h"

#include <QAbstractListModel>

/**
*	MVC Model Class for displaying a Label List
*	should be used with a QListView
*/


class LabelListModel : public QAbstractListModel
{
    Q_OBJECT

    public:
        explicit LabelListModel( const QStringList &m_labels, QObject *parent = 0 );

        int rowCount( const QModelIndex &parent = QModelIndex() ) const;
        QVariant data( const QModelIndex &index, int role ) const;
        QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
        Qt::ItemFlags flags( const QModelIndex &index ) const;
        bool setData( const QModelIndex &index, const QVariant &value, int role = Qt::EditRole );
        bool insertRows( int position, int rows, const QModelIndex &index = QModelIndex() );
        bool removeRows( int position, int rows, const QModelIndex &index = QModelIndex() );

        /**
        * Adds a label
        * @arg label Label that should be added
        */
        void addLabel( const QString label );

        /**
        * Removes a label
        * @arg label Label that should be removed
        */
        void removeLabel( const QString label );

        /**
        * Removes labels
        * @arg labels List of labels that should be removed
        */
        void removeLabels( const QStringList labels );

        /**
        * Sets the labels
        * @arg labels List of new Labels
        */
        void setLabels( const QStringList labels );

        /**
        * @returns List of labels
        */
        QStringList Labels();

    private:
        QStringList m_labels;
        bool isPresent( const QString label );
};

#endif

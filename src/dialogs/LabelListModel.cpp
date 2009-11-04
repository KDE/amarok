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

#include "dialogs/LabelListModel.h"

LabelListModel::LabelListModel( const QStringList &labels, QObject *parent )
    : QAbstractListModel( parent )
    , m_labels( labels )
{

}

int LabelListModel::rowCount( const QModelIndex &parent ) const
{
    return m_labels.count();
}

QVariant LabelListModel::data( const QModelIndex &index, int role ) const
{
    if ( !index.isValid() )
        return QVariant();

    if ( index.row() >= m_labels.size() )
        return QVariant();

    if ( role == Qt::DisplayRole )
        return m_labels.at( index.row() );
    else
        return QVariant();
}

QVariant LabelListModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    return QVariant();
}

Qt::ItemFlags LabelListModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;

    return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}

bool LabelListModel::setData( const QModelIndex &index, const QVariant &value, int role )
{
    if ( index.isValid() && role == Qt::EditRole )
    {
        m_labels.replace( index.row(), value.toString() );
        emit dataChanged( index, index );
        m_labels.sort();
        return true;
    }
    return false;
}

// Adds a label to the list if not present
void LabelListModel::addLabel( const QString label )
{
    if ( ( !label.isEmpty() ) && ( !isPresent( label ) ) )
    {
        beginInsertRows( QModelIndex(), 0 , 0 );
        m_labels << label ;
        m_labels.sort();
        endInsertRows();
    }
}

//Checks if a label is alread present
bool LabelListModel::isPresent( const QString label )
{
    if ( m_labels.indexOf( label ) > -1 )
    {
        return true;
    }

    return false;
}

// Removes the label "label" from the list if present
void LabelListModel::removeLabel( const QString label )
{
    int index = m_labels.indexOf( label );

    if ( index >= 0 )
    {
        m_labels.removeAt( index );
        emit dataChanged( QModelIndex(), QModelIndex() );
    }
}

//Wrapper function for remove label
void LabelListModel::removeLabels( const QStringList labels )
{
    if ( !labels.isEmpty() )
    {
        for (int x = 0; x < labels.size(); ++x)
        {
            removeLabel( labels.at( x ) );
        }
    }
}

QStringList LabelListModel::Labels()
{
    return m_labels;
}

void LabelListModel::setLabels( QStringList labels )
{
    m_labels = labels;
}

bool LabelListModel::insertRows( int position, int rows, const QModelIndex &parent )
{
    beginInsertRows( QModelIndex(), position, position+rows-1 );

    for ( int row = 0; row < rows; ++row )
    {
        m_labels.insert( position, "" );
    }

    endInsertRows();
    return true;
}

bool LabelListModel::removeRows( int position, int rows, const QModelIndex &parent )
{
    beginRemoveRows( QModelIndex(), position, position+rows-1 );

    for ( int row = 0; row < rows; ++row )
    {
        m_labels.removeAt( position );
    }

    endRemoveRows();
    return true;
}

/****************************************************************************************
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz>                                      *
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

#include "CommonModel.h"

#include "MetaValues.h"
#include "core/meta/support/MetaConstants.h"

#include <KGlobal>
#include <KIcon>
#include <KLocale>

#include <QFontMetrics>
#include <QHeaderView>

using namespace StatSyncing;

CommonModel::CommonModel( const QList<qint64> &columns )
    : m_columns( columns )
{
    Q_ASSERT( m_columns.value( 0 ) == Meta::valTitle );
    m_boldFont.setBold( true );
}

QVariant
CommonModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    if( orientation != Qt::Horizontal || section < 0 || section >= m_columns.count() )
        return QVariant();
    qint64 field = m_columns.at( section );
    switch( role )
    {
        case Qt::DisplayRole:
            return Meta::i18nForField( field );
        case Qt::SizeHintRole:
            return sizeHintData( field );
        case CommonModel::ResizeModeRole:
            switch( field )
            {
                case Meta::valTitle:
                    return QHeaderView::Stretch;
                case Meta::valRating:
                case Meta::valFirstPlayed:
                case Meta::valLastPlayed:
                case Meta::valPlaycount:
                    return QHeaderView::ResizeToContents;
                default:
                    return QHeaderView::Interactive;
            }
    }
    return QVariant();
}

QVariant
CommonModel::sizeHintData( qint64 field ) const
{
    static const QSize playCountSize = QFontMetrics( m_boldFont ).size( 0, "888 (88)" ) + QSize( 10, 0 );
    static const QSize dateSize = QFontMetrics( m_boldFont ).size( 0, "88.88.8888 88:88" ) + QSize( 10, 0 );

    switch( field )
    {
        case Meta::valPlaycount:
            return playCountSize;
        case Meta::valFirstPlayed:
        case Meta::valLastPlayed:
            return dateSize;
    }
    return QVariant();
}

QVariant
CommonModel::textAlignmentData( qint64 field ) const
{
    switch( field )
    {
        case Meta::valFirstPlayed:
        case Meta::valLastPlayed:
        case Meta::valPlaycount:
            return Qt::AlignRight;
    }
    return QVariant();
}

QVariant
CommonModel::trackData( const TrackPtr &track, qint64 field, int role ) const
{
    KLocale *locale = KGlobal::locale();
    switch( role )
    {
        case Qt::DisplayRole:
            switch( field )
            {
                case Meta::valTitle:
                    return trackTitleData( track );
                case Meta::valRating:
                    return track->rating();
                case Meta::valFirstPlayed:
                    return track->firstPlayed().isValid() ?
                        locale->formatDateTime( track->firstPlayed(), KLocale::FancyShortDate ) :
                        QVariant();
                case Meta::valLastPlayed:
                    return track->lastPlayed().isValid() ?
                        locale->formatDateTime( track->lastPlayed(), KLocale::FancyShortDate ) :
                        QVariant();
                case Meta::valPlaycount:
                {
                    int recent = track->recentPlayCount();
                    return recent ? i18nc( "%1 is play count and %2 is recent play count",
                        "%1 (%2)", track->playCount(), recent ) : QString::number( track->playCount() );
                }
                case Meta::valLabel:
                    return QStringList( track->labels().toList() ).join( i18nc( "comma between labels", ", " ) );
                default:
                    return QString( "Unknown field!" );
            }
            break;
        case Qt::ToolTipRole:
            switch( field )
            {
                case Meta::valTitle:
                    return trackToolTipData( track );
                case Meta::valPlaycount:
                    return i18np( "Played %2 times of which one play is recent and unique "
                        "to this source", "Played %2 times of which %1 plays are recent "
                        "and unique to this source", track->recentPlayCount(), track->playCount() );
            }
            break;
        case Qt::TextAlignmentRole:
            return textAlignmentData( field );
    }
    return QVariant();
}

QVariant
CommonModel::trackTitleData( const TrackPtr &track ) const
{
    return i18n( "%1 - %2 - %3", track->artist(), track->album(), track->name() );
}

QVariant
CommonModel::trackToolTipData( const TrackPtr &track ) const
{
    return trackTitleData( track ); // TODO nicer toolTip, display more fields
}

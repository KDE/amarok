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
#include "core/support/Debug.h"
#include "statsyncing/Options.h"

#include <QIcon>
#include <KLocalizedString>

#include <QApplication>
#include <QHeaderView>

using namespace StatSyncing;

const QSize CommonModel::s_ratingSize( 5*16, 16 );

CommonModel::CommonModel( const QList<qint64> &columns, const Options &options )
    : m_columns( columns )
    , m_options( options )
{
    Q_ASSERT( m_columns.value( 0 ) == Meta::valTitle );
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
        case ResizeModeRole:
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
        case FieldRole:
            return field;
    }
    return QVariant();
}

QVariant
CommonModel::sizeHintData( qint64 field ) const
{
    switch( field )
    {
        case Meta::valRating:
        {
            static QSize size;
            if( size.isValid() ) // optimization
                return size;
            QStyleOptionViewItemV4 opt;
            opt.features = QStyleOptionViewItemV2::HasDisplay
                         | QStyleOptionViewItemV2::HasCheckIndicator
                         | QStyleOptionViewItemV2::HasDecoration;
            opt.state = QStyle::State_Enabled;
            opt.decorationSize = s_ratingSize;

            const QWidget *widget = opt.widget;
            QStyle *style = widget ? widget->style() : QApplication::style();
            size = style->sizeFromContents( QStyle::CT_ItemViewItem, &opt, QSize(), widget );
            return size;
        }
        case Meta::valFirstPlayed:
        case Meta::valLastPlayed:
        {
            static QSize size;
            if( size.isValid() ) // optimization
                return size;
            QStyleOptionViewItemV4 opt;
            opt.features = QStyleOptionViewItemV2::HasDisplay;
            opt.state = QStyle::State_Enabled;
            opt.text = "88.88.8888 88:88";

            QStyle *style = QApplication::style();
            size = style->sizeFromContents( QStyle::CT_ItemViewItem, &opt, QSize(), 0 );
            return size;
        }
        case Meta::valPlaycount:
        {
            static QSize size;
            if( size.isValid() ) // optimization
                return size;
            QStyleOptionViewItemV4 opt;
            opt.features = QStyleOptionViewItemV2::HasDisplay;
            opt.state = QStyle::State_Enabled;
            opt.text = "888 (88)";
            opt.font.setBold( true );

            QStyle *style = QApplication::style();
            size = style->sizeFromContents( QStyle::CT_ItemViewItem, &opt, QSize(), 0 );
            return size;
        }
    }
    return QVariant();
}

QVariant
CommonModel::textAlignmentData( qint64 field ) const
{
    switch( field )
    {
        case Meta::valRating:
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
                    return track->firstPlayed();
                case Meta::valLastPlayed:
                    return track->lastPlayed();
                case Meta::valPlaycount:
                {
                    int recent = track->recentPlayCount();
                    return recent ? QVariant( i18nc( "%1 is play count and %2 is recent play count",
                        "%1 (%2)", track->playCount(), recent ) ) : QVariant( track->playCount() );
                }
                case Meta::valLabel:
                    return QStringList( ( track->labels() - m_options.excludedLabels() ).toList() ).join( i18nc(
                        "comma between list words", ", " ) );
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
                case Meta::valLabel:
                {
                    QSet<QString> labels = track->labels() - m_options.excludedLabels();
                    QSet<QString> excludedLabels = track->labels() & m_options.excludedLabels();
                    QStringList texts;
                    if( !labels.isEmpty() )
                        texts << i18n( "Labels: %1", QStringList( labels.toList() ).join( i18nc(
                        "comma between list words", ", " ) ) );
                    if( !excludedLabels.isEmpty() )
                        texts << i18n( "Ignored labels: %1", QStringList( excludedLabels.toList() ).join( i18nc(
                            "comma between list words", ", " ) ) );
                    return texts.isEmpty() ? QVariant() : texts.join( "\n" );
                }
            }
            break;
        case Qt::TextAlignmentRole:
            return textAlignmentData( field );
        case Qt::SizeHintRole:
            return sizeHintData( field );
        case FieldRole:
            return field;
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

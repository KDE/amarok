/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2010 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#include "InlineEditorWidget.h"

#include "SvgHandler.h"
#include "core/meta/Meta.h"
#include "core/support/Debug.h"
#include "moodbar/MoodbarManager.h"
#include "playlist/PlaylistDefines.h"
#include "playlist/layouts/LayoutManager.h"
#include "playlist/proxymodels/GroupingProxy.h"
#include "playlist/view/listview/PrettyItemDelegate.h"

#include <KRatingWidget>

#include <QBoxLayout>
#include <QEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>

using namespace Playlist;

InlineEditorWidget::InlineEditorWidget( QWidget * parent, const QModelIndex &index,
                                        const PlaylistLayout &layout, int height, int width )
    : BoxWidget( false, parent )
    , m_index( index )
    , m_layout( layout )
    , m_widgetHeight( height )
    , m_widgetWidth( width )
    , m_layoutChanged( false )
{
    // The line below is nice but sometimes (despite best effort) we are missing
    // a pixel or two (e.g. the width of the splitter widget handle is not present
    // in the delegate).
    // So, to fix BR: 300118 set it to "true" to debug or have the own playlist background set it to "false"
    setAutoFillBackground( true );

    const int frameHMargin = style()->pixelMetric( QStyle::PM_FocusFrameHMargin );
    const int frameVMargin = style()->pixelMetric( QStyle::PM_FocusFrameVMargin );
    setContentsMargins( frameHMargin, frameVMargin, frameHMargin, frameVMargin );

    //prevent editor closing when clicking a rating widget or pressing return in a line edit.
    setFocusPolicy( Qt::StrongFocus );

    createChildWidgets();
}

InlineEditorWidget::~InlineEditorWidget()
{
}

void InlineEditorWidget::createChildWidgets()
{
    QBoxLayout* boxLayout = qobject_cast<QBoxLayout*>( layout() );
    Q_ASSERT( boxLayout );
    boxLayout->setSpacing( 0 );

    //For now, we don't allow editing of the "head" data, just the body
    LayoutItemConfig config = m_layout.layoutForItem( m_index );

    const int rowCount = config.rows();
    if( rowCount == 0 )
        return;

    // we have to use the same metrics as the PrettyItemDelegate or else
    // the widgets will change places when editing
    const int horizontalSpace = style()->pixelMetric( QStyle::PM_LayoutHorizontalSpacing );
    const int frameHMargin = style()->pixelMetric( QStyle::PM_FocusFrameHMargin );
    const int frameVMargin = style()->pixelMetric( QStyle::PM_FocusFrameVMargin );

    int rowOffsetX = frameHMargin; // keep the text a little bit away from the border

    const int coverHeight = m_widgetHeight - frameVMargin * 2;

    const bool showCover = config.showCover();
    if( showCover )
        rowOffsetX += coverHeight + horizontalSpace/* + frameHMargin * 2*/;

    const int contentHeight = m_widgetHeight - frameVMargin * 2;
    int rowHeight = contentHeight / rowCount;
    const int rowWidth = m_widgetWidth - rowOffsetX - frameHMargin * 2;

    if( showCover )
    {
        QModelIndex coverIndex = m_index.model()->index( m_index.row(), CoverImage );
        QPixmap albumPixmap = coverIndex.data( Qt::DisplayRole ).value<QPixmap>();

        if( !albumPixmap.isNull() )
        {
            if( albumPixmap.width() > albumPixmap.height() )
                albumPixmap = albumPixmap.scaledToWidth( coverHeight );
            else
                albumPixmap = albumPixmap.scaledToHeight( coverHeight );

            QLabel *coverLabel = new QLabel( this );
            coverLabel->setPixmap( albumPixmap );
            if( albumPixmap.width() < coverHeight )
                coverLabel->setContentsMargins( ( coverHeight - albumPixmap.width()     ) / 2, 0,
                                                ( coverHeight - albumPixmap.width() + 1 ) / 2, 0 );
            boxLayout->setStretchFactor( coverLabel, 0 );

            boxLayout->addSpacing( horizontalSpace );
        }
    }

    BoxWidget *rowsWidget = new BoxWidget( true, this );

    // --- paint all the rows
    for( int i = 0; i < rowCount; i++ )
    {
        LayoutItemConfigRow row = config.row( i );
        const int elementCount = row.count();

        QSplitter *rowWidget = new QSplitter( rowsWidget );
        connect( rowWidget, &QSplitter::splitterMoved, this, &InlineEditorWidget::splitterMoved );

        m_splitterRowMap.insert( rowWidget, i );

        //we need to do a quick pass to figure out how much space is left for auto sizing elements
        qreal spareSpace = 1.0;
        int autoSizeElemCount = 0;
        for( int k = 0; k < elementCount; ++k )
        {
            spareSpace -= row.element( k ).size();
            if( row.element( k ).size() < 0.001 )
                autoSizeElemCount++;
        }

        const qreal spacePerAutoSizeElem = spareSpace / (qreal)autoSizeElemCount;

        //give left over pixels to the first rows. Widgets are doing it the same.
        if( i == 0 )
            rowHeight++;
        if( i == ( contentHeight % rowCount ) )
            rowHeight--;

        QList<int> itemWidths;
        int currentItemX = 0;
        for( int j = 0; j < elementCount; ++j )
        {
            LayoutItemConfigRowElement element = row.element( j );

            // -- calculate the size
            qreal size;
            if( element.size() < 0.001 )
                size = spacePerAutoSizeElem;
            else
                size = element.size();

            int itemWidth;
            if( j == elementCount - 1 )
                // use the full with for the last item
                itemWidth = rowWidth - currentItemX;
            else
                itemWidth = rowWidth * size;

            itemWidths.append( itemWidth );

            int value = element.value();

            QModelIndex textIndex = m_index.model()->index( m_index.row(), value );
            QString text = textIndex.data( Qt::DisplayRole ).toString();
            m_orgValues.insert( value, text );

            QWidget *widget = nullptr;
            //special case for painting the rating...
            if( value == Rating )
            {
                int rating = textIndex.data( Qt::DisplayRole ).toInt();

                KRatingWidget* ratingWidget = new KRatingWidget( nullptr );
                ratingWidget->setAlignment( element.alignment() );
                ratingWidget->setRating( rating );
                ratingWidget->setAttribute( Qt::WA_NoMousePropagation, true );

                connect( ratingWidget, QOverload<int>::of(&KRatingWidget::ratingChanged),
                         this, &InlineEditorWidget::ratingValueChanged );

                m_editorRoleMap.insert( ratingWidget, value );
                widget = ratingWidget;
            }
            else if( value == Divider )
            {
                QPixmap left = The::svgHandler()->renderSvg( QStringLiteral("divider_left"),
                                                             1, rowHeight,
                                                             QStringLiteral("divider_left") );

                QPixmap right = The::svgHandler()->renderSvg( QStringLiteral("divider_right"),
                                                              1, rowHeight,
                                                              QStringLiteral("divider_right") );

                QPixmap dividerPixmap( 2, rowHeight );
                dividerPixmap.fill( Qt::transparent );

                QPainter painter( &dividerPixmap );
                painter.drawPixmap( 0, 0, left );
                painter.drawPixmap( 1, 0, right );

                QLabel* dividerLabel = new QLabel( nullptr );
                dividerLabel->setPixmap( dividerPixmap );
                dividerLabel->setAlignment( element.alignment() );

                widget = dividerLabel;
            }
            else if( value == Moodbar )
            {
                //we cannot ask the model for the moodbar directly as we have no
                //way of asking for a specific size. Instead just get the track from
                //the model and ask the moodbar manager ourselves.
                Meta::TrackPtr track = m_index.data( TrackRole ).value<Meta::TrackPtr>();

                QLabel* moodbarLabel = new QLabel( nullptr );
                moodbarLabel->setScaledContents( true );
                if( The::moodbarManager()->hasMoodbar( track ) )
                {
                    QPixmap moodbar = The::moodbarManager()->getMoodbar( track, itemWidth, rowHeight - 8 );
                    moodbarLabel->setPixmap( moodbar );
                }
                widget = moodbarLabel;
            }
            //actual playlist item text is drawn here
            else
            {
                QLineEdit * edit = new QLineEdit( text, nullptr );
                edit->setFrame( false );
                edit->setAlignment( element.alignment() );
                edit->installEventFilter(this);

                // -- set font
                bool bold = element.bold();
                bool italic = element.italic();
                bool underline = element.underline();

                QFont font = edit->font();
                font.setBold( bold );
                font.setItalic( italic );
                font.setUnderline( underline );
                edit->setFont( font );

                connect( edit, &QLineEdit::editingFinished, this, &InlineEditorWidget::editValueChanged );

                //check if this is a column that is editable. If not, make the
                //line edit read only.
                if( !isEditableColumn( static_cast<Playlist::Column>(value) ) )
                {
                    edit->setReadOnly( true );
                    edit->setDisabled( true );
                }
                m_editorRoleMap.insert( edit, value );
                widget = edit;
            }

            widget->setSizePolicy( QSizePolicy::Ignored, QSizePolicy::Ignored ); // or else the widget size hint influences the space it get's which messes up the layout
            rowWidget->addWidget( widget );

            // handles are nice, but if we don't compensate for their sizes the layout
            // would be different from the item delegate
            if( j > 0 )
                widget->setContentsMargins( -( ( rowWidget->handleWidth() + 1 ) / 2 ), 0, 0, 0 );
            if( j < elementCount - 1 )
                widget->setContentsMargins( 0, 0, -( rowWidget->handleWidth() / 2 ), 0 );

            currentItemX += itemWidth;
        }
        rowWidget->setSizes( itemWidths );
    }
}

void InlineEditorWidget::editValueChanged()
{
    DEBUG_BLOCK

    QObject * senderObject = sender();

    QLineEdit * edit = dynamic_cast<QLineEdit *>( senderObject );
    if( !edit )
        return;

    int role = m_editorRoleMap.value( edit );

    //only save values if something has actually changed.
    if( m_orgValues.value( role ) != edit->text() )
    {
        debug() << "Storing changed value: " << edit->text();
        m_changedValues.insert( role, edit->text() );
    }


}

void InlineEditorWidget::ratingValueChanged()
{
    DEBUG_BLOCK

    KRatingWidget * edit = qobject_cast<KRatingWidget *>( sender() );
    if( !edit )
        return;

    int role = m_editorRoleMap.value( edit );
    m_changedValues.insert( role, QString::number( edit->rating() ) );
}

QMap<int, QString> InlineEditorWidget::changedValues()
{
    DEBUG_BLOCK
    if( m_layoutChanged )
        LayoutManager::instance()->updateCurrentLayout( m_layout );
    return m_changedValues;
}


void InlineEditorWidget::splitterMoved( int pos, int index )
{
    DEBUG_BLOCK

    Q_UNUSED( pos )
    Q_UNUSED( index )

    QSplitter * splitter = dynamic_cast<QSplitter *>( sender() );
    if ( !splitter )
        return;

    int row = m_splitterRowMap.value( splitter );
    debug() << "on row: " << row;

    //first, get total size of all items;
    QList<int> sizes = splitter->sizes();

    int total = 0;
    for( int size : sizes )
        total += size;

    //resize all items as the splitters take up some space, so we need to normalize the combined size to 1.
    QList<qreal> newSizes;

    for( int size : sizes )
    {
        qreal newSize = (qreal) size / (qreal) total;
        newSizes << newSize;
    }

    LayoutItemConfig itemConfig = m_layout.layoutForItem( m_index );

    LayoutItemConfigRow rowConfig = itemConfig.row( row );

    //and now we rebuild a new layout...
    LayoutItemConfigRow newRowConfig;

    for( int i = 0; i<rowConfig.count(); i++ )
    {
        LayoutItemConfigRowElement element = rowConfig.element( i );
        debug() << "item " << i << " old/new: " << element.size() << "/" << newSizes.at( i );
        element.setSize( newSizes.at( i ) );
        newRowConfig.addElement( element );
    }

    LayoutItemConfig newItemConfig;
    newItemConfig.setActiveIndicatorRow( itemConfig.activeIndicatorRow() );
    newItemConfig.setShowCover( itemConfig.showCover() );

    for( int i = 0; i<itemConfig.rows(); i++ )
    {
        if( i == row )
            newItemConfig.addRow( newRowConfig );
        else
            newItemConfig.addRow( itemConfig.row( i ) );
    }

    m_layout.setLayoutForPart( m_layout.partForItem( m_index ), newItemConfig );

    m_layoutChanged = true;
}

bool
InlineEditorWidget::eventFilter( QObject *obj, QEvent *event )
{
    QList<QWidget *> editWidgets = m_editorRoleMap.keys();
    QWidget *widget = qobject_cast<QWidget *>( obj );
    if( editWidgets.contains( widget ) )
    {
        if( event->type() == QEvent::KeyPress )
        {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>( event );
            switch( keyEvent->key() )
            {
                case Qt::Key_Enter:
                case Qt::Key_Return:
                    if( widget )
                    {
                        widget->clearFocus();
                        Q_EMIT editingDone( this );
                    }
                    return true;
            }
            return false;
        }
        else
            return false;
    }
    else
        return BoxWidget::eventFilter( obj, event );
}


/****************************************************************************************
 * Copyright (c) 2009 Casey Link <unnamedrambler@gmail.com>                             *
 * Copyright (c) 2009 Simon Bühler <simon@aktionspotenzial.de>                          *
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

#include "BookmarkPopup.h"

#include "SvgHandler.h"
#include "amarokurls/BookmarkModel.h"
#include "core/support/Debug.h"
#include "widgets/BookmarkTriangle.h"

#include <KLocalizedString>

#include <QPainter>
#include <QVBoxLayout>

BookmarkPopup::BookmarkPopup (QWidget* parent, const QString &label, BookmarkTriangle* triangle )
        : QWidget ( parent )
        , m_label ( label )
        , m_triangle ( triangle )

{
    m_timer = new QTimer ( this );
    connect ( m_timer, &QTimer::timeout, this, &BookmarkPopup::hideTimerAction );

    m_displayNeeded = true;
    m_hasMouseOver = false;
    m_overDelete = false;
    m_isEditMode = false;

    m_deleteIcon = QIcon::fromTheme( QStringLiteral("edit-delete") );
    adjustWidth();

    m_edit = new QLineEdit ( m_label, nullptr );
    m_edit->setVisible ( false );
    m_edit->setAlignment ( Qt::AlignHCenter );
    connect ( m_edit, &QLineEdit::returnPressed, this, &BookmarkPopup::editValueChanged );
    
    QVBoxLayout * layout = new QVBoxLayout;
    layout->setContentsMargins ( 1, 0, 0, 0 );
    layout->addSpacing ( m_lineHeight + 2 );
    layout->addWidget ( m_edit );
    setLayout ( layout );
    setMouseTracking ( true );
    setFocusPolicy(Qt::StrongFocus);
}

QSize BookmarkPopup::sizeHint() const
{
    return QSize ( m_width, m_height );
}

QSizePolicy BookmarkPopup::sizePolicy() const
{
    return QSizePolicy ( QSizePolicy::Preferred, QSizePolicy::Minimum );
}

QSize BookmarkPopup::minimumSizeHint() const
{
    return QSize ( m_width, m_height );
}

void BookmarkPopup::adjustWidth()
{
    //calculate height and width
    const int margin = 3;
    QFontMetrics fm ( font() );
    m_lineHeight = fm.height();
    int line1Width = fm.horizontalAdvance ( i18n ( "Bookmark" ) ) + 40; //padding and space for delete icon
    int line2Width = fm.horizontalAdvance ( m_label ) + 8 ;
    m_height = 44;
    m_width = qMax ( line1Width, line2Width ) + 2 * margin;
    resize ( m_width, m_height );
    m_deleteIconRect = QRect ( m_width - 20, 4, 16, 16 );
}

void BookmarkPopup::paintEvent ( QPaintEvent* event )
{
    QPainter p ( this );
    p.setRenderHint ( QPainter::Antialiasing );
    p.setBrush ( Qt::white );
    p.setOpacity ( 0.85 );
    QPen pen = QPen ( Qt::black );
    pen.setCosmetic ( true );
    p.setPen ( pen );
    QRect rect = QRect ( 0,0, m_width, m_height );
    p.drawRoundedRect ( rect, 5, 5 );

    if ( m_overDelete ) p.setOpacity ( m_overDelete ?  1 : 0.1 );
    p.drawPixmap ( m_deleteIconRect.x(), m_deleteIconRect.y(), m_deleteIcon.pixmap ( 16 ) );

    p.setOpacity ( 1 );
    p.drawPixmap ( 5, 1, The::svgHandler()->renderSvg ( QStringLiteral("bookmarks"), 6, 20, QStringLiteral("bookmarks") ) );

    p.setPen ( Qt::gray );
    rect = QRect ( 15, 3, m_width, m_lineHeight );
    p.drawText ( rect, Qt::AlignLeft, i18n ( "Bookmark" ) );

    if ( m_isEditMode ) // paint Label or render QLineEdit
    {
        event->accept();
    }
    else
    {
        p.setPen ( Qt::black );
        rect = QRect ( 0, m_lineHeight + 8, m_width, m_lineHeight );
        p.drawText ( rect, Qt::AlignCenter, m_label );
    }

}

void BookmarkPopup::mouseReleaseEvent ( QMouseEvent * event )
{
    if ( event->button() == Qt::LeftButton )
    {
        if ( isOverDeleteIcon ( event->pos() ) ) // handle close
        {
            m_triangle->deleteBookmark();
            return;
        }
        if ( isOverTitleLabel ( event->pos() ) ) // handle click in editable Area
        {
            if ( m_isEditMode )
                return;
            
            m_isEditMode = true; // switch to Editmode
            m_edit->setVisible ( m_isEditMode );
            m_edit->setFocus();
            update();
            return;
        }
        // other clicks discard changes and leave Editmode
        m_isEditMode = false;
        m_edit->setVisible ( m_isEditMode );
        m_edit->setText( m_label );
        update();
    }
}

void BookmarkPopup::mouseMoveEvent ( QMouseEvent * event )
{
    // Monitor for DeleteIcon highlighting
    bool state = isOverDeleteIcon ( event->pos() );
    if ( state != m_overDelete )
    {
        m_overDelete = state;
        this->update();
    }
}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
void BookmarkPopup::enterEvent ( QEvent * )
#else
void BookmarkPopup::enterEvent ( QEnterEvent * )
#endif
{
    m_hasMouseOver = true;
}

void BookmarkPopup::leaveEvent ( QEvent* )
{
    m_hasMouseOver = false;
    startHideTimer();

}

void BookmarkPopup::displayNeeded ( bool value )
{
    m_displayNeeded  = value;
    if ( !m_displayNeeded ) startHideTimer();
}

void BookmarkPopup::hideTimerAction ( )
{
    if ( m_hasMouseOver ||  m_isEditMode  || m_displayNeeded )
        return;

    m_timer->stop();
    hide();
}

void BookmarkPopup::editValueChanged()
{
    if ( m_label != m_edit->text() && m_edit->text().trimmed().length() > 0 )
    {
        BookmarkModel::instance()->renameBookmark( m_label, m_edit->text().trimmed() );
        return;
    }
    m_isEditMode = false;
    m_edit->setVisible ( m_isEditMode );
    update();
}

void BookmarkPopup::startHideTimer()
{
    m_timer->start ( 500 );
}

bool BookmarkPopup::isOverDeleteIcon ( QPoint pos )
{
    return m_deleteIconRect.contains ( pos );
}


bool BookmarkPopup::isOverTitleLabel ( QPoint pos )
{
    return ( pos.y() > m_lineHeight +2 );
}

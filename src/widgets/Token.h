/****************************************************************************************
 * Copyright (c) 2008 Téo Mrnjavac <teo@kde.org>                                        *
 * Copyright (c) 2008-2009 Seb Ruiz <ruiz@kde.org>                                      *
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

#ifndef AMAROK_TOKEN_H
#define AMAROK_TOKEN_H

#include <QIcon>
#include <QWidget>
#include <QLabel>
#include <QPixmap>

class Token;
class QMimeData;

/** The token factory is used by the TokenDropTarget to create suitable Token objects.
    The TokenWithLayout class has it's own TokenFactory to be used with the EditFilterDialog.
*/
class TokenFactory
{

public:
    virtual ~TokenFactory() {}
    virtual Token* createToken( const QString &text, const QString &iconName, qint64 value, QWidget* parent = nullptr ) const;
    virtual Token* createTokenFromMime( const QMimeData* mimeData, QWidget* parent = nullptr ) const;
};

/** A widget that is used in the FilenameLayoutWidget to display part of a filename
    It is drag&droppable in the TokenDropTarget from the TokenPool widget.

    Note: A disabled token cannot be dragged. See setEnabled().
*/
class Token : public QWidget
{
        Q_OBJECT

    public:

        explicit Token( const QString &text, const QString &iconName, qint64 value, QWidget *parent = nullptr );

        QIcon icon() const;
        QString iconName() const;
        QString name() const;
        qint64 value() const;

        QColor textColor() const;
        void setTextColor( QColor textColor );

        /** Return true if somebody has previously set the text color */
        bool hasCustomColor() const { return m_customColor; };

        /** Returns the mime data for this token.
            Caller has to free the QMimeData object.
        */
        QMimeData* mimeData() const;

        /** Returns the mime type for an amarok tag token */
        static QString mimeType();

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    Q_SIGNALS:
        void changed();

        /** Emitted when the token get's the focus */
        void gotFocus( Token* thisToken );

    protected:
        /** overloaded to update the cursor in case the token is set to inactive */
        void changeEvent( QEvent* event = 0 ) override;

        void focusInEvent( QFocusEvent* event ) override;

        void updateCursor();

        /** Handles start of drag. */
        void mousePressEvent( QMouseEvent* event ) override;

        /** Handles start of drag. */
        void mouseMoveEvent( QMouseEvent* event ) override;

        void paintEvent(QPaintEvent *pe) override;

        void performDrag();

    protected:
        QString     m_name;
        QIcon       m_icon;
        QString     m_iconName;
        qint64      m_value; // TODO: make this more typesave
        bool        m_customColor;

        QLabel      *m_iconContainer;
        QLabel      *m_label;

        /** Position of the mouse press event
            (used for drag and drop) */
        QPoint      m_startPos;
};

#endif // AMAROK_TOKEN_H


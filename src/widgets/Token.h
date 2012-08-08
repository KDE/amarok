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

#include <KIcon>
#include <QWidget>
#include <QLabel>
#include <QPixmap>

class Token;

class TokenFactory
{

public:
    virtual ~TokenFactory() {}
    virtual Token * createToken( const QString &text, const QString &iconName, qint64 value, QWidget *parent = 0 );
};

/** A widget that is used in the FilenameLayoutWidget to display part of a filename
    It is drag&droppable in the TokenDropTarget from the TokenPool widget.
*/
class Token : public QWidget
{
        Q_OBJECT

    public:

        explicit Token( const QString &text, const QString &iconName, qint64 value, QWidget *parent = 0 );

        KIcon icon() const;
        QString iconName() const;
        QString name() const;
        qint64 value() const;

        QColor textColor() const;
        void setTextColor( QColor textColor );

        /** Return true if somebody has previously set the text color */
        bool hasCustomColor() const { return m_customColor; };

    signals:
        void changed();

    protected:
        virtual void paintEvent(QPaintEvent *pe);

    protected:

        QString     m_name;
        KIcon       m_icon;
        QString     m_iconName;
        qint64      m_value;
        bool        m_customColor;

        QLabel      *m_iconContainer;
        QLabel      *m_label;
};

#endif // AMAROK_TOKEN_H


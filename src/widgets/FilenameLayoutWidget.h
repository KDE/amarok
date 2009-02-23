/******************************************************************************
 * Copyright (C) 2008 Teo Mrnjavac <teo.mrnjavac@gmail.com>                   *
 *           (C) 2008 Seb Ruiz <ruiz@kde.org>                                 *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License as             *
 * published by the Free Software Foundation; either version 2 of             *
 * the License, or (at your option) any later version.                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.      *
 ******************************************************************************/
#ifndef FILENAMELAYOUTWIDGET_H
#define FILENAMELAYOUTWIDGET_H

#include "Token.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPaintEvent>

class DragStack;

// Handles the graphical representation of the target filename as a bar that contains tokens.
class FilenameLayoutWidget : public QFrame
{
    Q_OBJECT

    public:
        FilenameLayoutWidget( QWidget *parent = 0 );

        unsigned int getTokenCount() const;
        QList<Token *> currentTokenLayout();
        void removeAllTokens();
        void setCustomTokenFactory( TokenFactory *factory );
    signals:
        void layoutChanged();

    protected:
        void paintEvent( QPaintEvent *event );

    public slots:
        void addToken( Token* token, int index = -1 /* append */ );

    private:
        QString      m_infoText;        // text in the back of the empty FilenameLayoutWidget
        DragStack *m_dragstack;          // main layout that holds the tokens
};

#endif    //FILENAMELAYOUTWIDGET_H

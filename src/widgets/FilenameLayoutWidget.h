/******************************************************************************
 * Copyright (C) 2008 Teo Mrnjavac <teo.mrnjavac@gmail.com>                   *
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

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>


class FilenameLayoutWidget
    : public QFrame
{
    Q_OBJECT

    public:
        FilenameLayoutWidget( QWidget *parent = 0 );
        void addToken( QString text, int index = 99);

        unsigned int getTokenCount();
    protected:
        void mouseMoveEvent( QMouseEvent *event );
        void mousePressEvent( QMouseEvent *event );
        void dragEnterEvent( QDragEnterEvent *event );
        void dragMoveEvent( QDragMoveEvent *event );
        void dropEvent( QDropEvent *event );
    private:
        //void performDrag();
        void performDrag( QMouseEvent *event );


        QLabel *backText;
        QHBoxLayout *layout;
        QPoint startPos;


        unsigned int tokenCount;

    //TODO: This slot is for testing, will probably be a normal method
    public slots:
        void slotAddToken();
};

class Token
    : public QLabel
{
    Q_OBJECT
    public:
        Token( const QString &string, QWidget *parent = 0 );
        QString getTokenString();
    private:
        void setTokenString(const QString &string );

        unsigned int myCount;
        QString tokenString;
        

};

#endif    //FILENAMELAYOUTWIDGET_H

/****************************************************************************************
 * Copyright (c) 2010 Sergey Ivanov <123kash@gmail.com>                                 *
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

#ifndef CHECKBUTTON_H
#define CHECKBUTTON_H

#include <QPushButton>
#include <QCheckBox>


class CheckButton : public QPushButton
{
    Q_OBJECT
    Q_PROPERTY(QString checkHint READ checkHint WRITE setCheckHint)
    public:
        CheckButton( QWidget *parent = 0);
        CheckButton( const QString& text, QWidget* parent = 0);
        CheckButton( const QIcon & icon, const QString & text, QWidget *parent = 0 );

        ~CheckButton();

        bool isChecked() const;
        QString checkHint() const;
        void setCheckHint( const QString &text );

        virtual void setText( const QString &text );
        virtual void resizeEvent( QResizeEvent *event );

    private:
        void init();
        void moveCheckBox();
        QCheckBox *checkBox;
};

#endif // CHECKBUTTON_H

/****************************************************************************************
 * Copyright (c) 2006 Martin Aumueller <aumuell@reserv.at>                              *
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

#ifndef HINTLINEEDIT_H
#define HINTLINEEDIT_H

#include <QLineEdit> //baseclass

class BoxWidget;
class QLabel;
class QWidget;

class HintLineEdit : public QLineEdit
{
    Q_OBJECT

public:
    explicit HintLineEdit( const QString &hint, const QString &text, QWidget *parent = nullptr );
    explicit HintLineEdit( const QString &text, QWidget *parent = nullptr );
    explicit HintLineEdit( QWidget *parent = nullptr );

    ~HintLineEdit() override;
    virtual QObject *parent();
    virtual void setHint( const QString &hint );

private:
    void init();

    BoxWidget *m_vbox;
    QLabel *m_hint;
};

#endif

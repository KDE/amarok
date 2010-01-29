/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/


#ifndef AMAROKDOCKWIDGET_H
#define AMAROKDOCKWIDGET_H

#include <QDockWidget>
#include <QString>


class AmarokDockWidget : public QDockWidget
{
    Q_OBJECT

public:
    explicit AmarokDockWidget( const QString & title, QWidget * parent = 0, Qt::WindowFlags flags = 0 );

signals:
    void layoutChanged();

protected:
    virtual void closeEvent( QCloseEvent* );
    virtual void resizeEvent( QResizeEvent* );
    virtual void showEvent( QShowEvent* );
    virtual void hideEvent( QHideEvent* );
};

#endif // AMAROKDOCKWIDGET_H

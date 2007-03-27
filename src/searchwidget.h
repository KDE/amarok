/***************************************************************************
 * copyright     : (C) 2007 Dan Meltzer <hydrogen@notyetimplemented.com>   *
 **************************************************************************/

 /***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef SEARCHWIDGET_H
#define SEARCHWIDGET_H

#include <QWidget>

class KLineEdit;
class KPushButton;
// A Custom Widget that can be used globally to implement
// searching a treeview.

class SearchWidget : public QWidget
{
    Q_OBJECT
    public:
        SearchWidget( QWidget *parent );
        SearchWidget( QWidget *parent, QWidget *caller );
        KLineEdit *lineEdit() { return m_sw; }
        void setup( QObject* caller );
    private:
        void init( QWidget* parent );

        KLineEdit *m_sw;
        KPushButton * m_filterButton;
};

#endif

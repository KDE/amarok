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

// A Custom Widget that can be used globally to implement
// searching a treeview.

class SearchWidget : public QWidget
{
    Q_OBJECT
    public:
        SearchWidget( QWidget *, QWidget * );
        KLineEdit *lineEdit() { return m_sw; }
    private:
        KLineEdit *m_sw;
        QWidget *m_caller;
};

#endif

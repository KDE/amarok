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

#include "amarok_export.h"

#include <QToolBar>
#include <QWidget>

class KLineEdit;
class KPushButton;
// A Custom Widget that can be used globally to implement
// searching a treeview.

class AMAROK_EXPORT SearchWidget : public QWidget
{
    Q_OBJECT
    public:
        explicit SearchWidget( QWidget *parent, bool advanced = true );
        SearchWidget( QWidget *parent, QWidget *caller, bool advanced = true  );
        KLineEdit *lineEdit() { return m_sw; }
        void setup( QObject* caller );
        void setSearchString( const QString &searchString );

        QToolBar * toolBar();

    signals:

        void filterNow();

    private slots:

        void slotShowFilterEditor();

    private:
        void init( QWidget *parent, bool advanced );

        KLineEdit   *m_sw;
        QAction *m_filterAction;
        QToolBar * m_toolBar;
};

#endif

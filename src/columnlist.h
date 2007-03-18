/*
  Copyright (c) 2006 Gábor Lehel <illissius@gmail.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#ifndef AMAROK_COLUMNLIST_H
#define AMAROK_COLUMNLIST_H

#include <khbox.h>
//Added by qt3to4:
#include <Q3ValueList>
#include <kdialog.h>

class K3ListView;
class KPushButton;
template<class T> class Q3ValueList;

class ColumnList: public KHBox
{
    Q_OBJECT
public:
    explicit ColumnList( QWidget *parent = 0, const char *name = 0 );
    Q3ValueList<int> visibleColumns() const;
    Q3ValueList<int> columnOrder() const;
    bool isChanged() const;
    void resetChanged();

signals:
    void changed();

private slots:
    void moveUp();
    void moveDown();
    void updateUI();
    void setChanged();

private:
    friend class MyCheckListItem;
    K3ListView *m_list;
    KPushButton *m_up, *m_down;
    bool m_changed;
};

class ColumnsDialog: public KDialog
{
    Q_OBJECT
public:
    static void display();

private:
    ColumnList *m_list;
    static ColumnsDialog *s_instance;
    ColumnsDialog();
    ~ColumnsDialog();
public:
    virtual void slotApply();
    virtual void slotOk();
    virtual void hide();
    void apply();
public slots:
    void slotChanged();
};

#endif

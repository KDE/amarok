/***************************************************************************
 * copyright            : (C) 2007 Leo Franchi <lfranchi@gmail.com>        *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef AMAROK_VERTICAL_LAYOUT_H
#define AMAROK_VERTICAL_LAYOUT_H

#include "amarok_export.h"

#include "plasma/layouts/layout.h"

#include <QtCore/QList>

namespace Context
{

/**
* This layout is kind of like a vertical BoxLayout, but is different in a few
* key ways: it does not take a fixed height, but rather calculates it based on the
* sizeHints of the items, and it uses getHeightForWidth in order to calculate this
* height.
*/

class AMAROK_EXPORT VerticalLayout : public Plasma::Layout
{
public:

    explicit VerticalLayout(LayoutItem *parent = 0);
    ~VerticalLayout();
    
    // reimplemented from Layout
    virtual void addItem(LayoutItem *l);
    virtual void removeItem(LayoutItem *l);
    virtual int indexOf(LayoutItem *l) const;
    virtual LayoutItem *itemAt(int i) const;
    virtual LayoutItem *takeAt(int i);
    virtual Qt::Orientations expandingDirections() const;
    virtual void setGeometry(const QRectF &geometry);
    virtual QRectF geometry() const;
    virtual int count() const;
    
    virtual QSizeF sizeHint() const;
    
    protected:
        void relayout();
        
    private:
        class Private;
        Private *const d;
};

}

#endif

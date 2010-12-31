#ifndef OPMLDIRECTORYVIEW_H
#define OPMLDIRECTORYVIEW_H

#include "src/widgets/PrettyTreeView.h"

class QContextMenuEvent;

class OpmlDirectoryView : public Amarok::PrettyTreeView
{
    Q_OBJECT
public:
    explicit OpmlDirectoryView( QWidget *parent = 0 );

    virtual void contextMenuEvent( QContextMenuEvent *event );

signals:

public slots:

};

#endif // OPMLDIRECTORYVIEW_H

//
// AmarokSystray
//
// Author: Stanislav Karchebny <berkus@users.sf.net>, (C) 2003
//
// Copyright: like rest of amaroK
//

#ifndef AMAROKSYSTRAY_H
#define AMAROKSYSTRAY_H

#include <ksystemtray.h>

class QMouseEvent;
class QWheelEvent;
class QDragEnterEvent;
class QDropEvent;

class KActionCollection;

class AmarokSystray : public KSystemTray
{
    Q_OBJECT

public:
    AmarokSystray( QWidget *, KActionCollection * );

private:
    void mousePressEvent( QMouseEvent * );
    void wheelEvent( QWheelEvent * );
    void dragEnterEvent( QDragEnterEvent * );
    void dropEvent( QDropEvent *e );
};

#endif

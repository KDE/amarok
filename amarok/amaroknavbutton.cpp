#include "amaroknavbutton.h"

#include <qpainter.h>

void AmarokNavButton::drawButton( QPainter *p )
{
    QRect r( rect() );
    p->fillRect( r, Qt::black );
    p->setPen( QColor( 0x80, 0xa0, 0xff ) );
    p->drawRoundRect( r.left() + 3, 
                      r.top() + 1,
                      r.right() - 3,
                      r.bottom() - 3 );
    drawButtonLabel(p);
}

#include "amaroknavbutton.moc"

#include "amaroknavbutton.h"

#include <qpainter.h>

void AmarokNavButton::drawButton( QPainter *p )
{
    QRect sz( p->window() );
    p->fillRect( sz, Qt::black );
    p->setPen( QColor( 0x80, 0xa0, 0xff ) );
    p->drawRoundRect( sz.left() + 2, sz.top() + 2, sz.right() - 2, sz.bottom() - 2 );
    drawButtonLabel(p);
}

#include "amaroknavbutton.moc"

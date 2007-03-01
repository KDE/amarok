#include "dockwidget.h"

DockWidget::DockWidget( const QString &title, QWidget *parent, Qt::WindowFlags flags ) :
    QDockWidget( title, parent, flags )
{
}

DockWidget::DockWidget() :
    QDockWidget()
{
    setFeatures( QDockWidget::NoDockWidgetFeatures );
}
void DockWidget::paintEvent(QPaintEvent *e)
{
    QPainter painter(this);
    painter.eraseRect( e->rect() );
}


#include <QGraphicsTextItem>
class QKeyEvent;

class MyGraphicsTextItem : public QGraphicsTextItem

{
    Q_OBJECT
public:
    MyGraphicsTextItem( QGraphicsItem * parent = 0, QGraphicsScene * scene = 0 );
signals:
    void editionValidated( QString editioncontent );
protected:
    virtual void keyPressEvent( QKeyEvent* Event );
    virtual void mousePressEvent ( QGraphicsSceneMouseEvent * event ); 
private:
    QString m_content;
};

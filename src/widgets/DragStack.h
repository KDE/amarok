#ifndef DRAGSTACK_H
#define DRAGSTACK_H

#include <QVBoxLayout>

class QDropEvent;
class Token;
class TokenDragger;
class TokenFactory;

class DragStack : public QVBoxLayout
{
    Q_OBJECT
public:
    DragStack( const QString &mimeType, QWidget *parent = 0);

    QWidget *childAt( const QPoint &pos ) const;
    void clear();
    virtual inline int count() const { return count( -1 ); }
    virtual int count ( int row ) const;
    void insertToken( Token*, int row = -1, int col = -1 ); // -1 -> append to last row
    int row ( Token* ) const;
    inline int rows() const { return QVBoxLayout::count() - 1; }
    inline uint rowLimit() const { return m_limits[1]; }
    inline void setRowLimit( uint r ) { m_limits[1] = r; }
    void setCustomTokenFactory( TokenFactory * factory );
    QList< Token *> drags( int row = -1 );

//     inline uint columnLimit() const { return m_limits[0]; }
//     inline void setColumnLimit( uint c ) { m_limits[0] = c; }
signals:
    void changed();
    void focussed( QWidget* );

protected:
    bool eventFilter( QObject *, QEvent * );
    QBoxLayout *rowBox( QWidget *w, QPoint *idx = 0 ) const;
    QBoxLayout *rowBox( const QPoint &pt ) const;
protected:
    friend class TokenDragger;
    void deleteEmptyRows();

private:
    bool accept( QDropEvent* );
    QHBoxLayout *appendRow();
    void drop( Token*, const QPoint &pos = QPoint(0,0) );

private:
    uint m_limits[2];
    QString m_mimeType;
    TokenDragger *m_tokenDragger;
    TokenFactory *m_tokenFactory;
};

#endif //DRAGSTACK_H

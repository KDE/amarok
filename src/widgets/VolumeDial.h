#include <QDial>

class VolumeDial : public QDial
{
    Q_OBJECT
public:
    VolumeDial( QWidget *parent = 0 );
    QSize sizeHint() const;
signals;
    void muteToggled( bool on );
protected:
    void paintEvent( QPaintEvent * );
    void mousePressEvent( QMouseEvent * );
    void mouseReleaseEvent( QMouseEvent * );
    void resizeEvent(QResizeEvent *);
private slots:
    void valueChangedSlot( int );
private:
    void updateIconBuffer();
    QImage m_icon, m_mutedIcon;
    QPixmap m_iconBuffer;
    int m_unmutedValue;
    bool m_isClick, m_muted;
};

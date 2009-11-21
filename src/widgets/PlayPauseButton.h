
#include <QWidget>

class PlayPauseButton : public QWidget
{
    Q_OBJECT
public:
    PlayPauseButton( QWidget *parent = 0 );
    QSize sizeHint() const;
    inline bool playing() const { return m_isPlaying; };
    void setPlaying( bool b );
signals:
    void toggled(bool playing);
protected:
    void enterEvent( QEvent * );
    void leaveEvent( QEvent * );
    void mousePressEvent( QMouseEvent * );
    void mouseReleaseEvent( QMouseEvent * );
    void paintEvent( QPaintEvent * );
    void resizeEvent(QResizeEvent *);
    void timerEvent ( QTimerEvent * );
private:
    void startFade();
    void stopFade();
    void updateIconBuffer();
    bool m_isPlaying, m_isClick;
    int m_animStep, m_animTimer;
    QPixmap m_iconBuffer;
    QImage m_iconPlay[2], m_iconPause[2];
};

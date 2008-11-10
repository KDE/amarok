
#ifndef SHAREDTIMERTEST_H
#define SHAREDTIMERTEST_H

#include <QList>
#include <QObject>

namespace Plasma
{
    class Timer;
} // namespace Plasma

class Tester : public QObject
{
    Q_OBJECT

public:
    Tester(int rounds);

private Q_SLOTS:
    void timeout();

private:
    int m_count;
    int m_round;
    int m_targetRounds;
    QList<Plasma::Timer*> m_order;
};

#endif


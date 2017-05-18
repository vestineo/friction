#ifndef MEMORYCHECKER_H
#define MEMORYCHECKER_H

#include <QObject>
#include <QTimer>

class MemoryChecker : public QObject {
    Q_OBJECT
public:
    explicit MemoryChecker(QObject *parent = 0);
    static MemoryChecker *getInstance() { return mInstance; }

    void decUsedMemory(const unsigned long long &used) {
        mUsedRam -= used;
    }

    void incUsedMemory(const unsigned long long &used) {
        mUsedRam += used;
    }
private:
    QTimer *mTimer;

    unsigned long long mMinFreeRam = 0ULL;
    unsigned long long mUsedRam = 0ULL;
    unsigned long long mLeaveUnused = 1500000000ULL;

    unsigned long long mFreeRam;
    unsigned long long mTotalRam;
    unsigned long long mMemUnit;
    static MemoryChecker *mInstance;
private slots:
    void checkMemory();
signals:
    void outOfMemory(unsigned long long);

};

#endif // MEMORYCHECKER_H

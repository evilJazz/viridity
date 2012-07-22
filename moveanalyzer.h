#ifndef MOVEANALYZER_H
#define MOVEANALYZER_H

#include <QVector>
#include <QRect>
#include <QImage>

typedef QVector<quint32> AreaFingerPrint;
typedef QVector<AreaFingerPrint> AreaFingerPrints;

AreaFingerPrint getAreaFingerPrint(QImage *image, const QRect &area);
AreaFingerPrints getAreaFingerPrintsSlow(QImage *image, const QRect &area, int templateWidth);
AreaFingerPrints getAreaFingerPrintsFast(QImage *image, const QRect &area, int templateWidth);

bool fingerPrintsEqual(const AreaFingerPrints &prints1, const AreaFingerPrints &prints2);

int indexOfFingerPrintInFingerPrint(const AreaFingerPrint &haystack, const AreaFingerPrint &needle);
bool findFingerPrintPosition(const AreaFingerPrints &prints, const AreaFingerPrint &print, QPoint &result);

class MoveAnalyzer
{
public:
    MoveAnalyzer(QImage *imageBefore, QImage *imageAfter, const QRect &hashArea, int templateWidth);

    QRect findMovedRect(const QRect &searchArea, const QRect &templateRect);
    QRect findMovedRectNaive(const QRect &searchArea, const QRect &templateRect);

private:
    QImage *imageBefore_;
    QImage *imageAfter_;
    QRect hashArea_;
    int templateWidth_;
    AreaFingerPrints searchAreaFingerPrints_;
};

#endif // MOVEANALYZER_H

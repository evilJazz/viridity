#ifndef MOVEANALYZERDEBUGVIEW_H
#define MOVEANALYZERDEBUGVIEW_H

#include <QWidget>
#include <QImage>

class MoveAnalyzer;

class MoveAnalyzerDebugView : public QWidget
{
    Q_OBJECT
public:
    explicit MoveAnalyzerDebugView(QWidget *parent = 0);

    void setMoveAnalyzer(MoveAnalyzer *moveAnalyzer);

protected:
    void paintEvent(QPaintEvent *pe);

private slots:
    void moveAnalyzerChanged();

private:
    MoveAnalyzer *moveAnalyzer_;
    QImage image_;
};

#endif // MOVEANALYZERDEBUGVIEW_H

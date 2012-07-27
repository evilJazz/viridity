#include "moveanalyzerdebugview.h"

#include <QPainter>

#include "moveanalyzer.h"

MoveAnalyzerDebugView::MoveAnalyzerDebugView(QWidget *parent) :
    QWidget(parent),
    moveAnalyzer_(NULL)
{
}

void MoveAnalyzerDebugView::setMoveAnalyzer(MoveAnalyzer *moveAnalyzer)
{
    if (moveAnalyzer_)
        disconnect(this, SLOT(moveAnalyzerChanged()));

    moveAnalyzer_ = moveAnalyzer;

    if (moveAnalyzer_)
    {
        connect(moveAnalyzer_, SIGNAL(changed()), this, SLOT(moveAnalyzerChanged()));
        moveAnalyzerChanged();
    }
}

void MoveAnalyzerDebugView::paintEvent(QPaintEvent *pe)
{
    QPainter p(this);
    p.drawImage(0, 0, image_);
}

void MoveAnalyzerDebugView::moveAnalyzerChanged()
{
    AreaFingerPrints *fps = &moveAnalyzer_->searchAreaFingerPrints_;

    image_ = QImage(fps->width(), fps->height(), QImage::Format_ARGB32);

    for (int y = 0; y < image_.height(); ++y)
    {
        for (int x = 0; x < image_.width(); ++x)
        {
            quint32 value = fps->fingerPrints()[x]->data()[y];
            image_.setPixel(x, y, value);
        }
    }

    resize(image_.size());
    update();
}

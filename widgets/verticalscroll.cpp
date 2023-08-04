#include "verticalscroll.h"

#include <QScrollBar>

VerticalScrollArea::VerticalScrollArea(QWidget *parent)
    : QScrollArea(parent)
{
    setWidgetResizable(true);
    setFrameStyle(QFrame::NoFrame);
    setAttribute(Qt::WA_StyledBackground, true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
}

void VerticalScrollArea::setContent(QWidget *content)
{
    content->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    setWidget(content);
}

void VerticalScrollArea::resizeEvent(QResizeEvent *e)
{
    widget()->setMaximumWidth(e->size().width());
    QScrollArea::resizeEvent(e);
}
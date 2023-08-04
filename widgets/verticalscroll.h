#ifndef VERTICALSCROLL
#define VERTICALSCROLL

#include <QScrollArea>
#include <QResizeEvent>

class VerticalScrollArea : public QScrollArea
{
public:
    VerticalScrollArea(QWidget *parent);

    void setContent(QWidget *content);
    
private:
    virtual void resizeEvent(QResizeEvent *e);
};

#endif
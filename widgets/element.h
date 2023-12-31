#ifndef ELEMENT
#define ELEMENT

#include "iconbutton.h"
#include "../tools/json.h"
#include "../tools/manager.h"

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>

class Element : public QWidget
{
    Q_OBJECT
public:
    Element(Id list, Id sublist, Id element, QWidget *p);

    void updateElement();

    void setNameAndContent(QString nameStr, QString contentStr);

    Id element;

signals:
    void changeButtonClicked();
    void deleteButtonClicked();
    void pressed(QPointF offset, QPointF pos);

private:
    QVBoxLayout *lay = nullptr; 
    QWidget *container = nullptr;
    QVBoxLayout *containerLay = nullptr;
    QWidget *header = nullptr;
    QHBoxLayout *headerLay = nullptr;
    QLabel *name = nullptr;
    IconButton *changeButton = nullptr;
    IconButton *deleteButton = nullptr;
    QLabel *content = nullptr;
    Id list, sublist;
    bool press = false;

    virtual void enterEvent(QEnterEvent *);
    virtual void leaveEvent(QEvent *);
    virtual void mousePressEvent(QMouseEvent *e);
};

#endif
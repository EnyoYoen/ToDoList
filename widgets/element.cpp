#include "element.h"

#include "../tools/json.h"
#include "../tools/manager.h"
#include "../tools/def.h"
#include "../tools/logger.h"

#include <QMouseEvent>

#include <iostream>

Element::Element(Id m_list, Id m_sublist, Id m_element, QWidget *p)
    : list(m_list), sublist(m_sublist), element(m_element), QWidget(p)
{
    JElement jelement = Manager::getElement(list, sublist, element);

    lay = new QVBoxLayout(this);
    container = new QWidget(this);
    containerLay = new QVBoxLayout(container);
    header = new QWidget(container);
    headerLay = new QHBoxLayout(header);
    name = new QLabel(jelement.name, header);
    changeButton = new IconButton([this]() {
        emit changeButtonClicked();
    }, "pen", header, 20);
    deleteButton = new IconButton([this]() {
        emit deleteButtonClicked();
    }, "bin", header, 20);
    content = new QLabel(jelement.content, container);

    changeButton->hide();
    deleteButton->hide();

    headerLay->addWidget(name);
    headerLay->addStretch();
    headerLay->addWidget(changeButton);
    headerLay->addWidget(deleteButton);
    headerLay->setContentsMargins(0, 0, 0, 0);
    name->setMinimumHeight(20);

    containerLay->addWidget(header);
    containerLay->addWidget(content);
    containerLay->setSpacing(5);
    containerLay->setContentsMargins(10, 8, 10, 8);

    lay->addWidget(container);
    lay->setContentsMargins(0, 0, 0, 0);

    name->setProperty("class", "element-title");
    content->setProperty("class", "element-content");
    setProperty("class", "element");
    setAttribute(Qt::WA_StyledBackground);
}

void Element::setNameAndContent(QString nameStr, QString contentStr)
{
    name->setText(nameStr);
    content->setText(contentStr);

    JElement jelement = Manager::getElement(list, sublist, element);
    jelement.content = contentStr;
    jelement.name = nameStr;
    Manager::replaceElement(list, sublist, element, jelement);
}

void Element::enterEvent(QEnterEvent *)
{
    changeButton->show();
    deleteButton->show();
}

void Element::leaveEvent(QEvent *)
{
    changeButton->hide();
    deleteButton->hide();
}

void Element::mousePressEvent(QMouseEvent *e)
{
    press = true;
    qDebug() << "pressed";
    emit pressed(e->position(), e->scenePosition());
}
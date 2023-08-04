#include "sublist.h"

#include "filters.h"
#include "../tools/json.h"
#include "../tools/manager.h"
#include "../tools/logger.h"

Sublist::Sublist(Id m_list, Id m_sublist, QWidget *p)
    : list(m_list), sublist(m_sublist), QLabel(p)
{
    JSublist jsublist = Manager::getSublist(list, sublist);

    lay = new QVBoxLayout(this);
    QWidget *titleContainer = new QWidget(this);
    QHBoxLayout *titleLay = new QHBoxLayout(titleContainer);
    renameButton = new IconButton([this]() {
        emit renameButtonClicked();
    }, "pen", titleContainer);
    deleteButton = new IconButton([this]() {
        emit deleteButtonClicked();
    }, "bin", titleContainer);
    title = new QLabel(jsublist.title, this);
    addButton = new QPushButton(this);
    elementsScroll = new QScrollArea(this);
    elementsContainer = new QWidget(elementsScroll);
    elementsLay = new QVBoxLayout(elementsContainer);

    elementsLay->setContentsMargins(0, 0, 0, 0);
    elementsLay->setSpacing(6);
    elementsLay->addStretch();
    elementsScroll->setWidget(elementsContainer);
    elementsScroll->setWidgetResizable(true);
    elementsScroll->setAttribute(Qt::WA_StyledBackground, true);
    elementsScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    HoverFilter *f = new HoverFilter(
        [this](QEnterEvent *) {
            renameButton->show();
            deleteButton->show();
        },
        [this](QEnterEvent *) {
            renameButton->hide();
            deleteButton->hide();
        });
    titleContainer->installEventFilter(f);

    renameButton->hide();
    deleteButton->hide();
    title->setMinimumHeight(30);

    titleLay->addWidget(renameButton, 0, Qt::AlignVCenter);
    titleLay->addStretch();
    titleLay->addWidget(title, 0, Qt::AlignHCenter);
    titleLay->addStretch();
    titleLay->addWidget(deleteButton, 0, Qt::AlignVCenter);
    titleLay->setContentsMargins(0, 0, 0, 0);
    lay->addWidget(titleContainer);
    lay->addWidget(elementsScroll);

    for (auto elementId : Manager::getElements(list, sublist))
        createElement(QString(), QString(), elementId);
    lay->addWidget(addButton, 0, Qt::AlignHCenter);
    lay->addSpacing(5);
    lay->setSpacing(6);

    title->setProperty("class", "sublist-title");
    elementsScroll->setProperty("class", "sublist-scroll");
    elementsContainer->setProperty("class", "sublist-container");
    addButton->setProperty("class", "sublist-add-button");
    setProperty("class", "sublist");

    QObject::connect(addButton, &QPushButton::clicked, this, &Sublist::newElement);
}

void Sublist::rename(QString name)
{
    title->setText(name);

    JSublist jsublist = Manager::getSublist(list, sublist);
    jsublist.title = name;
    Manager::replaceSublist(list, sublist, jsublist);
}

void Sublist::newElement()
{
    QList<QStringPair> prompts;
    prompts.append(QStringPair("Name", QString()));
    prompts.append(QStringPair("Content", QString()));
    emit popupRequest(prompts, "Create the new element", [this](bool cancelled, QStringList prompts) {
        if (!cancelled && !prompts.empty() && !prompts[0].isEmpty())
            createElement(prompts[0], prompts[1], InvalidId);
    });
}

void Sublist::deselect()
{
    // TODO : add border when selected and remove it in deselect
}

bool Sublist::dropElement(Id elementId, Id fromSublist, int y, int elementHeight)
{
    // TODO : Maybe replace with runtime values?
    #define L_LIMIT_OFFSET -10
    #define H_LIMIT_OFFSET 10 

    if (y >= this->y() + title->pos().y() + title->height() + L_LIMIT_OFFSET && y <= this->y() + addButton->y() + H_LIMIT_OFFSET) { // in the y limits
        y -= this->y() + title->pos().y() + title->height() + L_LIMIT_OFFSET;
        int pos = y * 2 / (elementHeight + 6) - 1;
        pos = (pos + 1) / 2;
        if (pos > elements.size())
            pos = elements.size();

        JElement jelement = Manager::getElement(list, fromSublist, elementId);
        Manager::removeElement(list, fromSublist, elementId);
        createElement(jelement.name, jelement.content, InvalidId, pos);

        return true;
    }
    return false;
}

void Sublist::createElement(QString name, QString content, Id elementId, int index)
{
    int position = elementsLay->count() - 1;
    if (index != -1 && index < elementsLay->count() - 1) {
        position = index;
    }
    if (elementId == InvalidId) {
        JElement jelement(name, content);
        elementId = Manager::addElement(list, sublist, jelement);
        if (index != -1)
            Manager::setIndexElement(list, sublist, elementId, position);
    }
    Element *element = new Element(list, sublist, elementId, this);
    elementsLay->insertWidget(position, element);
    elements.append(element);

    QObject::connect(element, &Element::changeButtonClicked, [this, element, elementId]() {
        QList<QStringPair> prompts;
        prompts.append(QStringPair("Name", Manager::getElement(list, sublist, elementId).name));
        prompts.append(QStringPair("Content", Manager::getElement(list, sublist, elementId).content));
        emit popupRequest(prompts, "Change the element", [this, element](bool cancelled, QStringList prompts) {
            if (!cancelled && !prompts.empty() && !prompts[0].isEmpty()) {
                element->setNameAndContent(prompts[0], prompts[1]);
            }
        });
    });
    QObject::connect(element, &Element::deleteButtonClicked, [this, element, elementId]() {
        emit popupRequest(QList<QStringPair>(), "Are you sure?", [this, element, elementId](bool cancelled, QStringList prompts) {
            if (!cancelled) {
                Manager::removeElement(list, sublist, elementId);
                element->deleteLater();
            }
        });
    });

    QObject::connect(element, &Element::pressed, [this, position, element, elementId](QPointF offsetPos, QPointF pos) {
        emit elementPressed(offsetPos, pos, elementId, position, element);
    });
}

void Sublist::mousePressEvent(QMouseEvent *e)
{
    qDebug() << "sublist";
    press = true;
    emit pressed(e->position());
}
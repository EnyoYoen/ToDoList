#ifndef POPUP
#define POPUP

#include "clickable.h"
#include "../tools/def.h"

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>

class PopUp : public Clickable
{
    Q_OBJECT
public:
    PopUp(QList<QStringPair> prompts, QString title, QWidget *p);

signals:
    void result(bool cancelled, QStringList prompts);

private:
    QHBoxLayout *lay = nullptr;
    QWidget *content = nullptr;
    QVBoxLayout *contentLay = nullptr;
    QLabel *header = nullptr;
    QWidget *buttonContainer = nullptr;
    QHBoxLayout *buttonLay = nullptr;
    QPushButton *okButton = nullptr;
    QPushButton *cancelButton = nullptr;
    std::vector<QLineEdit *> inputs;
    int focusedWidget = 0; // -2 : okButton / -1 : cancelButton / positive integer : prompt index
};

#endif
#include "min.h"
#include "ui_min.h"

Min::Min(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Min)
{
    ui->setupUi(this);
    this->show();
}

Min::~Min()
{
    delete ui;
}

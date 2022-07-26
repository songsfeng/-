#ifndef MIN_H
#define MIN_H

#include <QWidget>

namespace Ui {
class Min;
}

class Min : public QWidget
{
    Q_OBJECT

public:
    explicit Min(QWidget *parent = 0);
    ~Min();

private:
    Ui::Min *ui;
};

#endif // MIN_H

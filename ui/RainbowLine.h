#ifndef RAINBOWLINE_H
#define RAINBOWLINE_H

#include <QTimer>
#include <QWidget>

class RainbowLine : public QWidget {
    Q_OBJECT

public:
    explicit RainbowLine(QWidget* parent = nullptr);
    ~RainbowLine();

protected:
    void paintEvent(QPaintEvent* event) override;

private slots:
    void updateAnimation();

private:
    QTimer* m_timer;
    float m_offset;  // 0.0 to 1.0 for animation
};

#endif // RAINBOWLINE_H

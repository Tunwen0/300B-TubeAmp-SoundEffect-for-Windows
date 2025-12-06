#include "RainbowLine.h"
#include <QLinearGradient>
#include <QPainter>

RainbowLine::RainbowLine(QWidget* parent)
    : QWidget(parent)
    , m_timer(new QTimer(this))
    , m_offset(0.0f)
{
    setFixedHeight(2);

    // Update at ~60 FPS
    connect(m_timer, &QTimer::timeout, this, &RainbowLine::updateAnimation);
    m_timer->start(16);
}

RainbowLine::~RainbowLine() {
    if (m_timer->isActive()) {
        m_timer->stop();
    }
}

void RainbowLine::updateAnimation() {
    m_offset += 0.005f;
    if (m_offset > 1.0f) {
        m_offset -= 1.0f;
    }
    update();
}

void RainbowLine::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Create flowing gradient using RepeatSpread
    // Shift the start/end points based on offset to create movement effect
    QLinearGradient gradient(-m_offset * width(), 0, (1.0f - m_offset) * width(), 0);
    gradient.setSpread(QGradient::RepeatSpread);

    // Color stops: Dark Blue -> Purple -> Cyan -> Dark Blue
    gradient.setColorAt(0.0, QColor("#223355"));
    gradient.setColorAt(0.3, QColor("#aa00ff"));
    gradient.setColorAt(0.7, QColor("#00ffff"));
    gradient.setColorAt(1.0, QColor("#223355"));

    painter.fillRect(rect(), gradient);
}

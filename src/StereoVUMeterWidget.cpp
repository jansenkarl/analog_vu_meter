#include "StereoVUMeterWidget.h"

#include <algorithm>
#include <cmath>

#include <QPainter>
#include <QPainterPath>

static constexpr float kMinVu = -20.0f;
static constexpr float kMaxVu = 3.0f;
static constexpr float kPi = 3.14159265358979323846f;

static float clampf(float v, float lo, float hi)
{
    return std::max(lo, std::min(hi, v));
}

static float normalizedForVuDb(float vuDb)
{
    vuDb = clampf(vuDb, kMinVu, kMaxVu);

    if (vuDb <= -10.0f) {
        const float t = (vuDb - kMinVu) / (-10.0f - kMinVu);
        return 0.20f * std::pow(t, 2.2f);  // Compress -20 to -10 to 20% of scale
    }

    if (vuDb <= 0.0f) {
        const float t = (vuDb - (-10.0f)) / (0.0f - (-10.0f));
        return 0.20f + (0.78f - 0.20f) * std::pow(t, 1.4f);  // -10 to 0 gets 58% of scale
    }

    {
        const float t = (vuDb - 0.0f) / (kMaxVu - 0.0f);
        return 0.78f + (1.0f - 0.78f) * t;  // 0 to +3 gets 22% of scale
    }
}

static QPointF polarFromBottomPivot(const QPointF& pivot, float radius, float thetaDeg)
{
    const float theta = thetaDeg * (kPi / 180.0f);
    const float sx = std::sin(theta);
    const float cy = std::cos(theta);
    return QPointF(pivot.x() + radius * sx, pivot.y() - radius * cy);
}

StereoVUMeterWidget::StereoVUMeterWidget(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_NoSystemBackground);
}

void StereoVUMeterWidget::setLevels(float leftVuDb, float rightVuDb)
{
    left_ = leftVuDb;
    right_ = rightVuDb;
    update();
}

void StereoVUMeterWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);

    const QRectF r = rect();

    QLinearGradient bg(r.topLeft(), r.bottomRight());
    bg.setColorAt(0.0, QColor(20, 20, 22));
    bg.setColorAt(1.0, QColor(6, 6, 7));
    p.fillRect(r, bg);

    const qreal outerPad = std::max<qreal>(14.0, r.width() * 0.02);
    const QRectF inner = r.adjusted(outerPad, outerPad, -outerPad, -outerPad);

    const qreal gap = std::max<qreal>(16.0, inner.width() * 0.03);
    const qreal meterW = (inner.width() - gap) / 2.0;

    const QRectF leftRect(inner.left(), inner.top(), meterW, inner.height());
    const QRectF rightRect(leftRect.right() + gap, inner.top(), meterW, inner.height());

    drawMeter(p, leftRect, left_, "L");
    drawMeter(p, rightRect, right_, "R");
}

void StereoVUMeterWidget::drawMeter(QPainter &p, const QRectF &rect, float vuDb,
                                    const QString &cornerLabel) {
  p.save();

  // --- Frame ---
  const qreal frameRadius = std::min(rect.width(), rect.height()) * 0.06;

  QLinearGradient frameGrad(rect.topLeft(), rect.bottomRight());
  frameGrad.setColorAt(0.0, QColor(60, 62, 66));
  frameGrad.setColorAt(0.5, QColor(26, 27, 29));
  frameGrad.setColorAt(1.0, QColor(10, 10, 11));

  p.setPen(QPen(QColor(0, 0, 0, 160), 2.0));
  p.setBrush(frameGrad);
  p.drawRoundedRect(rect, frameRadius, frameRadius);

  // --- Face ---
  const qreal inset = std::max<qreal>(10.0, rect.width() * 0.04);
  const QRectF face = rect.adjusted(inset, inset, -inset, -inset);

  QLinearGradient faceGrad(face.topLeft(), face.bottomLeft());
  faceGrad.setColorAt(0.0, QColor(250, 246, 226));
  faceGrad.setColorAt(1.0, QColor(236, 230, 200));

  p.setPen(QPen(QColor(0, 0, 0, 90), 1.5));
  p.setBrush(faceGrad);
  p.drawRoundedRect(face, frameRadius * 0.75, frameRadius * 0.75);

  // --- Bezel ---
  const qreal bezelInset = std::max<qreal>(6.0, rect.width() * 0.02);
  const QRectF bezel =
      rect.adjusted(bezelInset, bezelInset, -bezelInset, -bezelInset);
  p.setPen(QPen(QColor(0, 0, 0, 45), 1.0));
  p.setBrush(Qt::NoBrush);
  p.drawRoundedRect(bezel, frameRadius * 0.85, frameRadius * 0.85);

  // --- Geometry ---
  const QPointF pivot(face.center().x(), face.bottom() - face.height() * 0.06);
  const qreal radius = std::min(face.width(), face.height()) * 0.52;

  const float norm = normalizedForVuDb(vuDb);
//   const float theta = -50.0f + 100.0f * norm;

  //   auto angleForVu = [](float vu) {
  //     const float n = normalizedForVuDb(vu);
  //     return -50.0f + 100.0f * n;
  //   };
  auto angleForVu = [](float vu) {
    static const QVector<QPair<float, float>> table = {
      {-20, -46},
      {-10, -34},
      {-7, -25},
      {-5, -16},
      {-3, -5},
      {-2, 2},
      {-1, 9},
      {0, 18},
      {1, 27},
      {2, 37},
      {3, 48} 
    };

    // clamp
    if (vu < -20)
      return -48.0f;
    if (vu >= 3)
      return 48.0f;

    // find segment
    for (int i = 0; i < table.size() - 1; ++i) {
      float v0 = table[i].first;
      float a0 = table[i].second;
      float v1 = table[i + 1].first;
      float a1 = table[i + 1].second;

      if (vu >= v0 && vu <= v1) {
        float t = (vu - v0) / (v1 - v0);
        return a0 + t * (a1 - a0);
      }
    }

    return 0.0f; // fallback
  };
  const float theta = angleForVu(vuDb);

  // --- Tick radii ---
  const qreal tickR1 = radius * 0.98;
  const qreal tickR2Major = radius * 1.10;
  const qreal tickR2Minor = radius * 1.04;

  // --- Tick values (major + minor) ---
  const QList<float> labels = {-22.0f,-20.0f, -10.0f, -7.0f, -6.0f, -5.0f,
                               -4.0f,  -3.0f,  -2.0f, -1.0f, -0.5f,
                               0.0f,   0.5f,   1.0f,  2.0f,  3.0f};

  // --- Arc background ---
  const qreal arcR = radius * .98;
  const QRectF arcRect(pivot.x() - arcR, pivot.y() - arcR, arcR * 2.0,
                       arcR * 2.0);

  const float redStart = angleForVu(0.0f);
  const float redEnd = angleForVu(3.0f);

  // Red zone
  p.setPen(QPen(QColor(170, 20, 20), std::max<qreal>(3.0, rect.width() * 0.012),
                Qt::SolidLine, Qt::RoundCap));
  p.drawArc(arcRect, static_cast<int>((90.0f - redEnd) * 16.0f),
            static_cast<int>((redEnd - redStart) * 16.0f));

  // Full arc outline
  p.setPen(QPen(QColor(0, 0, 0, 200), 2.0, Qt::SolidLine, Qt::RoundCap));
  p.drawArc(arcRect, static_cast<int>((90.0f - 48.0f) * 16.0f),
            static_cast<int>(96.0f * 16.0f));

  // --- Tick marks ---
  for (float v : labels) {
    bool major = (v == -20.0f || v == -10.0f || v == -7.0f || v == -5.0f ||
                  v == -3.0f || v == -2.0f || v == -1.0f || v == 0.0f ||
                  v == 1.0f || v == 2.0f || v == 3.0f);

    const float a = angleForVu(v);

    const QPointF p1 = polarFromBottomPivot(pivot, tickR1, a);
    const QPointF p2 =
        polarFromBottomPivot(pivot, major ? tickR2Major : tickR2Minor, a);

    QPen pen(QColor(0, 0, 0, 220), major ? 2.2 : 1.4, Qt::SolidLine,
             Qt::RoundCap);
    if (v >= 0.0f) {
      pen.setColor(QColor(170, 20, 20));
    }
    p.setPen(pen);
    p.drawLine(p1, p2);

    // --- Major tick labels only ---
    if (major) {
      QString t = (v > 0.0f) ? QString("+%1").arg(static_cast<int>(v))
                             : QString::number(static_cast<int>(v));

      QFont tf = p.font();
      tf.setBold(true);
      tf.setStretch(92);
      tf.setLetterSpacing(QFont::PercentageSpacing, 92);
      tf.setPointSizeF(rect.height() * 0.033);
      p.setFont(tf);

      const QPointF pt = polarFromBottomPivot(pivot, radius * 1.20, a);
      const QRectF tr(pt.x() - 18.0, pt.y() - 10.0, 36.0, 20.0);

      p.setPen(v >= 0.0f ? QColor(170, 20, 20) : QColor(0, 0, 0, 220));
      p.drawText(tr, Qt::AlignCenter, t);
    }
  }

  // --- VU text ---
  QFont vuFont = p.font();
  vuFont.setBold(true);
  vuFont.setStretch(90);
  vuFont.setLetterSpacing(QFont::PercentageSpacing, 95);
  vuFont.setPointSizeF(rect.height() * 0.045);
  p.setFont(vuFont);

  p.setPen(QColor(0, 0, 0, 210));

  const QRectF vuRect(face.left(), face.top() + face.height() * 0.12,
                      face.width(), face.height() * 0.12);

  p.drawText(vuRect, Qt::AlignHCenter | Qt::AlignVCenter, "VU");

//   // --- Corner label (L/R) ---
//   QFont corner = p.font();
//   corner.setBold(true);
//   corner.setStretch(90);
//   corner.setPointSizeF(rect.height() * 0.06);
//   p.setFont(corner);

//   const QRectF labelRect(face.left(), face.bottom() - face.height() * 0.32,
//                          face.width(), face.height() * 0.20);

//   p.setPen(QColor(0, 0, 0, 190));
//   p.drawText(labelRect, Qt::AlignHCenter | Qt::AlignVCenter, cornerLabel);

  // --- Needle shadow ---
  const QPointF needleTip = polarFromBottomPivot(pivot, radius * 0.98, theta);
  const QPointF shadowTip = needleTip + QPointF(2.0, 2.0);

  p.setPen(QPen(QColor(0, 0, 0, 80), std::max<qreal>(3.0, rect.width() * 0.008),
                Qt::SolidLine, Qt::RoundCap));
  p.drawLine(pivot + QPointF(2.0, 2.0), shadowTip);

  // --- Needle ---
  p.setPen(QPen(QColor(10, 10, 10), std::max<qreal>(3.0, rect.width() * 0.008),
                Qt::SolidLine, Qt::RoundCap));
  p.drawLine(pivot, needleTip);

  // --- Pivot cap ---
  const qreal capR = std::max<qreal>(10.0, rect.width() * 0.04);
  QRadialGradient capGrad(pivot, capR);
  capGrad.setColorAt(0.0, QColor(210, 210, 210));
  capGrad.setColorAt(0.6, QColor(90, 90, 92));
  capGrad.setColorAt(1.0, QColor(20, 20, 22));

  p.setPen(QPen(QColor(0, 0, 0, 120), 1.5));
  p.setBrush(capGrad);
  p.drawEllipse(pivot, capR, capR);

  p.setPen(QPen(QColor(0, 0, 0, 120), 2.0, Qt::SolidLine, Qt::RoundCap));
  p.drawLine(pivot + QPointF(-capR * 0.35, 0.0),
             pivot + QPointF(capR * 0.35, 0.0));

  p.restore();
}

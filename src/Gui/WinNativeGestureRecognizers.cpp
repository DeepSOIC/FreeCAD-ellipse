/*
 * DeepSOIC:
 * The implementation was copied from Qt's qwinnativepangesturerecognizer_win.cpp
 * and modified a little bit.
 */
#include "WinNativeGestureRecognizers.h"

#include <QPinchGesture>
#include <qevent.h>
#include <qgraphicsitem.h>
#include <qgesture.h>

#include <private/qgesture_p.h>
#include <private/qevent_p.h>
/*#include <private/qapplication_p.h>
#include <private/qwidget_p.h>*/

#include <private/qstandardgestures_p.h>
QT_BEGIN_NAMESPACE

#if !defined(QT_NO_NATIVE_GESTURES)

QGesture* WinNativeGestureRecognizerPinch::create(QObject* target)
{
  if (!target)
      return new QPinchGestureN; // a special case
  if (!target->isWidgetType())
      return 0;
  if (qobject_cast<QGraphicsObject *>(target))
      return 0;

 /* QWidget *q = static_cast<QWidget *>(target);
  QWidgetPrivate *d = q->d_func();
  d->nativeGesturePanEnabled = true;
  d->winSetupGestures();*/ //fails to compile =(, but we can rely on this being done by grabGesture(Pan...

  return new QPinchGestureN;
}

QGestureRecognizer::Result WinNativeGestureRecognizerPinch::recognize(QGesture *gesture, QObject *watched, QEvent *event)
{
    QPinchGestureN* q = static_cast<QPinchGestureN*> (gesture);
    //QPinchGesturePrivate* d = q->d_func();//this fails to compile =( But we can get away without it.

    QGestureRecognizer::Result result = QGestureRecognizer::Ignore;
    if (event->type() == QEvent::NativeGesture) {
        QNativeGestureEvent *ev = static_cast<QNativeGestureEvent*>(event);
        switch(ev->gestureType) {
        case QNativeGestureEvent::GestureBegin:
            break;
        case QNativeGestureEvent::Zoom:
            result = QGestureRecognizer::TriggerGesture;
            event->accept();
            break;
        case QNativeGestureEvent::GestureEnd:
            if (q->state() == Qt::NoGesture)
                return QGestureRecognizer::Ignore; // some other gesture has ended
            result = QGestureRecognizer::FinishGesture;
            break;
        default:
            return QGestureRecognizer::Ignore;
        }
        if (q->state() == Qt::NoGesture) {
            //start of a new gesture, prefill stuff
            //d->isNewSequence = true;
            q->setTotalChangeFlags(0); q->setChangeFlags(0);

            q->setLastCenterPoint(QPointF());
            q->setCenterPoint(
                  QPointF(
                    qreal(ev->position.x()),
                    qreal(ev->position.y())
                  )
            );
            q->setStartCenterPoint(q->centerPoint());
            q->setTotalRotationAngle(0.0); q->setLastRotationAngle(0.0);  q->setRotationAngle(0.0);
            q->setTotalScaleFactor(1.0); q->setLastScaleFactor(1.0); q->setScaleFactor(1.0);
            q->lastFingerDistance = ev->argument;
            q->fingerDistance = ev->argument;
        } else {//in the middle of gesture

            //store new last values
            q->setLastCenterPoint(q->centerPoint());
            q->setLastRotationAngle(q->rotationAngle());
            q->setLastScaleFactor(q->scaleFactor());
            q->lastFingerDistance = q->fingerDistance;

            //update the current values
            q->setScaleFactor(
                    (qreal)(q->fingerDistance) / (qreal)(q->lastFingerDistance)
            );
            q->setCenterPoint(
                  QPointF(
                    qreal(ev->position.x()),
                    qreal(ev->position.y())
                  )
            );
            q->fingerDistance = ev->argument;

            //compute the changes
            QPinchGesture::ChangeFlags cf = 0;
            if (q->lastScaleFactor() != q->scaleFactor())
                cf |= QPinchGesture::ScaleFactorChanged;
            if (q->lastCenterPoint() != q->centerPoint())
                cf |= QPinchGesture::CenterPointChanged;
            q->setChangeFlags(cf);

            //increment totals
            q->setTotalChangeFlags (q->totalChangeFlags() | q->changeFlags());
            q->setTotalScaleFactor (q->scaleFactor() * q->scaleFactor());
            //d->totalRotationAngle unsupported by Windows =(
        }
    }
    return result;
}


void WinNativeGestureRecognizerPinch::reset(QGesture* gesture)
{
  QGestureRecognizer::reset(gesture);//resets the state of the gesture, which is not write-accessible otherwise
  QPinchGestureN *q = static_cast<QPinchGestureN*>(gesture);
  q->lastFingerDistance = 0;
  q->setTotalChangeFlags(0); q->setChangeFlags(0);

  q->setLastCenterPoint(QPointF());
  q->setCenterPoint(
        QPointF(
          0.0,
          0.0
        )
  );
  q->setStartCenterPoint(q->centerPoint());
  q->setTotalRotationAngle(0.0); q->setLastRotationAngle(0.0);  q->setRotationAngle(0.0);
  q->setTotalScaleFactor(1.0); q->setLastScaleFactor(1.0); q->setScaleFactor(1.0);
  q->lastFingerDistance = 0;
  q->fingerDistance = 0;
}

#endif //!defined(QT_NO_NATIVE_GESTURES)

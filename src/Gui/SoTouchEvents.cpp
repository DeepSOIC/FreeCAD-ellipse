#include "PreCompiled.h"
#include "SoTouchEvents.h"
#include <Base/Exception.h>

SO_EVENT_ABSTRACT_SOURCE(SoGestureEvent);

SbBool SoGestureEvent::isSoGestureEvent(const SoEvent *ev) const
{
    return ev->isOfType(SoGestureEvent::getClassTypeId());
}

//----------------------------SoGesturePanEvent--------------------------------

SO_EVENT_SOURCE(SoGesturePanEvent);

SoGesturePanEvent::SoGesturePanEvent(QPanGesture* qpan)
{
    totalOffset = SbVec2f(qpan->offset().x(), qpan->offset().y());
    deltaOffset = SbVec2f(qpan->delta().x(), qpan->delta().y());
    state = SbGestureState(qpan->state());
}

SbBool SoGesturePanEvent::isSoGesturePanEvent(const SoEvent *ev) const
{
    return ev->isOfType(SoGesturePanEvent::getClassTypeId());
}

//----------------------------SoGesturePinchEvent--------------------------------

SO_EVENT_SOURCE(SoGesturePinchEvent);

SoGesturePinchEvent::SoGesturePinchEvent(QPinchGesture* qpinch)
{
    startCenter = SbVec2f(qpinch->startCenterPoint().x(), qpinch->startCenterPoint().y());
    curCenter = SbVec2f(qpinch->centerPoint().x(), qpinch->centerPoint().y());
    deltaCenter = curCenter - SbVec2f(qpinch->lastCenterPoint().x(), qpinch->lastCenterPoint().y());
    deltaZoom = qpinch->scaleFactor();
    totalZoom = qpinch->totalScaleFactor();
    deltaAngle = qpinch->rotationAngle();
    totalAngle = qpinch->totalRotationAngle();
    state = SbGestureState(qpinch->state());
}

SbBool SoGesturePinchEvent::isSoGesturePinchEvent(const SoEvent *ev) const
{
    return ev->isOfType(SoGesturePinchEvent::getClassTypeId());
}

//----------------------------SoGestureSwipeEvent--------------------------------

SO_EVENT_SOURCE(SoGestureSwipeEvent);

SoGestureSwipeEvent::SoGestureSwipeEvent(QSwipeGesture* qswipe)
{
    angle = qswipe->swipeAngle();
    switch (qswipe->verticalDirection()){
    case QSwipeGesture::Up :
        vertDir = +1;
    break;
    case QSwipeGesture::Down :
        vertDir = -1;
    break;
    default:
        vertDir = 0;
    break;
    }
    switch (qswipe->horizontalDirection()){
    case QSwipeGesture::Right :
        vertDir = +1;
    break;
    case QSwipeGesture::Left :
        vertDir = -1;
    break;
    default:
        vertDir = 0;
    break;
    }

    state = SbGestureState(qswipe->state());
}

SbBool SoGestureSwipeEvent::isSoGestureSwipeEvent(const SoEvent *ev) const
{
    return ev->isOfType(SoGestureSwipeEvent::getClassTypeId());
}


//----------------------------GesturesDevice-------------------------------

GesturesDevice::GesturesDevice(QWidget* widget)
{
    if (SoGestureEvent::getClassTypeId().isBad()){
        SoGestureEvent::initClass();
        SoGesturePanEvent::initClass();
        SoGesturePinchEvent::initClass();
        SoGestureSwipeEvent::initClass();
    }
    if (! widget)
        throw Base::Exception("Can't create a gestures quarter input device without widget (null pointer was passed).");
    this->widget = widget;
}

const SoEvent* GesturesDevice::translateEvent(QEvent* event)
{
    if (event->type() == QEvent::Gesture
            || event->type() == QEvent::GestureOverride) {
        QGestureEvent* gevent = static_cast<QGestureEvent*>(event);

        QPinchGesture* zg = static_cast<QPinchGesture*>(gevent->gesture(Qt::PinchGesture));
        if(zg){
            gevent->setAccepted(Qt::PinchGesture,true);//prefer it over pan
            return new SoGesturePinchEvent(zg);
        }

        QPanGesture* pg = static_cast<QPanGesture*>(gevent->gesture(Qt::PanGesture));
        if(pg){
            gevent->setAccepted(Qt::PanGesture,true);
            return new SoGesturePanEvent(pg);
        }

        QSwipeGesture* sg = static_cast<QSwipeGesture*>(gevent->gesture(Qt::SwipeGesture));
        if(sg){
            gevent->setAccepted(Qt::SwipeGesture,true);
            return new SoGesturePanEvent(pg);
        }
    }
    return 0;
}

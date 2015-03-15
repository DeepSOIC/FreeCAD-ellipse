#ifndef SOTOUCHEVENTS_H
#define SOTOUCHEVENTS_H

#include "PreCompiled.h"
#include <qgesture.h>
#include <Quarter/devices/InputDevice.h>
namespace Quarter = SIM::Coin3D::Quarter;

class SoGestureEvent : public SoEvent {
    SO_EVENT_ABSTRACT_HEADER();
public:
    static void initClass(){
        SO_EVENT_INIT_ABSTRACT_CLASS(SoGestureEvent, SoEvent);
    };
    SoGestureEvent(){};
    ~SoGestureEvent(){};
    SbBool isSoGestureEvent(const SoEvent* ev) const;

    enum SbGestureState {
        SbGSNoGesture = Qt::NoGesture,
        SbGSStart = Qt::GestureStarted,
        SbGSUpdate = Qt::GestureUpdated,
        SbGSEnd = Qt::GestureFinished,
        SbGsCanceled = Qt::GestureCanceled
    };
    SbGestureState state;
};

class SoGesturePanEvent : public SoGestureEvent {
    SO_EVENT_HEADER();
public:
    static void initClass(){//needs to be called before the class can be used. Initializes type IDs of the class.
        SO_EVENT_INIT_CLASS(SoGesturePanEvent, SoGestureEvent);
    };
    SoGesturePanEvent() {};
    SoGesturePanEvent(QPanGesture* qpan);
    ~SoGesturePanEvent(){};
    SbBool isSoGesturePanEvent(const SoEvent* ev) const;

    SbVec2f deltaOffset;
    SbVec2f totalOffset;
};

class SoGesturePinchEvent : public SoGestureEvent {
    SO_EVENT_HEADER();
public:
    static void initClass(){
        SO_EVENT_INIT_CLASS(SoGesturePinchEvent, SoGestureEvent);
    };
    SoGesturePinchEvent(){};
    SoGesturePinchEvent(QPinchGesture* qpinch);
    ~SoGesturePinchEvent(){};
    SbBool isSoGesturePinchEvent(const SoEvent* ev) const;

    SbVec2f startCenter;
    SbVec2f curCenter;
    SbVec2f deltaCenter;
    double deltaZoom;
    double totalZoom;
    double deltaAngle;
    double totalAngle;

};

class SoGestureSwipeEvent : public SoGestureEvent {
    SO_EVENT_HEADER();
public:
    static void initClass(){
        SO_EVENT_INIT_CLASS(SoGestureSwipeEvent, SoGestureEvent);
    };
    SoGestureSwipeEvent(){};
    SoGestureSwipeEvent(QSwipeGesture* qswipe);
    ~SoGestureSwipeEvent(){};
    SbBool isSoGestureSwipeEvent(const SoEvent* ev) const;

    double angle;
    int vertDir;//+1,0,-1 up/none/down
    int horzDir;//+1,0,-1 right/none/left
};


class GesturesDevice : public Quarter::InputDevice {
public:
    GesturesDevice(QWidget* widget);//it needs to know the widget to do coordinate translation

    virtual ~GesturesDevice()  {}
    virtual const SoEvent* translateEvent(QEvent* event);
protected:
    QWidget* widget;
};

#endif // SOTOUCHEVENTS_H

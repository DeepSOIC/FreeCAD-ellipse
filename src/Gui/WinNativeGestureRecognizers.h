/*! This file adds support for pinch gestures in Windows 8 to Qt4.8. I think it
 * may not be necessary for Qt5. I also think this was actually not absolutely
 * necessary, and it may be possible to force Qt gesture recognition from plain
 * touch input.
 */

#ifndef WINNATIVEGESTURERECOGNIZERS_H
#define WINNATIVEGESTURERECOGNIZERS_H

#include <QGestureRecognizer>
#include <private/qgesture_p.h>

class QPinchGestureN: public QPinchGesture
{
public:
    int lastFingerDistance;//distance between fingers, in pixels
    int fingerDistance;
};

class WinNativeGestureRecognizerPinch : public QGestureRecognizer
{
public:
    WinNativeGestureRecognizerPinch(){}
    virtual QGesture* create ( QObject* target );
    virtual Result recognize ( QGesture* gesture, QObject* watched, QEvent* event );
    virtual void reset ( QGesture* gesture );
};

#endif // WINNATIVEGESTURERECOGNIZERS_H

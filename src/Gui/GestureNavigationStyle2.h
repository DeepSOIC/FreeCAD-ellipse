#ifndef GESTURENAVIGATIONSTYLE2_H
#define GESTURENAVIGATIONSTYLE2_H

#include <FCConfig.h>

#include "NavigationStyle.h"

#include <queue>

namespace Gui {


class GestureNavigationStyle2: public UserNavigationStyle
{
    typedef UserNavigationStyle superclass;

    TYPESYSTEM_HEADER();

public:
    GestureNavigationStyle2();
    virtual ~GestureNavigationStyle2() override;
    const char* mouseButtons(ViewerMode) override;

protected:
    SbBool processSoEvent(const SoEvent* const ev) override;
public:
    ///calls processSoEvent of NavigationStyle.
    SbBool processSoEvent_bypass(const SoEvent* const ev);
    virtual void setViewingMode(const ViewerMode newmode) override;

protected://state machine classes
    class Event;
    //class ViewerAnimationBeginEvent;

    class StateMachine;

    class IdleState;
    ///when operating a dragger, for example
    class InteractState;
    ///button was pressed, but the cursor hasn't moved yet
    class AwaitingMoveState;
    class GestureState;
    ///rotating the viewed model with mouse drag
    class RotateState;
    ///panning with mouse drag
    class PanState;
    ///tilting with mouse drag
    class TiltState;
    ///this state discards all mouse input after a gesture, untill all buttons are released.
    class AwaitingReleaseState;
    class AnimatingState;

    class EventQueue: public std::queue<SoMouseButtonEvent>
    {
    public:
        EventQueue(GestureNavigationStyle2& ns):myNS(ns){}

        void post(const Event& ev);
        void discardAll();
        void forwardAll();
    public:
        GestureNavigationStyle2& myNS;
    };


protected: // members variables
    std::unique_ptr<StateMachine> myStateMachine;
    EventQueue myQueue;

    //settings:
    ///if false, tilting with touchscreen gestures will be disabled
    bool enableGestureTilt = false;
    ///distance in px to treat as a definite drag (noise gate)
    int mouseMoveThreshold = 5;

protected: //helper functions
    bool isDraggerUnderCursor(SbVec2s pos);
public:
    bool is2DViewing() const;

public: //gesture reactions
    ///Roll gesture is like: press LMB, press RMB, release LMB, release RMB.
    /// This function is called by state machine whenever it picks up roll gesture.
    void onRollGesture(int direction);
    ///Called by state machine, when set-rotation-center gesture is detected (MMB click, or H key)
    void onSetRotationCenter(SbVec2s cursor);
};

}
#endif // GESTURENAVIGATIONSTYLE2_H

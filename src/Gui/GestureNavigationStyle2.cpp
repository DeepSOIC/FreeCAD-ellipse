#include "PreCompiled.h"
#ifndef _PreComp_
# include <Inventor/actions/SoRayPickAction.h>
# include <Inventor/SoPickedPoint.h>
# include <Inventor/SoFullPath.h>
# include <Inventor/draggers/SoDragger.h>
#endif

#include "GestureNavigationStyle2.h"

#include <App/Application.h>
#include <Base/Console.h>
#include "View3DInventorViewer.h"
#include "Application.h"
#include "SoTouchEvents.h"

#include <boost/statechart/state_machine.hpp>
#include <boost/statechart/simple_state.hpp>
#include <boost/statechart/state.hpp>
#include <boost/statechart/custom_reaction.hpp>
#include <boost/mpl/list.hpp>

namespace sc = boost::statechart;
typedef Gui::GestureNavigationStyle2 NS;

namespace Gui {

class NS::Event : public sc::event<NS::Event>
{
public:
    Event():flags(new Flags){}
    virtual ~Event(){}

    void log() const {
        if (isPress(1))
            Base::Console().Log("button1 press ");
        if (isPress(2))
            Base::Console().Log("button2 press ");
        if (isPress(3))
            Base::Console().Log("button3 press ");
        if (isRelease(1))
            Base::Console().Log("button1 release ");
        if (isRelease(2))
            Base::Console().Log("button2 release ");
        if (isRelease(3))
            Base::Console().Log("button3 release ");
        if (isMouseButtonEvent())
            Base::Console().Log("%x \n", modifiers);
        if (isGestureEvent()){
            Base::Console().Log("Gesture ");
            switch(asGestureEvent()->state){
            case SoGestureEvent::SbGSStart:
                Base::Console().Log("start ");
            break;
            case SoGestureEvent::SbGSEnd:
                Base::Console().Log("end ");
            break;
            case SoGestureEvent::SbGSUpdate:
                Base::Console().Log("data ");
            break;
            default:
                Base::Console().Log("??? ");
            }

            Base::Console().Log(inventor_event->getTypeId().getName().getString());
            Base::Console().Log("\n");
        }
    }

    //cast shortcuts
    bool isMouseButtonEvent() const {
        return this->inventor_event->isOfType(SoMouseButtonEvent::getClassTypeId());
    }
    const SoMouseButtonEvent* asMouseButtonEvent() const {
        return static_cast<const SoMouseButtonEvent*>(this->inventor_event);
    }
    bool isPress(int button_index) const {
        if (! isMouseButtonEvent())
            return false;
        int sobtn = SoMouseButtonEvent::BUTTON1 + button_index - 1;
        return asMouseButtonEvent()->getButton() == sobtn && asMouseButtonEvent()->getState() == SoMouseButtonEvent::DOWN;
    }
    bool isRelease(int button_index) const {
        if (! isMouseButtonEvent())
            return false;
        int sobtn = SoMouseButtonEvent::BUTTON1 + button_index - 1;
        return asMouseButtonEvent()->getButton() == sobtn && asMouseButtonEvent()->getState() == SoMouseButtonEvent::UP;
    }
    bool isKeyboardEvent() const {
        return this->inventor_event->isOfType(SoKeyboardEvent::getClassTypeId());
    }
    const SoKeyboardEvent* asKeyboardEvent() const {
        return static_cast<const SoKeyboardEvent*>(this->inventor_event);
    }
    bool isLocation2Event() const {
        return this->inventor_event->isOfType(SoLocation2Event::getClassTypeId());
    }
    const SoLocation2Event* asLocation2Event() const {
        return static_cast<const SoLocation2Event*>(this->inventor_event);
    }
    bool isMotion3Event() const {
        return this->inventor_event->isOfType(SoMotion3Event::getClassTypeId());
    }
    bool isGestureEvent() const {
        return this->inventor_event->isOfType(SoGestureEvent::getClassTypeId());
    }
    const SoGestureEvent* asGestureEvent() const {
        return static_cast<const SoGestureEvent*>(this->inventor_event);
    }
    bool isGestureActive() const {
        if (!isGestureEvent())
            return false;
        if (asGestureEvent()->state == SoGestureEvent::SbGSStart
            || asGestureEvent()->state == SoGestureEvent::SbGSUpdate )
            return true;
        else
            return false;
    }
public:
    enum {
        // bits: 0-shift-ctrl-alt-0-lmb-mmb-rmb
        BUTTON1DOWN = 0x00000100,
        BUTTON2DOWN = 0x00000001,
        BUTTON3DOWN = 0x00000010,
        CTRLDOWN =    0x00100000,
        SHIFTDOWN =   0x01000000,
        ALTDOWN =     0x00010000,
        MASKBUTTONS = BUTTON1DOWN | BUTTON2DOWN | BUTTON3DOWN,
        MASKMODIFIERS = CTRLDOWN | SHIFTDOWN | ALTDOWN
    };

public:
    const SoEvent* inventor_event;
    unsigned int modifiers;
    unsigned int mbstate() const {return modifiers & MASKBUTTONS;}
    unsigned int kbdstate() const {return modifiers & MASKMODIFIERS;}

    struct Flags{
        bool processed = false; //the value to be returned by processSoEvent.
        bool propagated = false; //flag that the event had been passed to superclass
    };
    std::shared_ptr<Flags> flags;
    //storing these values as a separate unit allows to effectively write to
    //const object. Statechart passes all events as const, unfortunately, so
    //this is a workaround. I've considered casting away const instead, but
    //the internet seems to have mixed opinion if it's undefined behavior or
    //not. --DeepSOIC

};

//class NS::ViewerAnimationBeginEvent : public sc::event<NS::ViewerAnimationBeginEvent>
//{
//public:
//    ViewerMode old_mode;
//    ViewerMode new_mode;
//};

//------------------------------state machine ---------------------------

class NS::StateMachine : public sc::state_machine<NS::StateMachine, NS::IdleState>
{
public:
    typedef sc::state_machine<NS::StateMachine, NS::IdleState> superclass;

    StateMachine(NS& ns) : ns(ns) {}
    NS& ns;

public:
    virtual void processEvent(NS::Event& ev) {
        // #FIXME: maybe remove this routine?
        ev.log();
        this->process_event(ev);
    }
};

class NS::IdleState : public sc::state<NS::IdleState, NS::StateMachine>
{
public:
    typedef sc::custom_reaction<NS::Event> reactions;

    IdleState(my_context ctx):my_base(ctx)
    {
        this->outermost_context().ns.setViewingMode(NavigationStyle::IDLE);
        Base::Console().Log(" -> IdleState\n");
    }
    virtual ~IdleState(){}

    sc::result react(const NS::Event& ev){
        auto &ns = this->outermost_context().ns;

        auto posn = ns.normalizePixelPos(ev.inventor_event->getPosition());

        switch (ns.getViewingMode()) {
            case NavigationStyle::SEEK_WAIT_MODE:{
                if (ev.isPress(1)) {
                    ns.seekToPoint(ev.inventor_event->getPosition()); // implicitly calls interactiveCountInc()
                    ns.setViewingMode(NavigationStyle::SEEK_MODE);
                    ev.flags->processed = true;
                    return transit<NS::AwaitingReleaseState>();
                }
            } ; //not end of SEEK_WAIT_MODE. Fall through by design!!!
                /* FALLTHRU */
            case NavigationStyle::SPINNING:
            case NavigationStyle::SEEK_MODE: {
                //animation modes
                if (!ev.flags->processed) {
                    if (ev.isMouseButtonEvent()){
                        ev.flags->processed = true;
                        return transit<NS::AwaitingReleaseState>();
                    } else if (ev.isGestureEvent()
                                || ev.isKeyboardEvent()
                                || ev.isMotion3Event())
                        ns.setViewingMode(NavigationStyle::IDLE);
                }
            } break; //end of animation modes
            case BOXZOOM:
                return sc::result::no_reaction;
        }
        if(ev.isPress(1) && ev.mbstate() == 0x100){
            if (this->outermost_context().ns.isDraggerUnderCursor(ev.inventor_event->getPosition()))
                return transit<NS::InteractState>();
        }
        if (ev.isPress(1) && ev.mbstate() == 0x100){
            this->outermost_context().ns.myQueue.post(ev);
            ev.flags->processed = true;
            return transit<NS::AwaitingMoveState>();
        }
        if(ev.isPress(2) && ev.mbstate() == 0x001){
            this->outermost_context().ns.myQueue.post(ev);
            ev.flags->processed = true;
            return transit<NS::AwaitingMoveState>();
        }
        if(ev.isPress(3) && ev.mbstate() == 0x010){
            ev.flags->processed = true;
            this->outermost_context().ns.onSetRotationCenter(ev.inventor_event->getPosition());
            return transit<NS::AwaitingReleaseState>();
        }

        if(ev.isMouseButtonEvent() && ev.asMouseButtonEvent()->getButton() == SoMouseButtonEvent::BUTTON4){
            //wheel
            ns.doZoom(ns.viewer->getSoRenderManager()->getCamera(), true, posn);
            ev.flags->processed = true;
        }
        if(ev.isMouseButtonEvent() && ev.asMouseButtonEvent()->getButton() == SoMouseButtonEvent::BUTTON5){
            //wheel
            ns.doZoom(ns.viewer->getSoRenderManager()->getCamera(), false, posn);
            ev.flags->processed = true;
        }

        if(ev.isGestureActive()){
            ev.flags->processed = true;
            return transit<NS::GestureState>();
        }
        if(ev.isKeyboardEvent()){
            auto const &kbev = ev.asKeyboardEvent();
            ev.flags->processed = true;
            bool press = (kbev->getState() == SoKeyboardEvent::DOWN);
            switch (kbev->getKey()) {
                case SoKeyboardEvent::H:
                    if (press)
                        ns.onSetRotationCenter(kbev->getPosition());
                break;
                case SoKeyboardEvent::PAGE_UP:
                    if(press){
                        ns.doZoom(ns.viewer->getSoRenderManager()->getCamera(), true, posn);
                    }
                break;
                case SoKeyboardEvent::PAGE_DOWN:
                    if(press){
                        ns.doZoom(ns.viewer->getSoRenderManager()->getCamera(), false, posn);
                    }
                break;
                default:
                    ev.flags->processed = false;
            }
        }
        return sc::result::no_reaction;
    }
};

class NS::AwaitingMoveState : public sc::state<NS::AwaitingMoveState, NS::StateMachine>
{
public:
    typedef sc::custom_reaction<NS::Event> reactions;

private:
    SbVec2s base_pos;

public:
    AwaitingMoveState(my_context ctx):my_base(ctx)
    {
        this->outermost_context().ns.setViewingMode(NavigationStyle::IDLE);
        this->base_pos = static_cast<const NS::Event*>(this->triggering_event())->inventor_event->getPosition();
        Base::Console().Message(" -> AwaitingMoveState\n");
    }
    virtual ~AwaitingMoveState(){
        //always clear posponed events when leaving this state.
        this->outermost_context().ns.myQueue.discardAll();
    }

    sc::result react(const NS::Event& ev){
        ///refire(): process all posponed events + this event
        auto refire = [&]{
            this->outermost_context().ns.myQueue.forwardAll();
            ev.flags->processed = this->outermost_context().ns.processSoEvent_bypass(ev.inventor_event);
            ev.flags->propagated = true;
        };

        auto &ns = this->outermost_context().ns;

        //this state consumes all mouse events.
        ev.flags->processed = ev.isMouseButtonEvent() || ev.isLocation2Event();

        if (ev.isRelease(2)
           && ev.mbstate() == 0
           && !ns.viewer->isEditing()
           && ns.isPopupMenuEnabled()){
            ns.openPopupMenu(ev.inventor_event->getPosition());
            return transit<NS::IdleState>();
        } else if(ev.isMouseButtonEvent() && ev.mbstate() == 0){
            //all buttons released, refire all events && return to idle state
            ns.setViewingMode(NavigationStyle::SELECTION);
            refire();
            return transit<NS::IdleState>();
        } else if (ev.isPress(3)){
            //mmb pressed, exit navigation
            refire();
            return transit<NS::IdleState>();
        } else if (ev.isRelease(1) && ev.mbstate() == 0x001){
            this->outermost_context().ns.onRollGesture(+1);
            return transit<NS::AwaitingReleaseState>();
        } else if (ev.isRelease(2) && ev.mbstate() == 0x100){
            this->outermost_context().ns.onRollGesture(-1);
            return transit<NS::AwaitingReleaseState>();
        } else if (ev.isMouseButtonEvent()){
            this->outermost_context().ns.myQueue.post(ev);
        }
        if(ev.isLocation2Event()){
            auto mv = ev.inventor_event->getPosition() - this->base_pos;
            if(SbVec2f(mv).length() > this->outermost_context().ns.mouseMoveThreshold)
            {
                //mouse moved while buttons are held. decide how to navigate...
                this->outermost_context().ns.myQueue.discardAll();
                switch(ev.mbstate()){
                    case 0x100:{
                        bool alt = ev.modifiers & NS::Event::ALTDOWN;
                        bool allowSpin = alt == this->outermost_context().ns.is2DViewing();
                        if(allowSpin)
                            return transit<NS::RotateState>();
                        else {
                            refire();
                            return transit<NS::IdleState>();
                        }
                    }break;
                    case 0x001:
                        return transit<NS::PanState>();
                    break;
                    case (0x101):
                        return transit<NS::TiltState>();
                    break;
                    default:
                        //MMB was held? refire all events.
                        refire();
                        return transit<NS::IdleState>();
                }
            }
        }
        if(ev.isGestureActive()){
            ev.flags->processed = true;
            return transit<NS::GestureState>();
        }
        return sc::result::no_reaction;
    }
};

class NS::RotateState : public sc::state<NS::RotateState, NS::StateMachine>
{
public:
    typedef sc::custom_reaction<NS::Event> reactions;

private:
    SbVec2s base_pos;

public:
    RotateState(my_context ctx):my_base(ctx)
    {
        this->outermost_context().ns.setViewingMode(NavigationStyle::DRAGGING);
        this->base_pos = static_cast<const NS::Event*>(this->triggering_event())->inventor_event->getPosition();
        Base::Console().Message(" -> RotateState\n");
    }
    virtual ~RotateState(){}

    sc::result react(const NS::Event& ev){
        if(ev.isMouseButtonEvent()){
            ev.flags->processed = true;
            if (ev.mbstate() == 0x101){
                return transit<NS::TiltState>();
            }
            if (ev.mbstate() == 0){
                return transit<NS::IdleState>();
            }
        }
        if(ev.isLocation2Event()){
            ev.flags->processed = true;
            SbVec2s pos = ev.inventor_event->getPosition();
            auto &ns = this->outermost_context().ns;
            ns.spin_simplified(
                        ns.viewer->getSoRenderManager()->getCamera(),
                        ns.normalizePixelPos(pos), ns.normalizePixelPos(this->base_pos));
            this->base_pos = pos;
        }
        return sc::result::no_reaction;
    }
};

class NS::PanState : public sc::state<NS::PanState, NS::StateMachine>
{
public:
    typedef sc::custom_reaction<NS::Event> reactions;

private:
    SbVec2s base_pos;
    float ratio;

public:
    PanState(my_context ctx):my_base(ctx)
    {
        auto &ns = this->outermost_context().ns;
        ns.setViewingMode(NavigationStyle::PANNING);
        this->base_pos = static_cast<const NS::Event*>(this->triggering_event())->inventor_event->getPosition();
        Base::Console().Message(" -> PanState\n");
        this->ratio = ns.viewer->getSoRenderManager()->getViewportRegion().getViewportAspectRatio();
        ns.pan(ns.viewer->getSoRenderManager()->getCamera());//set up panningplane
    }
    virtual ~PanState(){}

    sc::result react(const NS::Event& ev){
        if(ev.isMouseButtonEvent()){
            ev.flags->processed = true;
            if (ev.mbstate() == 0x101){
                return transit<NS::TiltState>();
            }
            if (ev.mbstate() == 0){
                return transit<NS::IdleState>();
            }
        }
        if(ev.isLocation2Event()){
            ev.flags->processed = true;
            SbVec2s pos = ev.inventor_event->getPosition();
            auto &ns = this->outermost_context().ns;
            ns.panCamera(ns.viewer->getSoRenderManager()->getCamera(),
                         this->ratio,
                         ns.panningplane,
                         ns.normalizePixelPos(pos),
                         ns.normalizePixelPos(this->base_pos));
            this->base_pos = pos;
        }
        return sc::result::no_reaction;
    }
};

class NS::TiltState : public sc::state<NS::TiltState, NS::StateMachine>
{
public:
    typedef sc::custom_reaction<NS::Event> reactions;

private:
    SbVec2s base_pos;

public:
    TiltState(my_context ctx):my_base(ctx)
    {
        auto &ns = this->outermost_context().ns;
        ns.setViewingMode(NavigationStyle::DRAGGING);
        this->base_pos = static_cast<const NS::Event*>(this->triggering_event())->inventor_event->getPosition();
        Base::Console().Message(" -> TiltState\n");
        ns.pan(ns.viewer->getSoRenderManager()->getCamera());//set up panningplane
    }
    virtual ~TiltState(){}

    sc::result react(const NS::Event& ev){
        if(ev.isMouseButtonEvent()){
            ev.flags->processed = true;
            if (ev.mbstate() == 0x001){
                return transit<NS::PanState>();
            }
            if (ev.mbstate() == 0x100){
                return transit<NS::RotateState>();
            }
            if (ev.mbstate() == 0){
                return transit<NS::IdleState>();
            }
        }
        if(ev.isLocation2Event()){
            ev.flags->processed = true;
            auto &ns = this->outermost_context().ns;
            SbVec2s pos = ev.inventor_event->getPosition();
            float dx = (ns.normalizePixelPos(pos)-ns.normalizePixelPos(base_pos))[0];
            ns.doRotate(ns.viewer->getSoRenderManager()->getCamera(),
                        dx*(-2),
                        SbVec2f(0.5,0.5));
            this->base_pos = pos;
        }
        return sc::result::no_reaction;
    }
};


class NS::GestureState : public sc::state<NS::GestureState, NS::StateMachine>
{
public:
    typedef sc::custom_reaction<NS::Event> reactions;

private:
    SbVec2s base_pos;
    float ratio;
    bool enableTilt = false;

public:
    GestureState(my_context ctx):my_base(ctx)
    {
        auto &ns = this->outermost_context().ns;
        ns.setViewingMode(NavigationStyle::PANNING);
        this->base_pos = static_cast<const NS::Event*>(this->triggering_event())->inventor_event->getPosition();
        Base::Console().Log(" -> GestureState\n");
        ns.pan(ns.viewer->getSoRenderManager()->getCamera());//set up panningplane
        this->ratio = ns.viewer->getSoRenderManager()->getViewportRegion().getViewportAspectRatio();
        enableTilt = !(App::GetApplication().GetParameterGroupByPath
                ("User parameter:BaseApp/Preferences/View")->GetBool("DisableTouchTilt",true));
    }
    virtual ~GestureState(){}

    sc::result react(const NS::Event& ev){
        auto &ns = this->outermost_context().ns;
        if(ev.isMouseButtonEvent()){
            ev.flags->processed = true;
            if (ev.mbstate() == 0){
                //a fail-safe: if gesture end event doesn't arrive, a mouse click should be able to stop this mode.
                Base::Console().Warning("leaving gesture state by mouse-click (fail-safe)\n");
                return transit<NS::IdleState>();
            }
        }
        if(ev.isLocation2Event()){
            //consume all mouse events that Qt fires during the gesture (stupid Qt, so far it only causes trouble)
            ev.flags->processed = true;
        }
        if(ev.isGestureEvent()){
            ev.flags->processed = true;
            if(ev.asGestureEvent()->state == SoGestureEvent::SbGSEnd){
                return transit<NS::AwaitingReleaseState>();
            } else if (ev.asGestureEvent()->state == SoGestureEvent::SbGsCanceled){
                //should maybe undo the camera change caused by gesture events received so far...
                return transit<NS::AwaitingReleaseState>();
            //} else if (ev.asGestureEvent()->state == SoGestureEvent::SbGSStart){
            //    //ignore?
            } else if (ev.inventor_event->isOfType(SoGesturePanEvent::getClassTypeId())){
                auto const &pangesture = static_cast<const SoGesturePanEvent*>(ev.inventor_event);
                SbVec2f panDist = ns.normalizePixelPos(pangesture->deltaOffset);
                ns.panCamera(ns.viewer->getSoRenderManager()->getCamera(),
                             ratio,
                             ns.panningplane,
                             panDist,
                             SbVec2f(0,0));
            } else if (ev.inventor_event->isOfType(SoGesturePinchEvent::getClassTypeId())){
                const SoGesturePinchEvent* const pinch = static_cast<const SoGesturePinchEvent*>(ev.inventor_event);
                SbVec2f panDist = ns.normalizePixelPos(pinch->deltaCenter.getValue());
                Base::Console().Log("    pandist= %f,%f, deltazoom=%f\n", panDist[0], panDist[1], pinch->deltaZoom);
                ns.panCamera(ns.viewer->getSoRenderManager()->getCamera(),
                             ratio,
                             ns.panningplane,
                             panDist,
                             SbVec2f(0,0));
                ns.doZoom(ns.viewer->getSoRenderManager()->getCamera(),
                          -logf(float(pinch->deltaZoom)),
                          ns.normalizePixelPos(pinch->curCenter));
                if (pinch->deltaAngle != 0.0 && enableTilt){
                    ns.doRotate(ns.viewer->getSoRenderManager()->getCamera(),
                                float(pinch->deltaAngle),
                                ns.normalizePixelPos(pinch->curCenter));
                }
            } else {
                //unknown gesture
                ev.flags->processed = false;
            }
        }
        return sc::result::no_reaction;
    }
};

class NS::AwaitingReleaseState : public sc::state<NS::AwaitingReleaseState, NS::StateMachine>
{
public:
    typedef sc::custom_reaction<NS::Event> reactions;

public:
    AwaitingReleaseState(my_context ctx):my_base(ctx)
    {
        Base::Console().Message(" -> AwaitingReleaseState\n");
    }
    virtual ~AwaitingReleaseState(){}

    sc::result react(const NS::Event& ev){
        if(ev.isMouseButtonEvent()){
            ev.flags->processed = true;
            if (ev.mbstate() == 0){
                return transit<NS::IdleState>();
            }
            if (ev.isRelease(1) && ev.mbstate() == 0x001){
                this->outermost_context().ns.onRollGesture(+1);
                return transit<NS::AwaitingReleaseState>();
            } else if (ev.isRelease(2) && ev.mbstate() == 0x100){
                this->outermost_context().ns.onRollGesture(-1);
                return transit<NS::AwaitingReleaseState>();
            }
        }
        if(ev.isLocation2Event()){
            ev.flags->processed = true;
        }
        if(ev.isGestureActive()){
            ev.flags->processed = true;
            //another gesture can start...
            return transit<NS::GestureState>();
        }
        return sc::result::no_reaction;
    }
};

class NS::InteractState : public sc::state<NS::InteractState, NS::StateMachine>
{
public:
    typedef sc::custom_reaction<NS::Event> reactions;

public:
    InteractState(my_context ctx):my_base(ctx)
    {
        auto &ns = this->outermost_context().ns;
        ns.setViewingMode(NavigationStyle::INTERACT);
        Base::Console().Message(" -> InteractState\n");
    }
    virtual ~InteractState(){}

    sc::result react(const NS::Event& ev){
        if(ev.isMouseButtonEvent()){
            ev.flags->processed = false; //feed all events to the dragger/whatever
            if (ev.mbstate() == 0){ //all buttons released?
                return transit<NS::IdleState>();
            }
        }
        return sc::result::no_reaction;
    }
};

//------------------------------/state machine ---------------------------


/* TRANSLATOR Gui::GestureNavigationStyle */

TYPESYSTEM_SOURCE(Gui::GestureNavigationStyle2, Gui::UserNavigationStyle);


GestureNavigationStyle2::GestureNavigationStyle2()
    : myStateMachine(new NS::StateMachine(*this)),
      myQueue(*this)
{
    mouseMoveThreshold = QApplication::startDragDistance();
    myStateMachine->initiate();
}

GestureNavigationStyle2::~GestureNavigationStyle2()
{

}

const char* GestureNavigationStyle2::mouseButtons(ViewerMode mode)
{
    switch (mode) {
    case NavigationStyle::SELECTION:
        return QT_TR_NOOP("Tap OR click left mouse button.");
    case NavigationStyle::PANNING:
        return QT_TR_NOOP("Drag screen with two fingers OR press right mouse button.");
    case NavigationStyle::DRAGGING:
        return QT_TR_NOOP("Drag screen with one finger OR press left mouse button. In Sketcher && other edit modes, hold Alt in addition.");
    case NavigationStyle::ZOOMING:
        return QT_TR_NOOP("Pinch (place two fingers on the screen && drag them apart from || towards each other) OR scroll middle mouse button OR PgUp/PgDown on keyboard.");
    default:
        return "No description";
    }
}


SbBool GestureNavigationStyle2::processSoEvent(const SoEvent* const ev)
{
    // Events when in "ready-to-seek" mode are ignored, except those
    // which influence the seek mode itself -- these are handled further
    // up the inheritance hierarchy.
    if (this->isSeekMode()) { return superclass::processSoEvent(ev); }
    // Switch off viewing mode (Bug #0000911)
    if (!this->isSeekMode()&& !this->isAnimating() && this->isViewing() )
        this->setViewing(false); // by default disable viewing mode to render the scene

    NS::Event smev;
    smev.inventor_event = ev;

    //mode-independent spaceball/joystick handling
    if (ev->isOfType(SoMotion3Event::getClassTypeId())){
        smev.flags->processed = true;
        this->processMotionEvent(static_cast<const SoMotion3Event*>(ev));
        return true;
    }

    // give the nodes in the foreground root the chance to handle events (e.g color bar)
    if (!viewer->isEditing()) {
        bool processed = handleEventInForeground(ev);
        if (processed) return true;
    }


    if (smev.isMouseButtonEvent()) {
        const int button = smev.asMouseButtonEvent()->getButton();
        const SbBool press //the button was pressed (if false -> released)
                = smev.asMouseButtonEvent()->getState() == SoButtonEvent::DOWN ? true : false;
        switch (button) {
        case SoMouseButtonEvent::BUTTON1:
            this->button1down = press;
            break;
        case SoMouseButtonEvent::BUTTON2:
            this->button2down = press;
            break;
        case SoMouseButtonEvent::BUTTON3:
            this->button3down = press;
            break;
        //whatever else, we don't track
        }
    }
    this->ctrldown = ev->wasCtrlDown();
    this->shiftdown = ev->wasShiftDown();
    this->altdown = ev->wasAltDown();

    smev.modifiers =
        (this->button1down ? NS::Event::BUTTON1DOWN : 0) |
        (this->button2down ? NS::Event::BUTTON2DOWN : 0) |
        (this->button3down ? NS::Event::BUTTON3DOWN : 0) |
        (this->ctrldown    ? NS::Event::CTRLDOWN : 0) |
        (this->shiftdown   ? NS::Event::SHIFTDOWN : 0) |
        (this->altdown     ? NS::Event::ALTDOWN : 0);


    if (! smev.flags->processed)
        this->myStateMachine->processEvent(smev);
    if(! smev.flags->propagated && ! smev.flags->processed)
        return superclass::processSoEvent(ev);
    else
        return smev.flags->processed;
}

SbBool GestureNavigationStyle2::processSoEvent_bypass(const SoEvent* const ev)
{
    return superclass::processSoEvent(ev);
}

void GestureNavigationStyle2::setViewingMode(const NavigationStyle::ViewerMode newmode)
{
    //NS::ViewerAnimationBeginEvent ev;
    //ev.old_mode = ViewerMode(this->getViewingMode());
    //ev.new_mode = newmode;

    superclass::setViewingMode(newmode);

    //switch (newmode) {
    //    case SPINNING:
    //    case SEEK_MODE:
    //    case SEEK_WAIT_MODE:
    //        this->myStateMachine->process_event(ev);
    //    break;
    //    default:
    //        ;//
    //}
}

bool GestureNavigationStyle2::isDraggerUnderCursor(SbVec2s pos)
{
    SoRayPickAction rp(this->viewer->getSoRenderManager()->getViewportRegion());
    rp.setRadius(viewer->getPickRadius());
    rp.setPoint(pos);
    rp.apply(this->viewer->getSoRenderManager()->getSceneGraph());
    SoPickedPoint* pick = rp.getPickedPoint();
    if (pick){
        const SoFullPath* fullpath = static_cast<const SoFullPath*>(pick->getPath());
        for(int i = 0; i < fullpath->getLength(); ++i){
            if(fullpath->getNode(i)->isOfType(SoDragger::getClassTypeId()))
                return true;
        }
        return false;
    } else {
        return false;
    }
}

bool GestureNavigationStyle2::is2DViewing() const
{
    // #FIXME: detect sketch editing, ! any editing
    return this->viewer->isEditing();
}

void GestureNavigationStyle2::onRollGesture(int direction)
{
    if (direction == +1)
        Base::Console().Message("Roll forward gesture\n");
    else if (direction == -1)
        Base::Console().Message("Roll backward gesture\n");
}

void GestureNavigationStyle2::onSetRotationCenter(SbVec2s cursor){
    SbBool ret = NavigationStyle::lookAtPoint(cursor);
    if(!ret){
        this->interactiveCountDec(); //this was in original gesture nav. Not sure what is it needed for --DeepSOIC
        Base::Console().Warning(
            "No object under cursor! Can't set new center of rotation.\n");
    }

}

void GestureNavigationStyle2::EventQueue::post(const GestureNavigationStyle2::Event& ev)
{
    ev.flags->processed = true;
    this->push(*ev.asMouseButtonEvent());
}

void GestureNavigationStyle2::EventQueue::discardAll()
{
    this->empty();
}

void GestureNavigationStyle2::EventQueue::forwardAll()
{
    while(! this->empty()){
        auto v = this->front();
        this->myNS.processSoEvent_bypass(&v);
        this->pop();
    }
}

}//namespace Gui

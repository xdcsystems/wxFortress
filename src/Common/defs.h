#pragma once

#include <wx/event.h>

#define USE_LOGGER 0
#if defined( _DEBUG ) || !defined( NDEBUG )
#define SKIP_GL_CHECKS 0
#else
#define SKIP_GL_CHECKS 1
#endif
#define SKIP_AL_CHECKS 1

#define INITIAL_LIVES_VALUE 9
#define DEFAULT_DELTATIME 16

#define BEGIN_BALL_VELOCITY 3.f
#define BEGIN_BOARD_VELOCITY 0.48f

// Range of angles
#define BEGIN_ANGLE 30.0
#define END_RIGHT_ANGLE 70.0
#define BEGIN_LEFT_ANGLE 110.0
#define END_ANGLE 150.0

#define NUM_OF_CALCULATED_POINTS 5
#define INCREASE_VELOCITY_STEP .01f
#define INCREASE_DEFAULT_STEP 1.f


// from ShapesManager.h
DECLARE_LOCAL_EVENT_TYPE( wxEVT_CURRENT_SCORE_INCREASED, wxID_ANY )
DECLARE_LOCAL_EVENT_TYPE( wxEVT_ROUND_COMPLETED, wxID_ANY )
DECLARE_LOCAL_EVENT_TYPE( wxEVT_BALL_LOST, wxID_ANY )
DECLARE_LOCAL_EVENT_TYPE( wxEVT_PING, wxID_ANY )
DECLARE_LOCAL_EVENT_TYPE( wxEVT_PONG, wxID_ANY )
DECLARE_LOCAL_EVENT_TYPE( wxEVT_ARMOR_HIT, wxID_ANY )
DECLARE_LOCAL_EVENT_TYPE( wxEVT_PROTOCOL_ACTIVATED, wxID_ANY )

// from TextRenderer.h
DECLARE_LOCAL_EVENT_TYPE( wxEVT_CHAR_SHOW, wxID_ANY )

// from RenderWindow.h
DECLARE_LOCAL_EVENT_TYPE( wxEVT_WND_INITIALIZED, wxID_ANY )
DECLARE_LOCAL_EVENT_TYPE( wxEVT_LAUNCH_PRESSED, wxID_ANY )
DECLARE_LOCAL_EVENT_TYPE( wxEVT_NEW_ROUND_STARTED, wxID_ANY )
DECLARE_LOCAL_EVENT_TYPE( wxEVT_STAGE_FINISHED, wxID_ANY )
DECLARE_LOCAL_EVENT_TYPE( wxEVT_RESET, wxID_ANY )

// from MediaManager.h
DECLARE_LOCAL_EVENT_TYPE( wxEVT_VIDEO_FINISHED, wxID_ANY )
DECLARE_LOCAL_EVENT_TYPE( wxEVT_VIDEO_PLAY, wxID_ANY )


inline void queueCommandEvent( wxEvtHandler* handler, wxEventType eventType )
{
    if ( handler )
        handler->QueueEvent( new wxCommandEvent( eventType ) );
}

inline void queueCommandEvent( wxEvtHandler* handler, wxEventType eventType, int intPayload, long extraPayload = 0 )
{
    if ( handler )
    {
        auto* ev = new wxCommandEvent( eventType );
        ev->SetInt( intPayload );
        ev->SetExtraLong( extraPayload );
        handler->QueueEvent( ev );
    }
}

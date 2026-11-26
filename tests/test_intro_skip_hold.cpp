#include <gtest/gtest.h>
#include <limits>

#include "IntroSkipHold.hpp"

TEST( IntroSkipHoldTest, DoesNotTriggerBeforeThreeSeconds )
{
    IntroSkipHold::State s;
    EXPECT_TRUE( IntroSkipHold::onKeyDown( s, 10.0 ) );
    EXPECT_TRUE( s.holding );
    EXPECT_DOUBLE_EQ( s.progress, 0.0 );

    EXPECT_FALSE( IntroSkipHold::update( s, 11.5 ) );
    EXPECT_NEAR( s.progress, 0.5, 1e-9 );
    EXPECT_TRUE( s.holding );
    EXPECT_FALSE( s.triggered );

    EXPECT_FALSE( IntroSkipHold::update( s, 12.99 ) );
    EXPECT_NEAR( s.progress, 2.99 / 3.0, 1e-9 );
    EXPECT_FALSE( s.triggered );
}

TEST( IntroSkipHoldTest, TriggersExactlyAtThreeSecondsAndOnlyOnce )
{
    IntroSkipHold::State s;
    EXPECT_TRUE( IntroSkipHold::onKeyDown( s, 5.0 ) );
    EXPECT_TRUE( IntroSkipHold::update( s, 8.0 ) );
    EXPECT_TRUE( s.triggered );
    EXPECT_FALSE( s.holding );
    EXPECT_DOUBLE_EQ( s.progress, 1.0 );

    // Subsequent updates or key presses must never re-trigger.
    EXPECT_FALSE( IntroSkipHold::update( s, 10.0 ) );
    EXPECT_FALSE( IntroSkipHold::onKeyDown( s, 11.0 ) );
    EXPECT_FALSE( IntroSkipHold::update( s, 15.0 ) );
}

TEST( IntroSkipHoldTest, ReleaseBeforeThreeSecondsCancelsAndResetsProgress )
{
    IntroSkipHold::State s;
    EXPECT_TRUE( IntroSkipHold::onKeyDown( s, 1.0 ) );
    EXPECT_FALSE( IntroSkipHold::update( s, 3.5 ) );
    EXPECT_NEAR( s.progress, 2.5 / 3.0, 1e-9 );

    EXPECT_TRUE( IntroSkipHold::onKeyUp( s ) );
    EXPECT_FALSE( s.holding );
    EXPECT_FALSE( s.triggered );
    EXPECT_DOUBLE_EQ( s.progress, 0.0 );

    // Holding again must start fresh from 0 for a full 3 seconds.
    EXPECT_TRUE( IntroSkipHold::onKeyDown( s, 4.0 ) );
    EXPECT_FALSE( IntroSkipHold::update( s, 6.5 ) );
    EXPECT_NEAR( s.progress, 2.5 / 3.0, 1e-9 );
    EXPECT_TRUE( IntroSkipHold::update( s, 7.0 ) );
}

TEST( IntroSkipHoldTest, KeyAutoRepeatDoesNotResetHoldTimer )
{
    IntroSkipHold::State s;
    EXPECT_TRUE( IntroSkipHold::onKeyDown( s, 0.0 ) );
    EXPECT_FALSE( IntroSkipHold::update( s, 1.0 ) );
    // OS auto-repeat fires repeated KEY_DOWN events while held.
    EXPECT_FALSE( IntroSkipHold::onKeyDown( s, 1.1 ) );
    EXPECT_FALSE( IntroSkipHold::onKeyDown( s, 1.2 ) );
    EXPECT_TRUE( IntroSkipHold::update( s, 3.0 ) );
}

TEST( IntroSkipHoldTest, RapidDoublePressDoesNotTriggerOrCorruptState )
{
    IntroSkipHold::State s;
    EXPECT_TRUE( IntroSkipHold::onKeyDown( s, 2.0 ) );
    EXPECT_FALSE( IntroSkipHold::update( s, 2.05 ) );
    EXPECT_TRUE( IntroSkipHold::onKeyUp( s ) );
    EXPECT_TRUE( IntroSkipHold::onKeyDown( s, 2.10 ) );
    EXPECT_FALSE( IntroSkipHold::update( s, 2.15 ) );
    EXPECT_TRUE( IntroSkipHold::onKeyUp( s ) );
    EXPECT_FALSE( s.holding );
    EXPECT_FALSE( s.triggered );
    EXPECT_DOUBLE_EQ( s.progress, 0.0 );
}

TEST( IntroSkipHoldTest, NonMonotonicOrNaNTimeHandledSafely )
{
    IntroSkipHold::State s;
    EXPECT_FALSE( IntroSkipHold::onKeyDown( s, std::numeric_limits<double>::quiet_NaN() ) );
    EXPECT_TRUE( IntroSkipHold::onKeyDown( s, 10.0 ) );
    EXPECT_FALSE( IntroSkipHold::update( s, 9.0 ) );
    EXPECT_DOUBLE_EQ( s.progress, 0.0 );
    EXPECT_FALSE( IntroSkipHold::update( s, std::numeric_limits<double>::quiet_NaN() ) );
    EXPECT_DOUBLE_EQ( s.progress, 0.0 );
    EXPECT_FALSE( IntroSkipHold::update( s, std::numeric_limits<double>::infinity() ) );
    EXPECT_FALSE( IntroSkipHold::update( s, -std::numeric_limits<double>::infinity() ) );
    EXPECT_DOUBLE_EQ( s.progress, 0.0 );
}

TEST( IntroSkipHoldTest, ResetAndIdleOrPostTriggerKeyUp )
{
    IntroSkipHold::State s;
    EXPECT_FALSE( IntroSkipHold::onKeyUp( s ) );

    EXPECT_TRUE( IntroSkipHold::onKeyDown( s, 0.0 ) );
    EXPECT_TRUE( IntroSkipHold::update( s, 3.0 ) );
    EXPECT_TRUE( s.triggered );
    EXPECT_FALSE( IntroSkipHold::onKeyUp( s ) );
    EXPECT_DOUBLE_EQ( s.progress, 1.0 );

    IntroSkipHold::reset( s );
    EXPECT_FALSE( s.holding );
    EXPECT_FALSE( s.triggered );
    EXPECT_DOUBLE_EQ( s.progress, 0.0 );
    EXPECT_TRUE( IntroSkipHold::onKeyDown( s, 10.0 ) );
    EXPECT_TRUE( IntroSkipHold::update( s, 13.0 ) );
}

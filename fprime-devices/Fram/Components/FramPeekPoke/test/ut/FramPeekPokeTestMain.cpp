// ======================================================================
// \title  FramPeekPokeTestMain.cpp
// \author tumbar
// \brief  cpp file for FramPeekPoke component test main function
// ======================================================================

#include "Fw/Test/UnitTest.hpp"
#include "STest/Random/Random.hpp"
#include "fprime-devices/Fram/Components/FramPeekPoke/test/ut/FramPeekPokeTester.hpp"

TEST(Peek, Widths) {
    REQUIREMENT("FRAM-PP-001");
    COMMENT("PEEK of each width reads the big-endian word and reports it in Peeked");
    Fram::FramPeekPokeTester tester;
    tester.testPeek();
}

TEST(Poke, Widths) {
    REQUIREMENT("FRAM-PP-002");
    COMMENT("POKE of each width writes exactly the big-endian word");
    Fram::FramPeekPokeTester tester;
    tester.testPoke();
}

TEST(Poke, TooWide) {
    REQUIREMENT("FRAM-PP-003");
    COMMENT("POKE of a value wider than the width is rejected before touching F-RAM");
    Fram::FramPeekPokeTester tester;
    tester.testPokeTooWide();
}

TEST(Fram, AccessFailed) {
    REQUIREMENT("FRAM-PP-004");
    COMMENT("a failed completion fails the command");
    Fram::FramPeekPokeTester tester;
    tester.testAccessFailed();
}

TEST(Fram, RequestRejected) {
    REQUIREMENT("FRAM-PP-004");
    COMMENT("a request the driver rejects fails the command");
    Fram::FramPeekPokeTester tester;
    tester.testRequestRejected();
}

TEST(Command, Busy) {
    REQUIREMENT("FRAM-PP-005");
    COMMENT("a command while one is outstanding is BUSY");
    Fram::FramPeekPokeTester tester;
    tester.testBusy();
}

TEST(Fram, InCallCompletion) {
    REQUIREMENT("FRAM-PP-006");
    COMMENT("PEEK and POKE complete when the driver completes inside the request call");
    Fram::FramPeekPokeTester tester;
    tester.testInCallCompletion();
}

int main(int argc, char** argv) {
    STest::Random::seed();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

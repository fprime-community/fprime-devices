// ======================================================================
// \title  FramDriverTestMain.cpp
// \brief  cpp file for FramDriver component test main function
// ======================================================================

#include "FramDriverTester.hpp"
#include "Fw/Test/UnitTest.hpp"
#include "STest/Random/Random.hpp"

#define FRAM_TEST(NAME, REQS)          \
    TEST(FramDriver, NAME) {           \
        COMMENT(#NAME);                \
        REQUIREMENT(REQS);             \
        Fram::FramDriverTester tester; \
        tester.NAME();                 \
    }

FRAM_TEST(ReadNominal, "REQ-FramDriver-001, 003, 005")
FRAM_TEST(WriteNominal, "REQ-FramDriver-002, 003, 005")
FRAM_TEST(CompletionPortIndex, "REQ-FramDriver-004")
FRAM_TEST(CompletionBufferIdentity, "REQ-FramDriver-011")
FRAM_TEST(AcceptedBeforeComplete, "REQ-FramDriver-005")
FRAM_TEST(OutOfRangeLimits, "REQ-FramDriver-006, 008")
FRAM_TEST(BusyWhileTransfer, "REQ-FramDriver-007, 008")
FRAM_TEST(BusyBeforeOutOfRange, "REQ-FramDriver-007")
FRAM_TEST(SpiErrorAfterReturn, "REQ-FramDriver-009, 016")
FRAM_TEST(InCallRefusalBusy, "REQ-FramDriver-021, 008")
FRAM_TEST(InCallRefusalError, "REQ-FramDriver-021, 008")
FRAM_TEST(InCallRefusalMidTransfer, "REQ-FramDriver-009")
FRAM_TEST(RequestInsideCompletion, "REQ-FramDriver-010")
FRAM_TEST(RequestAfterCompletion, "REQ-FramDriver-010")
FRAM_TEST(StartupReads, "REQ-FramDriver-012, 015, 017")
FRAM_TEST(StartupIdMismatch, "REQ-FramDriver-013")
FRAM_TEST(StartupReadFailed, "REQ-FramDriver-014")
FRAM_TEST(StartupInCallRefusal, "REQ-FramDriver-014")
FRAM_TEST(BlockProtectWarning, "REQ-FramDriver-015")
FRAM_TEST(WriteProtectedRange, "REQ-FramDriver-022")
FRAM_TEST(WriteProtectedUnknownRange, "REQ-FramDriver-022")
FRAM_TEST(TelemetryCounters, "REQ-FramDriver-017")
FRAM_TEST(EventsRequestRejected, "REQ-FramDriver-016")
FRAM_TEST(EventsSpiError, "REQ-FramDriver-016")
FRAM_TEST(OutOfRangeBeforeWriteProtected, "REQ-FramDriver-006, 022")
FRAM_TEST(InCallOkCompletion, "REQ-FramDriver-004, 010")
FRAM_TEST(InCallOkThenRefusal, "REQ-FramDriver-009, 021")
FRAM_TEST(UnsolicitedCompletionAsserts, "REQ-FramDriver-009")
FRAM_TEST(StartupTwiceAsserts, "REQ-FramDriver-012")
FRAM_TEST(NullBufferRejected, "REQ-FramDriver-006")
FRAM_TEST(StartupDuringClientTransferAsserts, "REQ-FramDriver-012")
FRAM_TEST(ReentrantFromHandlerDrain, "REQ-FramDriver-010")
FRAM_TEST(RandomSequence, "REQ-FramDriver-001, 002, 004, 006, 007, 009, 010, 011, 017, 022")

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    STest::Random::seed();
    return RUN_ALL_TESTS();
}

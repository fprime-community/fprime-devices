// ======================================================================
// \title  FramDriverTestMain.cpp
// \brief  cpp file for FramDriver component test main function
// ======================================================================

#include "FramDriverTester.hpp"
#include "Fw/Test/UnitTest.hpp"
#include "STest/Random/Random.hpp"

TEST(Nominal, Read) {
    COMMENT("READ frame layout and data copy after the header; completion on the requesting index");
    REQUIREMENT("FRAM-001");
    REQUIREMENT("FRAM-003");
    REQUIREMENT("FRAM-005");
    REQUIREMENT("FRAM-009");
    Fram::FramDriverTester tester;
    tester.testReadNominal();
}

TEST(Nominal, Write) {
    COMMENT("WREN then WRITE in separate chip-select cycles; client data left untouched");
    REQUIREMENT("FRAM-002");
    REQUIREMENT("FRAM-003");
    REQUIREMENT("FRAM-006");
    Fram::FramDriverTester tester;
    tester.testWriteNominal();
}

TEST(Nominal, ReentrantRequest) {
    COMMENT("A client may issue its next request from inside the completion callback");
    REQUIREMENT("FRAM-009");
    Fram::FramDriverTester tester;
    tester.testReentrantRequest();
}

TEST(Nominal, RandomTransactions) {
    COMMENT("Random ports, directions, offsets, sizes and SPI faults; the driver always returns to idle");
    REQUIREMENT("FRAM-004");
    Fram::FramDriverTester tester;
    tester.testRandomTransactions();
}

TEST(OffNominal, Rejections) {
    COMMENT("Zero, oversize and out-of-range requests are refused synchronously without SPI traffic");
    REQUIREMENT("FRAM-007");
    Fram::FramDriverTester tester;
    tester.testRejections();
}

TEST(OffNominal, Busy) {
    COMMENT("A second request while one is in flight or awaiting delivery is BUSY");
    REQUIREMENT("FRAM-004");
    Fram::FramDriverTester tester;
    tester.testBusy();
}

TEST(OffNominal, SpiErrorRead) {
    COMMENT("SPI failure of a READ maps to SPI_ERROR and leaves the client buffer untouched");
    REQUIREMENT("FRAM-008");
    Fram::FramDriverTester tester;
    tester.testSpiErrorRead();
}

TEST(OffNominal, SpiErrorWren) {
    COMMENT("SPI failure of the WREN aborts the write before the WRITE frame");
    REQUIREMENT("FRAM-008");
    Fram::FramDriverTester tester;
    tester.testSpiErrorWren();
}

TEST(OffNominal, SpiErrorWrite) {
    COMMENT("SPI failure of the WRITE frame maps to SPI_ERROR");
    REQUIREMENT("FRAM-008");
    Fram::FramDriverTester tester;
    tester.testSpiErrorWrite();
}

TEST(OffNominal, SpiSynchronousReject) {
    COMMENT("The SPI driver refusing from inside the request call is an SPI_ERROR delivered on activeIn");
    REQUIREMENT("FRAM-008");
    Fram::FramDriverTester tester;
    tester.testSpiSynchronousReject();
}

TEST(OffNominal, UnexpectedSpiReply) {
    COMMENT("An SPI reply with nothing in flight is an internal error");
    REQUIREMENT("FRAM-010");
    Fram::FramDriverTester tester;
    tester.testUnexpectedSpiReply();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    STest::Random::seed();
    return RUN_ALL_TESTS();
}

// ======================================================================
// \title  FramDriverTester.cpp
// \brief  cpp file for FramDriver component test harness implementation class
// ======================================================================

#include "FramDriverTester.hpp"

#include <cstring>

#include "STest/Pick/Pick.hpp"

namespace Fram {

// C++14: ODR-used static constexpr members (the gtest macros take them by
// reference) need definitions
constexpr FwSizeType FramDriverTester::HEADER;
constexpr FwSizeType FramDriverTester::FRAME;
constexpr FwSizeType FramDriverTester::ID_SIZE;
constexpr FwSizeType FramDriverTester::MAX_FRAMES;
constexpr FwSizeType FramDriverTester::MAX_COMPLETIONS;
constexpr U32 FramDriverTester::SIZE;
constexpr FwSizeType FramDriverTester::MAX_TRANSFER;
constexpr FwIndexType FramDriverTester::PORTS;
constexpr U8 FramDriverTester::SR_BP0;
constexpr U8 FramDriverTester::SR_BP1;

//! RDID reply that matches the configured EXPECTED_DEVICE_ID
// Death-test site: FW_FILENAME_ASSERT prints "<path>/FramDriver.cpp:<line>" <arg>, FW_FILEID_ASSERT prints
// 0x<crc>:<line> <arg> (no quotes). Match either so the tests hold under any deployment's FW_ASSERT_LEVEL.
#define FRAM_ASSERT_SITE "(FramDriver\\.cpp|0x[0-9a-fA-F]+):[0-9]+\"?"

static const U8* const EXPECTED_ID = Fram::FramConfig::EXPECTED_DEVICE_ID;  // the deployment's configured
                                                                            // identity
//! RDID reply of another family member (-50SXE)
static const U8 OTHER_ID[FramDriverTester::ID_SIZE] = {  // byte 0 (sub type) flipped: another family member
    static_cast<U8>(Fram::FramConfig::EXPECTED_DEVICE_ID[0] ^ 0x60),
    Fram::FramConfig::EXPECTED_DEVICE_ID[1],
    Fram::FramConfig::EXPECTED_DEVICE_ID[2],
    Fram::FramConfig::EXPECTED_DEVICE_ID[3],
    Fram::FramConfig::EXPECTED_DEVICE_ID[4],
    Fram::FramConfig::EXPECTED_DEVICE_ID[5],
    Fram::FramConfig::EXPECTED_DEVICE_ID[6],
    Fram::FramConfig::EXPECTED_DEVICE_ID[7],
    Fram::FramConfig::EXPECTED_DEVICE_ID[8]};

// ----------------------------------------------------------------------
// Construction and destruction
// ----------------------------------------------------------------------

FramDriverTester ::FramDriverTester()
    : FramDriverGTestBase("FramDriverTester", FramDriverTester::MAX_HISTORY_SIZE),
      component("FramDriver"),
      m_spiMode(SpiMode::HOLD),
      m_spiStatus(Drv::SpiStatus::SPI_OK),
      m_reply(),
      m_replyLength(0),
      m_reply2(),
      m_reply2Length(0),
      m_reply2Set(false),
      m_seq(),
      m_seqCount(0),
      m_seqIndex(0),
      m_heldWrite(),
      m_heldRead(),
      m_held(false),
      m_frames(),
      m_frameCount(0),
      m_completions(),
      m_completionCount(0),
      m_reentrantRequest(false),
      m_reentrantStatus(Fram::Status::OK),
      m_reentrantData(),
      m_reentrantBuffer(m_reentrantData, 8),
      m_data(),
      m_big() {
    this->initComponents();
    this->connectPorts();
}

FramDriverTester ::~FramDriverTester() {
    this->component.deinit();
}

// ----------------------------------------------------------------------
// Tests
// ----------------------------------------------------------------------

void FramDriverTester ::ReadNominal() {
    this->runStartup(EXPECTED_ID, 0);
    const FwSizeType n = 16;
    const U32 offset = 0x012345;
    Fw::Buffer buffer(this->m_data[0], n);
    this->setReadReply(n, 0xA5);
    ASSERT_EQ(this->requestRead(0, offset, buffer), Fram::Status::ACCEPTED);
    this->assertFrames(1);
    this->assertFrame(0, OP_READ, offset, HEADER + n);
    this->assertCompletions(0);
    this->completeSpi(Drv::SpiStatus::SPI_OK);
    this->assertCompletions(1);
    this->assertCompletion(0, Fram::Op::READ, 0, offset, buffer, Fram::Status::OK);
    ASSERT_EQ(::memcmp(this->m_data[0], &this->m_reply[HEADER], n), 0);
    ASSERT_TLM_ReadsCompleted_SIZE(1);
    ASSERT_TLM_ReadsCompleted(0, 1);
    ASSERT_TLM_WritesCompleted_SIZE(0);
    ASSERT_EVENTS_SIZE(0);
}

void FramDriverTester ::WriteNominal() {
    this->runStartup(EXPECTED_ID, 0);
    const FwSizeType n = 32;
    const U32 offset = 0x03FFC0;
    FramDriverTester::fill(this->m_data[1], n, 0x11);
    Fw::Buffer buffer(this->m_data[1], n);
    ASSERT_EQ(this->requestWrite(1, offset, buffer), Fram::Status::ACCEPTED);
    this->assertFrames(1);
    this->assertFrame(0, OP_WREN, -1, 1);
    this->completeSpi(Drv::SpiStatus::SPI_OK);
    this->assertCompletions(0);
    this->assertFrames(2);
    this->assertFrame(1, OP_WRITE, offset, HEADER + n);
    this->assertFrameData(1, this->m_data[1], n);
    this->completeSpi(Drv::SpiStatus::SPI_OK);
    this->assertCompletions(1);
    this->assertCompletion(0, Fram::Op::WRITE, 1, offset, buffer, Fram::Status::OK);
    ASSERT_TLM_WritesCompleted_SIZE(1);
    ASSERT_TLM_WritesCompleted(0, 1);
    ASSERT_TLM_ReadsCompleted_SIZE(0);
    ASSERT_EVENTS_SIZE(0);
}

void FramDriverTester ::CompletionPortIndex() {
    this->runStartup(EXPECTED_ID, 0);
    for (FwIndexType port = 0; port < PORTS; port++) {
        this->clearAll();
        Fw::Buffer buffer(this->m_data[port], 4);
        this->setReadReply(4, static_cast<U8>(port));
        ASSERT_EQ(this->requestRead(port, 16, buffer), Fram::Status::ACCEPTED);
        this->completeSpi(Drv::SpiStatus::SPI_OK);
        this->assertCompletions(1);
        this->assertCompletion(0, Fram::Op::READ, port, 16, buffer, Fram::Status::OK);

        ASSERT_EQ(this->requestWrite(port, 32, buffer), Fram::Status::ACCEPTED);
        this->completeWrite();
        this->assertCompletions(2);
        this->assertCompletion(1, Fram::Op::WRITE, port, 32, buffer, Fram::Status::OK);
    }
}

void FramDriverTester ::CompletionBufferIdentity() {
    this->runStartup(EXPECTED_ID, 0);
    const FwSizeType n = STest::Pick::lowerUpper(1, MAX_TRANSFER);
    Fw::Buffer buffer(&this->m_data[0][MAX_TRANSFER - n], n, 7);
    this->setReadReply(n, 0x3C);
    ASSERT_EQ(this->requestRead(0, 0, buffer), Fram::Status::ACCEPTED);
    this->completeSpi(Drv::SpiStatus::SPI_OK);
    this->assertCompletions(1);
    ASSERT_EQ(this->m_completions[0].data, &this->m_data[0][MAX_TRANSFER - n]);
    ASSERT_EQ(this->m_completions[0].size, n);
    ASSERT_EQ(this->m_completions[0].context, 7u);
    // The client's buffer never reaches the SPI driver
    ASSERT_NE(this->m_heldWrite.getData(), buffer.getData());
    ASSERT_NE(this->m_heldRead.getData(), buffer.getData());
}

void FramDriverTester ::AcceptedBeforeComplete() {
    this->runStartup(EXPECTED_ID, 0);
    Fw::Buffer buffer(this->m_data[0], 8);
    ASSERT_EQ(this->requestRead(0, 100, buffer), Fram::Status::ACCEPTED);
    this->assertFrames(1);
    this->assertCompletions(0);
    ASSERT_TRUE(this->m_held);
    this->setReadReply(8, 1);
    this->completeSpi(Drv::SpiStatus::SPI_OK);
    this->assertCompletions(1);
}

void FramDriverTester ::OutOfRangeLimits() {
    this->runStartup(EXPECTED_ID, 0);
    this->clearAll();
    Fw::Buffer zero(this->m_data[0], 0);
    ASSERT_EQ(this->requestRead(0, 0, zero), Fram::Status::OUT_OF_RANGE);
    this->assertRejected(0, Fram::Op::READ, 0, 0, 0, Fram::Status::OUT_OF_RANGE, Fram::RejectCause::OUT_OF_RANGE);

    U8 big[MAX_TRANSFER + 1] = {};
    Fw::Buffer tooBig(big, MAX_TRANSFER + 1);
    ASSERT_EQ(this->requestWrite(1, 0, tooBig), Fram::Status::OUT_OF_RANGE);
    this->assertRejected(1, Fram::Op::WRITE, 1, 0, MAX_TRANSFER + 1, Fram::Status::OUT_OF_RANGE,
                         Fram::RejectCause::OUT_OF_RANGE);

    Fw::Buffer eight(this->m_data[0], 8);
    ASSERT_EQ(this->requestRead(0, SIZE - 8 + 1, eight), Fram::Status::OUT_OF_RANGE);
    this->assertRejected(2, Fram::Op::READ, 0, SIZE - 8 + 1, 8, Fram::Status::OUT_OF_RANGE,
                         Fram::RejectCause::OUT_OF_RANGE);
    ASSERT_EQ(this->requestWrite(0, 0xFFFFFFFFu, eight), Fram::Status::OUT_OF_RANGE);
    this->assertRejected(3, Fram::Op::WRITE, 0, 0xFFFFFFFFu, 8, Fram::Status::OUT_OF_RANGE,
                         Fram::RejectCause::OUT_OF_RANGE);
    ASSERT_EQ(this->requestRead(0, 0xFFFFFFF8u, eight), Fram::Status::OUT_OF_RANGE);

    this->assertFrames(0);
    this->assertCompletions(0);
    ASSERT_EVENTS_RequestRejected_SIZE(5);
    ASSERT_TLM_RequestsRejected_SIZE(5);
    ASSERT_TLM_RequestsRejected(4, 5);

    // Last valid byte is in range
    Fw::Buffer one(this->m_data[0], 1);
    this->setReadReply(1, 0);
    ASSERT_EQ(this->requestRead(0, SIZE - 1, one), Fram::Status::ACCEPTED);
    this->assertFrame(0, OP_READ, SIZE - 1, HEADER + 1);
    this->completeSpi(Drv::SpiStatus::SPI_OK);
    this->assertCompletions(1);
}

void FramDriverTester ::BusyWhileTransfer() {
    // During a startup read
    this->component.startup();
    this->assertFrames(1);
    Fw::Buffer buffer(this->m_data[0], 8);
    ASSERT_EQ(this->requestRead(0, 0, buffer), Fram::Status::BUSY);
    this->assertRejected(0, Fram::Op::READ, 0, 0, 8, Fram::Status::BUSY, Fram::RejectCause::TRANSFER_IN_PROGRESS);
    this->setRdidReply(EXPECTED_ID);
    this->completeSpi(Drv::SpiStatus::SPI_OK);
    ASSERT_EQ(this->requestWrite(1, 0, buffer), Fram::Status::BUSY);
    this->assertRejected(1, Fram::Op::WRITE, 1, 0, 8, Fram::Status::BUSY, Fram::RejectCause::TRANSFER_IN_PROGRESS);
    U8 rdsr[2] = {0, 0};
    this->setReply(rdsr, sizeof(rdsr));
    this->completeSpi(Drv::SpiStatus::SPI_OK);
    ASSERT_EVENTS_StartupComplete_SIZE(1);
    this->clearAll();

    // During a client transfer
    ASSERT_EQ(this->requestRead(0, 0, buffer), Fram::Status::ACCEPTED);
    Fw::Buffer other(this->m_data[1], 8);
    ASSERT_EQ(this->requestRead(1, 8, other), Fram::Status::BUSY);
    ASSERT_EQ(this->requestWrite(1, 8, other), Fram::Status::BUSY);
    this->assertRejected(0, Fram::Op::READ, 1, 8, 8, Fram::Status::BUSY, Fram::RejectCause::TRANSFER_IN_PROGRESS);
    this->assertRejected(1, Fram::Op::WRITE, 1, 8, 8, Fram::Status::BUSY, Fram::RejectCause::TRANSFER_IN_PROGRESS);
    this->assertFrames(1);
    this->setReadReply(8, 2);
    this->completeSpi(Drv::SpiStatus::SPI_OK);
    this->assertCompletions(1);
    this->assertCompletion(0, Fram::Op::READ, 0, 0, buffer, Fram::Status::OK);
    ASSERT_TLM_RequestsRejected_SIZE(2);
    ASSERT_TLM_RequestsRejected(1, 4);
}

void FramDriverTester ::BusyBeforeOutOfRange() {
    this->runStartup(EXPECTED_ID, 0);
    Fw::Buffer buffer(this->m_data[0], 8);
    ASSERT_EQ(this->requestRead(0, 0, buffer), Fram::Status::ACCEPTED);
    Fw::Buffer zero(this->m_data[1], 0);
    ASSERT_EQ(this->requestRead(1, SIZE, zero), Fram::Status::BUSY);
    this->assertRejected(0, Fram::Op::READ, 1, SIZE, 0, Fram::Status::BUSY, Fram::RejectCause::TRANSFER_IN_PROGRESS);
}

void FramDriverTester ::SpiErrorAfterReturn() {
    this->runStartup(EXPECTED_ID, 0);
    Fw::Buffer buffer(this->m_data[0], 8);
    ASSERT_EQ(this->requestRead(0, 40, buffer), Fram::Status::ACCEPTED);
    this->completeSpi(Drv::SpiStatus::SPI_OTHER_ERR);
    this->assertFrames(1);
    this->assertCompletions(1);
    this->assertCompletion(0, Fram::Op::READ, 0, 40, buffer, Fram::Status::SPI_ERROR);
    this->assertSpiError(0, Fram::Op::READ, 0, 40, 8, Drv::SpiStatus::SPI_OTHER_ERR, Fram::SpiErrorSite::READ_FRAME);
    ASSERT_TLM_SpiErrors_SIZE(1);
    ASSERT_TLM_SpiErrors(0, 1);
    ASSERT_TLM_ReadsCompleted_SIZE(0);

    // Write: error on the WREN frame ends the transfer, no WRITE frame
    this->clearAll();
    ASSERT_EQ(this->requestWrite(1, 48, buffer), Fram::Status::ACCEPTED);
    this->completeSpi(Drv::SpiStatus::SPI_WRITE_ERR);
    this->assertFrames(1);
    this->assertCompletions(1);
    this->assertCompletion(0, Fram::Op::WRITE, 1, 48, buffer, Fram::Status::SPI_ERROR);
    this->assertSpiError(0, Fram::Op::WRITE, 1, 48, 8, Drv::SpiStatus::SPI_WRITE_ERR, Fram::SpiErrorSite::WREN_FRAME);

    // Write: error on the WRITE frame
    this->clearAll();
    ASSERT_EQ(this->requestWrite(1, 56, buffer), Fram::Status::ACCEPTED);
    this->completeSpi(Drv::SpiStatus::SPI_OK);
    this->completeSpi(Drv::SpiStatus::SPI_MISMATCH_ERR);
    this->assertFrames(2);
    this->assertCompletions(1);
    this->assertCompletion(0, Fram::Op::WRITE, 1, 56, buffer, Fram::Status::SPI_ERROR);
    ASSERT_TLM_SpiErrors(0, 3);
    ASSERT_TLM_WritesCompleted_SIZE(0);

    // The component is idle again
    this->clearAll();
    this->setReadReply(8, 9);
    ASSERT_EQ(this->requestRead(0, 0, buffer), Fram::Status::ACCEPTED);
    this->completeSpi(Drv::SpiStatus::SPI_OK);
    this->assertCompletion(0, Fram::Op::READ, 0, 0, buffer, Fram::Status::OK);
}

void FramDriverTester ::InCallRefusalBusy() {
    this->runStartup(EXPECTED_ID, 0);
    this->setSpi(SpiMode::IN_CALL, Drv::SpiStatus::SPI_BUSY_ERR);
    Fw::Buffer buffer(this->m_data[0], 8);
    ASSERT_EQ(this->requestRead(0, 8, buffer), Fram::Status::BUSY);
    this->assertFrames(1);
    this->assertCompletions(0);
    this->assertRejected(0, Fram::Op::READ, 0, 8, 8, Fram::Status::BUSY, Fram::RejectCause::SPI_DRIVER_BUSY);
    ASSERT_EVENTS_SpiError_SIZE(0);
    ASSERT_TLM_RequestsRejected(0, 1);
    ASSERT_TLM_SpiErrors_SIZE(0);
    ASSERT_EQ(this->requestWrite(1, 8, buffer), Fram::Status::BUSY);
    this->assertRejected(1, Fram::Op::WRITE, 1, 8, 8, Fram::Status::BUSY, Fram::RejectCause::SPI_DRIVER_BUSY);
    // Idle again: the next request is taken
    this->setSpi(SpiMode::HOLD, Drv::SpiStatus::SPI_OK);
    ASSERT_EQ(this->requestRead(0, 8, buffer), Fram::Status::ACCEPTED);
}

void FramDriverTester ::InCallRefusalError() {
    this->runStartup(EXPECTED_ID, 0);
    this->setSpi(SpiMode::IN_CALL, Drv::SpiStatus::SPI_CONFIG_ERR);
    Fw::Buffer buffer(this->m_data[0], 8);
    ASSERT_EQ(this->requestRead(0, 8, buffer), Fram::Status::SPI_ERROR);
    this->assertFrames(1);
    this->assertCompletions(0);
    this->assertSpiError(0, Fram::Op::READ, 0, 8, 8, Drv::SpiStatus::SPI_CONFIG_ERR,
                         Fram::SpiErrorSite::REQUEST_REFUSED);
    ASSERT_EVENTS_RequestRejected_SIZE(0);
    ASSERT_TLM_SpiErrors(0, 1);
    ASSERT_TLM_RequestsRejected_SIZE(0);
    ASSERT_EQ(this->requestWrite(1, 8, buffer), Fram::Status::SPI_ERROR);
    this->assertSpiError(1, Fram::Op::WRITE, 1, 8, 8, Drv::SpiStatus::SPI_CONFIG_ERR,
                         Fram::SpiErrorSite::REQUEST_REFUSED);
    this->setSpi(SpiMode::HOLD, Drv::SpiStatus::SPI_OK);
    ASSERT_EQ(this->requestRead(0, 8, buffer), Fram::Status::ACCEPTED);
}

void FramDriverTester ::InCallRefusalMidTransfer() {
    this->runStartup(EXPECTED_ID, 0);
    Fw::Buffer buffer(this->m_data[0], 8);
    ASSERT_EQ(this->requestWrite(0, 64, buffer), Fram::Status::ACCEPTED);
    this->assertFrames(1);
    // The WRITE frame issued from the WREN completion is refused inside the call
    this->setSpi(SpiMode::IN_CALL, Drv::SpiStatus::SPI_BUSY_ERR);
    this->completeSpi(Drv::SpiStatus::SPI_OK);
    this->assertFrames(2);
    this->assertFrame(1, OP_WRITE, 64, HEADER + 8);
    this->assertCompletions(1);
    this->assertCompletion(0, Fram::Op::WRITE, 0, 64, buffer, Fram::Status::SPI_ERROR);
    this->assertSpiError(0, Fram::Op::WRITE, 0, 64, 8, Drv::SpiStatus::SPI_BUSY_ERR, Fram::SpiErrorSite::WRITE_FRAME);
    ASSERT_EVENTS_RequestRejected_SIZE(0);
    this->setSpi(SpiMode::HOLD, Drv::SpiStatus::SPI_OK);
    ASSERT_EQ(this->requestRead(0, 8, buffer), Fram::Status::ACCEPTED);
}

void FramDriverTester ::RequestInsideCompletion() {
    this->runStartup(EXPECTED_ID, 0);
    Fw::Buffer buffer(this->m_data[0], 8);
    this->setReadReply(8, 4);
    this->m_reentrantRequest = true;
    ASSERT_EQ(this->requestRead(0, 0, buffer), Fram::Status::ACCEPTED);
    this->completeSpi(Drv::SpiStatus::SPI_OK);
    ASSERT_EQ(this->m_reentrantStatus, Fram::Status::ACCEPTED);
    this->assertFrames(2);
    this->assertFrame(1, OP_READ, 0x1000, HEADER + 8);
    this->assertCompletions(1);
    ASSERT_EVENTS_RequestRejected_SIZE(0);
    this->completeSpi(Drv::SpiStatus::SPI_OK);
    this->assertCompletions(2);
    this->assertCompletion(1, Fram::Op::READ, 1, 0x1000, this->m_reentrantBuffer, Fram::Status::OK);

    // Also from a write completion, and with the completion delivered inside the
    // request call
    this->clearAll();
    this->m_reentrantRequest = true;
    this->setSpi(SpiMode::IN_CALL, Drv::SpiStatus::SPI_OK);
    ASSERT_EQ(this->requestWrite(0, 0, buffer), Fram::Status::ACCEPTED);
    // The completion was delivered inside the request call: the re-entrant
    // request is rejected BUSY so that the recursion is bounded
    // (REENTRANT_IN_CALL)
    ASSERT_EQ(this->m_reentrantStatus, Fram::Status::BUSY);
    this->assertFrames(2);  // WREN, WRITE only
    this->assertCompletions(1);
    this->assertCompletion(0, Fram::Op::WRITE, 0, 0, buffer, Fram::Status::OK);
    ASSERT_EVENTS_RequestRejected_SIZE(1);
    this->assertRejected(0, Fram::Op::READ, 1, 0x1000, 8, Fram::Status::BUSY, Fram::RejectCause::REENTRANT_IN_CALL);
    // Once the call has returned the client may request again
    this->setSpi(SpiMode::HOLD, Drv::SpiStatus::SPI_OK);
    ASSERT_EQ(this->requestRead(1, 0x1000, this->m_reentrantBuffer), Fram::Status::ACCEPTED);
    this->assertFrames(3);
    this->assertFrame(2, OP_READ, 0x1000, HEADER + 8);
}

void FramDriverTester ::RequestAfterCompletion() {
    this->runStartup(EXPECTED_ID, 0);
    Fw::Buffer buffer(this->m_data[0], 8);
    this->setReadReply(8, 4);
    ASSERT_EQ(this->requestRead(0, 0, buffer), Fram::Status::ACCEPTED);
    this->completeSpi(Drv::SpiStatus::SPI_OK);
    ASSERT_EQ(this->requestWrite(1, 0, buffer), Fram::Status::ACCEPTED);
    this->completeWrite();
    ASSERT_EQ(this->requestRead(1, 0, buffer), Fram::Status::ACCEPTED);
    this->assertCompletions(2);
    ASSERT_EVENTS_RequestRejected_SIZE(0);
}

void FramDriverTester ::StartupReads() {
    // Before startup the device checks are not run and requests are still served
    // (deployment's responsibility)
    ASSERT_TLM_StartupState_SIZE(0);
    this->component.startup();
    ASSERT_TLM_StartupState_SIZE(1);
    ASSERT_TLM_StartupState(0, Fram::StartupState::IN_PROGRESS);
    ASSERT_TLM_DeviceIdMatch_SIZE(1);
    ASSERT_TLM_DeviceIdMatch(0, false);
    ASSERT_TLM_StatusRegister_SIZE(1);
    ASSERT_TLM_StatusRegister(0, 0);
    this->assertFrames(1);
    this->assertFrame(0, OP_RDID, -1, 1 + ID_SIZE);
    this->setRdidReply(EXPECTED_ID);
    this->completeSpi(Drv::SpiStatus::SPI_OK);
    ASSERT_TLM_DeviceIdMatch_SIZE(2);
    ASSERT_TLM_DeviceIdMatch(1, true);
    ASSERT_EVENTS_StartupComplete_SIZE(0);
    this->assertFrames(2);
    this->assertFrame(1, OP_RDSR, -1, 2);
    U8 rdsr[2] = {0, 0x40};
    this->setReply(rdsr, sizeof(rdsr));
    this->completeSpi(Drv::SpiStatus::SPI_OK);
    ASSERT_TLM_StatusRegister_SIZE(2);
    ASSERT_TLM_StatusRegister(1, 0x40);
    ASSERT_TLM_StartupState_SIZE(2);
    ASSERT_TLM_StartupState(1, Fram::StartupState::COMPLETE);
    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_StartupComplete_SIZE(1);
    this->assertFrames(2);
}

void FramDriverTester ::StartupIdMismatch() {
    this->runStartup(OTHER_ID, 0, false);
    ASSERT_EVENTS_DeviceIdMismatch_SIZE(1);
    ASSERT_EVENTS_DeviceIdMismatch(0, Fram::DeviceId(OTHER_ID));
    ASSERT_TLM_DeviceIdMatch(1, false);
    ASSERT_TLM_StartupState(1, Fram::StartupState::COMPLETE);
    ASSERT_EVENTS_StartupComplete_SIZE(1);
    Fw::Buffer buffer(this->m_data[0], 8);
    ASSERT_EQ(this->requestRead(0, 0, buffer), Fram::Status::ACCEPTED);
}

void FramDriverTester ::StartupReadFailed() {
    // RDID fails: RDSR still runs, startup ends READ_FAILED
    this->component.startup();
    this->completeSpi(Drv::SpiStatus::SPI_OTHER_ERR);
    ASSERT_EVENTS_StartupReadFailed_SIZE(1);
    ASSERT_EVENTS_StartupReadFailed(0, Fram::StartupRead::DEVICE_ID, Drv::SpiStatus::SPI_OTHER_ERR);
    ASSERT_TLM_DeviceIdMatch_SIZE(1);  // not updated by the failed read
    this->assertFrames(2);
    this->assertFrame(1, OP_RDSR, -1, 2);
    U8 rdsr[2] = {0, SR_BP0};
    this->setReply(rdsr, sizeof(rdsr));
    this->completeSpi(Drv::SpiStatus::SPI_OK);
    ASSERT_TLM_StatusRegister(1, SR_BP0);
    ASSERT_TLM_StartupState(1, Fram::StartupState::READ_FAILED);
    ASSERT_EVENTS_StartupComplete_SIZE(1);
    ASSERT_EVENTS_BlockProtectSet_SIZE(1);
    // The status register was read: its protection applies
    Fw::Buffer buffer(this->m_data[0], 8);
    ASSERT_EQ(this->requestWrite(0, SIZE - 8, buffer), Fram::Status::WRITE_PROTECTED);

    // RDID OK, RDSR fails: startup still completes, READ_FAILED,
    // StartupReadFailed names the status register
    FramDriverTester second;
    second.setRdidReply(EXPECTED_ID);
    second.component.startup();
    second.completeSpi(Drv::SpiStatus::SPI_OK);
    second.assertFrames(2);
    second.completeSpi(Drv::SpiStatus::SPI_OTHER_ERR);
    ASSERT_EQ(second.eventHistory_StartupReadFailed->size(), 1u);
    ASSERT_EQ(second.eventHistory_StartupReadFailed->at(0).read, Fram::StartupRead::STATUS_REGISTER);
    ASSERT_EQ(second.eventHistory_StartupReadFailed->at(0).status, Drv::SpiStatus::SPI_OTHER_ERR);
    ASSERT_EQ(second.tlmHistory_DeviceIdMatch->at(1).arg, true);
    ASSERT_EQ(second.tlmHistory_StartupState->at(1).arg, Fram::StartupState::READ_FAILED);
    ASSERT_EQ(second.tlmHistory_StatusRegister->size(),
              1u);  // not updated by the failed read
    ASSERT_EQ(second.eventsSize_StartupComplete, 1u);
    ASSERT_EQ(second.eventHistory_BlockProtectSet->size(), 0u);
    // Protection unknown: treated as empty
    Fw::Buffer last(second.m_data[0], 8);
    ASSERT_EQ(second.requestWrite(0, SIZE - 8, last), Fram::Status::ACCEPTED);
}

void FramDriverTester ::StartupInCallRefusal() {
    this->setSpi(SpiMode::IN_CALL, Drv::SpiStatus::SPI_BUSY_ERR);
    this->component.startup();
    // Both reads were refused inside startup(); it finished before returning
    this->assertFrames(2);
    this->assertFrame(0, OP_RDID, -1, 1 + ID_SIZE);
    this->assertFrame(1, OP_RDSR, -1, 2);
    ASSERT_EVENTS_StartupReadFailed_SIZE(2);
    ASSERT_EVENTS_StartupReadFailed(0, Fram::StartupRead::DEVICE_ID, Drv::SpiStatus::SPI_BUSY_ERR);
    ASSERT_EVENTS_StartupReadFailed(1, Fram::StartupRead::STATUS_REGISTER, Drv::SpiStatus::SPI_BUSY_ERR);
    ASSERT_EVENTS_StartupComplete_SIZE(1);
    ASSERT_TLM_StartupState(1, Fram::StartupState::READ_FAILED);
    ASSERT_TLM_StatusRegister_SIZE(1);  // only the initial 0
    this->setSpi(SpiMode::HOLD, Drv::SpiStatus::SPI_OK);
    Fw::Buffer buffer(this->m_data[0], 8);
    ASSERT_EQ(this->requestRead(0, 0, buffer), Fram::Status::ACCEPTED);

    // RDSR failing after the second read is completed in the call with SPI_OK is
    // also startup-complete
    FramDriverTester second;
    second.setSpi(SpiMode::IN_CALL, Drv::SpiStatus::SPI_OK);
    second.setRdidReply(EXPECTED_ID);
    const U8 rdsr[2] = {0, SR_BP0};
    second.queueReply(rdsr, sizeof(rdsr));
    second.component.startup();
    second.assertFrames(2);
    ASSERT_EQ(second.tlmHistory_StartupState->at(1).arg, Fram::StartupState::COMPLETE);
    ASSERT_EQ(second.tlmHistory_DeviceIdMatch->at(1).arg, true);
    ASSERT_EQ(second.tlmHistory_StatusRegister->at(1).arg, SR_BP0);
    ASSERT_EQ(second.eventHistory_DeviceIdMismatch->size(), 0u);
    ASSERT_EQ(second.eventHistory_StartupReadFailed->size(), 0u);
    ASSERT_EQ(second.eventHistory_BlockProtectSet->size(), 1u);
    ASSERT_EQ(second.eventsSize_StartupComplete, 1u);
}

void FramDriverTester ::BlockProtectWarning() {
    const U8 values[3] = {SR_BP0, SR_BP1, static_cast<U8>(SR_BP0 | SR_BP1)};
    for (FwSizeType i = 0; i < 3; i++) {
        FramDriverTester tester;
        tester.runStartup(EXPECTED_ID, static_cast<U8>(values[i] | 0x02),
                          false);  // WEL set too, irrelevant
        ASSERT_EQ(tester.eventHistory_BlockProtectSet->size(), 1u);
        ASSERT_EQ(tester.eventHistory_BlockProtectSet->at(0).statusRegister, static_cast<U8>(values[i] | 0x02));
        ASSERT_EQ(tester.tlmHistory_StatusRegister->at(1).arg, static_cast<U8>(values[i] | 0x02));
    }
    this->runStartup(EXPECTED_ID, 0x02, false);
    ASSERT_EVENTS_BlockProtectSet_SIZE(0);
}

void FramDriverTester ::WriteProtectedRange() {
    struct Case {
        U8 sr;
        U32 start;  //!< first protected address
    };
    const Case cases[3] = {{SR_BP0, SIZE - SIZE / 4}, {SR_BP1, SIZE - SIZE / 2}, {static_cast<U8>(SR_BP0 | SR_BP1), 0}};
    for (FwSizeType i = 0; i < 3; i++) {
        FramDriverTester tester;
        tester.runStartup(EXPECTED_ID, cases[i].sr);
        Fw::Buffer two(tester.m_data[0], 2);
        Fw::Buffer eight(tester.m_data[1], 8);
        // Overlapping writes are rejected
        ASSERT_EQ(tester.requestWrite(0, cases[i].start, two), Fram::Status::WRITE_PROTECTED);
        ASSERT_EQ(tester.requestWrite(1, SIZE - 8, eight), Fram::Status::WRITE_PROTECTED);
        if (cases[i].start > 0) {
            ASSERT_EQ(tester.requestWrite(0, cases[i].start - 1, two), Fram::Status::WRITE_PROTECTED);
            tester.assertRejected(2, Fram::Op::WRITE, 0, cases[i].start - 1, 2, Fram::Status::WRITE_PROTECTED,
                                  Fram::RejectCause::WRITE_PROTECTED);
            // A write ending at the range start passes
            ASSERT_EQ(tester.requestWrite(0, cases[i].start - 2, two), Fram::Status::ACCEPTED);
            tester.completeWrite();
            ASSERT_EQ(tester.requestWrite(1, 0, eight), Fram::Status::ACCEPTED);
            tester.completeWrite();
        }
        // Reads of the protected range pass
        tester.setReadReply(8, 1);
        ASSERT_EQ(tester.requestRead(1, SIZE - 8, eight), Fram::Status::ACCEPTED);
        tester.completeSpi(Drv::SpiStatus::SPI_OK);
        ASSERT_EQ(tester.m_completions[tester.m_completionCount - 1].status, Fram::Status::OK);
    }
}

void FramDriverTester ::WriteProtectedUnknownRange() {
    // RDSR failed: range unknown, treated as empty
    this->component.startup();
    this->setRdidReply(EXPECTED_ID);
    this->completeSpi(Drv::SpiStatus::SPI_OK);
    this->completeSpi(Drv::SpiStatus::SPI_OTHER_ERR);
    ASSERT_TLM_StartupState(1, Fram::StartupState::READ_FAILED);
    ASSERT_TLM_StatusRegister_SIZE(1);
    Fw::Buffer eight(this->m_data[0], 8);
    ASSERT_EQ(this->requestWrite(0, SIZE - 8, eight), Fram::Status::ACCEPTED);
    this->completeWrite();
    this->assertCompletion(0, Fram::Op::WRITE, 0, SIZE - 8, eight, Fram::Status::OK);
    ASSERT_EVENTS_RequestRejected_SIZE(0);

    // startup() never called: same
    FramDriverTester fresh;
    Fw::Buffer other(fresh.m_data[0], 8);
    ASSERT_EQ(fresh.requestWrite(0, 0, other), Fram::Status::ACCEPTED);
}

void FramDriverTester ::TelemetryCounters() {
    this->runStartup(EXPECTED_ID, 0);
    Fw::Buffer eight(this->m_data[0], 8);
    Fw::Buffer zero(this->m_data[1], 0);
    // Each outcome counts once
    this->setReadReply(8, 1);
    ASSERT_EQ(this->requestRead(0, 0, eight), Fram::Status::ACCEPTED);
    this->completeSpi(Drv::SpiStatus::SPI_OK);
    ASSERT_EQ(this->requestWrite(0, 0, eight), Fram::Status::ACCEPTED);
    this->completeWrite();
    ASSERT_EQ(this->requestRead(0, 0, zero), Fram::Status::OUT_OF_RANGE);
    ASSERT_EQ(this->requestRead(0, 0, eight), Fram::Status::ACCEPTED);
    this->completeSpi(Drv::SpiStatus::SPI_OTHER_ERR);
    ASSERT_TLM_ReadsCompleted_SIZE(1);
    ASSERT_TLM_ReadsCompleted(0, 1);
    ASSERT_TLM_WritesCompleted_SIZE(1);
    ASSERT_TLM_WritesCompleted(0, 1);
    ASSERT_TLM_RequestsRejected_SIZE(1);
    ASSERT_TLM_RequestsRejected(0, 1);
    ASSERT_TLM_SpiErrors_SIZE(1);
    ASSERT_TLM_SpiErrors(0, 1);

    // Wrap modulo 2^32, no assert
    this->clearAll();
    this->component.m_readsCompleted = 0xFFFFFFFFu;
    this->component.m_writesCompleted = 0xFFFFFFFFu;
    this->component.m_requestsRejected = 0xFFFFFFFFu;
    this->component.m_spiErrors = 0xFFFFFFFFu;
    this->setReadReply(8, 1);
    ASSERT_EQ(this->requestRead(0, 0, eight), Fram::Status::ACCEPTED);
    this->completeSpi(Drv::SpiStatus::SPI_OK);
    ASSERT_EQ(this->requestWrite(0, 0, eight), Fram::Status::ACCEPTED);
    this->completeWrite();
    ASSERT_EQ(this->requestRead(0, 0, zero), Fram::Status::OUT_OF_RANGE);
    ASSERT_EQ(this->requestRead(0, 0, eight), Fram::Status::ACCEPTED);
    this->completeSpi(Drv::SpiStatus::SPI_OTHER_ERR);
    ASSERT_TLM_ReadsCompleted(0, 0);
    ASSERT_TLM_WritesCompleted(0, 0);
    ASSERT_TLM_RequestsRejected(0, 0);
    ASSERT_TLM_SpiErrors(0, 0);
}

void FramDriverTester ::EventsRequestRejected() {
    this->runStartup(EXPECTED_ID, SR_BP0);
    this->clearAll();
    Fw::Buffer eight(this->m_data[0], 8);
    Fw::Buffer zero(this->m_data[1], 0);
    ASSERT_EQ(this->requestRead(1, 0, zero), Fram::Status::OUT_OF_RANGE);
    this->assertRejected(0, Fram::Op::READ, 1, 0, 0, Fram::Status::OUT_OF_RANGE, Fram::RejectCause::OUT_OF_RANGE);
    ASSERT_EQ(this->requestWrite(0, SIZE - 8, eight), Fram::Status::WRITE_PROTECTED);
    this->assertRejected(1, Fram::Op::WRITE, 0, SIZE - 8, 8, Fram::Status::WRITE_PROTECTED,
                         Fram::RejectCause::WRITE_PROTECTED);
    ASSERT_EQ(this->requestRead(0, 16, eight), Fram::Status::ACCEPTED);
    ASSERT_EQ(this->requestWrite(1, 24, eight), Fram::Status::BUSY);
    this->assertRejected(2, Fram::Op::WRITE, 1, 24, 8, Fram::Status::BUSY, Fram::RejectCause::TRANSFER_IN_PROGRESS);
    // Throttle: 5 per boot
    for (FwSizeType i = 0; i < 4; i++) {
        ASSERT_EQ(this->requestWrite(1, 24, eight), Fram::Status::BUSY);
    }
    ASSERT_EVENTS_RequestRejected_SIZE(5);
    ASSERT_TLM_RequestsRejected(6, 7);
}

void FramDriverTester ::EventsSpiError() {
    this->runStartup(EXPECTED_ID, 0);
    Fw::Buffer eight(this->m_data[0], 8);
    FwSizeType completions = 0;
    for (FwSizeType i = 0; i < 7; i++) {
        this->m_frameCount = 0;  // frames are not under test here; keep the record within its bound
        ASSERT_EQ(this->requestWrite(1, static_cast<U32>(0x100 + i), eight), Fram::Status::ACCEPTED);
        this->completeSpi(Drv::SpiStatus::SPI_WRITE_ERR);
        completions++;
        this->assertCompletions(completions);
    }
    ASSERT_EVENTS_SpiError_SIZE(5);
    this->assertSpiError(0, Fram::Op::WRITE, 1, 0x100, 8, Drv::SpiStatus::SPI_WRITE_ERR,
                         Fram::SpiErrorSite::WREN_FRAME);
    this->assertSpiError(4, Fram::Op::WRITE, 1, 0x104, 8, Drv::SpiStatus::SPI_WRITE_ERR,
                         Fram::SpiErrorSite::WREN_FRAME);
    ASSERT_TLM_SpiErrors(6, 7);
    this->assertCompletions(7);
}

void FramDriverTester ::OutOfRangeBeforeWriteProtected() {
    this->runStartup(EXPECTED_ID, static_cast<U8>(SR_BP0 | SR_BP1));
    Fw::Buffer two(this->m_data[0], 2);
    ASSERT_EQ(this->requestWrite(0, SIZE - 1, two), Fram::Status::OUT_OF_RANGE);
    this->assertRejected(0, Fram::Op::WRITE, 0, SIZE - 1, 2, Fram::Status::OUT_OF_RANGE,
                         Fram::RejectCause::OUT_OF_RANGE);
    ASSERT_EQ(this->requestWrite(0, SIZE - 2, two), Fram::Status::WRITE_PROTECTED);
}

void FramDriverTester ::InCallOkCompletion() {
    this->runStartup(EXPECTED_ID, 0);
    this->setSpi(SpiMode::IN_CALL, Drv::SpiStatus::SPI_OK);
    Fw::Buffer eight(this->m_data[0], 8);
    this->setReadReply(8, 0x77);
    ASSERT_EQ(this->requestRead(0, 8, eight), Fram::Status::ACCEPTED);
    this->assertFrames(1);
    this->assertCompletions(1);
    this->assertCompletion(0, Fram::Op::READ, 0, 8, eight, Fram::Status::OK);
    ASSERT_EQ(::memcmp(this->m_data[0], &this->m_reply[HEADER], 8), 0);

    ASSERT_EQ(this->requestWrite(1, 16, eight), Fram::Status::ACCEPTED);
    this->assertFrames(3);
    this->assertFrame(1, OP_WREN, -1, 1);
    this->assertFrame(2, OP_WRITE, 16, HEADER + 8);
    this->assertCompletions(2);
    this->assertCompletion(1, Fram::Op::WRITE, 1, 16, eight, Fram::Status::OK);
    ASSERT_EVENTS_SIZE(0);
    ASSERT_TLM_ReadsCompleted(0, 1);
    ASSERT_TLM_WritesCompleted(0, 1);
}

void FramDriverTester ::RandomSequence() {
    // Shadow model: idle / busy with an outstanding request; checks every return
    // and completion
    this->runStartup(EXPECTED_ID, SR_BP0);
    const U32 protectStart = SIZE - SIZE / 4;
    const U32 THROTTLE = 5;  // RequestRejected / SpiError throttle per boot
    const Drv::SpiStatus errors[3] = {Drv::SpiStatus::SPI_OTHER_ERR, Drv::SpiStatus::SPI_BUSY_ERR,
                                      Drv::SpiStatus::SPI_CONFIG_ERR};
    bool busy = false;
    bool writePending = false;  // write: WREN done, WRITE frame outstanding
    Fram::Op op = Fram::Op::READ;
    FwIndexType port = 0;
    U32 offset = 0;
    FwSizeType length = 0;
    U32 reads = 0;
    U32 writes = 0;
    U32 rejected = 0;
    U32 spiErrors = 0;
    for (U32 step = 0; step < 2000; step++) {
        this->clearAll();
        const U32 action = STest::Pick::lowerUpper(0, 9);
        if (action < 5) {
            // A request
            const Fram::Op reqOp = (STest::Pick::lowerUpper(0, 1) == 0) ? Fram::Op::READ : Fram::Op::WRITE;
            const FwIndexType reqPort = static_cast<FwIndexType>(STest::Pick::lowerUpper(0, PORTS - 1));
            const FwSizeType n = STest::Pick::lowerUpper(0, MAX_TRANSFER + 1);
            const U32 off =
                (STest::Pick::lowerUpper(0, 3) == 0) ? STest::Pick::any() : STest::Pick::lowerUpper(0, SIZE);
            Fw::Buffer buffer((n > MAX_TRANSFER) ? this->m_big : this->m_data[reqPort], n);
            Fram::Status expected = Fram::Status::ACCEPTED;
            if (busy) {
                expected = Fram::Status::BUSY;
            } else if ((n == 0) || (n > MAX_TRANSFER) || (off > SIZE - n)) {
                expected = Fram::Status::OUT_OF_RANGE;
            } else if ((reqOp == Fram::Op::WRITE) && (off + n > protectStart)) {
                expected = Fram::Status::WRITE_PROTECTED;
            }
            if (!busy && (n <= MAX_TRANSFER)) {  // never overwrite the reply of a read
                                                 // still outstanding
                this->setReadReply(n, static_cast<U8>(step));
            }
            const Fram::Status got = (reqOp == Fram::Op::READ) ? this->requestRead(reqPort, off, buffer)
                                                               : this->requestWrite(reqPort, off, buffer);
            ASSERT_EQ(got, expected) << "step " << step;
            if (expected == Fram::Status::ACCEPTED) {
                busy = true;
                writePending = false;
                op = reqOp;
                port = reqPort;
                offset = off;
                length = n;
                this->assertFrames(1);
                this->assertFrame(0, (reqOp == Fram::Op::READ) ? OP_READ : OP_WREN,
                                  (reqOp == Fram::Op::READ) ? static_cast<I64>(off) : static_cast<I64>(-1),
                                  (reqOp == Fram::Op::READ) ? HEADER + n : 1);
            } else {
                rejected++;
                this->assertFrames(0);
                if (rejected <= THROTTLE) {
                    const Fram::RejectCause cause = (expected == Fram::Status::BUSY)
                                                        ? Fram::RejectCause(Fram::RejectCause::TRANSFER_IN_PROGRESS)
                                                        : ((expected == Fram::Status::OUT_OF_RANGE)
                                                               ? Fram::RejectCause(Fram::RejectCause::OUT_OF_RANGE)
                                                               : Fram::RejectCause(Fram::RejectCause::WRITE_PROTECTED));
                    this->assertRejected(0, reqOp, reqPort, off, n, expected, cause);
                } else {
                    ASSERT_EVENTS_RequestRejected_SIZE(0);
                }
                ASSERT_TLM_RequestsRejected(0, rejected);
            }
            this->assertCompletions(0);
        } else if (busy) {
            // Complete the outstanding frame, OK or with an error
            const bool ok = (STest::Pick::lowerUpper(0, 3) != 0);
            const Drv::SpiStatus status =
                ok ? Drv::SpiStatus(Drv::SpiStatus::SPI_OK) : errors[STest::Pick::lowerUpper(0, 2)];
            Fw::Buffer buffer(this->m_data[port], length);
            this->completeSpi(status);
            if (!ok) {
                spiErrors++;
                busy = false;
                this->assertCompletions(1);
                this->assertCompletion(0, op, port, offset, buffer, Fram::Status::SPI_ERROR);
                if (spiErrors <= THROTTLE) {
                    const Fram::SpiErrorSite site =
                        (op == Fram::Op::READ) ? Fram::SpiErrorSite(Fram::SpiErrorSite::READ_FRAME)
                                               : (writePending ? Fram::SpiErrorSite(Fram::SpiErrorSite::WRITE_FRAME)
                                                               : Fram::SpiErrorSite(Fram::SpiErrorSite::WREN_FRAME));
                    this->assertSpiError(0, op, port, offset, length, status, site);
                } else {
                    ASSERT_EVENTS_SpiError_SIZE(0);
                }
                ASSERT_TLM_SpiErrors(0, spiErrors);
            } else if ((op == Fram::Op::WRITE) && !writePending) {
                writePending = true;
                this->assertCompletions(0);
                this->assertFrames(1);
                this->assertFrame(0, OP_WRITE, offset, HEADER + length);
                this->assertFrameData(0, this->m_data[port], length);
            } else {
                busy = false;
                if (op == Fram::Op::READ) {
                    reads++;
                    ASSERT_TLM_ReadsCompleted(0, reads);
                    ASSERT_EQ(::memcmp(this->m_data[port], &this->m_reply[HEADER], length), 0);
                } else {
                    writes++;
                    ASSERT_TLM_WritesCompleted(0, writes);
                }
                this->assertCompletions(1);
                this->assertCompletion(0, op, port, offset, buffer, Fram::Status::OK);
            }
        }
    }
    ASSERT_GT(reads, 0u);
    ASSERT_GT(writes, 0u);
    ASSERT_GT(spiErrors, 0u);
    ASSERT_GT(rejected, 0u);
}

void FramDriverTester ::InCallOkThenRefusal() {
    this->runStartup(EXPECTED_ID, 0);
    // WREN completes OK inside the request call; the chained WRITE frame is
    // refused inside the same call
    const Drv::SpiStatus seq[2] = {Drv::SpiStatus::SPI_OK, Drv::SpiStatus::SPI_CONFIG_ERR};
    this->setSpi(SpiMode::IN_CALL, Drv::SpiStatus::SPI_OK);
    this->setSpiSequence(seq, 2);
    Fw::Buffer buffer(this->m_data[0], 8);
    fill(this->m_data[0], 8, 0x5A);
    ASSERT_EQ(this->requestWrite(0, 16, buffer), Fram::Status::ACCEPTED);
    // The client's SPI_ERROR completion was delivered before ACCEPTED was
    // returned (REQ-005 in-call exception)
    this->assertFrames(2);
    this->assertFrame(0, OP_WREN, -1, 1);
    this->assertFrame(1, OP_WRITE, 16, HEADER + 8);
    this->assertCompletions(1);
    this->assertCompletion(0, Fram::Op::WRITE, 0, 16, buffer, Fram::Status::SPI_ERROR);
    ASSERT_EVENTS_SpiError_SIZE(1);
    this->assertSpiError(0, Fram::Op::WRITE, 0, 16, 8, Drv::SpiStatus::SPI_CONFIG_ERR, Fram::SpiErrorSite::WRITE_FRAME);
    ASSERT_EVENTS_RequestRejected_SIZE(0);
    ASSERT_TLM_SpiErrors(0, 1);
    ASSERT_TLM_WritesCompleted_SIZE(0);
    // Idle again
    this->setSpi(SpiMode::HOLD, Drv::SpiStatus::SPI_OK);
    ASSERT_EQ(this->requestRead(1, 16, buffer), Fram::Status::ACCEPTED);
}

void FramDriverTester ::UnsolicitedCompletionAsserts() {
    this->runStartup(EXPECTED_ID, 0);
    // Nothing outstanding: a completion the SPI driver owes nobody breaks its
    // contract and is a programming error
    Fw::Buffer stray(this->m_data[0], 8);
    ASSERT_DEATH_IF_SUPPORTED(this->invoke_to_spiWriteReadComplete(0, stray, stray, Drv::SpiStatus::SPI_OK),
                              FRAM_ASSERT_SITE);
    this->assertFrames(0);
    this->assertCompletions(0);
    ASSERT_EVENTS_SIZE(0);
}

void FramDriverTester ::StartupTwiceAsserts() {
    this->runStartup(EXPECTED_ID, 0);
    ASSERT_DEATH_IF_SUPPORTED(this->component.startup(),
                              FRAM_ASSERT_SITE " 2");  // arg = COMPLETE (2)
    // While a startup read is outstanding, startup() is a programming error as
    // well
    FramDriverTester other;
    other.component.startup();
    ASSERT_DEATH_IF_SUPPORTED(other.component.startup(),
                              FRAM_ASSERT_SITE " 1");  // arg = IN_PROGRESS (1)
}

void FramDriverTester ::NullBufferRejected() {
    this->runStartup(EXPECTED_ID, 0);
    Fw::Buffer null(nullptr, 8);
    ASSERT_EQ(this->requestRead(0, 0, null), Fram::Status::OUT_OF_RANGE);
    this->assertRejected(0, Fram::Op::READ, 0, 0, 8, Fram::Status::OUT_OF_RANGE, Fram::RejectCause::OUT_OF_RANGE);
    ASSERT_EQ(this->requestWrite(1, 0, null), Fram::Status::OUT_OF_RANGE);
    this->assertRejected(1, Fram::Op::WRITE, 1, 0, 8, Fram::Status::OUT_OF_RANGE, Fram::RejectCause::OUT_OF_RANGE);
    this->assertFrames(0);
    this->assertCompletions(0);
    ASSERT_TLM_RequestsRejected(1, 2);
}

void FramDriverTester ::StartupDuringClientTransferAsserts() {
    // Requests are served before startup(); a startup() while one is outstanding is a programming error (phase READ =
    // 3)
    Fw::Buffer buffer(this->m_data[0], 8);
    ASSERT_EQ(this->requestRead(0, 0, buffer), Fram::Status::ACCEPTED);
    ASSERT_DEATH_IF_SUPPORTED(this->component.startup(), FRAM_ASSERT_SITE " 3");
}

void FramDriverTester ::ReentrantFromHandlerDrain() {
    // WREN completes from the driver's own entry (HOLD); the chained WRITE completes inside that handler call
    // (IN_CALL): the client's completion is delivered from the handler's drain() and its re-entrant request is rejected
    this->runStartup(EXPECTED_ID, 0);
    Fw::Buffer buffer(this->m_data[0], 8);
    ASSERT_EQ(this->requestWrite(0, 32, buffer), Fram::Status::ACCEPTED);
    this->assertFrames(1);
    this->m_reentrantRequest = true;
    this->setSpi(SpiMode::IN_CALL, Drv::SpiStatus::SPI_OK);
    this->completeSpi(Drv::SpiStatus::SPI_OK);
    this->assertFrames(2);
    this->assertCompletions(1);
    this->assertCompletion(0, Fram::Op::WRITE, 0, 32, buffer, Fram::Status::OK);
    ASSERT_EQ(this->m_reentrantStatus, Fram::Status::BUSY);
    this->assertRejected(0, Fram::Op::READ, 1, 0x1000, 8, Fram::Status::BUSY, Fram::RejectCause::REENTRANT_IN_CALL);
}

// ----------------------------------------------------------------------
// Handlers for typed from ports
// ----------------------------------------------------------------------

void FramDriverTester ::from_spiWriteRead_handler(FwIndexType portNum,
                                                  Fw::Buffer& writeBuffer,
                                                  Fw::Buffer& readBuffer) {
    ASSERT_EQ(portNum, 0);
    ASSERT_LT(this->m_frameCount, MAX_FRAMES);
    ASSERT_EQ(writeBuffer.getSize(), readBuffer.getSize());
    ASSERT_LE(writeBuffer.getSize(), FRAME);
    Frame& frame = this->m_frames[this->m_frameCount];
    this->m_frameCount++;
    frame.length = writeBuffer.getSize();
    frame.rxLength = readBuffer.getSize();
    (void)::memcpy(frame.tx, writeBuffer.getData(), frame.length);
    ASSERT_FALSE(this->m_held) << "frame issued while the previous one was outstanding";
    this->m_heldWrite = writeBuffer;
    this->m_heldRead = readBuffer;
    this->m_held = true;
    if (this->m_spiMode == SpiMode::IN_CALL) {
        Drv::SpiStatus status = this->m_spiStatus;
        if (this->m_seqIndex < this->m_seqCount) {
            status = this->m_seq[this->m_seqIndex];
            this->m_seqIndex++;
        }
        this->completeSpi(status);
    }
}

void FramDriverTester ::from_readComplete_handler(FwIndexType portNum,
                                                  U32 offset,
                                                  Fw::Buffer& buffer,
                                                  const Fram::Status& status) {
    this->recordCompletion(Fram::Op::READ, portNum, offset, buffer, status);
}

void FramDriverTester ::from_writeComplete_handler(FwIndexType portNum,
                                                   U32 offset,
                                                   Fw::Buffer& buffer,
                                                   const Fram::Status& status) {
    this->recordCompletion(Fram::Op::WRITE, portNum, offset, buffer, status);
}

// ----------------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------------

void FramDriverTester ::clearAll() {
    this->clearHistory();
    this->m_frameCount = 0;
    this->m_completionCount = 0;
}

void FramDriverTester ::setSpi(SpiMode mode, Drv::SpiStatus status) {
    this->m_spiMode = mode;
    this->m_spiStatus = status;
}

void FramDriverTester ::setSpiSequence(const Drv::SpiStatus* statuses, FwSizeType count) {
    ASSERT_LE(count, FW_NUM_ARRAY_ELEMENTS(this->m_seq));
    for (FwSizeType i = 0; i < count; i++) {
        this->m_seq[i] = statuses[i];
    }
    this->m_seqCount = count;
    this->m_seqIndex = 0;
}

void FramDriverTester ::queueReply(const U8* reply, FwSizeType length) {
    ASSERT_LE(length, FRAME);
    (void)::memset(this->m_reply2, 0, sizeof(this->m_reply2));
    (void)::memcpy(this->m_reply2, reply, length);
    this->m_reply2Length = length;
    this->m_reply2Set = true;
}

void FramDriverTester ::setRdidReply(const U8* deviceId) {
    U8 rdid[1 + ID_SIZE] = {};
    (void)::memcpy(&rdid[1], deviceId, ID_SIZE);
    this->setReply(rdid, sizeof(rdid));
}

void FramDriverTester ::completeWrite() {
    this->completeSpi(Drv::SpiStatus::SPI_OK);
    this->completeSpi(Drv::SpiStatus::SPI_OK);
}

void FramDriverTester ::recordCompletion(Fram::Op op,
                                         FwIndexType port,
                                         U32 offset,
                                         Fw::Buffer& buffer,
                                         Fram::Status status) {
    ASSERT_LT(this->m_completionCount, MAX_COMPLETIONS);
    Completion& c = this->m_completions[this->m_completionCount];
    this->m_completionCount++;
    c.op = op;
    c.port = port;
    c.offset = offset;
    c.data = buffer.getData();
    c.size = buffer.getSize();
    c.context = buffer.getContext();
    c.status = status;
    if (this->m_reentrantRequest) {
        this->m_reentrantRequest = false;  // once
        this->m_reentrantStatus = this->invoke_to_read(1, 0x1000, this->m_reentrantBuffer);
    }
}

void FramDriverTester ::setReply(const U8* reply, FwSizeType length) {
    ASSERT_LE(length, FRAME);
    (void)::memset(this->m_reply, 0, sizeof(this->m_reply));
    (void)::memcpy(this->m_reply, reply, length);
    this->m_replyLength = length;
}

void FramDriverTester ::setReadReply(FwSizeType length, U8 seed) {
    ASSERT_LE(length, MAX_TRANSFER);
    (void)::memset(this->m_reply, 0, sizeof(this->m_reply));
    FramDriverTester::fill(&this->m_reply[HEADER], length, seed);
    this->m_replyLength = HEADER + length;
}

void FramDriverTester ::completeSpi(Drv::SpiStatus status) {
    ASSERT_TRUE(this->m_held) << "no frame outstanding";
    this->m_held = false;
    Fw::Buffer write = this->m_heldWrite;
    Fw::Buffer read = this->m_heldRead;
    if (status == Drv::SpiStatus::SPI_OK) {
        const FwSizeType n = (read.getSize() < sizeof(this->m_reply)) ? read.getSize() : sizeof(this->m_reply);
        (void)::memcpy(read.getData(), this->m_reply, n);
    }
    if (this->m_reply2Set) {
        (void)::memcpy(this->m_reply, this->m_reply2, sizeof(this->m_reply));
        this->m_replyLength = this->m_reply2Length;
        this->m_reply2Set = false;
    }
    this->invoke_to_spiWriteReadComplete(0, write, read, status);
}

Fram::Status FramDriverTester ::requestRead(FwIndexType port, U32 offset, Fw::Buffer& buffer) {
    return this->invoke_to_read(port, offset, buffer);
}

Fram::Status FramDriverTester ::requestWrite(FwIndexType port, U32 offset, Fw::Buffer& buffer) {
    return this->invoke_to_write(port, offset, buffer);
}

void FramDriverTester ::runStartup(const U8* deviceId, U8 statusRegister, bool clear) {
    this->setSpi(SpiMode::HOLD, Drv::SpiStatus::SPI_OK);
    this->component.startup();
    this->assertFrames(1);
    this->assertFrame(0, OP_RDID, -1, 1 + ID_SIZE);
    this->setRdidReply(deviceId);
    this->completeSpi(Drv::SpiStatus::SPI_OK);
    this->assertFrames(2);
    this->assertFrame(1, OP_RDSR, -1, 2);
    U8 rdsr[2] = {0, statusRegister};
    this->setReply(rdsr, sizeof(rdsr));
    this->completeSpi(Drv::SpiStatus::SPI_OK);
    ASSERT_EVENTS_StartupComplete_SIZE(1);
    ASSERT_TLM_StartupState_SIZE(2);
    ASSERT_TLM_StartupState(1, Fram::StartupState::COMPLETE);
    ASSERT_TLM_StatusRegister(1, statusRegister);
    if (clear) {
        this->clearAll();
    }
}

void FramDriverTester ::assertFrame(FwSizeType index, U8 opcode, I64 address, FwSizeType length) {
    ASSERT_LT(index, this->m_frameCount);
    const Frame& frame = this->m_frames[index];
    ASSERT_EQ(frame.length, length);
    ASSERT_EQ(frame.rxLength, length);
    ASSERT_EQ(frame.tx[0], opcode);
    if (address >= 0) {
        const U32 got = (static_cast<U32>(frame.tx[1]) << 16) | (static_cast<U32>(frame.tx[2]) << 8) |
                        static_cast<U32>(frame.tx[3]);
        ASSERT_EQ(got, static_cast<U32>(address));
    }
}

void FramDriverTester ::assertFrameData(FwSizeType index, const U8* data, FwSizeType length) {
    ASSERT_LT(index, this->m_frameCount);
    ASSERT_EQ(this->m_frames[index].length, HEADER + length);
    ASSERT_EQ(::memcmp(&this->m_frames[index].tx[HEADER], data, length), 0);
}

void FramDriverTester ::assertFrames(FwSizeType count) {
    ASSERT_EQ(this->m_frameCount, count);
}

void FramDriverTester ::assertCompletion(FwSizeType index,
                                         Fram::Op op,
                                         FwIndexType port,
                                         U32 offset,
                                         const Fw::Buffer& buffer,
                                         Fram::Status status) {
    ASSERT_LT(index, this->m_completionCount);
    const Completion& c = this->m_completions[index];
    ASSERT_EQ(c.op, op);
    ASSERT_EQ(c.port, port);
    ASSERT_EQ(c.offset, offset);
    ASSERT_EQ(c.data, buffer.getData());
    ASSERT_EQ(c.size, buffer.getSize());
    ASSERT_EQ(c.status, status);
}

void FramDriverTester ::assertCompletions(FwSizeType count) {
    ASSERT_EQ(this->m_completionCount, count);
}

void FramDriverTester ::assertRejected(FwSizeType index,
                                       Fram::Op op,
                                       FwIndexType port,
                                       U32 offset,
                                       FwSizeType length,
                                       Fram::Status status,
                                       Fram::RejectCause cause) {
    ASSERT_LT(index, this->eventHistory_RequestRejected->size());
    const EventEntry_RequestRejected& e = this->eventHistory_RequestRejected->at(static_cast<U32>(index));
    ASSERT_EQ(e.op, op);
    ASSERT_EQ(e.client, port);
    ASSERT_EQ(e.offset, offset);
    ASSERT_EQ(e.length, length);
    ASSERT_EQ(e.status, status);
    ASSERT_EQ(e.cause, cause);
}

void FramDriverTester ::assertSpiError(FwSizeType index,
                                       Fram::Op op,
                                       FwIndexType port,
                                       U32 offset,
                                       FwSizeType length,
                                       Drv::SpiStatus status,
                                       Fram::SpiErrorSite site) {
    ASSERT_LT(index, this->eventHistory_SpiError->size());
    const EventEntry_SpiError& e = this->eventHistory_SpiError->at(static_cast<U32>(index));
    ASSERT_EQ(e.op, op);
    ASSERT_EQ(e.client, port);
    ASSERT_EQ(e.offset, offset);
    ASSERT_EQ(e.length, length);
    ASSERT_EQ(e.status, status);
    ASSERT_EQ(e.site, site);
}

void FramDriverTester ::fill(U8* data, FwSizeType length, U8 seed) {
    for (FwSizeType i = 0; i < length; i++) {
        data[i] = static_cast<U8>(seed + (i * 7));
    }
}

}  // namespace Fram

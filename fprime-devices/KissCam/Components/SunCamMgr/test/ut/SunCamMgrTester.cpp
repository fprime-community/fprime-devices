// ======================================================================
// \title  SunCamMgrTester.cpp
// \author lestarch-autobot
// \brief  cpp file for SunCamMgr component test harness implementation class
// ======================================================================

#include "SunCamMgrTester.hpp"

#include <cstring>

#include "Fw/Dp/DpContainer.hpp"
#include "fprime-devices/KissCam/KissCamConfig/FppConstantsAc.hpp"

namespace KissCam {

// Definitions for the static constants that are odr-used by the gtest assertions
const U16 SunCamMgrTester::DEFAULT_EXPOSURE;
const U8 SunCamMgrTester::FRAME_SIZE;
const FwSizeType SunCamMgrTester::MAX_REPLY;

namespace {
// Image geometry (datasheet §7.4.5, §7.4.6)
const U16 FULL_PIXELS = 640;
const U16 PREVIEW_PIXELS = 80;
const U16 FULL_LINES = 480;
const U16 PREVIEW_LINES = 60;
const U16 EXPOSURE_REGISTER = 0x3012;

//! Builds the FrameHead array argument of FrameDiscarded
KissCam::FrameHead head(U8 b0, U8 b1, U8 b2, U8 b3, U8 b4, U8 b5, U8 b6, U8 b7) {
    const U8 bytes[8] = {b0, b1, b2, b3, b4, b5, b6, b7};
    return KissCam::FrameHead(bytes);
}
}  // namespace

// ----------------------------------------------------------------------
// Construction and destruction
// ----------------------------------------------------------------------

SunCamMgrTester::SunCamMgrTester()
    : SunCamMgrGTestBase("SunCamMgrTester", SunCamMgrTester::MAX_HISTORY_SIZE),
      component("SunCamMgr"),
      m_lastFrameSize(0),
      m_frameCount(0),
      m_sendStatus(Drv::ByteStreamStatus::OP_OK),
      m_productGetStatus(Fw::Success::SUCCESS) {
    ::memset(this->m_lastFrame, 0, sizeof this->m_lastFrame);
    ::memset(this->m_rxStore, 0, sizeof this->m_rxStore);
    this->initComponents();
    this->connectPorts();
    this->component.loadParameters();
}

SunCamMgrTester::~SunCamMgrTester() {}

// ----------------------------------------------------------------------
// Handlers for typed from ports
// ----------------------------------------------------------------------

Drv::ByteStreamStatus SunCamMgrTester::from_drvSendOut_handler(FwIndexType portNum, Fw::Buffer& sendBuffer) {
    this->pushFromPortEntry_drvSendOut(sendBuffer);
    this->m_lastFrameSize = FW_MIN(sendBuffer.getSize(), static_cast<FwSizeType>(FRAME_SIZE));
    ::memcpy(this->m_lastFrame, sendBuffer.getData(), this->m_lastFrameSize);
    this->m_frameCount++;
    return this->m_sendStatus;
}

// ----------------------------------------------------------------------
// Handlers for data product ports
// ----------------------------------------------------------------------

Fw::Success::T SunCamMgrTester::productGet_handler(FwDpIdType id, FwSizeType dataSize, Fw::Buffer& buffer) {
    this->pushProductGetEntry(id, dataSize);
    const FwSizeType packetSize = Fw::DpContainer::getPacketSizeForDataSize(dataSize);
    if ((this->m_productGetStatus != Fw::Success::SUCCESS) || (packetSize > DP_STORE_SIZE)) {
        return Fw::Success::FAILURE;
    }
    buffer.setData(this->m_dpStore);
    buffer.setSize(packetSize);
    return Fw::Success::SUCCESS;
}

// ----------------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------------

void SunCamMgrTester::dispatchAll() {
    // Bounded by the queue depth: every queued message is dispatched once
    for (FwSizeType i = 0; i < TEST_INSTANCE_QUEUE_DEPTH; i++) {
        if (this->component.doDispatch() == Fw::QueuedComponentBase::MSG_DISPATCH_EMPTY) {
            break;
        }
    }
}

void SunCamMgrTester::tick(U32 n) {
    for (U32 i = 0; i < n; i++) {
        this->invoke_to_run(0, 0);
        this->dispatchAll();
    }
}

void SunCamMgrTester::receive(const U8* bytes, FwSizeType size, Drv::ByteStreamStatus status) {
    ASSERT_LE(size, sizeof this->m_rxStore);
    if (size > 0) {
        ::memcpy(this->m_rxStore, bytes, size);
    }
    Fw::Buffer buffer(this->m_rxStore, size);
    const U32 returned = static_cast<U32>(this->fromPortHistory_drvReceiveReturnOut->size());
    this->invoke_to_drvReceiveIn(0, buffer, status);
    this->dispatchAll();
    // Every receive buffer that carried data comes back; a null notice has nothing to return
    if (size > 0) {
        ASSERT_EQ(this->fromPortHistory_drvReceiveReturnOut->size(), returned + 1);
        ASSERT_EQ(this->fromPortHistory_drvReceiveReturnOut->at(returned).fwBuffer.getData(), this->m_rxStore);
    } else {
        ASSERT_EQ(this->fromPortHistory_drvReceiveReturnOut->size(), returned);
    }
}

void SunCamMgrTester::reply(U8 ack, U8 mode, const U8* payload, U16 length) {
    U8 frame[MAX_REPLY];
    ASSERT_LE(static_cast<FwSizeType>(length) + 6, sizeof frame);
    frame[0] = START;
    frame[1] = ack;
    frame[2] = mode;
    frame[3] = static_cast<U8>(length >> 8);
    frame[4] = static_cast<U8>(length & 0xFF);
    if (length > 0) {
        ::memcpy(&frame[5], payload, length);
    }
    frame[5 + length] = END;
    this->receive(frame, static_cast<FwSizeType>(length) + 6);
}

void SunCamMgrTester::ack(U8 cmd, U8 mode) {
    this->reply(cmd, mode, nullptr, 0);
}

void SunCamMgrTester::errorReply(U8 cmd, U8 errorType, U8 count, U8 mode) {
    const U8 payload[3] = {cmd, errorType, count};
    this->reply(CMD_ERROR, mode, payload, sizeof payload);
}

void SunCamMgrTester::pingReply(U8 valid, U8 invalid, U16 uptime, U8 mode) {
    const U8 payload[4] = {valid, invalid, static_cast<U8>(uptime >> 8), static_cast<U8>(uptime & 0xFF)};
    this->reply(CMD_PING, mode, payload, sizeof payload);
}

void SunCamMgrTester::registerReply(U16 address, U16 value, U8 mode) {
    const U8 payload[4] = {static_cast<U8>(address >> 8), static_cast<U8>(address & 0xFF), static_cast<U8>(value >> 8),
                           static_cast<U8>(value & 0xFF)};
    this->reply(CMD_READ_REGISTER, mode, payload, sizeof payload);
}

void SunCamMgrTester::memoryStatusReply(const U8 modes[4], const U16 exposures[4], const U16 uptimes[4], U8 mode) {
    U8 payload[20];
    for (U32 i = 0; i < 4; i++) {
        payload[5 * i] = modes[i];
        payload[5 * i + 1] = static_cast<U8>(exposures[i] >> 8);
        payload[5 * i + 2] = static_cast<U8>(exposures[i] & 0xFF);
        payload[5 * i + 3] = static_cast<U8>(uptimes[i] >> 8);
        payload[5 * i + 4] = static_cast<U8>(uptimes[i] & 0xFF);
    }
    this->reply(CMD_MEMORY_STATUS, mode, payload, sizeof payload);
}

void SunCamMgrTester::memoryStatusAllSd(U8 mode) {
    const U8 modes[4] = {MODE_SD, MODE_SD, MODE_SD, MODE_SD};
    const U16 exposures[4] = {10, 20, 30, 0};
    const U16 uptimes[4] = {100, 200, 300, 400};
    this->memoryStatusReply(modes, exposures, uptimes, mode);
}

void SunCamMgrTester::lineReply(U16 line, U16 pixels, U8 fill, U8 mode) {
    U8 payload[2 + FULL_PIXELS];
    ASSERT_LE(pixels, FULL_PIXELS);
    payload[0] = static_cast<U8>(line >> 8);
    payload[1] = static_cast<U8>(line & 0xFF);
    ::memset(&payload[2], fill, pixels);
    this->reply(CMD_DOWNLOAD, mode, payload, static_cast<U16>(pixels + 2));
}

void SunCamMgrTester::assertFrame(U8 cmd, U8 p1, U8 p2, U8 p3, U8 p4, U32 expectedCount) {
    const U8 expected[FRAME_SIZE] = {START, cmd, p1, p2, p3, p4, END};
    ASSERT_EQ(this->m_frameCount, expectedCount);
    ASSERT_EQ(this->m_lastFrameSize, static_cast<FwSizeType>(FRAME_SIZE));
    for (U32 i = 0; i < FRAME_SIZE; i++) {
        ASSERT_EQ(this->m_lastFrame[i], expected[i]) << "frame byte " << i;
    }
}

void SunCamMgrTester::sendSetup(U32 cmdSeq) {
    this->sendCmd_SETUP_CAMERA(0, cmdSeq);
    this->dispatchAll();
}

void SunCamMgrTester::setupNominal(U8 modeByte, U16 exposure) {
    const U32 base = this->m_frameCount;
    this->sendSetup();
    this->assertFrame(CMD_PING, 0, 0, 0, 0, base + 1);
    this->pingReply(1, 0, 5, MODE_SLEEP);
    this->assertFrame(CMD_SET_MODE, modeByte, 0, 0, 0, base + 2);
    this->ack(CMD_SET_MODE, modeByte);
    this->assertFrame(CMD_SET_EXPOSURE, static_cast<U8>(exposure >> 8), static_cast<U8>(exposure & 0xFF), 0, 0,
                      base + 3);
    this->ack(CMD_SET_EXPOSURE, modeByte);
    this->assertFrame(CMD_READ_REGISTER, 0x30, 0x12, 0, 0, base + 4);
    this->registerReply(EXPOSURE_REGISTER, exposure, modeByte);
    this->assertFrame(CMD_PING, 1, 0, 0, 0, base + 5);
    this->pingReply(5, 0, 6, modeByte);
    ASSERT_EQ(this->tlmHistory_SetupState->at(this->tlmHistory_SetupState->size() - 1).arg, SetupState::READY);
    this->clearHistory();
}

void SunCamMgrTester::startDownload(Slot slot, Resolution resolution, U32 cmdSeq) {
    const U32 base = this->m_frameCount;
    this->sendCmd_DOWNLOAD(0, cmdSeq, slot, resolution);
    this->dispatchAll();
    this->assertFrame(CMD_MEMORY_STATUS, 0, 0, 0, 0, base + 1);
    this->memoryStatusAllSd();
    this->assertFrame(CMD_DOWNLOAD, static_cast<U8>(slot.e), static_cast<U8>(resolution.e), 0, 0, base + 2);
}

U32 SunCamMgrTester::checkProduct(const KissCam::ImageHeader& expectedHeader,
                                  U16 pixels,
                                  U32 expectedLines,
                                  U8 firstSlot) {
    EXPECT_EQ(this->productSendHistory->size(), 1u);
    if (this->productSendHistory->size() != 1) {
        return 0;
    }
    const Fw::Buffer buffer = this->productSendHistory->at(0).buffer;
    EXPECT_EQ(this->productSendHistory->at(0).id, static_cast<FwDpIdType>(0));
    Fw::DpContainer container(0, buffer);
    EXPECT_EQ(container.deserializeHeader(), Fw::FW_SERIALIZE_OK);
    EXPECT_EQ(container.getPriority(), static_cast<FwDpPriorityType>(KissCamConfig::IMAGE_DP_PRIORITY));
    const FwSizeType dataSize = container.getDataSize();
    const FwSizeType lineRecord = SunCamMgr::SIZE_OF_ImageLine_RECORD(static_cast<FwSizeType>(pixels) + 3);
    EXPECT_EQ(dataSize, SunCamMgr::SIZE_OF_ImageHeader_RECORD + expectedLines * lineRecord);
    Fw::ExternalSerializeBuffer data(&buffer.getData()[Fw::DpContainer::DATA_OFFSET], dataSize);
    EXPECT_EQ(data.setBuffLen(dataSize), Fw::FW_SERIALIZE_OK);
    FwDpIdType id = 0;
    EXPECT_EQ(data.deserializeTo(id), Fw::FW_SERIALIZE_OK);
    EXPECT_EQ(id, static_cast<FwDpIdType>(0));
    KissCam::ImageHeader header;
    EXPECT_EQ(data.deserializeTo(header), Fw::FW_SERIALIZE_OK);
    EXPECT_EQ(header, expectedHeader);
    U32 lines = 0;
    const U16 linesPerSlot = (pixels == FULL_PIXELS) ? FULL_LINES : PREVIEW_LINES;
    while ((data.getDeserializeSizeLeft() > 0) && (lines < expectedLines)) {
        EXPECT_EQ(data.deserializeTo(id), Fw::FW_SERIALIZE_OK);
        EXPECT_EQ(id, static_cast<FwDpIdType>(1));
        FwSizeStoreType size = 0;
        EXPECT_EQ(data.deserializeTo(size), Fw::FW_SERIALIZE_OK);
        EXPECT_EQ(size, static_cast<FwSizeStoreType>(pixels + 3));
        const U8* record = data.getBuffAddrLeft();
        const U8 expectedSlot = static_cast<U8>(firstSlot + lines / linesPerSlot);
        const U16 expectedLine = static_cast<U16>(lines % linesPerSlot);
        EXPECT_EQ(record[0], expectedSlot) << "line record " << lines;
        EXPECT_EQ(record[1], static_cast<U8>(expectedLine >> 8)) << "line record " << lines;
        EXPECT_EQ(record[2], static_cast<U8>(expectedLine & 0xFF)) << "line record " << lines;
        EXPECT_EQ(record[3], static_cast<U8>(expectedLine & 0xFF)) << "pixel fill of line record " << lines;
        EXPECT_EQ(data.moveDeserToOffset(dataSize - data.getDeserializeSizeLeft() + size), Fw::FW_SERIALIZE_OK);
        lines++;
    }
    EXPECT_EQ(data.getDeserializeSizeLeft(), 0u);
    return lines;
}

// ----------------------------------------------------------------------
// Tests
// ----------------------------------------------------------------------

void SunCamMgrTester::testSetupNominal() {
    this->tick();  // first tick: initial state channels
    ASSERT_TLM_SetupState(0, SetupState::NOT_SETUP);
    ASSERT_TLM_CurrentOperation(0, Operation::NONE);
    this->clearHistory();

    this->sendSetup(3);
    ASSERT_EVENTS_SetupStarted_SIZE(1);
    ASSERT_TLM_SetupState(0, SetupState::SETTING_UP);
    ASSERT_TLM_CurrentOperation(0, Operation::SETUP_CAMERA);
    ASSERT_CMD_RESPONSE_SIZE(0);
    this->assertFrame(CMD_PING, 0, 0, 0, 0, 1);
    this->pingReply(1, 0, 5, MODE_SLEEP);
    ASSERT_EVENTS_PingReply_SIZE(0);  // set-up ping is not reported as a PING command outcome
    this->assertFrame(CMD_SET_MODE, MODE_SD, 0, 0, 0, 2);
    this->ack(CMD_SET_MODE, MODE_SD);
    ASSERT_EVENTS_SettingApplied_SIZE(1);
    ASSERT_EVENTS_SettingApplied(0, Setting::CAMERA_MODE, 1);
    this->assertFrame(CMD_SET_EXPOSURE, 0x02, 0xA0, 0, 0, 3);
    this->ack(CMD_SET_EXPOSURE, MODE_SD);
    ASSERT_EVENTS_SettingApplied_SIZE(2);
    ASSERT_EVENTS_SettingApplied(1, Setting::EXPOSURE, DEFAULT_EXPOSURE);
    this->assertFrame(CMD_READ_REGISTER, 0x30, 0x12, 0, 0, 4);
    this->registerReply(EXPOSURE_REGISTER, DEFAULT_EXPOSURE, MODE_SD);
    ASSERT_EVENTS_ExposureReadback_SIZE(1);
    ASSERT_EVENTS_ExposureReadback(0, DEFAULT_EXPOSURE, DEFAULT_EXPOSURE);
    ASSERT_CMD_RESPONSE_SIZE(0);
    this->assertFrame(CMD_PING, 1, 0, 0, 0, 5);
    this->pingReply(5, 0, 6, MODE_SD);

    ASSERT_EVENTS_SetupComplete_SIZE(1);
    ASSERT_EVENTS_SetupComplete(0, CaptureMode::SD, DEFAULT_EXPOSURE);
    ASSERT_EVENTS_SetupFailed_SIZE(0);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, SunCamMgr::OPCODE_SETUP_CAMERA, 3, Fw::CmdResponse::OK);
    ASSERT_EQ(this->tlmHistory_SetupState->at(this->tlmHistory_SetupState->size() - 1).arg, SetupState::READY);
    ASSERT_EQ(this->tlmHistory_CurrentOperation->at(this->tlmHistory_CurrentOperation->size() - 1).arg,
              Operation::NONE);
    ASSERT_EQ(this->tlmHistory_CameraMode->at(this->tlmHistory_CameraMode->size() - 1).arg, Mode::SD);
    ASSERT_EQ(this->tlmHistory_Exposure->at(this->tlmHistory_Exposure->size() - 1).arg, DEFAULT_EXPOSURE);
    ASSERT_TLM_ExposureReadback(0, DEFAULT_EXPOSURE);
    ASSERT_EQ(this->tlmHistory_Exchanges->at(this->tlmHistory_Exchanges->size() - 1).arg, 5u);
    ASSERT_EQ(this->m_frameCount, 5u);
}

void SunCamMgrTester::testSetupFailsEachStep() {
    const U8 cmds[5] = {CMD_PING, CMD_SET_MODE, CMD_SET_EXPOSURE, CMD_READ_REGISTER, CMD_PING};
    const SetupStep::T steps[5] = {SetupStep::PING_MCU, SetupStep::SET_MODE, SetupStep::SET_EXPOSURE,
                                   SetupStep::READ_EXPOSURE, SetupStep::PING_SENSOR};
    for (U32 failAt = 0; failAt < 5; failAt++) {
        for (U32 byTimeout = 0; byTimeout < 2; byTimeout++) {
            this->clearHistory();
            this->m_frameCount = 0;
            this->sendSetup(failAt);
            for (U32 step = 0; step < failAt; step++) {
                switch (cmds[step]) {
                    case CMD_PING:
                        this->pingReply(1, 0, 1, (step == 0) ? MODE_SLEEP : MODE_SD);
                        break;
                    case CMD_READ_REGISTER:
                        this->registerReply(EXPOSURE_REGISTER, DEFAULT_EXPOSURE);
                        break;
                    default:
                        this->ack(cmds[step]);
                        break;
                }
            }
            ASSERT_EQ(this->m_frameCount, failAt + 1);
            ASSERT_EQ(this->m_lastFrame[1], cmds[failAt]);
            if (byTimeout == 1) {
                this->tick(KissCamConfig::REPLY_TIMEOUT_TICKS);
                ASSERT_EVENTS_ReplyTimeout_SIZE(1);
                ASSERT_EVENTS_ReplyTimeout(0, Operation::SETUP_CAMERA, cmds[failAt]);
                ASSERT_EVENTS_SetupFailed(0, steps[failAt], FailCause::TIMEOUT);
            } else {
                this->errorReply(cmds[failAt], 0x03, 7);
                ASSERT_EVENTS_CameraError_SIZE(1);
                ASSERT_EVENTS_CameraError(0, Operation::SETUP_CAMERA, cmds[failAt], ErrorType::WRONG_MODE, 7);
                ASSERT_EVENTS_SetupFailed(0, steps[failAt], FailCause::CAMERA_ERROR);
            }
            ASSERT_EVENTS_SetupFailed_SIZE(1);
            ASSERT_EVENTS_SetupComplete_SIZE(0);
            ASSERT_CMD_RESPONSE_SIZE(1);
            ASSERT_CMD_RESPONSE(0, SunCamMgr::OPCODE_SETUP_CAMERA, failAt, Fw::CmdResponse::EXECUTION_ERROR);
            ASSERT_EQ(this->tlmHistory_SetupState->at(this->tlmHistory_SetupState->size() - 1).arg,
                      SetupState::NOT_SETUP);
            // no further frame after the failure
            ASSERT_EQ(this->m_frameCount, failAt + 1);
        }
    }
    // Out-of-range persisted EXPOSURE fails SET_EXPOSURE locally before any Set Exposure frame
    this->clearHistory();
    this->m_frameCount = 0;
    this->paramSet_EXPOSURE(1, Fw::ParamValid::VALID);
    this->sendSetup(9);
    this->pingReply(1, 0, 1, MODE_SLEEP);
    this->ack(CMD_SET_MODE);
    ASSERT_EQ(this->m_frameCount, 2u);
    ASSERT_EVENTS_SettingRejected_SIZE(1);
    ASSERT_EVENTS_SettingRejected(0, Setting::EXPOSURE, 1, KissCamConfig::EXPOSURE_MIN, KissCamConfig::EXPOSURE_MAX);
    ASSERT_EVENTS_SetupFailed_SIZE(1);
    ASSERT_EVENTS_SetupFailed(0, SetupStep::SET_EXPOSURE, FailCause::OUT_OF_RANGE);
    ASSERT_CMD_RESPONSE(0, SunCamMgr::OPCODE_SETUP_CAMERA, 9, Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_EQ(this->tlmHistory_SetupState->at(this->tlmHistory_SetupState->size() - 1).arg, SetupState::NOT_SETUP);
}

void SunCamMgrTester::testSetupReentrant() {
    // after a failed set-up
    this->sendSetup();
    this->tick(KissCamConfig::REPLY_TIMEOUT_TICKS);
    ASSERT_EVENTS_SetupFailed_SIZE(1);
    this->clearHistory();
    this->setupNominal();
    ASSERT_EQ(this->m_frameCount, 6u);
    // while READY: the full sequence again
    this->setupNominal();
    ASSERT_EQ(this->m_frameCount, 11u);
}

void SunCamMgrTester::testNoFramesBeforeSetup() {
    this->tick(3);
    this->invoke_to_drvConnected(0);
    this->dispatchAll();
    this->paramSet_EXPOSURE(10, Fw::ParamValid::VALID);
    this->paramSend_EXPOSURE(0, 0);
    this->tick(3);
    ASSERT_EQ(this->m_frameCount, 0u);

    this->sendCmd_MEMORY_STATUS(0, 1);
    this->sendCmd_CAPTURE(0, 2, Slot::SLOT_1, TestPattern::OFF);
    this->sendCmd_DOWNLOAD(0, 3, Slot::SLOT_1, Resolution::FULL);
    this->sendCmd_READ_REGISTER(0, 4, 0x3012);
    this->dispatchAll();
    ASSERT_EQ(this->m_frameCount, 0u);
    ASSERT_EVENTS_CommandRejected_SIZE(4);
    ASSERT_EVENTS_CommandRejected(0, Operation::MEMORY_STATUS, RejectCause::NOT_READY);
    ASSERT_EVENTS_CommandRejected(1, Operation::CAPTURE, RejectCause::NOT_READY);
    ASSERT_EVENTS_CommandRejected(2, Operation::DOWNLOAD, RejectCause::NOT_READY);
    ASSERT_EVENTS_CommandRejected(3, Operation::READ_REGISTER, RejectCause::NOT_READY);
    ASSERT_CMD_RESPONSE_SIZE(4);
    ASSERT_CMD_RESPONSE(0, SunCamMgr::OPCODE_MEMORY_STATUS, 1, Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_CMD_RESPONSE(1, SunCamMgr::OPCODE_CAPTURE, 2, Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_CMD_RESPONSE(2, SunCamMgr::OPCODE_DOWNLOAD, 3, Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_CMD_RESPONSE(3, SunCamMgr::OPCODE_READ_REGISTER, 4, Fw::CmdResponse::EXECUTION_ERROR);
}

void SunCamMgrTester::testSleepAndPingNotSetup() {
    this->sendCmd_PING(0, 1, PingType::MCU_AND_SENSOR);
    this->dispatchAll();
    this->assertFrame(CMD_PING, 1, 0, 0, 0, 1);
    this->pingReply(3, 1, 77, MODE_SLEEP);
    ASSERT_EVENTS_PingReply_SIZE(1);
    ASSERT_EVENTS_PingReply(0, PingType::MCU_AND_SENSOR, 3, 1, 77);
    ASSERT_CMD_RESPONSE(0, SunCamMgr::OPCODE_PING, 1, Fw::CmdResponse::OK);

    // SLEEP success from NOT_SETUP
    this->sendCmd_SLEEP(0, 2);
    this->dispatchAll();
    this->assertFrame(CMD_SET_MODE, MODE_SLEEP, 0, 0, 0, 2);
    this->ack(CMD_SET_MODE, MODE_SLEEP);
    ASSERT_EVENTS_SleepEntered_SIZE(1);
    ASSERT_CMD_RESPONSE(1, SunCamMgr::OPCODE_SLEEP, 2, Fw::CmdResponse::OK);
    ASSERT_EQ(this->tlmHistory_SetupState->at(this->tlmHistory_SetupState->size() - 1).arg, SetupState::NOT_SETUP);

    // SLEEP from READY, success: NOT_SETUP
    this->setupNominal();
    this->sendCmd_SLEEP(0, 3);
    this->dispatchAll();
    this->ack(CMD_SET_MODE, MODE_SLEEP);
    ASSERT_EVENTS_SleepEntered_SIZE(1);
    ASSERT_EVENTS_ModeMismatch_SIZE(0);
    ASSERT_CMD_RESPONSE(0, SunCamMgr::OPCODE_SLEEP, 3, Fw::CmdResponse::OK);
    ASSERT_EQ(this->tlmHistory_SetupState->at(this->tlmHistory_SetupState->size() - 1).arg, SetupState::NOT_SETUP);

    // SLEEP from READY, failure: still NOT_SETUP
    this->setupNominal();
    this->sendCmd_SLEEP(0, 4);
    this->dispatchAll();
    this->errorReply(CMD_SET_MODE, 0x04, 1);
    ASSERT_EVENTS_SleepEntered_SIZE(0);
    ASSERT_EVENTS_CameraError_SIZE(1);
    ASSERT_CMD_RESPONSE(0, SunCamMgr::OPCODE_SLEEP, 4, Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_EQ(this->tlmHistory_SetupState->at(this->tlmHistory_SetupState->size() - 1).arg, SetupState::NOT_SETUP);
}

void SunCamMgrTester::testBusyRejection() {
    this->setupNominal();
    this->sendCmd_MEMORY_STATUS(0, 1);
    this->dispatchAll();
    ASSERT_EQ(this->m_frameCount, 6u);

    this->sendCmd_SETUP_CAMERA(0, 10);
    this->sendCmd_SLEEP(0, 11);
    this->sendCmd_PING(0, 12, PingType::MCU);
    this->sendCmd_MEMORY_STATUS(0, 13);
    this->sendCmd_CAPTURE(0, 14, Slot::SLOT_2, TestPattern::ON);
    this->sendCmd_DOWNLOAD(0, 15, Slot::SLOT_1, Resolution::FULL);
    this->sendCmd_READ_REGISTER(0, 16, 0x3012);
    this->dispatchAll();
    ASSERT_EVENTS_CommandRejected_SIZE(7);
    const Operation::T ops[7] = {Operation::SETUP_CAMERA, Operation::SLEEP,    Operation::PING,         Operation::MEMORY_STATUS,
                                 Operation::CAPTURE,      Operation::DOWNLOAD, Operation::READ_REGISTER};
    const FwOpcodeType opcodes[7] = {SunCamMgr::OPCODE_SETUP_CAMERA, SunCamMgr::OPCODE_SLEEP,
                                     SunCamMgr::OPCODE_PING,         SunCamMgr::OPCODE_MEMORY_STATUS,
                                     SunCamMgr::OPCODE_CAPTURE,      SunCamMgr::OPCODE_DOWNLOAD,
                                     SunCamMgr::OPCODE_READ_REGISTER};
    for (U32 i = 0; i < 7; i++) {
        ASSERT_EVENTS_CommandRejected(i, ops[i], RejectCause::BUSY);
        ASSERT_CMD_RESPONSE(i, opcodes[i], 10 + i, Fw::CmdResponse::BUSY);
    }
    // DOWNLOAD_ABORT during a non-download operation: NO_DOWNLOAD, not BUSY
    this->sendCmd_DOWNLOAD_ABORT(0, 17);
    this->dispatchAll();
    ASSERT_EVENTS_CommandRejected_SIZE(8);
    ASSERT_EVENTS_CommandRejected(7, Operation::DOWNLOAD_ABORT, RejectCause::NO_DOWNLOAD);
    ASSERT_CMD_RESPONSE(7, SunCamMgr::OPCODE_DOWNLOAD_ABORT, 17, Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_EQ(this->m_frameCount, 6u);
    // the operation in progress is unaffected
    this->memoryStatusAllSd();
    ASSERT_CMD_RESPONSE(8, SunCamMgr::OPCODE_MEMORY_STATUS, 1, Fw::CmdResponse::OK);
}

void SunCamMgrTester::testParameterPushIdleOnly() {
    // changed while NOT_SETUP: never pushed on its own, applied by SETUP_CAMERA
    this->paramSet_EXPOSURE(100, Fw::ParamValid::VALID);
    this->paramSend_EXPOSURE(0, 0);
    this->tick(3);
    ASSERT_EQ(this->m_frameCount, 0u);
    this->setupNominal(MODE_SD, 100);
    this->tick(3);
    ASSERT_EQ(this->m_frameCount, 5u);

    // changed during an operation: deferred to the next idle tick
    this->sendCmd_PING(0, 1, PingType::MCU);
    this->dispatchAll();
    this->paramSet_EXPOSURE(200, Fw::ParamValid::VALID);
    this->paramSend_EXPOSURE(0, 0);
    this->tick(2);
    ASSERT_EQ(this->m_frameCount, 6u);
    this->pingReply(1, 0, 1);
    ASSERT_CMD_RESPONSE(0, SunCamMgr::OPCODE_PING, 1, Fw::CmdResponse::OK);
    ASSERT_EQ(this->m_frameCount, 6u);
    this->tick();
    this->assertFrame(CMD_SET_EXPOSURE, 0, 200, 0, 0, 7);
    ASSERT_EQ(this->tlmHistory_CurrentOperation->at(this->tlmHistory_CurrentOperation->size() - 1).arg,
              Operation::PARAMETER_PUSH);
    this->ack(CMD_SET_EXPOSURE);
    ASSERT_EVENTS_SettingApplied(0, Setting::EXPOSURE, 200);
    this->assertFrame(CMD_READ_REGISTER, 0x30, 0x12, 0, 0, 8);
    this->registerReply(EXPOSURE_REGISTER, 200);
    ASSERT_EVENTS_ExposureReadback(0, 200, 200);
    ASSERT_EQ(this->tlmHistory_CurrentOperation->at(this->tlmHistory_CurrentOperation->size() - 1).arg,
              Operation::NONE);
    ASSERT_EQ(this->tlmHistory_SetupState->at(this->tlmHistory_SetupState->size() - 1).arg, SetupState::READY);
    this->tick(3);
    ASSERT_EQ(this->m_frameCount, 8u);
}

void SunCamMgrTester::testParameterPushFailure() {
    // timeout
    this->setupNominal();
    this->paramSet_EXPOSURE(300, Fw::ParamValid::VALID);
    this->paramSend_EXPOSURE(0, 0);
    this->tick();
    this->assertFrame(CMD_SET_EXPOSURE, 0x01, 0x2C, 0, 0, 6);
    this->tick(KissCamConfig::REPLY_TIMEOUT_TICKS);
    ASSERT_EVENTS_ReplyTimeout(0, Operation::PARAMETER_PUSH, CMD_SET_EXPOSURE);
    ASSERT_EVENTS_SettingPushFailed_SIZE(1);
    ASSERT_EVENTS_SettingPushFailed(0, Setting::EXPOSURE, FailCause::TIMEOUT);
    ASSERT_EQ(this->tlmHistory_SetupState->at(this->tlmHistory_SetupState->size() - 1).arg, SetupState::NOT_SETUP);
    this->tick(3);
    ASSERT_EQ(this->m_frameCount, 6u);

    // camera error on the mode push
    this->setupNominal(MODE_SD, 300);
    this->paramSet_CAMERA_MODE(CaptureMode::HD, Fw::ParamValid::VALID);
    this->paramSend_CAMERA_MODE(0, 0);
    this->tick();
    this->assertFrame(CMD_SET_MODE, MODE_HD, 0, 0, 0, 12);
    this->errorReply(CMD_SET_MODE, 0x04, 2, MODE_SD);
    ASSERT_EVENTS_CameraError(0, Operation::PARAMETER_PUSH, CMD_SET_MODE, ErrorType::SENSOR_PROBLEM, 2);
    ASSERT_EVENTS_SettingPushFailed_SIZE(1);
    ASSERT_EVENTS_SettingPushFailed(0, Setting::CAMERA_MODE, FailCause::CAMERA_ERROR);
    ASSERT_EQ(this->tlmHistory_SetupState->at(this->tlmHistory_SetupState->size() - 1).arg, SetupState::NOT_SETUP);
    ASSERT_CMD_RESPONSE_SIZE(0);
}

void SunCamMgrTester::testExposureReadbackMismatch() {
    // at set-up
    this->sendSetup(1);
    this->pingReply(1, 0, 1, MODE_SLEEP);
    this->ack(CMD_SET_MODE);
    this->ack(CMD_SET_EXPOSURE);
    this->registerReply(EXPOSURE_REGISTER, 0x02A1);
    ASSERT_EVENTS_ExposureReadback_SIZE(1);
    ASSERT_EVENTS_ExposureReadback(0, DEFAULT_EXPOSURE, 0x02A1);
    ASSERT_TLM_ExposureReadback(0, 0x02A1);
    ASSERT_EVENTS_SetupFailed(0, SetupStep::READ_EXPOSURE, FailCause::EXPOSURE_MISMATCH);
    ASSERT_CMD_RESPONSE(0, SunCamMgr::OPCODE_SETUP_CAMERA, 1, Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_EQ(this->m_frameCount, 4u);
    // on a push
    this->setupNominal();
    this->paramSet_EXPOSURE(50, Fw::ParamValid::VALID);
    this->paramSend_EXPOSURE(0, 0);
    this->tick();
    this->ack(CMD_SET_EXPOSURE);
    this->registerReply(EXPOSURE_REGISTER, 49);
    ASSERT_EVENTS_ExposureReadback(0, 50, 49);
    ASSERT_EVENTS_SettingPushFailed(0, Setting::EXPOSURE, FailCause::EXPOSURE_MISMATCH);
    ASSERT_EQ(this->tlmHistory_SetupState->at(this->tlmHistory_SetupState->size() - 1).arg, SetupState::NOT_SETUP);
}

void SunCamMgrTester::testCaptureReportsExposure() {
    this->setupNominal();
    this->sendCmd_CAPTURE(0, 1, Slot::SLOT_3, TestPattern::ON);
    this->dispatchAll();
    this->assertFrame(CMD_CAPTURE, 3, 1, 0, 0, 6);
    const U8 payload[2] = {0x00, 0x0A};
    this->reply(CMD_CAPTURE, MODE_SD, payload, sizeof payload);
    ASSERT_EVENTS_CaptureComplete_SIZE(1);
    ASSERT_EVENTS_CaptureComplete(0, Slot::SLOT_3, TestPattern::ON, 10, Mode::SD);
    ASSERT_TLM_ImagesCaptured(0, 1);
    ASSERT_TLM_Exposure(0, 10);
    ASSERT_CMD_RESPONSE(0, SunCamMgr::OPCODE_CAPTURE, 1, Fw::CmdResponse::OK);
}

void SunCamMgrTester::testCommandFrameBytes() {
    this->sendCmd_PING(0, 0, PingType::MCU);
    this->dispatchAll();
    this->assertFrame(CMD_PING, 0x00, 0, 0, 0, 1);
    this->pingReply(1, 0, 1, MODE_SLEEP);
    this->sendCmd_SLEEP(0, 0);
    this->dispatchAll();
    this->assertFrame(CMD_SET_MODE, 0x00, 0, 0, 0, 2);
    this->ack(CMD_SET_MODE, MODE_SLEEP);
    this->paramSet_CAMERA_MODE(CaptureMode::HD, Fw::ParamValid::VALID);
    this->paramSet_EXPOSURE(0x1234 & 0x03FF, Fw::ParamValid::VALID);  // 0x0234 = 564
    this->setupNominal(MODE_HD, 0x0234);                               // Set Mode 02, Set Exposure 02 34, Read 30 12
    this->sendCmd_MEMORY_STATUS(0, 0);
    this->dispatchAll();
    this->assertFrame(CMD_MEMORY_STATUS, 0, 0, 0, 0, 8);
    this->memoryStatusAllSd(MODE_HD);
    this->sendCmd_CAPTURE(0, 0, Slot::SLOT_4, TestPattern::OFF);
    this->dispatchAll();
    this->assertFrame(CMD_CAPTURE, 0x04, 0x00, 0, 0, 9);
    const U8 payload[2] = {0x02, 0x34};
    this->reply(CMD_CAPTURE, MODE_HD, payload, sizeof payload);
    this->sendCmd_READ_REGISTER(0, 0, 0xBEEF);
    this->dispatchAll();
    this->assertFrame(CMD_READ_REGISTER, 0xBE, 0xEF, 0, 0, 10);
    this->registerReply(0xBEEF, 1, MODE_HD);
    this->sendCmd_DOWNLOAD(0, 0, Slot::SLOT_2, Resolution::PREVIEW);
    this->dispatchAll();
    this->assertFrame(CMD_MEMORY_STATUS, 0, 0, 0, 0, 11);
    this->memoryStatusAllSd(MODE_HD);
    this->assertFrame(CMD_DOWNLOAD, 0x02, 0x01, 0x00, 0x00, 12);
    this->lineReply(0, PREVIEW_PIXELS, 0, MODE_HD);
    this->assertFrame(CMD_DOWNLOAD, 0x02, 0x01, 0x00, 0x01, 13);
    // line number is 16-bit MSB first
    for (U16 line = 1; line < 0x0101; line++) {
        if (line == PREVIEW_LINES) {
            break;
        }
        this->lineReply(line, PREVIEW_PIXELS, static_cast<U8>(line), MODE_HD);
    }
    this->sendCmd_DOWNLOAD_ABORT(0, 0);
    this->dispatchAll();
    this->lineReply(PREVIEW_LINES - 1, PREVIEW_PIXELS, 0, MODE_HD);
    ASSERT_EVENTS_DownloadComplete_SIZE(1);
    // a full-resolution line request past 255 carries the MSB
    this->sendCmd_DOWNLOAD(0, 0, Slot::SLOT_1, Resolution::FULL);
    this->dispatchAll();
    this->memoryStatusAllSd(MODE_HD);
    for (U16 line = 0; line < 0x0100; line++) {
        this->lineReply(line, FULL_PIXELS, static_cast<U8>(line), MODE_HD);
    }
    this->assertFrame(CMD_DOWNLOAD, 0x01, 0x00, 0x01, 0x00, this->m_frameCount);
}

void SunCamMgrTester::testChannelsFromReplies() {
    this->setupNominal(MODE_SD);
    this->sendCmd_PING(0, 0, PingType::MCU);
    this->dispatchAll();
    this->pingReply(12, 3, 0xABCD, MODE_SD);
    ASSERT_TLM_CameraUptime(0, 0xABCD);
    ASSERT_TLM_CameraValidCmds(0, 12);
    ASSERT_TLM_CameraInvalidCmds(0, 3);
    ASSERT_TLM_CameraMode(0, Mode::SD);

    this->sendCmd_MEMORY_STATUS(0, 0);
    this->dispatchAll();
    const U8 modes[4] = {MODE_SD, MODE_HD, MODE_SLEEP, MODE_SD};
    const U16 exposures[4] = {10, 672, 0, 960};
    const U16 uptimes[4] = {1, 2, 3, 0xFFFF};
    this->memoryStatusReply(modes, exposures, uptimes);
    ASSERT_TLM_MemorySlots_SIZE(1);
    const MemorySlots& slots = this->tlmHistory_MemorySlots->at(0).arg;
    for (U32 i = 0; i < 4; i++) {
        ASSERT_EQ(slots[i].get_mode(), Mode(static_cast<Mode::T>(modes[i])));
        ASSERT_EQ(slots[i].get_exposure(), exposures[i]);
        ASSERT_EQ(slots[i].get_uptime(), uptimes[i]);
    }
    ASSERT_EVENTS_MemoryStatusReply_SIZE(1);
    ASSERT_EVENTS_MemoryStatusReply(0, slots);

    this->sendCmd_READ_REGISTER(0, 0, 0x3012);
    this->dispatchAll();
    this->registerReply(0x3012, 0x0123);
    ASSERT_TLM_RegisterValue_SIZE(1);
    ASSERT_TLM_RegisterValue(0, RegisterValue(0x3012, 0x0123));

    this->sendCmd_READ_REGISTER(0, 0, 0x3000);
    this->dispatchAll();
    this->errorReply(CMD_READ_REGISTER, 0x02, 9);
    ASSERT_TLM_LastCameraError_SIZE(1);
    ASSERT_TLM_LastCameraError(0, CameraErrorInfo(CMD_READ_REGISTER, ErrorType::INVALID_PARAMETER, 9));
    ASSERT_TLM_CameraInvalidCmds(1, 9);
    ASSERT_EQ(this->tlmHistory_CameraErrors->at(this->tlmHistory_CameraErrors->size() - 1).arg, 1u);
    ASSERT_EQ(this->tlmHistory_Exchanges->at(this->tlmHistory_Exchanges->size() - 1).arg, 4u);
}

void SunCamMgrTester::testOutcomeEventPerCommand() {
    // OK outcomes
    this->setupNominal();  // SetupComplete checked in testSetupNominal
    this->sendCmd_PING(0, 1, PingType::MCU);
    this->dispatchAll();
    this->pingReply(1, 2, 3);
    ASSERT_EVENTS_PingReply(0, PingType::MCU, 1, 2, 3);
    this->sendCmd_MEMORY_STATUS(0, 2);
    this->dispatchAll();
    this->memoryStatusAllSd();
    ASSERT_EVENTS_MemoryStatusReply_SIZE(1);
    this->sendCmd_CAPTURE(0, 3, Slot::SLOT_1, TestPattern::OFF);
    this->dispatchAll();
    const U8 payload[2] = {0x02, 0xA0};
    this->reply(CMD_CAPTURE, MODE_SD, payload, 2);
    ASSERT_EVENTS_CaptureComplete(0, Slot::SLOT_1, TestPattern::OFF, DEFAULT_EXPOSURE, Mode::SD);
    this->sendCmd_READ_REGISTER(0, 4, 0x3012);
    this->dispatchAll();
    this->registerReply(0x3012, DEFAULT_EXPOSURE);
    ASSERT_EVENTS_RegisterRead_SIZE(1);
    ASSERT_EVENTS_RegisterRead(0, 0x3012, DEFAULT_EXPOSURE);
    this->sendCmd_SLEEP(0, 5);
    this->dispatchAll();
    this->ack(CMD_SET_MODE, MODE_SLEEP);
    ASSERT_EVENTS_SleepEntered_SIZE(1);
    ASSERT_CMD_RESPONSE_SIZE(5);
    for (U32 i = 0; i < 5; i++) {
        ASSERT_EQ(this->cmdResponseHistory->at(i).response, Fw::CmdResponse(Fw::CmdResponse::OK));
        ASSERT_EQ(this->cmdResponseHistory->at(i).cmdSeq, i + 1);
    }
    // rejected outcomes are reported by CommandRejected with the operation name
    this->clearHistory();
    this->sendCmd_CAPTURE(0, 6, Slot::SLOT_1, TestPattern::OFF);
    this->sendCmd_DOWNLOAD_ABORT(0, 7);
    this->dispatchAll();
    ASSERT_EVENTS_CommandRejected_SIZE(2);
    ASSERT_EVENTS_CommandRejected(0, Operation::CAPTURE, RejectCause::NOT_READY);
    ASSERT_EVENTS_CommandRejected(1, Operation::DOWNLOAD_ABORT, RejectCause::NO_DOWNLOAD);
    ASSERT_CMD_RESPONSE(0, SunCamMgr::OPCODE_CAPTURE, 6, Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_CMD_RESPONSE(1, SunCamMgr::OPCODE_DOWNLOAD_ABORT, 7, Fw::CmdResponse::EXECUTION_ERROR);
}

void SunCamMgrTester::testDownloadFullAndPreview() {
    this->setupNominal();
    // FULL
    this->startDownload(Slot::SLOT_2, Resolution::FULL, 1);
    ASSERT_PRODUCT_GET_SIZE(1);
    ASSERT_PRODUCT_GET(0, 0,
                       SunCamMgr::SIZE_OF_ImageHeader_RECORD +
                           FULL_LINES * SunCamMgr::SIZE_OF_ImageLine_RECORD(FULL_PIXELS + 3));
    ASSERT_EVENTS_DownloadStarted_SIZE(1);
    ASSERT_EVENTS_DownloadStarted(0, Slot::SLOT_2, Mode::SD, Resolution::FULL, FULL_LINES);
    for (U16 line = 0; line < FULL_LINES; line++) {
        this->assertFrame(CMD_DOWNLOAD, 2, 0, static_cast<U8>(line >> 8), static_cast<U8>(line & 0xFF), 7 + line);
        this->lineReply(line, FULL_PIXELS, static_cast<U8>(line));
    }
    ASSERT_EQ(this->m_frameCount, 7u + FULL_LINES - 1);
    ASSERT_EVENTS_DownloadComplete_SIZE(1);
    ASSERT_EVENTS_DownloadFailed_SIZE(0);
    const ImageHeader fullHeader(Slot::SLOT_2, Mode::SD, Resolution::FULL, 1280 / 2, 960 / 2, Mode::SD, FULL_LINES);
    ASSERT_EQ(this->checkProduct(fullHeader, FULL_PIXELS, FULL_LINES, 2), FULL_LINES);
    ASSERT_EVENTS_DownloadComplete(0, Slot::SLOT_2, Mode::SD, Resolution::FULL, FULL_LINES,
                                   static_cast<U32>(this->productGetHistory->at(0).size));
    ASSERT_TLM_ProductsSent(0, 1);
    ASSERT_EQ(this->tlmHistory_LinesDownloaded->at(this->tlmHistory_LinesDownloaded->size() - 1).arg, FULL_LINES);
    ASSERT_CMD_RESPONSE(0, SunCamMgr::OPCODE_DOWNLOAD, 1, Fw::CmdResponse::OK);
    ASSERT_EQ(this->tlmHistory_CurrentOperation->at(this->tlmHistory_CurrentOperation->size() - 1).arg,
              Operation::NONE);

    // PREVIEW
    this->clearHistory();
    this->m_frameCount = 0;
    this->startDownload(Slot::SLOT_4, Resolution::PREVIEW, 2);
    ASSERT_PRODUCT_GET(0, 0,
                       SunCamMgr::SIZE_OF_ImageHeader_RECORD +
                           PREVIEW_LINES * SunCamMgr::SIZE_OF_ImageLine_RECORD(PREVIEW_PIXELS + 3));
    for (U16 line = 0; line < PREVIEW_LINES; line++) {
        this->assertFrame(CMD_DOWNLOAD, 4, 1, 0, static_cast<U8>(line), 2 + line);
        this->lineReply(line, PREVIEW_PIXELS, static_cast<U8>(line));
    }
    const ImageHeader previewHeader(Slot::SLOT_4, Mode::SD, Resolution::PREVIEW, 80, 60, Mode::SD, PREVIEW_LINES);
    ASSERT_EQ(this->checkProduct(previewHeader, PREVIEW_PIXELS, PREVIEW_LINES, 4), PREVIEW_LINES);
    ASSERT_EVENTS_DownloadComplete_SIZE(1);
    ASSERT_TLM_ProductsSent(0, 2);
    ASSERT_CMD_RESPONSE(0, SunCamMgr::OPCODE_DOWNLOAD, 2, Fw::CmdResponse::OK);
}

void SunCamMgrTester::testDownloadHdImage() {
    this->paramSet_CAMERA_MODE(CaptureMode::HD, Fw::ParamValid::VALID);
    this->setupNominal(MODE_HD);
    this->sendCmd_DOWNLOAD(0, 1, Slot::SLOT_3, Resolution::PREVIEW);
    this->dispatchAll();
    this->assertFrame(CMD_MEMORY_STATUS, 0, 0, 0, 0, 6);
    const U8 modes[4] = {MODE_HD, MODE_HD, MODE_HD, MODE_HD};
    const U16 exposures[4] = {10, 10, 10, 10};
    const U16 uptimes[4] = {100, 101, 99, 100};  // within +-1 s
    this->memoryStatusReply(modes, exposures, uptimes, MODE_HD);
    const U16 hdLines = 4 * PREVIEW_LINES;
    ASSERT_PRODUCT_GET(0, 0,
                       SunCamMgr::SIZE_OF_ImageHeader_RECORD +
                           hdLines * SunCamMgr::SIZE_OF_ImageLine_RECORD(PREVIEW_PIXELS + 3));
    ASSERT_EVENTS_DownloadStarted(0, Slot::SLOT_3, Mode::HD, Resolution::PREVIEW, hdLines);
    for (U16 i = 0; i < hdLines; i++) {
        const U8 slot = static_cast<U8>(1 + i / PREVIEW_LINES);
        const U16 line = static_cast<U16>(i % PREVIEW_LINES);
        this->assertFrame(CMD_DOWNLOAD, slot, 1, 0, static_cast<U8>(line), 7 + i);
        this->lineReply(line, PREVIEW_PIXELS, static_cast<U8>(line), MODE_HD);
    }
    const ImageHeader header(Slot::SLOT_1, Mode::HD, Resolution::PREVIEW, 160, 120, Mode::HD, hdLines);
    ASSERT_EQ(this->checkProduct(header, PREVIEW_PIXELS, hdLines, 1), hdLines);
    ASSERT_EVENTS_DownloadComplete(0, Slot::SLOT_3, Mode::HD, Resolution::PREVIEW, hdLines,
                                   static_cast<U32>(this->productGetHistory->at(0).size));
    ASSERT_CMD_RESPONSE(0, SunCamMgr::OPCODE_DOWNLOAD, 1, Fw::CmdResponse::OK);
    // FULL HD header geometry and line count
    this->clearHistory();
    this->sendCmd_DOWNLOAD(0, 2, Slot::SLOT_1, Resolution::FULL);
    this->dispatchAll();
    this->memoryStatusReply(modes, exposures, uptimes, MODE_HD);
    ASSERT_EVENTS_DownloadStarted(0, Slot::SLOT_1, Mode::HD, Resolution::FULL, 4 * FULL_LINES);
    ASSERT_PRODUCT_GET(0, 0,
                       SunCamMgr::SIZE_OF_ImageHeader_RECORD +
                           4 * FULL_LINES * SunCamMgr::SIZE_OF_ImageLine_RECORD(FULL_PIXELS + 3));
    this->sendCmd_DOWNLOAD_ABORT(0, 3);
    this->dispatchAll();
    this->lineReply(0, FULL_PIXELS, 0, MODE_HD);
    const ImageHeader fullHeader(Slot::SLOT_1, Mode::HD, Resolution::FULL, 1280, 960, Mode::HD, 4 * FULL_LINES);
    ASSERT_EQ(this->checkProduct(fullHeader, FULL_PIXELS, 1, 1), 1u);
}

void SunCamMgrTester::testDownloadLineRetryAndFail() {
    this->setupNominal();
    this->startDownload(Slot::SLOT_1, Resolution::PREVIEW, 1);
    this->lineReply(0, PREVIEW_PIXELS, 0);
    this->lineReply(1, PREVIEW_PIXELS, 1);
    this->assertFrame(CMD_DOWNLOAD, 1, 1, 0, 2, 9);
    for (U8 attempt = 1; attempt <= KissCamConfig::LINE_RETRIES; attempt++) {
        this->tick(KissCamConfig::REPLY_TIMEOUT_TICKS);
        ASSERT_EVENTS_ReplyTimeout_SIZE(attempt);
        ASSERT_EVENTS_LineRetry_SIZE(attempt);
        ASSERT_EVENTS_LineRetry(attempt - 1, 2, attempt);
        this->assertFrame(CMD_DOWNLOAD, 1, 1, 0, 2, 9u + attempt);
        ASSERT_EVENTS_DownloadFailed_SIZE(0);
    }
    // an error reply on the last retry fails the download
    this->errorReply(CMD_DOWNLOAD, 0x02, 4);
    ASSERT_EVENTS_LineRetry_SIZE(KissCamConfig::LINE_RETRIES);
    ASSERT_EVENTS_DownloadFailed_SIZE(1);
    ASSERT_EVENTS_DownloadFailed(0, Slot::SLOT_1, 2, DownloadFailCause::LINE_FAILED);
    ASSERT_EVENTS_DownloadComplete_SIZE(0);
    ASSERT_CMD_RESPONSE(0, SunCamMgr::OPCODE_DOWNLOAD, 1, Fw::CmdResponse::EXECUTION_ERROR);
    const ImageHeader header(Slot::SLOT_1, Mode::SD, Resolution::PREVIEW, 80, 60, Mode::SD, PREVIEW_LINES);
    ASSERT_EQ(this->checkProduct(header, PREVIEW_PIXELS, 2, 1), 2u);
    ASSERT_TLM_ProductsSent(0, 1);
    ASSERT_EQ(this->m_frameCount, 9u + KissCamConfig::LINE_RETRIES);
    ASSERT_EQ(this->tlmHistory_CurrentOperation->at(this->tlmHistory_CurrentOperation->size() - 1).arg,
              Operation::NONE);
    ASSERT_EQ(this->tlmHistory_SetupState->at(this->tlmHistory_SetupState->size() - 1).arg, SetupState::READY);
}

void SunCamMgrTester::testDownloadNoImage() {
    this->setupNominal();
    // slot mode SLEEP
    this->sendCmd_DOWNLOAD(0, 1, Slot::SLOT_2, Resolution::FULL);
    this->dispatchAll();
    const U8 sleepModes[4] = {MODE_SD, MODE_SLEEP, MODE_SD, MODE_SD};
    const U16 exposures[4] = {1, 1, 1, 1};
    const U16 uptimes[4] = {5, 5, 5, 5};
    this->memoryStatusReply(sleepModes, exposures, uptimes);
    ASSERT_EVENTS_DownloadFailed_SIZE(1);
    ASSERT_EVENTS_DownloadFailed(0, Slot::SLOT_2, 0, DownloadFailCause::NO_IMAGE);
    ASSERT_CMD_RESPONSE(0, SunCamMgr::OPCODE_DOWNLOAD, 1, Fw::CmdResponse::EXECUTION_ERROR);
    // one quadrant SD
    const U8 mixedModes[4] = {MODE_HD, MODE_HD, MODE_SD, MODE_HD};
    this->sendCmd_DOWNLOAD(0, 2, Slot::SLOT_1, Resolution::FULL);
    this->dispatchAll();
    this->memoryStatusReply(mixedModes, exposures, uptimes);
    ASSERT_EVENTS_DownloadFailed_SIZE(2);
    ASSERT_EVENTS_DownloadFailed(1, Slot::SLOT_1, 0, DownloadFailCause::NO_IMAGE);
    // HD uptimes differing by more than 1 s
    const U8 hdModes[4] = {MODE_HD, MODE_HD, MODE_HD, MODE_HD};
    const U16 skewed[4] = {100, 100, 102, 100};
    this->sendCmd_DOWNLOAD(0, 3, Slot::SLOT_4, Resolution::PREVIEW);
    this->dispatchAll();
    this->memoryStatusReply(hdModes, exposures, skewed);
    ASSERT_EVENTS_DownloadFailed_SIZE(3);
    ASSERT_EVENTS_DownloadFailed(2, Slot::SLOT_4, 0, DownloadFailCause::NO_IMAGE);
    ASSERT_CMD_RESPONSE(2, SunCamMgr::OPCODE_DOWNLOAD, 3, Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_PRODUCT_GET_SIZE(0);
    ASSERT_PRODUCT_SEND_SIZE(0);
    ASSERT_EVENTS_DownloadStarted_SIZE(0);
    ASSERT_EQ(this->m_frameCount, 8u);  // 5 set-up + 3 Memory Status, no Download frame
    ASSERT_EQ(this->tlmHistory_SetupState->at(this->tlmHistory_SetupState->size() - 1).arg, SetupState::READY);
}

void SunCamMgrTester::testDownloadFailedSendAndProductFull() {
    this->setupNominal();
    // Download frame refused by the driver
    this->sendCmd_DOWNLOAD(0, 1, Slot::SLOT_1, Resolution::PREVIEW);
    this->dispatchAll();
    this->m_sendStatus = Drv::ByteStreamStatus::OTHER_ERROR;
    this->memoryStatusAllSd();
    this->m_sendStatus = Drv::ByteStreamStatus::OP_OK;
    ASSERT_EVENTS_SendFailed_SIZE(1);
    ASSERT_EVENTS_SendFailed(0, Operation::DOWNLOAD, Drv::ByteStreamStatus::OTHER_ERROR);
    ASSERT_EVENTS_DownloadFailed_SIZE(1);
    ASSERT_EVENTS_DownloadFailed(0, Slot::SLOT_1, 0, DownloadFailCause::SEND_FAILED);
    ASSERT_CMD_RESPONSE(0, SunCamMgr::OPCODE_DOWNLOAD, 1, Fw::CmdResponse::EXECUTION_ERROR);
    const ImageHeader header(Slot::SLOT_1, Mode::SD, Resolution::PREVIEW, 80, 60, Mode::SD, PREVIEW_LINES);
    ASSERT_EQ(this->checkProduct(header, PREVIEW_PIXELS, 0, 1), 0u);  // empty partial product delivered
    this->clearHistory();

    // Memory Status failure
    this->sendCmd_DOWNLOAD(0, 2, Slot::SLOT_1, Resolution::PREVIEW);
    this->dispatchAll();
    this->tick(KissCamConfig::REPLY_TIMEOUT_TICKS);
    ASSERT_EVENTS_DownloadFailed_SIZE(1);
    ASSERT_EVENTS_DownloadFailed(0, Slot::SLOT_1, 0, DownloadFailCause::MEMORY_STATUS_FAILED);
    ASSERT_PRODUCT_GET_SIZE(0);
    ASSERT_PRODUCT_SEND_SIZE(0);
    ASSERT_CMD_RESPONSE(0, SunCamMgr::OPCODE_DOWNLOAD, 2, Fw::CmdResponse::EXECUTION_ERROR);
    this->clearHistory();

    // Container full: a line reply longer than requested does not fit (parsed by length, declared 642 for a preview)
    this->startDownload(Slot::SLOT_1, Resolution::PREVIEW, 3);
    this->lineReply(0, PREVIEW_PIXELS, 0);
    this->lineReply(1, FULL_PIXELS, 1);  // oversize line: record does not fit the preview-sized container
    ASSERT_EVENTS_DownloadFailed_SIZE(1);
    ASSERT_EVENTS_DownloadFailed(0, Slot::SLOT_1, 1, DownloadFailCause::PRODUCT_FULL);
    ASSERT_EQ(this->checkProduct(header, PREVIEW_PIXELS, 1, 1), 1u);
    ASSERT_CMD_RESPONSE(0, SunCamMgr::OPCODE_DOWNLOAD, 3, Fw::CmdResponse::EXECUTION_ERROR);
}

void SunCamMgrTester::testDownloadAbortCommanded() {
    this->setupNominal();
    this->sendCmd_DOWNLOAD_ABORT(0, 1);
    this->dispatchAll();
    ASSERT_EVENTS_CommandRejected(0, Operation::DOWNLOAD_ABORT, RejectCause::NO_DOWNLOAD);
    ASSERT_CMD_RESPONSE(0, SunCamMgr::OPCODE_DOWNLOAD_ABORT, 1, Fw::CmdResponse::EXECUTION_ERROR);
    this->clearHistory();

    this->startDownload(Slot::SLOT_3, Resolution::FULL, 2);
    this->lineReply(0, FULL_PIXELS, 0);
    this->lineReply(1, FULL_PIXELS, 1);
    this->sendCmd_DOWNLOAD_ABORT(0, 3);
    this->dispatchAll();
    ASSERT_CMD_RESPONSE_SIZE(0);  // the pending line is still awaited
    ASSERT_EQ(this->m_frameCount, 9u);
    this->lineReply(2, FULL_PIXELS, 2);
    ASSERT_EQ(this->m_frameCount, 9u);  // no further request
    ASSERT_EVENTS_DownloadAborted_SIZE(1);
    ASSERT_EVENTS_DownloadAborted(0, Slot::SLOT_3, 3);
    ASSERT_EVENTS_DownloadFailed_SIZE(0);
    ASSERT_CMD_RESPONSE_SIZE(2);
    ASSERT_CMD_RESPONSE(0, SunCamMgr::OPCODE_DOWNLOAD, 2, Fw::CmdResponse::OK);
    ASSERT_CMD_RESPONSE(1, SunCamMgr::OPCODE_DOWNLOAD_ABORT, 3, Fw::CmdResponse::OK);
    const ImageHeader header(Slot::SLOT_3, Mode::SD, Resolution::FULL, 640, 480, Mode::SD, FULL_LINES);
    ASSERT_EQ(this->checkProduct(header, FULL_PIXELS, 3, 3), 3u);
    ASSERT_EQ(this->tlmHistory_CurrentOperation->at(this->tlmHistory_CurrentOperation->size() - 1).arg,
              Operation::NONE);
    // abort while the pending line times out: no retry, abort completes
    this->clearHistory();
    this->startDownload(Slot::SLOT_1, Resolution::PREVIEW, 4);
    this->sendCmd_DOWNLOAD_ABORT(0, 5);
    this->dispatchAll();
    this->tick(KissCamConfig::REPLY_TIMEOUT_TICKS);
    ASSERT_EVENTS_LineRetry_SIZE(0);
    ASSERT_EVENTS_DownloadAborted(0, Slot::SLOT_1, 0);
    ASSERT_CMD_RESPONSE(0, SunCamMgr::OPCODE_DOWNLOAD, 4, Fw::CmdResponse::OK);
    ASSERT_CMD_RESPONSE(1, SunCamMgr::OPCODE_DOWNLOAD_ABORT, 5, Fw::CmdResponse::OK);
}

void SunCamMgrTester::testReplyTimeoutTicks() {
    this->setupNominal();
    this->sendCmd_PING(0, 1, PingType::MCU);
    this->dispatchAll();
    this->tick(KissCamConfig::REPLY_TIMEOUT_TICKS - 1);
    ASSERT_EVENTS_ReplyTimeout_SIZE(0);
    ASSERT_CMD_RESPONSE_SIZE(0);
    this->tick();
    ASSERT_EVENTS_ReplyTimeout_SIZE(1);
    ASSERT_EVENTS_ReplyTimeout(0, Operation::PING, CMD_PING);
    ASSERT_TLM_Timeouts(0, 1);
    ASSERT_CMD_RESPONSE(0, SunCamMgr::OPCODE_PING, 1, Fw::CmdResponse::EXECUTION_ERROR);
    // a reply arriving on the last tick before expiry is accepted
    this->clearHistory();
    this->sendCmd_PING(0, 2, PingType::MCU);
    this->dispatchAll();
    this->tick(KissCamConfig::REPLY_TIMEOUT_TICKS - 1);
    this->pingReply(1, 0, 1);
    ASSERT_EVENTS_ReplyTimeout_SIZE(0);
    ASSERT_CMD_RESPONSE(0, SunCamMgr::OPCODE_PING, 2, Fw::CmdResponse::OK);
    // the timeout counter restarts per exchange
    this->tick(KissCamConfig::REPLY_TIMEOUT_TICKS * 2);
    ASSERT_EVENTS_ReplyTimeout_SIZE(0);
}

void SunCamMgrTester::testCameraErrorReply() {
    this->setupNominal();
    this->sendCmd_CAPTURE(0, 1, Slot::SLOT_1, TestPattern::OFF);
    this->dispatchAll();
    this->errorReply(CMD_CAPTURE, 0x03, 42);
    ASSERT_EVENTS_CameraError_SIZE(1);
    ASSERT_EVENTS_CameraError(0, Operation::CAPTURE, CMD_CAPTURE, ErrorType::WRONG_MODE, 42);
    ASSERT_TLM_LastCameraError(0, CameraErrorInfo(CMD_CAPTURE, ErrorType::WRONG_MODE, 42));
    ASSERT_TLM_CameraErrors(0, 1);
    ASSERT_EVENTS_CaptureComplete_SIZE(0);
    ASSERT_CMD_RESPONSE(0, SunCamMgr::OPCODE_CAPTURE, 1, Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_EQ(this->tlmHistory_SetupState->at(this->tlmHistory_SetupState->size() - 1).arg, SetupState::READY);
    // an error reply naming another command is not this exchange's reply
    this->sendCmd_PING(0, 2, PingType::MCU);
    this->dispatchAll();
    this->errorReply(CMD_CAPTURE, 0x01, 43);
    ASSERT_EVENTS_CameraError_SIZE(1);
    ASSERT_EVENTS_FrameDiscarded_SIZE(1);
    ASSERT_CMD_RESPONSE_SIZE(1);
    this->pingReply(1, 0, 1);
    ASSERT_CMD_RESPONSE(1, SunCamMgr::OPCODE_PING, 2, Fw::CmdResponse::OK);
}

void SunCamMgrTester::testMalformedAndStrayFrames() {
    this->setupNominal();
    // no operation in progress
    this->pingReply(1, 0, 1);
    ASSERT_EVENTS_FrameDiscarded_SIZE(1);
    ASSERT_EVENTS_FrameDiscarded(0, DiscardCause::NO_OPERATION, 10, head(0x40, 0x50, 0x01, 0x00, 0x04, 0x01, 0x00, 0x00));
    ASSERT_TLM_DiscardedBytes(0, 10);

    this->sendCmd_PING(0, 1, PingType::MCU);
    this->dispatchAll();
    // wrong command byte
    this->ack(CMD_SET_MODE);
    ASSERT_EVENTS_FrameDiscarded_SIZE(2);
    ASSERT_EVENTS_FrameDiscarded(1, DiscardCause::WRONG_COMMAND, 6, head(0x40, 0x4D, 0x01, 0x00, 0x00, 0x0D, 0, 0));
    ASSERT_TLM_DiscardedBytes(1, 16);
    // bad length: declared length larger than the assembler
    const U8 tooLong[6] = {START, CMD_PING, MODE_SD, 0x02, 0x83, END};
    this->receive(tooLong, sizeof tooLong);
    ASSERT_EVENTS_FrameDiscarded_SIZE(3);
    ASSERT_EVENTS_FrameDiscarded(2, DiscardCause::MALFORMED, 5, head(0x40, 0x50, 0x01, 0x02, 0x83, 0, 0, 0));
    ASSERT_TLM_MalformedReplies(0, 1);
    // missing terminator: the frame ends on a byte other than 0x0D, then resync on the next 0x40
    const U8 noEnd[10] = {START, CMD_PING, MODE_SD, 0x00, 0x04, 1, 2, 3, 4, 0x55};
    this->receive(noEnd, sizeof noEnd);
    ASSERT_EVENTS_FrameDiscarded_SIZE(4);
    ASSERT_EVENTS_FrameDiscarded(3, DiscardCause::MALFORMED, 10, head(0x40, 0x50, 0x01, 0x00, 0x04, 1, 2, 3));
    ASSERT_TLM_MalformedReplies(1, 2);
    // stray bytes before a start byte are discarded; the frame after them is accepted
    const U8 stray[13] = {0x11, 0x22, 0x33, START, CMD_PING, MODE_SD, 0x00, 0x04, 9, 8, 0x00, 0x07, END};
    this->receive(stray, sizeof stray);
    ASSERT_EVENTS_FrameDiscarded_SIZE(5);
    ASSERT_EVENTS_FrameDiscarded(4, DiscardCause::MALFORMED, 3, head(0x11, 0x22, 0x33, 0, 0, 0, 0, 0));
    ASSERT_EVENTS_PingReply_SIZE(1);
    ASSERT_EVENTS_PingReply(0, PingType::MCU, 9, 8, 7);
    ASSERT_CMD_RESPONSE(0, SunCamMgr::OPCODE_PING, 1, Fw::CmdResponse::OK);
    // late reply after the exchange timed out
    this->sendCmd_PING(0, 2, PingType::MCU);
    this->dispatchAll();
    this->tick(KissCamConfig::REPLY_TIMEOUT_TICKS);
    this->pingReply(1, 0, 1);
    ASSERT_EVENTS_FrameDiscarded_SIZE(6);
    ASSERT_EVENTS_FrameDiscarded(5, DiscardCause::NO_OPERATION, 10, head(0x40, 0x50, 0x01, 0x00, 0x04, 0x01, 0x00, 0x00));
    // non-OK buffer with data: discarded and returned; a null notice has nothing to return
    const U8 junk[4] = {1, 2, 3, 4};
    this->receive(junk, sizeof junk, Drv::ByteStreamStatus::OTHER_ERROR);
    ASSERT_EVENTS_FrameDiscarded_SIZE(7);
    ASSERT_EVENTS_FrameDiscarded(6, DiscardCause::MALFORMED, 4, head(1, 2, 3, 4, 0, 0, 0, 0));
    this->receive(nullptr, 0, Drv::ByteStreamStatus::RECV_NO_DATA);
    ASSERT_EVENTS_FrameDiscarded_SIZE(7);
    ASSERT_EQ(this->tlmHistory_DiscardedBytes->at(this->tlmHistory_DiscardedBytes->size() - 1).arg, 48u);
}

void SunCamMgrTester::testSplitAndCrPayloadReply() {
    this->setupNominal();
    this->startDownload(Slot::SLOT_1, Resolution::PREVIEW, 1);
    // one line reply (88 bytes) whose pixels are all 0x0D, delivered in three buffers
    U8 frame[88];
    frame[0] = START;
    frame[1] = CMD_DOWNLOAD;
    frame[2] = MODE_SD;
    frame[3] = 0x00;
    frame[4] = 0x52;
    frame[5] = 0x00;
    frame[6] = 0x00;
    ::memset(&frame[7], END, PREVIEW_PIXELS);
    frame[87] = END;
    this->receive(frame, 3);
    this->receive(&frame[3], 40);
    ASSERT_EQ(this->m_frameCount, 7u);  // still waiting for the rest of the line
    ASSERT_EVENTS_FrameDiscarded_SIZE(0);
    this->receive(&frame[43], 45);
    ASSERT_EVENTS_FrameDiscarded_SIZE(0);
    this->assertFrame(CMD_DOWNLOAD, 1, 1, 0, 1, 8);
    ASSERT_EQ(this->tlmHistory_LinesDownloaded->at(this->tlmHistory_LinesDownloaded->size() - 1).arg, 1u);
    // two replies in one buffer: the second line follows the first
    U8 two[2 * 88];
    ::memcpy(two, frame, 88);
    ::memcpy(&two[88], frame, 88);
    two[6] = 0x01;
    two[88 + 6] = 0x02;
    this->receive(two, sizeof two);
    this->assertFrame(CMD_DOWNLOAD, 1, 1, 0, 3, 10);
    ASSERT_EQ(this->tlmHistory_LinesDownloaded->at(this->tlmHistory_LinesDownloaded->size() - 1).arg, 3u);
    // a reply echoing the wrong line number is not this exchange's reply
    this->lineReply(7, PREVIEW_PIXELS, 0);
    ASSERT_EVENTS_FrameDiscarded_SIZE(1);
    ASSERT_EVENTS_FrameDiscarded(0, DiscardCause::WRONG_COMMAND, 88, head(0x40, 0x44, 0x01, 0x00, 0x52, 0x00, 0x07, 0x00));
    this->assertFrame(CMD_DOWNLOAD, 1, 1, 0, 3, 10);
}

void SunCamMgrTester::testModeMismatch() {
    this->setupNominal();
    this->sendCmd_PING(0, 1, PingType::MCU);
    this->dispatchAll();
    this->pingReply(1, 0, 1, MODE_HD);
    ASSERT_EVENTS_ModeMismatch_SIZE(1);
    ASSERT_EVENTS_ModeMismatch(0, Mode::SD, Mode::HD);
    ASSERT_EVENTS_PingReply_SIZE(0);
    ASSERT_TLM_CameraMode(0, Mode::HD);
    ASSERT_CMD_RESPONSE(0, SunCamMgr::OPCODE_PING, 1, Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_EQ(this->tlmHistory_SetupState->at(this->tlmHistory_SetupState->size() - 1).arg, SetupState::NOT_SETUP);
    // during set-up the mode is not yet commanded: the first ping may report Sleep (see setupNominal)
    // a push reply reporting the wrong mode fails the push with MODE_MISMATCH
    this->setupNominal();
    this->paramSet_EXPOSURE(10, Fw::ParamValid::VALID);
    this->paramSend_EXPOSURE(0, 0);
    this->tick();
    this->ack(CMD_SET_EXPOSURE, MODE_SLEEP);
    ASSERT_EVENTS_ModeMismatch(0, Mode::SD, Mode::SLEEP);
    ASSERT_EVENTS_SettingPushFailed(0, Setting::EXPOSURE, FailCause::MODE_MISMATCH);
    ASSERT_EQ(this->tlmHistory_SetupState->at(this->tlmHistory_SetupState->size() - 1).arg, SetupState::NOT_SETUP);
}

void SunCamMgrTester::testMemoryStatusAndRegister() {
    this->setupNominal();
    this->sendCmd_MEMORY_STATUS(0, 1);
    this->dispatchAll();
    this->assertFrame(CMD_MEMORY_STATUS, 0, 0, 0, 0, 6);
    const U8 modes[4] = {MODE_HD, MODE_HD, MODE_HD, MODE_HD};
    const U16 exposures[4] = {5, 6, 7, 8};
    const U16 uptimes[4] = {9, 10, 11, 12};
    this->memoryStatusReply(modes, exposures, uptimes);
    ASSERT_TLM_MemorySlots_SIZE(1);
    ASSERT_EQ(this->tlmHistory_MemorySlots->at(0).arg[3], MemorySlot(Mode::HD, 8, 12));
    ASSERT_CMD_RESPONSE(0, SunCamMgr::OPCODE_MEMORY_STATUS, 1, Fw::CmdResponse::OK);

    this->sendCmd_READ_REGISTER(0, 2, 0x3012);
    this->dispatchAll();
    this->assertFrame(CMD_READ_REGISTER, 0x30, 0x12, 0, 0, 7);
    // address echo mismatch: discarded, exchange still pending
    this->registerReply(0x3013, 0x0100);
    ASSERT_EVENTS_FrameDiscarded_SIZE(1);
    ASSERT_EVENTS_FrameDiscarded(0, DiscardCause::WRONG_COMMAND, 10, head(0x40, 0x52, 0x01, 0x00, 0x04, 0x30, 0x13, 0x01));
    ASSERT_EVENTS_RegisterRead_SIZE(0);
    this->registerReply(0x3012, 0x0100);
    ASSERT_EVENTS_RegisterRead(0, 0x3012, 0x0100);
    ASSERT_TLM_RegisterValue(0, RegisterValue(0x3012, 0x0100));
    ASSERT_TLM_ExposureReadback_SIZE(0);  // a commanded read is not the exposure readback
    ASSERT_CMD_RESPONSE(1, SunCamMgr::OPCODE_READ_REGISTER, 2, Fw::CmdResponse::OK);
}

void SunCamMgrTester::testModeChangeRepushesExposure() {
    this->setupNominal();
    this->paramSet_CAMERA_MODE(CaptureMode::HD, Fw::ParamValid::VALID);
    this->paramSend_CAMERA_MODE(0, 0);
    this->tick();
    this->assertFrame(CMD_SET_MODE, MODE_HD, 0, 0, 0, 6);
    this->ack(CMD_SET_MODE, MODE_HD);
    ASSERT_EVENTS_SettingApplied(0, Setting::CAMERA_MODE, 2);
    this->assertFrame(CMD_SET_EXPOSURE, 0x02, 0xA0, 0, 0, 7);
    this->ack(CMD_SET_EXPOSURE, MODE_HD);
    ASSERT_EVENTS_SettingApplied(1, Setting::EXPOSURE, DEFAULT_EXPOSURE);
    this->assertFrame(CMD_READ_REGISTER, 0x30, 0x12, 0, 0, 8);
    this->registerReply(EXPOSURE_REGISTER, DEFAULT_EXPOSURE, MODE_HD);
    ASSERT_EVENTS_ExposureReadback(0, DEFAULT_EXPOSURE, DEFAULT_EXPOSURE);
    ASSERT_EQ(this->tlmHistory_SetupState->at(this->tlmHistory_SetupState->size() - 1).arg, SetupState::READY);
    this->tick(3);
    ASSERT_EQ(this->m_frameCount, 8u);
    // the new mode is now the commanded mode for the mode check
    this->sendCmd_PING(0, 1, PingType::MCU);
    this->dispatchAll();
    this->pingReply(1, 0, 1, MODE_HD);
    ASSERT_EVENTS_ModeMismatch_SIZE(0);
    ASSERT_CMD_RESPONSE(0, SunCamMgr::OPCODE_PING, 1, Fw::CmdResponse::OK);
}

void SunCamMgrTester::testSendAndAllocationFailure() {
    // send failure at set-up
    this->m_sendStatus = Drv::ByteStreamStatus::SEND_RETRY;
    this->sendSetup(1);
    ASSERT_EVENTS_SendFailed(0, Operation::SETUP_CAMERA, Drv::ByteStreamStatus::SEND_RETRY);
    ASSERT_EVENTS_SetupFailed(0, SetupStep::PING_MCU, FailCause::SEND_FAILED);
    ASSERT_CMD_RESPONSE(0, SunCamMgr::OPCODE_SETUP_CAMERA, 1, Fw::CmdResponse::EXECUTION_ERROR);
    this->m_sendStatus = Drv::ByteStreamStatus::OP_OK;
    this->clearHistory();
    this->setupNominal();
    // send failure on a command
    this->m_sendStatus = Drv::ByteStreamStatus::OTHER_ERROR;
    this->sendCmd_PING(0, 2, PingType::MCU);
    this->dispatchAll();
    ASSERT_EVENTS_SendFailed(0, Operation::PING, Drv::ByteStreamStatus::OTHER_ERROR);
    ASSERT_CMD_RESPONSE(0, SunCamMgr::OPCODE_PING, 2, Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_EQ(this->tlmHistory_CurrentOperation->at(this->tlmHistory_CurrentOperation->size() - 1).arg,
              Operation::NONE);
    // send failure on a push
    this->paramSet_EXPOSURE(10, Fw::ParamValid::VALID);
    this->paramSend_EXPOSURE(0, 0);
    this->tick();
    ASSERT_EVENTS_SettingPushFailed(0, Setting::EXPOSURE, FailCause::SEND_FAILED);
    ASSERT_EQ(this->tlmHistory_SetupState->at(this->tlmHistory_SetupState->size() - 1).arg, SetupState::NOT_SETUP);
    this->m_sendStatus = Drv::ByteStreamStatus::OP_OK;
    this->clearHistory();
    // product allocation failure
    this->setupNominal();
    this->m_productGetStatus = Fw::Success::FAILURE;
    this->sendCmd_DOWNLOAD(0, 3, Slot::SLOT_1, Resolution::FULL);
    this->dispatchAll();
    this->memoryStatusAllSd();
    ASSERT_EVENTS_ProductAllocationFailed_SIZE(1);
    ASSERT_EVENTS_ProductAllocationFailed(
        0, static_cast<U32>(SunCamMgr::SIZE_OF_ImageHeader_RECORD +
                            FULL_LINES * SunCamMgr::SIZE_OF_ImageLine_RECORD(FULL_PIXELS + 3)));
    ASSERT_EVENTS_DownloadStarted_SIZE(0);
    ASSERT_PRODUCT_SEND_SIZE(0);
    ASSERT_CMD_RESPONSE(0, SunCamMgr::OPCODE_DOWNLOAD, 3, Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_EQ(this->m_frameCount, 11u);  // no Download frame
    ASSERT_EQ(this->tlmHistory_SetupState->at(this->tlmHistory_SetupState->size() - 1).arg, SetupState::READY);
}

void SunCamMgrTester::testQueueFullHooks() {
    // fill the queue with ticks that will not be dispatched, then overflow each input
    for (U32 i = 0; i < TEST_INSTANCE_QUEUE_DEPTH; i++) {
        this->sendCmd_PING(0, i, PingType::MCU);
    }
    this->sendCmd_CAPTURE(0, 100, Slot::SLOT_1, TestPattern::OFF);
    this->sendCmd_SLEEP(0, 101);
    ASSERT_CMD_RESPONSE_SIZE(2);
    ASSERT_CMD_RESPONSE(0, SunCamMgr::OPCODE_CAPTURE, 100, Fw::CmdResponse::BUSY);
    ASSERT_CMD_RESPONSE(1, SunCamMgr::OPCODE_SLEEP, 101, Fw::CmdResponse::BUSY);
    const U8 bytes[3] = {1, 2, 3};
    Fw::Buffer buffer(this->m_rxStore, sizeof bytes);
    ::memcpy(this->m_rxStore, bytes, sizeof bytes);
    this->invoke_to_drvReceiveIn(0, buffer, Drv::ByteStreamStatus::OP_OK);
    ASSERT_from_drvReceiveReturnOut_SIZE(1);
    this->invoke_to_pingIn(0, 7);
    this->invoke_to_pingIn(0, 8);
    this->invoke_to_pingIn(0, 9);
    this->invoke_to_run(0, 0);
    this->invoke_to_run(0, 0);  // second tick is not even queued (atomic set)
    this->dispatchAll();
    ASSERT_from_pingOut_SIZE(0);
    ASSERT_EVENTS_QueueOverflow_SIZE(0);
    // the next tick reports the drops per source
    this->tick();
    ASSERT_EVENTS_QueueOverflow_SIZE(4);
    ASSERT_EVENTS_QueueOverflow(0, QueueInput::COMMAND, 2);
    ASSERT_EVENTS_QueueOverflow(1, QueueInput::UART_RECEIVE, 1);
    ASSERT_EVENTS_QueueOverflow(2, QueueInput::RUN_TICK, 1);
    ASSERT_EVENTS_QueueOverflow(3, QueueInput::HEALTH_PING, 3);
    ASSERT_EQ(this->tlmHistory_QueueOverflows->at(this->tlmHistory_QueueOverflows->size() - 1).arg, 7u);
    this->tick();
    ASSERT_EVENTS_QueueOverflow_SIZE(4);
}

void SunCamMgrTester::testPeriodicChannelWrite() {
    this->tick();
    ASSERT_TLM_SetupState_SIZE(1);
    ASSERT_TLM_CurrentOperation_SIZE(1);
    ASSERT_TLM_Exchanges_SIZE(1);
    ASSERT_TLM_Exchanges(0, 0);
    ASSERT_TLM_QueueOverflows(0, 0);
    ASSERT_TLM_CameraMode_SIZE(0);  // camera channels only on reply
    this->tick(KissCamConfig::TLM_PERIOD_TICKS - 1);
    ASSERT_TLM_SetupState_SIZE(1);
    this->tick();
    ASSERT_TLM_SetupState_SIZE(2);
    ASSERT_TLM_CurrentOperation_SIZE(2);
    ASSERT_TLM_Exchanges_SIZE(2);
    ASSERT_TLM_CameraErrors_SIZE(2);
    ASSERT_TLM_Timeouts_SIZE(2);
    ASSERT_TLM_DiscardedBytes_SIZE(2);
    ASSERT_TLM_MalformedReplies_SIZE(2);
    ASSERT_TLM_ImagesCaptured_SIZE(2);
    ASSERT_TLM_LinesDownloaded_SIZE(2);
    ASSERT_TLM_ProductsSent_SIZE(2);
    ASSERT_TLM_QueueOverflows_SIZE(2);
    ASSERT_TLM_CameraMode_SIZE(0);
    // on change
    this->clearHistory();
    this->sendSetup();
    ASSERT_TLM_SetupState_SIZE(1);
    ASSERT_TLM_SetupState(0, SetupState::SETTING_UP);
    ASSERT_TLM_CurrentOperation(0, Operation::SETUP_CAMERA);
    ASSERT_TLM_Exchanges(0, 1);
}

void SunCamMgrTester::testExposureRangeCheck() {
    this->setupNominal();
    const U16 rejected[2] = {KissCamConfig::EXPOSURE_MIN - 1, KissCamConfig::EXPOSURE_MAX + 1};
    for (U32 i = 0; i < 2; i++) {
        this->paramSet_EXPOSURE(rejected[i], Fw::ParamValid::VALID);
        this->paramSend_EXPOSURE(0, 0);
        this->tick(2);
        ASSERT_EVENTS_SettingRejected_SIZE(i + 1);
        ASSERT_EVENTS_SettingRejected(i, Setting::EXPOSURE, rejected[i], KissCamConfig::EXPOSURE_MIN,
                                      KissCamConfig::EXPOSURE_MAX);
        ASSERT_EQ(this->m_frameCount, 5u);
        ASSERT_EQ(this->tlmHistory_SetupState->at(this->tlmHistory_SetupState->size() - 1).arg, SetupState::READY);
    }
    const U16 accepted[2] = {KissCamConfig::EXPOSURE_MIN, KissCamConfig::EXPOSURE_MAX};
    for (U32 i = 0; i < 2; i++) {
        this->paramSet_EXPOSURE(accepted[i], Fw::ParamValid::VALID);
        this->paramSend_EXPOSURE(0, 0);
        this->tick();
        this->assertFrame(CMD_SET_EXPOSURE, static_cast<U8>(accepted[i] >> 8), static_cast<U8>(accepted[i] & 0xFF), 0,
                          0, 6 + 2 * i);
        this->ack(CMD_SET_EXPOSURE);
        this->registerReply(EXPOSURE_REGISTER, accepted[i]);
    }
    ASSERT_EVENTS_SettingApplied_SIZE(2);
    ASSERT_EVENTS_SettingRejected_SIZE(2);
    // persisted out-of-range value fails set-up at SET_EXPOSURE
    this->paramSet_EXPOSURE(KissCamConfig::EXPOSURE_MAX + 1, Fw::ParamValid::VALID);
    this->clearHistory();
    this->sendSetup(5);
    this->pingReply(1, 0, 1);
    this->ack(CMD_SET_MODE);
    ASSERT_EVENTS_SettingRejected_SIZE(1);
    ASSERT_EVENTS_SetupFailed(0, SetupStep::SET_EXPOSURE, FailCause::OUT_OF_RANGE);
    ASSERT_EQ(this->m_frameCount, 11u);
}

void SunCamMgrTester::testRunPortSingleQueuedTick() {
    for (U32 i = 0; i < 5; i++) {
        this->invoke_to_run(0, 0);
    }
    // exactly one message is queued: the first dispatch runs the tick, the second finds the queue empty
    ASSERT_EQ(this->component.doDispatch(), Fw::QueuedComponentBase::MSG_DISPATCH_OK);
    ASSERT_TLM_SetupState_SIZE(1);
    ASSERT_EQ(this->component.doDispatch(), Fw::QueuedComponentBase::MSG_DISPATCH_EMPTY);
    // after dispatch, the next run queues again
    this->invoke_to_run(0, 0);
    ASSERT_EQ(this->component.doDispatch(), Fw::QueuedComponentBase::MSG_DISPATCH_OK);
    // the overflow hook clears the atomic: a tick dropped from a full queue does not block later ticks
    for (U32 i = 0; i < TEST_INSTANCE_QUEUE_DEPTH; i++) {
        this->invoke_to_pingIn(0, i);
    }
    this->invoke_to_run(0, 0);  // dropped by the hook
    this->dispatchAll();
    this->invoke_to_run(0, 0);
    ASSERT_EQ(this->component.doDispatch(), Fw::QueuedComponentBase::MSG_DISPATCH_OK);
    ASSERT_EVENTS_QueueOverflow_SIZE(1);
    ASSERT_EVENTS_QueueOverflow(0, QueueInput::RUN_TICK, 1);
}

void SunCamMgrTester::testHealthPingAndDrvConnected() {
    this->invoke_to_pingIn(0, 0xCAFE);
    this->dispatchAll();
    ASSERT_from_pingOut_SIZE(1);
    ASSERT_from_pingOut(0, 0xCAFE);
    this->invoke_to_drvConnected(0);
    this->dispatchAll();
    ASSERT_EQ(this->m_frameCount, 0u);
    ASSERT_EVENTS_SIZE(0);
    this->setupNominal();
    this->invoke_to_drvConnected(0);
    this->dispatchAll();
    ASSERT_EQ(this->m_frameCount, 5u);
    ASSERT_EQ(this->tlmHistory_SetupState->at(this->tlmHistory_SetupState->size() - 1).arg, SetupState::READY);
}

}  // namespace KissCam

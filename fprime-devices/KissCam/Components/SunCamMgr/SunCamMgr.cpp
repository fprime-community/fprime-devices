// ======================================================================
// \title  SunCamMgr.cpp
// \author ubuntu
// \brief  cpp file for SunCamMgr component implementation class
// ======================================================================

#include "fprime-devices/KissCam/Components/SunCamMgr/SunCamMgr.hpp"

namespace KissCam {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

SunCamMgr ::SunCamMgr(const char* const compName) : SunCamMgrComponentBase(compName) {}

SunCamMgr ::~SunCamMgr() {}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void SunCamMgr ::drvConnected_handler(FwIndexType portNum) {
    // TODO
}

void SunCamMgr ::drvReceiveIn_handler(FwIndexType portNum, Fw::Buffer& buffer, const Drv::ByteStreamStatus& status) {
    // TODO
}

void SunCamMgr ::pingIn_handler(FwIndexType portNum, U32 key) {
    // TODO
}

void SunCamMgr ::run_handler(FwIndexType portNum, U32 context) {
    // TODO
}

// ----------------------------------------------------------------------
// Handler implementations for commands
// ----------------------------------------------------------------------

void SunCamMgr ::SETUP_CAMERA_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    // TODO
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void SunCamMgr ::SLEEP_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    // TODO
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void SunCamMgr ::PING_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, const KissCam::PingType& pingType) {
    // TODO
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void SunCamMgr ::MEMORY_STATUS_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    // TODO
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void SunCamMgr ::CAPTURE_cmdHandler(FwOpcodeType opCode,
                                    U32 cmdSeq,
                                    const KissCam::Slot& slot,
                                    const KissCam::TestPattern& testPattern) {
    // TODO
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void SunCamMgr ::DOWNLOAD_cmdHandler(FwOpcodeType opCode,
                                     U32 cmdSeq,
                                     const KissCam::Slot& slot,
                                     const KissCam::Resolution& resolution) {
    // TODO
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void SunCamMgr ::DOWNLOAD_ABORT_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    // TODO
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void SunCamMgr ::READ_REGISTER_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, U16 address) {
    // TODO
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

// ----------------------------------------------------------------------
// Handler implementations for user-defined internal interfaces
// ----------------------------------------------------------------------

void SunCamMgr ::runInternal_internalInterfaceHandler() {
    // TODO
}

// ----------------------------------------------------------------------
// Overflow hook implementations for typed input ports
// ----------------------------------------------------------------------

void SunCamMgr ::drvReceiveIn_overflowHook(FwIndexType portNum,
                                           Fw::Buffer& buffer,
                                           const Drv::ByteStreamStatus& status) {
    // TODO
}

void SunCamMgr ::pingIn_overflowHook(FwIndexType portNum, U32 key) {
    // TODO
}

// ----------------------------------------------------------------------
// Overflow hook implementations for internal ports
// ----------------------------------------------------------------------

void SunCamMgr ::runInternal_overflowHook() {
    // TODO
}

// ----------------------------------------------------------------------
// Overflow hook implementations for commands
// ----------------------------------------------------------------------

void SunCamMgr ::SETUP_CAMERA_cmdOverflowHook(FwOpcodeType opCode, U32 cmdSeq) {
    // TODO
}

void SunCamMgr ::SLEEP_cmdOverflowHook(FwOpcodeType opCode, U32 cmdSeq) {
    // TODO
}

void SunCamMgr ::PING_cmdOverflowHook(FwOpcodeType opCode, U32 cmdSeq) {
    // TODO
}

void SunCamMgr ::MEMORY_STATUS_cmdOverflowHook(FwOpcodeType opCode, U32 cmdSeq) {
    // TODO
}

void SunCamMgr ::CAPTURE_cmdOverflowHook(FwOpcodeType opCode, U32 cmdSeq) {
    // TODO
}

void SunCamMgr ::DOWNLOAD_cmdOverflowHook(FwOpcodeType opCode, U32 cmdSeq) {
    // TODO
}

void SunCamMgr ::DOWNLOAD_ABORT_cmdOverflowHook(FwOpcodeType opCode, U32 cmdSeq) {
    // TODO
}

void SunCamMgr ::READ_REGISTER_cmdOverflowHook(FwOpcodeType opCode, U32 cmdSeq) {
    // TODO
}

}  // namespace KissCam

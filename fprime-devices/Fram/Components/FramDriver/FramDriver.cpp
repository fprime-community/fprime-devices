// ======================================================================
// \title  FramDriver.cpp
// \brief  Driver for the Infineon CY15B102QN serial F-RAM over an asynchronous
// SPI driver (SDD docs/sdd.md)
// ======================================================================

#include "fprime-devices/Fram/Components/FramDriver/FramDriver.hpp"

#include <cstring>

#include "Fw/Types/Assert.hpp"

namespace Fram {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

FramDriver ::FramDriver(const char* const compName)
    : FramDriverComponentBase(compName),
      m_phase(Phase::IDLE),
      m_client(0),
      m_op(Fram::Op::READ),
      m_offset(0),
      m_clientBuffer(),
      m_issuing(false),
      m_inCallPending(false),
      m_inCallChain(false),
      m_inCallStatus(Drv::SpiStatus::SPI_OK),
      m_startupFailed(false),
      m_startupState(Fram::StartupState::NOT_STARTED),
      m_protectStart(PROTECT_NONE),
      m_readsCompleted(0),
      m_writesCompleted(0),
      m_requestsRejected(0),
      m_spiErrors(0),
      m_tx(),
      m_rx(),
      m_txBuffer(m_tx, FRAME_BYTES),
      m_rxBuffer(m_rx, FRAME_BYTES) {}

FramDriver ::~FramDriver() {}

// ----------------------------------------------------------------------
// Startup
// ----------------------------------------------------------------------

void FramDriver ::startup() {
    // Programming error: the deployment calls startup() exactly once, before
    // clients run
    FW_ASSERT(this->m_startupState == Fram::StartupState::NOT_STARTED,
              static_cast<FwAssertArgType>(this->m_startupState));
    FW_ASSERT(this->m_phase == Phase::IDLE, static_cast<FwAssertArgType>(this->m_phase));
    this->m_startupState = Fram::StartupState::IN_PROGRESS;
    this->m_startupFailed = false;
    this->tlmWrite_StartupState(this->m_startupState);
    this->tlmWrite_DeviceIdMatch(false);
    this->tlmWrite_StatusRegister(0);

    this->m_phase = Phase::STARTUP_RDID;
    (void)::memset(this->m_tx, 0, sizeof(this->m_tx));
    this->m_tx[0] = OPCODE_RDID;
    this->issue(RDID_FRAME_BYTES);
    this->drain();
}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

Fram::Status FramDriver ::read_handler(FwIndexType portNum, U32 offset, Fw::Buffer& buffer) {
    return this->request(Fram::Op::READ, portNum, offset, buffer);
}

Fram::Status FramDriver ::write_handler(FwIndexType portNum, U32 offset, Fw::Buffer& buffer) {
    return this->request(Fram::Op::WRITE, portNum, offset, buffer);
}

void FramDriver ::spiWriteReadComplete_handler(FwIndexType portNum,
                                               Fw::Buffer& writeBuffer,
                                               Fw::Buffer& readBuffer,
                                               const Drv::SpiStatus& status) {
    if (this->m_issuing) {
        // Delivered inside our own spiWriteRead call: record only, the issuer acts
        // after the call returns
        this->m_inCallPending = true;
        this->m_inCallStatus = status;
        return;
    }
    // Exactly one completion follows every request (Drv.SpiWriteReadRequest
    // contract): one with nothing outstanding is a peer-contract breach, a
    // programming error
    FW_ASSERT(this->m_phase != Phase::IDLE);
    this->process(status);
    this->drain();
}

// ----------------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------------

Fram::Status FramDriver ::request(Fram::Op op, FwIndexType portNum, U32 offset, Fw::Buffer& buffer) {
    const FwSizeType length = buffer.getSize();
    if (this->m_phase != Phase::IDLE) {
        this->m_requestsRejected++;
        this->tlmWrite_RequestsRejected(this->m_requestsRejected);
        this->log_WARNING_LO_RequestRejected(op, portNum, offset, length, Fram::Status::BUSY,
                                             Fram::RejectCause::TRANSFER_IN_PROGRESS);
        return Fram::Status::BUSY;
    }
    if (this->m_inCallChain) {
        // Issued from a completion that was itself delivered inside a request call
        // (a driver completing in-call): accepting it would recurse once per
        // chained request, so it is rejected to bound the recursion (REQ-010 holds
        // for completions delivered from the driver's own entry, the deployment
        // constraint)
        this->m_requestsRejected++;
        this->tlmWrite_RequestsRejected(this->m_requestsRejected);
        this->log_WARNING_LO_RequestRejected(op, portNum, offset, length, Fram::Status::BUSY,
                                             Fram::RejectCause::REENTRANT_IN_CALL);
        return Fram::Status::BUSY;
    }
    // Idle: the request's identity is recorded before any check so reject() can
    // report it
    this->m_client = portNum;
    this->m_op = op;
    this->m_offset = offset;
    this->m_clientBuffer = buffer;
    if ((buffer.getData() == nullptr) || (length == 0) || (length > Fram::FramConfig::MaxTransfer) ||
        (offset > (static_cast<U32>(Fram::FramConfig::Size) - static_cast<U32>(length)))) {
        return this->reject(Fram::Status::OUT_OF_RANGE, Fram::RejectCause::OUT_OF_RANGE);
    }
    // offset + length <= Size here, so the sum cannot wrap
    if ((op == Fram::Op::WRITE) && ((offset + static_cast<U32>(length)) > this->m_protectStart)) {
        return this->reject(Fram::Status::WRITE_PROTECTED, Fram::RejectCause::WRITE_PROTECTED);
    }

    (void)::memset(this->m_tx, 0, sizeof(this->m_tx));
    FwSizeType frameLength = 0;
    if (op == Fram::Op::READ) {
        this->m_phase = Phase::READ;
        this->setHeader(OPCODE_READ, offset);
        frameLength = HEADER_BYTES + length;
    } else {
        this->m_phase = Phase::WRITE_WREN;
        this->m_tx[0] = OPCODE_WREN;
        frameLength = WREN_FRAME_BYTES;
    }
    this->issue(frameLength);
    if (this->m_inCallPending && (this->m_inCallStatus != Drv::SpiStatus::SPI_OK)) {
        // The SPI driver refused the first frame inside the call (REQ-021): no
        // completion follows
        this->m_inCallPending = false;
        if (this->m_inCallStatus == Drv::SpiStatus::SPI_BUSY_ERR) {
            return this->reject(Fram::Status::BUSY, Fram::RejectCause::SPI_DRIVER_BUSY);
        }
        this->m_phase = Phase::IDLE;
        this->m_spiErrors++;
        this->tlmWrite_SpiErrors(this->m_spiErrors);
        this->log_WARNING_HI_SpiError(op, portNum, offset, length, this->m_inCallStatus,
                                      Fram::SpiErrorSite::REQUEST_REFUSED);
        return Fram::Status::SPI_ERROR;
    }
    // Accepted. An in-call SPI_OK is acted on now, as if it had arrived later
    // (the completion may be delivered before this call returns).
    this->drain();
    return Fram::Status::ACCEPTED;
}

Fram::Status FramDriver ::reject(Fram::Status status, Fram::RejectCause cause) {
    this->m_phase = Phase::IDLE;
    this->m_requestsRejected++;
    this->tlmWrite_RequestsRejected(this->m_requestsRejected);
    this->log_WARNING_LO_RequestRejected(this->m_op, this->m_client, this->m_offset, this->m_clientBuffer.getSize(),
                                         status, cause);
    return status;
}

void FramDriver ::issue(FwSizeType length) {
    FW_ASSERT(length <= FRAME_BYTES, static_cast<FwAssertArgType>(length));
    FW_ASSERT(!this->m_issuing);
    (void)::memset(this->m_rx, 0, sizeof(this->m_rx));
    // The views keep their full capacity; only the represented size changes per
    // frame
    this->m_txBuffer.setSize(length);
    this->m_rxBuffer.setSize(length);
    this->m_inCallPending = false;
    this->m_issuing = true;
    this->spiWriteRead_out(0, this->m_txBuffer, this->m_rxBuffer);
    this->m_issuing = false;
}

void FramDriver ::drain() {
    // Each process() may issue one more frame, which may again complete in-call:
    // bounded by the frames per transfer
    this->m_inCallChain = true;
    for (FwSizeType i = 0; (i < MAX_FRAMES_PER_TRANSFER) && this->m_inCallPending; i++) {
        this->m_inCallPending = false;
        this->process(this->m_inCallStatus);
    }
    this->m_inCallChain = false;
    FW_ASSERT(!this->m_inCallPending);
}

void FramDriver ::process(Drv::SpiStatus status) {
    switch (this->m_phase) {
        case Phase::STARTUP_RDID: {
            if (status == Drv::SpiStatus::SPI_OK) {
                U8 id[Fram::FramConfig::DeviceIdSize] = {};
                (void)::memcpy(id, &this->m_rx[1], sizeof(id));
                const bool match = (::memcmp(id, Fram::FramConfig::EXPECTED_DEVICE_ID, sizeof(id)) == 0);
                this->tlmWrite_DeviceIdMatch(match);
                if (!match) {
                    this->log_WARNING_HI_DeviceIdMismatch(Fram::DeviceId(id));
                }
            } else {
                this->m_startupFailed = true;
                this->log_WARNING_HI_StartupReadFailed(Fram::StartupRead::DEVICE_ID, status);
            }
            this->m_phase = Phase::STARTUP_RDSR;
            (void)::memset(this->m_tx, 0, sizeof(this->m_tx));
            this->m_tx[0] = OPCODE_RDSR;
            this->issue(RDSR_FRAME_BYTES);
            break;
        }
        case Phase::STARTUP_RDSR: {
            if (status == Drv::SpiStatus::SPI_OK) {
                const U8 statusRegister = this->m_rx[1];
                this->m_protectStart = FramDriver::protectStart(statusRegister);
                this->tlmWrite_StatusRegister(statusRegister);
                if (this->m_protectStart != PROTECT_NONE) {
                    this->log_WARNING_HI_BlockProtectSet(statusRegister);
                }
            } else {
                this->m_startupFailed = true;
                this->m_protectStart = PROTECT_NONE;
                this->log_WARNING_HI_StartupReadFailed(Fram::StartupRead::STATUS_REGISTER, status);
            }
            this->m_phase = Phase::IDLE;
            this->m_startupState =
                this->m_startupFailed ? Fram::StartupState::READ_FAILED : Fram::StartupState::COMPLETE;
            this->tlmWrite_StartupState(this->m_startupState);
            this->log_ACTIVITY_LO_StartupComplete();
            break;
        }
        case Phase::READ: {
            if (status == Drv::SpiStatus::SPI_OK) {
                (void)::memcpy(this->m_clientBuffer.getData(), &this->m_rx[HEADER_BYTES],
                               this->m_clientBuffer.getSize());
                this->m_readsCompleted++;
                this->tlmWrite_ReadsCompleted(this->m_readsCompleted);
                this->finishClient(Fram::Status::OK);
            } else {
                this->m_spiErrors++;
                this->tlmWrite_SpiErrors(this->m_spiErrors);
                this->log_WARNING_HI_SpiError(this->m_op, this->m_client, this->m_offset,
                                              this->m_clientBuffer.getSize(), status, Fram::SpiErrorSite::READ_FRAME);
                this->finishClient(Fram::Status::SPI_ERROR);
            }
            break;
        }
        case Phase::WRITE_WREN: {
            if (status == Drv::SpiStatus::SPI_OK) {
                this->m_phase = Phase::WRITE_DATA;
                const FwSizeType length = this->m_clientBuffer.getSize();
                (void)::memset(this->m_tx, 0, sizeof(this->m_tx));
                this->setHeader(OPCODE_WRITE, this->m_offset);
                (void)::memcpy(&this->m_tx[HEADER_BYTES], this->m_clientBuffer.getData(), length);
                this->issue(HEADER_BYTES + length);
            } else {
                this->m_spiErrors++;
                this->tlmWrite_SpiErrors(this->m_spiErrors);
                this->log_WARNING_HI_SpiError(this->m_op, this->m_client, this->m_offset,
                                              this->m_clientBuffer.getSize(), status, Fram::SpiErrorSite::WREN_FRAME);
                this->finishClient(Fram::Status::SPI_ERROR);
            }
            break;
        }
        case Phase::WRITE_DATA: {
            if (status == Drv::SpiStatus::SPI_OK) {
                this->m_writesCompleted++;
                this->tlmWrite_WritesCompleted(this->m_writesCompleted);
                this->finishClient(Fram::Status::OK);
            } else {
                this->m_spiErrors++;
                this->tlmWrite_SpiErrors(this->m_spiErrors);
                this->log_WARNING_HI_SpiError(this->m_op, this->m_client, this->m_offset,
                                              this->m_clientBuffer.getSize(), status, Fram::SpiErrorSite::WRITE_FRAME);
                this->finishClient(Fram::Status::SPI_ERROR);
            }
            break;
        }
        case Phase::IDLE:
        default:
            FW_ASSERT(0, static_cast<FwAssertArgType>(this->m_phase));
            break;
    }
}

void FramDriver ::finishClient(Fram::Status status) {
    // Copy out, idle first: the completion may make a new request re-entrantly
    // (REQ-010)
    const FwIndexType client = this->m_client;
    const Fram::Op op = this->m_op;
    const U32 offset = this->m_offset;
    Fw::Buffer buffer = this->m_clientBuffer;
    this->m_phase = Phase::IDLE;
    // `match` in the FPP forces both connections of a pair; an unconnected port
    // is a topology error and asserts
    if (op == Fram::Op::READ) {
        this->readComplete_out(client, offset, buffer, status);
    } else {
        this->writeComplete_out(client, offset, buffer, status);
    }
}

void FramDriver ::setHeader(U8 opcode, U32 offset) {
    this->m_tx[0] = opcode;
    this->m_tx[1] = static_cast<U8>((offset >> 16) & 0xFF);
    this->m_tx[2] = static_cast<U8>((offset >> 8) & 0xFF);
    this->m_tx[3] = static_cast<U8>(offset & 0xFF);
}

U32 FramDriver ::protectStart(U8 statusRegister) {
    // Datasheet Table 5: BP1:BP0 = 01 protects the upper quarter, 10 the upper
    // half, 11 the whole array
    const U8 bp = static_cast<U8>((statusRegister >> SR_BP_SHIFT) & SR_BP_MASK);
    U32 start = PROTECT_NONE;
    switch (bp) {
        case BP_UPPER_QUARTER:
            start = Fram::FramConfig::Size - (Fram::FramConfig::Size / 4);
            break;
        case BP_UPPER_HALF:
            start = Fram::FramConfig::Size - (Fram::FramConfig::Size / 2);
            break;
        case BP_ALL:
            start = 0;
            break;
        default:
            start = PROTECT_NONE;
            break;
    }
    return start;
}

}  // namespace Fram

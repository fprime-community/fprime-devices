// ======================================================================
// \title  FramDriverTester.hpp
// \brief  hpp file for FramDriver component test harness implementation class
// ======================================================================

#ifndef Fram_FramDriverTester_HPP
#define Fram_FramDriverTester_HPP

#include "fprime-devices/Fram/Components/FramDriver/FramDriver.hpp"
#include "fprime-devices/Fram/Components/FramDriver/FramDriverGTestBase.hpp"

namespace Fram {

class FramDriverTester final : public FramDriverGTestBase {
  public:
    // Maximum size of histories storing events, telemetry, and port outputs
    static const FwSizeType MAX_HISTORY_SIZE = 64;
    // Instance ID supplied to the component instance under test
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    static constexpr FwSizeType HEADER = 4;                                      //!< opcode + 3 address bytes
    static constexpr FwSizeType FRAME = HEADER + Fram::FramConfig::MaxTransfer;  //!< largest frame
    static constexpr FwSizeType ID_SIZE = Fram::FramConfig::DeviceIdSize;
    static constexpr FwSizeType MAX_FRAMES = 8;       //!< frames recorded per test
    static constexpr FwSizeType MAX_COMPLETIONS = 8;  //!< client completions recorded per test
    static constexpr U32 SIZE = Fram::FramConfig::Size;
    static constexpr FwSizeType MAX_TRANSFER = Fram::FramConfig::MaxTransfer;
    static constexpr FwIndexType PORTS = Fram::FramConfig::ClientPorts;
    static constexpr U8 OP_WREN = 0x06;
    static constexpr U8 OP_WRITE = 0x02;
    static constexpr U8 OP_READ = 0x03;
    static constexpr U8 OP_RDSR = 0x05;
    static constexpr U8 OP_RDID = 0x9F;
    static constexpr U8 SR_BP0 = 0x04;
    static constexpr U8 SR_BP1 = 0x08;

    //! How the simulated SPI driver answers a frame
    enum class SpiMode : U8 {
        HOLD,    //!< record the frame; the test completes it later with completeSpi()
        IN_CALL  //!< complete the frame inside the spiWriteRead call (refusal or
                 //!< immediate completion)
    };

    //! A frame the component sent
    struct Frame {
        U8 tx[FRAME];
        FwSizeType length;    //!< write buffer size
        FwSizeType rxLength;  //!< read buffer size
    };

    //! A client completion the component delivered
    struct Completion {
        Fram::Op op;
        FwIndexType port;
        U32 offset;
        U8* data;
        FwSizeType size;
        U32 context;
        Fram::Status status;
    };

  public:
    FramDriverTester();
    ~FramDriverTester();

  public:
    // ----------------------------------------------------------------------
    // Tests (SDD Unit-test list)
    // ----------------------------------------------------------------------
    void ReadNominal();
    void WriteNominal();
    void CompletionPortIndex();
    void CompletionBufferIdentity();
    void AcceptedBeforeComplete();
    void OutOfRangeLimits();
    void BusyWhileTransfer();
    void BusyBeforeOutOfRange();
    void SpiErrorAfterReturn();
    void InCallRefusalBusy();
    void InCallRefusalError();
    void InCallRefusalMidTransfer();
    void RequestInsideCompletion();
    void RequestAfterCompletion();
    void StartupReads();
    void StartupIdMismatch();
    void StartupReadFailed();
    void StartupInCallRefusal();
    void BlockProtectWarning();
    void WriteProtectedRange();
    void WriteProtectedUnknownRange();
    void TelemetryCounters();
    void EventsRequestRejected();
    void EventsSpiError();
    void OutOfRangeBeforeWriteProtected();
    void InCallOkCompletion();
    void InCallOkThenRefusal();
    void RandomSequence();
    void UnsolicitedCompletionAsserts();
    void StartupTwiceAsserts();
    void NullBufferRejected();
    void StartupDuringClientTransferAsserts();
    void ReentrantFromHandlerDrain();

  private:
    // ----------------------------------------------------------------------
    // Handlers for typed from ports
    // ----------------------------------------------------------------------
    void from_spiWriteRead_handler(FwIndexType portNum, Fw::Buffer& writeBuffer, Fw::Buffer& readBuffer) override;
    void from_readComplete_handler(FwIndexType portNum,
                                   U32 offset,
                                   Fw::Buffer& buffer,
                                   const Fram::Status& status) override;
    void from_writeComplete_handler(FwIndexType portNum,
                                    U32 offset,
                                    Fw::Buffer& buffer,
                                    const Fram::Status& status) override;

    // ----------------------------------------------------------------------
    // Helpers
    // ----------------------------------------------------------------------
    //! Clear frame and completion records (and the generated histories)
    void clearAll();
    //! Set the simulated SPI driver's mode and status for the next frames
    void setSpi(SpiMode mode, Drv::SpiStatus status);
    //! In IN_CALL mode, answer the next `count` frames with these statuses in
    //! order (then fall back to setSpi's)
    void setSpiSequence(const Drv::SpiStatus* statuses, FwSizeType count);
    //! Queue a second reply, used for the frame after the one that consumes the
    //! current reply
    void queueReply(const U8* reply, FwSizeType length);
    //! Set the RDID reply frame: opcode slot, then the 9 identity bytes
    void setRdidReply(const U8* deviceId);
    //! Complete both frames of a write with SPI_OK
    void completeWrite();
    //! Record a client completion (shared by both completion handlers)
    void recordCompletion(Fram::Op op, FwIndexType port, U32 offset, Fw::Buffer& buffer, Fram::Status status);
    //! Set the reply bytes the simulated SPI driver returns in the read buffer
    //! (whole frame, from index 0)
    void setReply(const U8* reply, FwSizeType length);
    //! Set a reply frame for a read: zero header, then `length` pattern bytes
    //! derived from `seed`
    void setReadReply(FwSizeType length, U8 seed);
    //! Complete the held frame with status (reply bytes copied into the
    //! component's read buffer first)
    void completeSpi(Drv::SpiStatus status);
    //! Issue a read request
    Fram::Status requestRead(FwIndexType port, U32 offset, Fw::Buffer& buffer);
    //! Issue a write request
    Fram::Status requestWrite(FwIndexType port, U32 offset, Fw::Buffer& buffer);
    //! Run startup to completion with the given RDID reply and status register
    //! (HOLD mode)
    void runStartup(const U8* deviceId, U8 statusRegister, bool clear = true);
    //! Assert frame `index`: opcode, 24-bit address (or -1 to skip), total length
    void assertFrame(FwSizeType index, U8 opcode, I64 address, FwSizeType length);
    //! Assert the tail of frame `index` equals `data` (write data)
    void assertFrameData(FwSizeType index, const U8* data, FwSizeType length);
    //! Assert the number of frames sent
    void assertFrames(FwSizeType count);
    //! Assert completion `index`
    void assertCompletion(FwSizeType index,
                          Fram::Op op,
                          FwIndexType port,
                          U32 offset,
                          const Fw::Buffer& buffer,
                          Fram::Status status);
    //! Assert the number of client completions
    void assertCompletions(FwSizeType count);
    //! Assert a RequestRejected event at `index`
    void assertRejected(FwSizeType index,
                        Fram::Op op,
                        FwIndexType port,
                        U32 offset,
                        FwSizeType length,
                        Fram::Status status,
                        Fram::RejectCause cause);
    //! Assert a SpiError event at `index`
    void assertSpiError(FwSizeType index,
                        Fram::Op op,
                        FwIndexType port,
                        U32 offset,
                        FwSizeType length,
                        Drv::SpiStatus status,
                        Fram::SpiErrorSite site);
    //! Fill a pattern into a buffer
    static void fill(U8* data, FwSizeType length, U8 seed);

    //! Connect ports
    void connectPorts();
    //! Initialize components
    void initComponents();

  private:
    //! The component under test
    FramDriver component;

    // Simulated SPI driver
    SpiMode m_spiMode;
    Drv::SpiStatus m_spiStatus;
    U8 m_reply[FRAME];
    FwSizeType m_replyLength;
    U8 m_reply2[FRAME];  //!< queued reply for the following frame
    FwSizeType m_reply2Length;
    bool m_reply2Set;
    Drv::SpiStatus m_seq[4];  //!< per-frame IN_CALL statuses
    FwSizeType m_seqCount;
    FwSizeType m_seqIndex;
    Fw::Buffer m_heldWrite;  //!< the component's buffers from the held frame
    Fw::Buffer m_heldRead;
    bool m_held;
    Frame m_frames[MAX_FRAMES];
    FwSizeType m_frameCount;

    // Client completions
    Completion m_completions[MAX_COMPLETIONS];
    FwSizeType m_completionCount;

    // Re-entrant request from inside a completion
    bool m_reentrantRequest;
    Fram::Status m_reentrantStatus;
    U8 m_reentrantData[MAX_TRANSFER];
    Fw::Buffer m_reentrantBuffer;

    // Client buffers
    U8 m_data[PORTS][MAX_TRANSFER];
    U8 m_big[MAX_TRANSFER + 1];  //!< over-size request view
};

}  // namespace Fram

#endif

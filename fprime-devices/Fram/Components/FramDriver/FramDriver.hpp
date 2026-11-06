// ======================================================================
// \title  FramDriver.hpp
// \brief  Driver for the Infineon CY15B102QN serial F-RAM over an asynchronous
// SPI driver (SDD docs/sdd.md)
// ======================================================================

#ifndef Fram_FramDriver_HPP
#define Fram_FramDriver_HPP

#include "FramConfig/FramConfig.hpp"
#include "fprime-devices/Fram/Components/FramDriver/FramDriverComponentAc.hpp"

namespace Fram {

//! \brief F-RAM driver over an asynchronous SPI driver (Drv.AsyncGuardedSpi)
//!
//! Passive and unguarded: the deployment must deliver spiWriteReadComplete in
//! the same execution context as the clients' read/write calls (e.g. a passive
//! SPI driver completing from its scheduler entry, as Samd21.SpiDriver does).
//! One transfer at a time; frames go out of component-owned buffers; the
//! client's buffer is handed back, unchanged in pointer and size, with exactly
//! one completion on the requesting port index.
//!
//! Any spiWriteRead call may complete before it returns (a refusal, or an
//! immediate completion from a polling driver). Such a completion is only
//! recorded while the call is in progress and acted on right after it returns,
//! exactly as if it had arrived later (SDD §Behavior). Consequence for clients:
//! with a driver that completes inside the call, the client's completion may be
//! delivered before its read/write call returns ACCEPTED, so a client must be
//! ready for its completion before it calls.
class FramDriver final : public FramDriverComponentBase {
  public:
    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct FramDriver object
    explicit FramDriver(const char* const compName  //!< The component name
    );

    //! Destroy FramDriver object
    ~FramDriver();

    FramDriver(const FramDriver&) = delete;
    FramDriver& operator=(const FramDriver&) = delete;

    //! Start the device checks: read the device ID (RDID) then the status
    //! register (RDSR). Called once by the deployment after the SPI driver is
    //! configured; a passive-only topology has no port to trigger it. Startup may
    //! complete, StartupComplete included, before this call returns (in-call SPI
    //! completion).
    void startup();

  private:
    friend class FramDriverTester;  //!< Unit-test access (counter wrap)

    // ----------------------------------------------------------------------
    // Device constants (datasheet 002-26764 Rev. *D, Table 2 and Table 5)
    // ----------------------------------------------------------------------

    static constexpr U8 OPCODE_WREN = 0x06;   //!< Set write enable latch
    static constexpr U8 OPCODE_WRITE = 0x02;  //!< Write memory data
    static constexpr U8 OPCODE_READ = 0x03;   //!< Read memory data
    static constexpr U8 OPCODE_RDSR = 0x05;   //!< Read status register
    static constexpr U8 OPCODE_RDID = 0x9F;   //!< Read device ID

    static constexpr U8 SR_BP_SHIFT = 2;  //!< BP0 is status register bit 2, BP1 bit 3
    static constexpr U8 SR_BP_MASK = 0x03;
    static constexpr U8 BP_UPPER_QUARTER = 1;  //!< BP1:BP0 = 01, datasheet Table 5
    static constexpr U8 BP_UPPER_HALF = 2;     //!< BP1:BP0 = 10
    static constexpr U8 BP_ALL = 3;            //!< BP1:BP0 = 11

    static constexpr FwSizeType ADDRESS_BYTES = 3;                 //!< 24-bit address, MSB first
    static constexpr FwSizeType HEADER_BYTES = 1 + ADDRESS_BYTES;  //!< Opcode + address
    static constexpr FwSizeType FRAME_BYTES = HEADER_BYTES + Fram::FramConfig::MaxTransfer;
    static constexpr FwSizeType RDID_FRAME_BYTES = 1 + Fram::FramConfig::DeviceIdSize;
    static constexpr FwSizeType RDSR_FRAME_BYTES = 2;
    static constexpr FwSizeType WREN_FRAME_BYTES = 1;
    static constexpr U32 PROTECT_NONE = Fram::FramConfig::Size;  //!< Protected range start when nothing is
                                                                 //!< protected
    //! Most frames a transfer issues (write: WREN + WRITE; startup: RDID + RDSR);
    //! bounds the in-call drain loop
    static constexpr FwSizeType MAX_FRAMES_PER_TRANSFER = 2;

    static_assert(FRAME_BYTES >= RDID_FRAME_BYTES, "Frame buffers must fit the RDID frame");
    static_assert(static_cast<U32>(Fram::FramConfig::MaxTransfer) <= static_cast<U32>(Fram::FramConfig::Size),
                  "MaxTransfer must not exceed Size");
    static_assert(static_cast<U32>(Fram::FramConfig::Size) <= (1u << (8 * ADDRESS_BYTES)), "Size must fit the address");
    static_assert(Fram::FramConfig::ClientPorts >= 1, "At least one client port");
    static_assert(Fram::FramConfig::DeviceIdSize == 9, "RDID reply is 9 bytes");

    //! What the outstanding SPI frame belongs to
    enum class Phase : U8 {
        IDLE,          //!< No frame outstanding; a request is accepted
        STARTUP_RDID,  //!< startup(): device ID read outstanding
        STARTUP_RDSR,  //!< startup(): status register read outstanding
        READ,          //!< Client read: READ frame outstanding
        WRITE_WREN,    //!< Client write: WREN frame outstanding
        WRITE_DATA     //!< Client write: WRITE frame outstanding
    };

    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for read
    Fram::Status read_handler(FwIndexType portNum,  //!< The port number
                              U32 offset,           //!< Byte offset of the first byte transferred
                              Fw::Buffer& buffer    //!< Destination; its size is N
                              ) override;

    //! Handler implementation for write
    Fram::Status write_handler(FwIndexType portNum,  //!< The port number
                               U32 offset,           //!< Byte offset of the first byte transferred
                               Fw::Buffer& buffer    //!< Source; its size is N
                               ) override;

    //! Handler implementation for spiWriteReadComplete
    void spiWriteReadComplete_handler(FwIndexType portNum,          //!< The port number
                                      Fw::Buffer& writeBuffer,      //!< Write buffer passed in the request
                                      Fw::Buffer& readBuffer,       //!< Read buffer passed in the request
                                      const Drv::SpiStatus& status  //!< Transaction result status
                                      ) override;

    // ----------------------------------------------------------------------
    // Helpers
    // ----------------------------------------------------------------------

    //! Common request path: checks, first frame, in-call refusal
    Fram::Status request(Fram::Op op, FwIndexType portNum, U32 offset, Fw::Buffer& buffer);

    //! Reject the current request: event, counter, back to idle
    Fram::Status reject(Fram::Status status, Fram::RejectCause cause);

    //! Send the frame in m_tx / m_rx of the given length, marking the call so an
    //! in-call completion is only recorded
    void issue(FwSizeType length);

    //! Act on in-call completions recorded during issue(), as if they had arrived
    //! later
    void drain();

    //! Act on the completion of the outstanding frame
    void process(Drv::SpiStatus status);

    //! Finish the client transfer: idle first, then the completion on the
    //! requesting index
    void finishClient(Fram::Status status);

    //! Fill opcode and 3-byte address into m_tx
    void setHeader(U8 opcode, U32 offset);

    //! Protected range start for a BP1:BP0 field
    static U32 protectStart(U8 statusRegister);

    // ----------------------------------------------------------------------
    // State
    // ----------------------------------------------------------------------

    Phase m_phase;                  //!< Outstanding frame
    FwIndexType m_client;           //!< Client index of the transfer in progress
    Fram::Op m_op;                  //!< Direction of the transfer in progress
    U32 m_offset;                   //!< Offset of the transfer in progress
    Fw::Buffer m_clientBuffer;      //!< The client's buffer (data pointer and size),
                                    //!< handed back unchanged
    bool m_issuing;                 //!< A spiWriteRead call is in progress: completions are only
                                    //!< recorded
    bool m_inCallPending;           //!< A completion arrived during the call
    bool m_inCallChain;             //!< drain() is acting on in-call completions; a
                                    //!< re-entrant request is rejected (bounded recursion)
    Drv::SpiStatus m_inCallStatus;  //!< Its status
    bool m_startupFailed;           //!< A startup read ended non-OK
    Fram::StartupState m_startupState;
    U32 m_protectStart;  //!< First protected address (Size = none); from the
                         //!< startup RDSR
    U32 m_readsCompleted;
    U32 m_writesCompleted;
    U32 m_requestsRejected;
    U32 m_spiErrors;

    U8 m_tx[FRAME_BYTES];   //!< Transmit frame
    U8 m_rx[FRAME_BYTES];   //!< Receive frame
    Fw::Buffer m_txBuffer;  //!< View of m_tx for the SPI driver
    Fw::Buffer m_rxBuffer;  //!< View of m_rx for the SPI driver
};

}  // namespace Fram

#endif

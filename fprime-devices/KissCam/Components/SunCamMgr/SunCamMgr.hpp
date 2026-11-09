// ======================================================================
// \title  SunCamMgr.hpp
// \author ubuntu
// \brief  hpp file for SunCamMgr component implementation class
// ======================================================================

#ifndef KissCam_SunCamMgr_HPP
#define KissCam_SunCamMgr_HPP

#include "fprime-devices/KissCam/Components/SunCamMgr/SunCamMgrComponentAc.hpp"

namespace KissCam {

class SunCamMgr final : public SunCamMgrComponentBase {
  public:
    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct SunCamMgr object
    SunCamMgr(const char* const compName  //!< The component name
    );

    //! Destroy SunCamMgr object
    ~SunCamMgr();

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for drvConnected
    //!
    //! Driver (re)connected: handler is empty, no state is kept, no probe is sent (the camera may be off)
    void drvConnected_handler(FwIndexType portNum  //!< The port number
                              ) override;

    //! Handler implementation for drvReceiveIn
    //!
    //! Bytes from the camera, framed on the component thread (one reply arrives in several buffers).
    //! Queue-full hook, in the driver's thread: returns the buffer and counts the drop — nothing else
    void drvReceiveIn_handler(FwIndexType portNum,  //!< The port number
                              Fw::Buffer& buffer,
                              const Drv::ByteStreamStatus& status) override;

    //! Handler implementation for pingIn
    //!
    //! Health ping; queue-full hook only counts the drop
    void pingIn_handler(FwIndexType portNum,  //!< The port number
                        U32 key               //!< Value to return to pinger
                        ) override;

    //! Handler implementation for run
    //!
    //! Rate-group tick (10 Hz), synchronous: the handler test-and-sets the atomic "run queued" flag and, when it
    //! was clear, invokes runInternal — at most one run message is ever on the queue
    void run_handler(FwIndexType portNum,  //!< The port number
                     U32 context           //!< The call order
                     ) override;

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for commands
    // ----------------------------------------------------------------------

    //! Handler implementation for command SETUP_CAMERA
    //!
    //! Ping MCU, Set Mode (CAMERA_MODE), Set Exposure (EXPOSURE), Read Register 0x3012, Ping MCU+sensor; then READY
    void SETUP_CAMERA_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                                 U32 cmdSeq            //!< The command sequence number
                                 ) override;

    //! Handler implementation for command SLEEP
    //!
    //! Set Mode Sleep; the component returns to NOT_SETUP. Accepted when not set up
    void SLEEP_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                          U32 cmdSeq            //!< The command sequence number
                          ) override;

    //! Handler implementation for command PING
    //!
    //! Ping the camera; reply counters and uptime go to events and telemetry. Accepted when not set up
    void PING_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                         U32 cmdSeq,           //!< The command sequence number
                         const KissCam::PingType& pingType) override;

    //! Handler implementation for command MEMORY_STATUS
    //!
    //! Memory Status: mode, exposure and uptime of the image in each of the four slots (needs READY)
    void MEMORY_STATUS_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                                  U32 cmdSeq            //!< The command sequence number
                                  ) override;

    //! Handler implementation for command CAPTURE
    //!
    //! Capture an image into a slot, optionally the test pattern (needs READY). In HD mode the camera
    //! ignores the slot and the image occupies slots 1-4; CaptureComplete echoes the argument and the mode
    void CAPTURE_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                            U32 cmdSeq,           //!< The command sequence number
                            const KissCam::Slot& slot,
                            const KissCam::TestPattern& testPattern) override;

    //! Handler implementation for command DOWNLOAD
    //!
    //! Download the image in a slot into one Image data product (needs READY): Memory Status first; an SD slot
    //! is 480 (60 preview) lines, an HD image is its four quadrant slots 1-4 in order, 1920 (240) lines
    void DOWNLOAD_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                             U32 cmdSeq,           //!< The command sequence number
                             const KissCam::Slot& slot,
                             const KissCam::Resolution& resolution) override;

    //! Handler implementation for command DOWNLOAD_ABORT
    //!
    //! Stop the download in progress after the pending line; the lines received are still delivered (ACTIVITY_HI)
    void DOWNLOAD_ABORT_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                                   U32 cmdSeq            //!< The command sequence number
                                   ) override;

    //! Handler implementation for command READ_REGISTER
    //!
    //! Read a 16-bit image sensor register (needs READY)
    void READ_REGISTER_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                                  U32 cmdSeq,           //!< The command sequence number
                                  U16 address) override;

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for user-defined internal interfaces
    // ----------------------------------------------------------------------

    //! Handler implementation for runInternal
    //!
    //! The tick work on the component thread: reply timeouts (KissCamConfig.REPLY_TIMEOUT_TICKS), deferred
    //! parameter push, periodic channels. The handler clears the atomic; the queue-full hook clears it too
    //! and counts the drop (QueueInput.RUN_TICK)
    void runInternal_internalInterfaceHandler() override;

  private:
    // ----------------------------------------------------------------------
    // Overflow hook implementations for typed input ports
    // ----------------------------------------------------------------------

    //! Overflow hook implementation for drvReceiveIn
    void drvReceiveIn_overflowHook(FwIndexType portNum,  //!< The port number
                                   Fw::Buffer& buffer,
                                   const Drv::ByteStreamStatus& status) override;

    //! Overflow hook implementation for pingIn
    void pingIn_overflowHook(FwIndexType portNum,  //!< The port number
                             U32 key               //!< Value to return to pinger
                             ) override;

  private:
    // ----------------------------------------------------------------------
    // Overflow hook implementations for internal ports
    // ----------------------------------------------------------------------

    //! Overflow hook implementation for runInternal
    void runInternal_overflowHook() override;

  private:
    // ----------------------------------------------------------------------
    // Overflow hook implementations for commands
    // ----------------------------------------------------------------------

    //! Overflow hook implementation for SETUP_CAMERA
    void SETUP_CAMERA_cmdOverflowHook(FwOpcodeType opCode,  //!< The opcode
                                      U32 cmdSeq            //!< The command sequence number
                                      ) override;

    //! Overflow hook implementation for SLEEP
    void SLEEP_cmdOverflowHook(FwOpcodeType opCode,  //!< The opcode
                               U32 cmdSeq            //!< The command sequence number
                               ) override;

    //! Overflow hook implementation for PING
    void PING_cmdOverflowHook(FwOpcodeType opCode,  //!< The opcode
                              U32 cmdSeq            //!< The command sequence number
                              ) override;

    //! Overflow hook implementation for MEMORY_STATUS
    void MEMORY_STATUS_cmdOverflowHook(FwOpcodeType opCode,  //!< The opcode
                                       U32 cmdSeq            //!< The command sequence number
                                       ) override;

    //! Overflow hook implementation for CAPTURE
    void CAPTURE_cmdOverflowHook(FwOpcodeType opCode,  //!< The opcode
                                 U32 cmdSeq            //!< The command sequence number
                                 ) override;

    //! Overflow hook implementation for DOWNLOAD
    void DOWNLOAD_cmdOverflowHook(FwOpcodeType opCode,  //!< The opcode
                                  U32 cmdSeq            //!< The command sequence number
                                  ) override;

    //! Overflow hook implementation for DOWNLOAD_ABORT
    void DOWNLOAD_ABORT_cmdOverflowHook(FwOpcodeType opCode,  //!< The opcode
                                        U32 cmdSeq            //!< The command sequence number
                                        ) override;

    //! Overflow hook implementation for READ_REGISTER
    void READ_REGISTER_cmdOverflowHook(FwOpcodeType opCode,  //!< The opcode
                                       U32 cmdSeq            //!< The command sequence number
                                       ) override;
};

}  // namespace KissCam

#endif

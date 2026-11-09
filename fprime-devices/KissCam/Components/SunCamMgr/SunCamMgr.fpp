module KissCam {

    @ Manager of an MVP Aerospace KissCAM V2 sun camera over a UART byte-stream driver.
    @ One camera operation at a time; settings (parameters) applied by SETUP_CAMERA; images as data products.
    active component SunCamMgr {

        # ----------------------------------------------------------------------
        # UART driver (Drv.ByteStreamDriver client, receive made asynchronous)
        # ----------------------------------------------------------------------

        @ Driver (re)connected: handler is empty, no state is kept, no probe is sent (the camera may be off)
        async input port drvConnected: Drv.ByteStreamReady drop

        @ Bytes from the camera, framed on the component thread (one reply arrives in several buffers).
        @ Queue-full hook, in the driver's thread: returns the buffer and counts the drop — nothing else
        async input port drvReceiveIn: Drv.ByteStreamData hook

        @ Returns the driver's receive buffers
        output port drvReceiveReturnOut: Fw.BufferSend

        @ Sends one seven-byte command frame; the component keeps ownership of the frame buffer
        output port drvSendOut: Drv.ByteStreamSend

        # ----------------------------------------------------------------------
        # Framework services
        # ----------------------------------------------------------------------

        @ Rate-group tick (10 Hz): reply timeouts are counted in these ticks (KissCamConfig.REPLY_TIMEOUT_TICKS).
        @ Queue-full hook only counts the drop
        async input port run: Svc.Sched hook

        @ Health ping; queue-full hook only counts the drop
        async input port pingIn: Svc.Ping hook

        @ Health ping reply
        output port pingOut: Svc.Ping

        @ Data product allocation (synchronous)
        product get port productGetOut

        @ Data product delivery
        product send port productSendOut

        command recv port cmdIn
        command reg port cmdRegOut
        command resp port cmdResponseOut
        event port eventOut
        text event port textEventOut
        telemetry port tlmOut
        time get port timeGetOut
        param get port prmGetOut
        param set port prmSetOut

        # ----------------------------------------------------------------------
        # Data products (SDD Table 4)
        # ----------------------------------------------------------------------

        @ First record of an Image product
        product record ImageHeader: KissCam.ImageHeader id 0

        @ One image line as received: line number (2 bytes, MSB first) then 640 or 80 pixel bytes
        product record ImageLine: U8 array id 1

        @ One product per DOWNLOAD
        product container Image id 0 default priority KissCamConfig.IMAGE_DP_PRIORITY

        # ----------------------------------------------------------------------
        # Parameters (SDD Table 1) — pushed by SETUP_CAMERA and, while READY, on change
        # ----------------------------------------------------------------------

        @ Capture mode pushed with Set Mode (SD: four 640x480 slots; HD: one 1280x960 image)
        param CAMERA_MODE: CaptureMode default CaptureMode.SD

        @ Exposure pushed with Set Exposure, in steps of 89.5 us, 2..960 (camera default 672)
        param EXPOSURE: U16 default 672

        # ----------------------------------------------------------------------
        # Commands (SDD Table 2). Queue-full hooks run in the dispatcher's thread and only answer
        # the command BUSY and count the drop; the component thread reports QueueOverflow on its next tick.
        # ----------------------------------------------------------------------

        @ Ping MCU, Set Mode (CAMERA_MODE), Set Exposure (EXPOSURE), Read Register 0x3012, Ping MCU+sensor; then READY
        async command SETUP_CAMERA hook

        @ Set Mode Sleep; the component returns to NOT_SETUP. Accepted when not set up
        async command SLEEP hook

        @ Ping the camera; reply counters and uptime go to events and telemetry. Accepted when not set up
        async command PING(pingType: PingType) hook

        @ Memory Status: mode, exposure and uptime of the image in each of the four slots (needs READY)
        async command MEMORY_STATUS hook

        @ Capture an image into a slot, optionally the test pattern (needs READY). In HD mode the camera
        @ ignores the slot and the image occupies slots 1-4; CaptureComplete echoes the argument and the mode
        async command CAPTURE(slot: Slot, testPattern: TestPattern) hook

        @ Download every line of a slot into one Image data product (needs READY). An HD image is four
        @ slot downloads reassembled on the ground; MEMORY_STATUS tells which slots hold HD quadrants
        async command DOWNLOAD(slot: Slot, resolution: Resolution) hook

        @ Stop the download in progress after the pending line; the lines received are still delivered
        async command DOWNLOAD_ABORT hook

        @ Read a 16-bit image sensor register (needs READY)
        async command READ_REGISTER(address: U16) hook

        # ----------------------------------------------------------------------
        # Events
        # ----------------------------------------------------------------------

        @ SETUP_CAMERA accepted; exchanges begin
        event SetupStarted severity activity low format "Camera set-up started"

        @ Every set-up exchange succeeded; settings applied and read back
        event SetupComplete(mode: CaptureMode, exposure: U16) severity activity high \
            format "Camera set-up complete: mode {} exposure {}"

        @ A set-up exchange failed; the component is NOT_SETUP
        event SetupFailed(step: SetupStep, cause: FailCause) severity warning high \
            format "Camera set-up failed at {}: {}; component NOT_SETUP"

        @ A Table 1 setting was acknowledged by the camera
        event SettingApplied(setting: Setting, value: U16) severity activity high \
            format "Camera setting {} applied: {}"

        @ A parameter push outside set-up failed; the component is NOT_SETUP
        event SettingPushFailed(setting: Setting, cause: FailCause) severity warning high \
            format "Camera setting {} push failed: {}; component NOT_SETUP"

        @ Exposure register 0x3012 read back after an exposure push
        event ExposureReadback(pushed: U16, readback: U16) severity activity low \
            format "Exposure pushed {} read back {}"

        @ Set Mode Sleep acknowledged; the component is NOT_SETUP
        event SleepEntered severity activity high format "Camera in Sleep; component NOT_SETUP"

        @ Ping reply
        event PingReply(pingType: PingType, validCmds: U8, invalidCmds: U8, uptime: U16) severity activity low \
            format "Camera ping {}: {} valid, {} invalid commands, uptime {} s"

        @ Memory Status reply
        event MemoryStatusReply(slots: MemorySlots) severity activity low format "Camera memory slots: {}"

        @ Capture acknowledged
        event CaptureComplete(slot: Slot, testPattern: TestPattern, exposure: U16, mode: Mode) severity activity high \
            format "Captured slot {} (test pattern {}) at exposure {} in mode {}"

        @ DOWNLOAD accepted
        event DownloadStarted(slot: Slot, resolution: Resolution, lines: U16) severity activity low \
            format "Download of slot {} at {} started: {} lines"

        @ Every line received; product sent
        event DownloadComplete(slot: Slot, resolution: Resolution, lines: U16, bytes: U32) severity activity high \
            format "Download of slot {} at {} complete: {} lines, {} bytes"

        @ Download ended early; lines received so far were delivered when a product existed
        event DownloadAborted(slot: Slot, linesReceived: U16, cause: AbortCause) severity warning low \
            format "Download of slot {} aborted after {} lines: {}"

        @ An image line is being re-requested
        event LineRetry(line: U16, attempt: U8) severity diagnostic format "Re-requesting line {} (attempt {})"

        @ Read Register reply
        event RegisterRead(address: U16, value: U16) severity activity high \
            format "Register 0x{x} = 0x{x}"

        @ A ground command was rejected (response EXECUTION_ERROR, or BUSY for cause BUSY)
        event CommandRejected(operation: Operation, cause: RejectCause) severity warning low \
            format "{} rejected: {}"

        @ The camera answered with an error reply
        event CameraError(operation: Operation, cmdByte: U8, errorType: ErrorType, count: U8) severity warning high \
            format "Camera error during {}: command 0x{x} {} (invalid count {})"

        @ No reply within REPLY_TIMEOUT_TICKS
        event ReplyTimeout(operation: Operation, cmdByte: U8) severity warning high \
            format "Camera reply timeout during {} (command 0x{x})"

        @ Received bytes discarded; throttled, SETUP_CAMERA clears the throttle
        event FrameDiscarded(cause: DiscardCause, bytes: U32, head: FrameHead) severity warning low \
            format "Discarded {} bytes: {} head {}" throttle 10

        @ The camera reported a mode other than the commanded one while READY
        event ModeMismatch(commanded: Mode, reported: Mode) severity warning high \
            format "Camera mode {} expected, {} reported; component NOT_SETUP"

        @ The UART driver refused a command frame
        event SendFailed(operation: Operation, status: Drv.ByteStreamStatus) severity warning high \
            format "UART send failed during {}: {}"

        @ No data-product container of the needed size was available
        event ProductAllocationFailed(bytes: U32) severity warning high \
            format "Image data product allocation of {} bytes failed"

        @ Queue-full drops since the last tick, reported from the component thread; SETUP_CAMERA clears the throttle
        event QueueOverflow(source: QueueInput, dropped: U32) severity warning high \
            format "Queue full: {} dropped {} times" throttle 10

        # ----------------------------------------------------------------------
        # Telemetry (SDD Table 3)
        # ----------------------------------------------------------------------

        @ Component set-up state
        telemetry SetupState: SetupState
        @ Operation in progress; NONE when idle
        telemetry CurrentOperation: Operation
        @ Camera mode from the mode byte of the last reply
        telemetry CameraMode: Mode
        @ EXPOSURE value the camera acknowledged, or the exposure of the last Capture reply (steps of 89.5 us)
        telemetry Exposure: U16
        @ Sensor register 0x3012 from the last readback (steps of 89.5 us)
        telemetry ExposureReadback: U16
        @ Camera uptime from the last Ping reply (s, rolls over at 65536)
        telemetry CameraUptime: U16 format "{} s"
        @ Valid commands counted by the camera since its boot (last Ping reply; rolls over at 256)
        telemetry CameraValidCmds: U8
        @ Invalid commands counted by the camera since its boot (last Ping or error reply; rolls over at 256)
        telemetry CameraInvalidCmds: U8
        @ Memory Status reply: mode, exposure and uptime of the image in each slot
        telemetry MemorySlots: MemorySlots
        @ Last Read Register reply
        telemetry RegisterValue: RegisterValue
        @ Last camera error reply
        telemetry LastCameraError: CameraErrorInfo
        @ Command/reply exchanges completed with an acknowledgement (since boot)
        telemetry Exchanges: U32
        @ Error replies received (since boot)
        telemetry CameraErrors: U32
        @ Reply timeouts (since boot)
        telemetry Timeouts: U32
        @ Bytes discarded outside an operation, malformed or for another command (since boot)
        telemetry DiscardedBytes: U32
        @ Discarded frames (since boot)
        telemetry MalformedReplies: U32
        @ Capture acknowledgements (since boot)
        telemetry ImagesCaptured: U32
        @ Image lines stored in products (since boot)
        telemetry LinesDownloaded: U32
        @ Image data products sent (since boot)
        telemetry ProductsSent: U32
        @ Inputs dropped by queue-full hooks (since boot; mutex-guarded count read on the run tick)
        telemetry QueueOverflows: U32
    }
}

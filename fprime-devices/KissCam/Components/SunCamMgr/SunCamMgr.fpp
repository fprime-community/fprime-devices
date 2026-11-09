module KissCam {

    @ Manager of an MVP Aerospace KissCAM V2 sun camera over a UART byte-stream driver.
    @ One camera operation at a time; settings (parameters) applied by SETUP_CAMERA; images as data products.
    active component SunCamMgr {

        # ----------------------------------------------------------------------
        # UART driver (Drv.ByteStreamDriver client, receive made asynchronous)
        # ----------------------------------------------------------------------

        @ Driver (re)connected; recorded only — no probe is sent (the camera may be off)
        sync input port drvConnected: Drv.ByteStreamReady

        @ Bytes from the camera; one reply may arrive in several buffers. Queue full: buffer returned, counted
        async input port drvReceiveIn: Drv.ByteStreamData hook

        @ Returns the driver's receive buffers
        output port drvReceiveReturnOut: Fw.BufferSend

        @ Sends one seven-byte command frame; the component keeps ownership of the frame buffer
        output port drvSendOut: Drv.ByteStreamSend

        # ----------------------------------------------------------------------
        # Framework services
        # ----------------------------------------------------------------------

        @ Rate-group tick: reply timeouts are counted in these ticks (KissCamConfig.REPLY_TIMEOUT_TICKS)
        async input port run: Svc.Sched hook

        @ Health ping
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
        # Commands (SDD Table 2)
        # ----------------------------------------------------------------------

        @ Ping MCU, Set Mode (CAMERA_MODE), Set Exposure (EXPOSURE), Read Register 0x3012, Ping MCU+sensor; then READY
        async command SETUP_CAMERA hook

        @ Set Mode Sleep; the component returns to NOT_SETUP. Accepted when not set up
        async command SLEEP hook

        @ Ping the camera; reply counters and uptime go to events and telemetry. Accepted when not set up
        async command PING(pingType: PingType) hook

        @ Memory Status: mode, exposure and uptime of the image in each of the four slots (needs READY)
        async command MEMORY_STATUS hook

        @ Capture an image into a slot (ignored by the camera in HD), optionally the test pattern (needs READY)
        async command CAPTURE(slot: U8 @< Memory slot 1..4
                              testPattern: TestPattern) hook

        @ Download every line of a slot into one Image data product (needs READY)
        async command DOWNLOAD(slot: U8 @< Memory slot 1..4
                               resolution: Resolution) hook

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
        event CaptureComplete(slot: U8, testPattern: TestPattern, exposure: U16) severity activity high \
            format "Captured slot {} (test pattern {}) at exposure {}"

        @ DOWNLOAD accepted
        event DownloadStarted(slot: U8, resolution: Resolution, lines: U16) severity activity low \
            format "Download of slot {} at {} started: {} lines"

        @ Every line received; product sent
        event DownloadComplete(slot: U8, resolution: Resolution, lines: U16, bytes: U32) severity activity high \
            format "Download of slot {} at {} complete: {} lines, {} bytes"

        @ Download ended early; lines received so far were delivered when a product existed
        event DownloadAborted(slot: U8, linesReceived: U16, cause: AbortCause) severity warning low \
            format "Download of slot {} aborted after {} lines: {}"

        @ An image line is being re-requested
        event LineRetry(line: U16, attempt: U8) severity diagnostic format "Re-requesting line {} (attempt {})"

        @ Read Register reply
        event RegisterRead(address: U16, value: U16) severity activity high \
            format "Register 0x{x} = 0x{x}"

        @ A ground command was rejected
        event CommandRejected(operation: Operation, cause: RejectCause) severity warning low \
            format "{} rejected: {}"

        @ The camera answered with an error reply
        event CameraError(operation: Operation, cmdByte: U8, errorType: ErrorType, count: U8) severity warning high \
            format "Camera error during {}: command 0x{x} {} (invalid count {})"

        @ No reply within REPLY_TIMEOUT_TICKS
        event ReplyTimeout(operation: Operation, cmdByte: U8) severity warning high \
            format "Camera reply timeout during {} (command 0x{x})"

        @ Received bytes discarded
        event FrameDiscarded(cause: DiscardCause, bytes: U32) severity warning low \
            format "Discarded {} bytes: {}"

        @ The camera reported a mode other than the commanded one while READY
        event ModeMismatch(commanded: Mode, reported: Mode) severity warning high \
            format "Camera mode {} expected, {} reported; component NOT_SETUP"

        @ The UART driver refused a command frame
        event SendFailed(operation: Operation, status: Drv.ByteStreamStatus) severity warning high \
            format "UART send failed during {}: {}"

        @ No data-product container of the needed size was available
        event ProductAllocationFailed(bytes: U32) severity warning high \
            format "Image data product allocation of {} bytes failed"

        @ The message queue was full; the newest input was dropped
        event QueueOverflow(source: QueueInput) severity warning high format "Queue full: {} dropped"

        # ----------------------------------------------------------------------
        # Telemetry (SDD Table 3)
        # ----------------------------------------------------------------------

        telemetry SetupState: SetupState
        telemetry CameraMode: Mode
        telemetry Exposure: U16
        telemetry ExposureReadback: U16
        telemetry CameraUptime: U16
        telemetry CameraValidCmds: U8
        telemetry CameraInvalidCmds: U8
        telemetry MemorySlots: MemorySlots
        telemetry RegisterValue: RegisterValue
        telemetry LastCameraError: CameraErrorInfo
        telemetry Exchanges: U32
        telemetry CameraErrors: U32
        telemetry Timeouts: U32
        telemetry DiscardedBytes: U32
        telemetry MalformedReplies: U32
        telemetry ImagesCaptured: U32
        telemetry LinesDownloaded: U32
        telemetry ProductsSent: U32
        telemetry QueueOverflows: U32
    }
}

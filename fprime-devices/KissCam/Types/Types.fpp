module KissCam {
    @ Camera mode as the camera reports it in every reply (datasheet §6.2, §7.2)
    enum Mode : U8 {
        SLEEP = 0 @< Image sensor held in reset
        SD = 1 @< Standard definition, 640 x 480, four slots
        HD = 2 @< High definition, 1280 x 960, one image across all four slots
    }
    @ Capture modes the CAMERA_MODE parameter may select (Sleep is commanded with SLEEP)
    enum CaptureMode : U8 {
        SD = 1
        HD = 2
    }
    @ Ping type (datasheet §7.4.1)
    enum PingType : U8 {
        MCU = 0 @< Command/reply check of the microcontroller only
        MCU_AND_SENSOR = 1 @< Also reads the image sensor ID register over I2C
    }
    @ Capture test-pattern flag (datasheet §7.4.5)
    enum TestPattern : U8 {
        OFF = 0
        ON = 1
    }
    @ Download resolution (datasheet §7.4.6)
    enum Resolution : U8 {
        FULL = 0 @< 640 x 480, 480 lines of 640 pixels
        PREVIEW = 1 @< 80 x 60, 60 lines of 80 pixels
    }
    @ Camera memory slot (datasheet §7.4.5)
    enum Slot : U8 {
        SLOT_1 = 1
        SLOT_2 = 2
        SLOT_3 = 3
        SLOT_4 = 4
    }
    @ Component set-up state (SDD Table 3)
    enum SetupState : U8 {
        NOT_SETUP = 0 @< Settings not applied; only SETUP_CAMERA, SLEEP, PING accepted
        SETTING_UP = 1 @< SETUP_CAMERA exchanges in progress
        READY = 2 @< Settings applied and read back; all commands accepted
    }
    @ Camera error type (datasheet §7.4.9)
    enum ErrorType : U8 {
        UNKNOWN = 0
        UNRECOGNISED_COMMAND = 1
        INVALID_PARAMETER = 2
        WRONG_MODE = 3
        SENSOR_PROBLEM = 4
    }
    @ Why an exchange, set-up step or parameter push failed
    enum FailCause : U8 {
        CAMERA_ERROR = 0 @< Error reply (CameraError event carries the detail)
        TIMEOUT = 1 @< No reply within REPLY_TIMEOUT_TICKS
        SEND_FAILED = 2 @< The UART driver refused the command frame
        MODE_MISMATCH = 3 @< Reply mode differs from the commanded mode
        EXPOSURE_MISMATCH = 4 @< Register 0x3012 readback differs from the pushed EXPOSURE
    }
    @ Why a ground command was rejected
    enum RejectCause : U8 {
        NOT_READY = 0 @< Command needs READY (SDD Table 2)
        BUSY = 1 @< An operation is in progress
        NO_DOWNLOAD = 2 @< DOWNLOAD_ABORT with no download in progress
    }
    @ Why received bytes were discarded
    enum DiscardCause : U8 {
        NO_OPERATION = 0 @< Bytes arrived with no exchange outstanding
        MALFORMED = 1 @< No start byte, bad length, or missing terminator
        WRONG_COMMAND = 2 @< Well-formed reply acknowledging a command other than the outstanding one
    }
    @ Why a download ended before every line was received
    enum AbortCause : U8 {
        COMMANDED = 0 @< DOWNLOAD_ABORT
        LINE_FAILED = 1 @< A line failed LINE_RETRIES re-requests
        SEND_FAILED = 2 @< The UART driver refused a Download frame
        PRODUCT_FULL = 3 @< The data-product container could not hold the next record
    }
    @ Persisted camera settings (SDD Table 1)
    enum Setting : U8 {
        CAMERA_MODE = 0
        EXPOSURE = 1
    }
    @ SETUP_CAMERA exchanges, in order (SDD Table 2)
    enum SetupStep : U8 {
        PING_MCU = 0
        SET_MODE = 1
        SET_EXPOSURE = 2
        READ_EXPOSURE = 3
        PING_SENSOR = 4
    }
    @ Ground commands and the parameter push, for rejection and outcome events
    enum Operation : U8 {
        SETUP_CAMERA = 0
        SLEEP = 1
        PING = 2
        MEMORY_STATUS = 3
        CAPTURE = 4
        DOWNLOAD = 5
        DOWNLOAD_ABORT = 6
        READ_REGISTER = 7
        PARAMETER_PUSH = 8
        NONE = 9 @< No operation in progress (CurrentOperation channel only)
    }
    @ Queue inputs that can overflow (SDD REQ-029)
    enum QueueInput : U8 {
        COMMAND = 0
        UART_RECEIVE = 1
        RUN_TICK = 2
        HEALTH_PING = 3
    }
    @ First bytes of a discarded frame, zero-padded (FrameDiscarded event)
    array FrameHead = [8] U8

    @ One memory slot of the Memory Status reply (datasheet §7.4.3)
    struct MemorySlot {
        mode: Mode @< Camera mode when the image was taken
        exposure: U16 @< Exposure of the image; 0 = test pattern
        uptime: U16 @< Camera uptime (s) when the image was taken
    }
    @ The four memory slots, slot 1 first
    array MemorySlots = [4] MemorySlot
    @ The last camera error reply (datasheet §7.4.9)
    struct CameraErrorInfo {
        cmdByte: U8 @< Command byte that caused the error
        errorType: ErrorType
        count: U8 @< Invalid commands since camera boot (rolls over at 256)
    }
    @ A Read Register reply (datasheet §7.4.7)
    struct RegisterValue {
        address: U16
        value: U16
    }
    @ First record of every Image data product (SDD Table 4)
    struct ImageHeader {
        slot: Slot
        resolution: Resolution
        width: U16 @< Pixels per line: 640 or 80
        height: U16 @< Lines in a complete image: 480 or 60
        cameraMode: Mode @< Camera mode at download time; whether the slot is an SD image or an HD quadrant comes from MemorySlots (Memory Status IMD)
        linesExpected: U16 @< Lines requested; the record count tells how many were received
    }
}

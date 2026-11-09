module KissCam {
    @ Camera mode as the camera reports it in every reply (datasheet §6.2, §7.2)
    enum Mode : U8 {
        SLEEP = 0 @< Image sensor held in reset
        SD = 1 @< Standard definition, 640 x 480 (preview 64 x 48), one of four slots
        ZOOM = 2 @< Centre 640 x 480 of the sensor (preview 64 x 48), one of four slots
        HD = 3 @< High definition, 1280 x 960 (preview 128 x 96) across slots 0..3 as quadrants
    }
    @ Capture modes the CAMERA_MODE parameter may select (Sleep is commanded with SLEEP)
    enum CaptureMode : U8 {
        SD = 1
        ZOOM = 2
        HD = 3
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
        PREVIEW = 1 @< 64 x 48, 48 lines of 64 pixels (SDD §Hardware notes)
    }
    @ Camera memory slot, the Capture/Download slot byte (datasheet §7.4.5; HD: SLOT_0 names the four-slot image)
    enum Slot : U8 {
        SLOT_0 = 0
        SLOT_1 = 1
        SLOT_2 = 2
        SLOT_3 = 3
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
        OUT_OF_RANGE = 5 @< A persisted parameter is outside its Table 1 range; nothing was sent
    }
    @ Why a ground command was rejected
    enum RejectCause : U8 {
        NOT_READY = 0 @< Command needs READY (SDD Table 2)
        BUSY = 1 @< An operation is in progress
        NO_DOWNLOAD = 2 @< DOWNLOAD_ABORT with no download in progress
        HD_SLOT = 3 @< CAPTURE or DOWNLOAD with a slot other than SLOT_0 while CAMERA_MODE is HD (REQ-035)
    }
    @ Why received bytes were discarded
    enum DiscardCause : U8 {
        NO_OPERATION = 0 @< Bytes arrived with no exchange outstanding
        MALFORMED = 1 @< No start byte, bad length, or missing terminator
        WRONG_COMMAND = 2 @< Well-formed reply acknowledging a command other than the outstanding one
    }
    @ Why a download failed before every line was received (a commanded abort is DownloadAborted)
    enum DownloadFailCause : U8 {
        LINE_FAILED = 0 @< A line failed LINE_RETRIES re-requests
        SEND_FAILED = 1 @< The UART driver refused a Download frame
        PRODUCT_FULL = 2 @< The data-product container could not hold the next record
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
        CAPTURE = 3
        DOWNLOAD = 4
        DOWNLOAD_ABORT = 5
        READ_REGISTER = 6
        PARAMETER_PUSH = 7
        NONE = 8 @< No operation in progress (CurrentOperation channel only)
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
        slot: Slot @< Slot commanded (SLOT_0 for an HD image, which spans slots 0..3)
        imageMode: Mode @< Mode byte of the first Download reply (a later reply whose mode byte differs is a ModeMismatch, REQ-021)
        resolution: Resolution
        width: U16 @< Image width in pixels: 640 (FULL) / 64 (PREVIEW); HD 1280 / 128
        height: U16 @< Image height in lines: 480 (FULL) / 48 (PREVIEW); HD 960 / 96
        exposure: U16 @< The component's last pushed or read-back exposure at download time (the camera reports none)
        linesExpected: U16 @< ImageLine records in a complete product: 480 (FULL) / 48 (PREVIEW); HD 1920 / 192
    }
}

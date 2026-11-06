module Fram {
    @ Status of a read or write request (SDD Table 4)
    enum Status : U8 {
        ACCEPTED = 0 @< Request taken; exactly one completion is delivered on the requesting port index
        OK = 1 @< Transfer completed, data valid
        OUT_OF_RANGE = 2 @< N = 0, N > Fram.FramConfig.MaxTransfer, offset > Fram.FramConfig.Size - N, or a null buffer data pointer
        BUSY = 3 @< A transfer (client request or startup read) was in progress, the SPI driver answered SPI_BUSY_ERR inside the call, or the request came from a completion delivered inside a spiWriteRead call (REENTRANT_IN_CALL)
        SPI_ERROR = 4 @< The SPI driver reported an error for a frame of this request: by completion, or by the call itself when it refused the first frame with other than SPI_BUSY_ERR
        WRITE_PROTECTED = 5 @< The write overlaps the block-protected range from a successful startup RDSR (datasheet Table 5; unknown range = empty)
    }

    @ Direction of a client request
    enum Op : U8 {
        READ = 0
        WRITE = 1
    }

    @ Where a client request's SPI error was reported (SpiError event, CPP-36 traceability)
    enum SpiErrorSite : U8 {
        REQUEST_REFUSED = 0 @< The SPI driver refused the first frame inside the request call (REQ-021); no completion
        READ_FRAME = 1 @< The READ frame completed non-OK
        WREN_FRAME = 2 @< The write-enable frame completed non-OK
        WRITE_FRAME = 3 @< The WRITE frame completed non-OK
    }

    @ Why a request was rejected (RequestRejected event; the port returns Fram.Status)
    enum RejectCause : U8 {
        TRANSFER_IN_PROGRESS = 0 @< A client transfer or startup read was outstanding (status BUSY)
        SPI_DRIVER_BUSY = 1 @< The SPI driver answered SPI_BUSY_ERR inside the call (status BUSY)
        REENTRANT_IN_CALL = 2 @< Issued from a completion that was itself delivered inside a spiWriteRead call, from any handler (status BUSY)
        OUT_OF_RANGE = 3 @< Table 1 limits violated (status OUT_OF_RANGE)
        WRITE_PROTECTED = 4 @< Write overlaps the block-protected range (status WRITE_PROTECTED)
    }

    @ The two reads the driver issues at startup, in order
    enum StartupRead : U8 {
        DEVICE_ID = 0 @< RDID: 9-byte device ID
        STATUS_REGISTER = 1 @< RDSR: status register (block-protect bits)
    }

    @ Progress of the startup sequence (telemetry)
    enum StartupState : U8 {
        NOT_STARTED = 0 @< startup() not yet called
        IN_PROGRESS = 1 @< RDID / RDSR frames outstanding; client requests are rejected BUSY
        COMPLETE = 2 @< Both startup reads returned OK; DeviceIdMatch and StatusRegister are valid
        READ_FAILED = 3 @< Set with StartupComplete when either startup read failed (StartupReadFailed says which); the failed read's channel was not updated
    }

    @ RDID reply in wire order (first byte received first)
    array DeviceId = [Fram.FramConfig.DeviceIdSize] U8 format "{x}"
}

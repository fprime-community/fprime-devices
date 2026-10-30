module Fram {
    @ Driver for the Infineon CY15B102QN serial F-RAM over the framework's asynchronous SPI ports
    @ (Drv.SpiWriteReadRequest / Drv.SpiWriteReadCallback). Passive and unguarded: the deployment must
    @ deliver the SPI completion in the same execution context as the clients' requests (a passive
    @ Drv.AsyncGuardedSpi driver completing from its scheduler entry, e.g. Samd21.SpiDriver). A request is
    @ accepted or rejected inside the call; an accepted request's frames complete through the callback,
    @ from which the driver issues the next frame or the client completion. One transfer at a time.
    passive component FramDriver {

        # ----------------------------------------------------------------------
        # Client interface: index = client; the completion goes out on the index the request came in on
        # ----------------------------------------------------------------------

        @ Read N = buffer.size bytes at offset into the caller's buffer
        sync input port read: [Fram.FramConfig.ClientPorts] Fram.Request

        @ Write N = buffer.size bytes at offset from the caller's buffer (write-enable frame, then write frame)
        sync input port write: [Fram.FramConfig.ClientPorts] Fram.Request

        @ Completion of an accepted read, on the index that made it
        output port readComplete: [Fram.FramConfig.ClientPorts] Fram.Complete

        @ Completion of an accepted write, on the index that made it
        output port writeComplete: [Fram.FramConfig.ClientPorts] Fram.Complete

        match readComplete with read
        match writeComplete with write

        # ----------------------------------------------------------------------
        # SPI bus: one slot (chip select) of an asynchronous SPI driver
        # ----------------------------------------------------------------------

        @ One full-duplex frame (opcode, address, data; write and read frames the same size) to the SPI driver
        output port spiWriteRead: Drv.SpiWriteReadRequest

        @ Frame completion from the SPI driver (non-interrupt context per the port contract; same context as the
        @ request ports per the deployment constraint above). Issues the next frame of the transfer in progress or
        @ delivers its completion. A refusal arriving inside any spiWriteRead call (request handler, completion
        @ handler or startup()) is only recorded and acted on when that call returns.
        sync input port spiWriteReadComplete: Drv.SpiWriteReadCallback

        # ----------------------------------------------------------------------
        # Events (SDD Table 2)
        # ----------------------------------------------------------------------

        @ The startup RDID reply differs from the configured FramConfig::EXPECTED_DEVICE_ID (deployment override
        @ selects the family member). Requests stay accepted.
        event DeviceIdMismatch(
            deviceId: Fram.DeviceId @< ID read, wire order
        ) severity warning high format "F-RAM device ID {} differs from the configured ID"

        @ The SPI driver completed a startup read with a non-OK status. The remaining startup read still runs.
        event StartupReadFailed(
            read: Fram.StartupRead @< Which startup read failed
            $status: Drv.SpiStatus @< SPI driver status
        ) severity warning high format "F-RAM startup read {} failed with SPI status {}"

        @ The startup status-register read shows BP1 or BP0 set — block protect: writes to the upper part of the
        @ array are silently discarded by the device (datasheet Table 5). Reported, not cleared.
        event BlockProtectSet(
            statusRegister: U8 @< Status register as read
        ) severity warning high format "F-RAM block protect set (status register {x}): writes to protected range are discarded"

        @ A request was rejected (REQ-006 / REQ-007 / REQ-021 / REQ-022). Throttled: 5 per boot.
        event RequestRejected(
            op: Fram.Op @< read or write
            client: FwIndexType @< Client port index
            offset: U32 @< Requested offset
            length: FwSizeType @< Requested N
            $status: Fram.Status @< BUSY (incl. SPI_BUSY_ERR refused inside the call), OUT_OF_RANGE or WRITE_PROTECTED
            cause: Fram.RejectCause @< Which check rejected it
        ) severity warning low format "F-RAM {} on port {} at offset {} size {} rejected: {} ({})" throttle 5

        @ The SPI driver reported a non-OK status for a frame of a client request: by completion after the call returned
        @ (REQ-009) or by a refusal other than SPI_BUSY_ERR inside the call (REQ-021). Throttled: 5 per boot.
        event SpiError(
            op: Fram.Op @< read or write
            client: FwIndexType @< Client port index
            offset: U32 @< Requested offset
            length: FwSizeType @< Requested N
            $status: Drv.SpiStatus @< SPI driver status
            site: Fram.SpiErrorSite @< Which frame, or the in-call refusal
        ) severity warning high format "F-RAM {} on port {} at offset {} size {} failed with SPI status {} at {}" throttle 5

        @ Both startup reads have completed, with or without warnings. Ground and test checkers may sequence on it;
        @ FSW clients cannot see events and retry on BUSY during the startup window.
        event StartupComplete() severity activity low format "F-RAM startup complete"

        # ----------------------------------------------------------------------
        # Telemetry (SDD Table 3). Counters are free-running U32 and wrap modulo 2^32.
        # ----------------------------------------------------------------------

        @ Reads completed with status OK
        telemetry ReadsCompleted: U32

        @ Writes completed with status OK
        telemetry WritesCompleted: U32

        @ Requests rejected BUSY (incl. SPI_BUSY_ERR refused inside the call), OUT_OF_RANGE or WRITE_PROTECTED
        telemetry RequestsRejected: U32

        @ Requests ended SPI_ERROR: by completion (REQ-009) or by an SPI-driver refusal other than SPI_BUSY_ERR inside the call (REQ-021)
        telemetry SpiErrors: U32

        @ Progress of the startup sequence
        telemetry StartupState: Fram.StartupState

        @ True once the startup RDID reply equalled FramConfig::EXPECTED_DEVICE_ID; false before, on mismatch and
        @ on a failed read (valid only when StartupState = COMPLETE)
        telemetry DeviceIdMatch: bool

        @ Status register as read at startup (BP1 = bit 3, BP0 = bit 2); 0 before the read and after a failed RDSR
        @ (StartupState = READ_FAILED)
        telemetry StatusRegister: U8

        # ----------------------------------------------------------------------
        # Special ports
        # ----------------------------------------------------------------------

        event port eventOut
        text event port textEventOut
        telemetry port tlmOut
        time get port timeGetOut
    }
}

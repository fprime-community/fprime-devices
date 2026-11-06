module Fram {
    @ Compile-time limits of the F-RAM driver (SDD Table 1). The implementation static_asserts
    @ 6 <= MaxTransfer <= Size <= 2^24 and ClientPorts >= 1.
    module FramConfig {
        @ Addressable bytes of the fitted device (CY15B102QN: 2 Mbit = 256 KiB). At most 2^24: the
        @ address goes out in three bytes. A request needs 1 <= N and offset <= Size - N.
        constant Size = 262144

        @ Largest N per request. Sizes the driver's two frame buffers (4 + MaxTransfer bytes each);
        @ at least 6 so the 10-byte startup RDID frame fits.
        constant MaxTransfer = 64

        @ Client port pairs: read/write request ports and their completion ports.
        constant ClientPorts = 2

        @ Bytes in the RDID (9Fh) reply. Fixed by the device, not a knob; defined here so Fram.DeviceId
        @ and the C++ EXPECTED_DEVICE_ID array share one definition.
        constant DeviceIdSize = 9
    }
}

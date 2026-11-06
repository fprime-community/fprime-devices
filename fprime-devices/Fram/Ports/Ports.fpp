module Fram {
    @ Read or write N = buffer.size bytes at byte offset. Returns ACCEPTED when the request is taken: exactly one
    @ Fram.Complete is delivered on the same port index (after this call returns with a driver completing from its own
    @ entry; possibly before it returns with a driver completing inside spiWriteRead), and the buffer (the caller's) must
    @ not be touched until then.
    @ Returns BUSY, OUT_OF_RANGE or (writes) WRITE_PROTECTED, checked in that order, when it is rejected (BUSY also for a
    @ request made from a completion delivered inside a spiWriteRead call, REENTRANT_IN_CALL); when the SPI driver refuses
    @ the first frame inside this call, BUSY (SPI_BUSY_ERR) or SPI_ERROR. No completion follows a rejection.
    port Request(
        offset: U32 @< Byte offset into the F-RAM of the first byte transferred; valid when offset <= Size - N
        ref buffer: Fw.Buffer @< Destination (read) or source (write); its size is N; a null data pointer is OUT_OF_RANGE
    ) -> Fram.Status

    @ Completion of an accepted request, on the index that made it. The buffer is the request's, with its
    @ original data pointer and size. status is OK or SPI_ERROR.
    port Complete(
        offset: U32 @< Byte offset of the request
        ref buffer: Fw.Buffer @< The buffer given with the request
        $status: Fram.Status @< OK or SPI_ERROR
    )
}

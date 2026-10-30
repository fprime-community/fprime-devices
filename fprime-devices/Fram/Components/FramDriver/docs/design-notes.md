# FramDriver design notes (not part of the SDD)

- **Startup entry = C++ `startup()` method**, not a port: nothing in a passive baremetal topology emits a start
  signal; `configure()/open()`-style calls from an instance phase are the F Prime convention for passive drivers.
  It runs after `configComponents` (SPI driver configured) in `startTasks`.
- **Single-context deployment constraint** (round-1 finding, all three reviewers): the `Drv.SpiWriteReadCallback`
  contract promises only non-interrupt context; the design is unguarded and relies on the SPI driver completing in the
  clients' context (true of `Samd21.SpiDriver`, which calls back from `activeIn`). Stated in the FPP doc comment and
  SDD §Ports. `guarded` would self-deadlock under REQ-010 (client re-requests from inside its completion).
- **In-call refusal** (round-1/2 must-fix): `Drv.AsyncGuardedSpi` lets the SPI driver deliver SPI_BUSY_ERR /
  SPI_CONFIG_ERR re-entrantly from inside any `spiWriteRead` — and for the guarded variant its non-recursive mutex is
  then still held. Uniform mechanism: every issuer (request handler, completion handler, `startup()`) sets an
  `issuing` mark before the call; `spiWriteReadComplete_handler` seeing the mark records the status and returns; the
  issuer acts after `spiWriteRead` returns (request → BUSY + RequestRejected on SPI_BUSY_ERR, else SPI_ERROR + SpiError, REQ-021; completion handler →
  client SPI_ERROR completion, REQ-009; startup → StartupReadFailed + next read). Nothing is issued or delivered from
  inside a re-entrant callback. Chosen over "constrain the driver" so the component stays safe (bounded, no lost or doubled
  completion) under the generic contract; REQ-005/REQ-010 ordering is claimed only under the §Ports deployment constraint
  (completions delivered from the driver's own entry) — see Waivers.
- **Startup telemetry** (DR-4.4.6.4): `StartupState.READ_FAILED` separates "read failed" from "mismatch"; a failed
  RDSR leaves the protected range empty (writes accepted per REQ-014) and READ_FAILED tells the ground protection is unknown.
- **Chaining frames from inside the SPI callback** is legal against `Samd21.SpiDriver` (fprime-samd `056d4d3`,
  `SpiDriver.cpp` `deliver()`): the pending record is cleared and the busy bits are down before
  `SpiWriteReadComplete_out` is called, and its SDD says the callback may start the next transfer. No scheduler port,
  no queue.
- **Component-owned frame buffers** (tx/rx, 4 + MaxTransfer each) rather than lending the client's buffer: the
  opcode/address header precedes the data on the wire, Samd21 needs equal-size buffers, and the client's buffer must
  come back with its original pointer and size (REQ-011). Floor `MaxTransfer >= 6` because the RDID frame is 10 bytes.
- **Block protect** (REQ-022, datasheet Table 5 [V cy15b102qn.pdf]): BP1:BP0 = 01 → 30000h–3FFFFh, 10 → 20000h–3FFFFh,
  11 → whole array. Range held from the startup RDSR; writes overlapping it are rejected WRITE_PROTECTED (checked after
  BUSY and OUT_OF_RANGE). No WRSR: protection is a deliberate configuration. WPEN/WP only guard the status register.
- **Telemetry split** (DR-4.4.6.1/4): `RequestsRejected` (BUSY incl. in-call SPI_BUSY_ERR, OUT_OF_RANGE, WRITE_PROTECTED) vs `SpiErrors` (SPI_ERROR outcomes); standing state
  `StartupState`, `DeviceIdMatch`, `StatusRegister` so the ground can tell after boot what the one-shot events said.
- **One SPI slot**: the component is one device; a second device is a second instance on another slot.
- **Events carry `op` / `read` discriminators** so each emission site is distinguishable (CPP-36).
- **No subtopology in fprime-devices**: the only `Drv.AsyncGuardedSpi` implementation is `Samd21.SpiDriver`
  (platform-specific); a subtopology would make the library depend on fprime-samd (design constraint, formerly REQ-019). The deployment wires
  `framDriver` to its SPI driver (one instance line plus two connections).
- **`DeviceIdSize` lives in FramConfig.fpp** (not a knob) so `Fram.DeviceId` and the C++ `EXPECTED_DEVICE_ID` share
  one generated constant (`FramConfig/FppConstantsAc.hpp`).
- **Status enum**: ACCEPTED is distinct from OK so a client cannot mistake acceptance for completion.

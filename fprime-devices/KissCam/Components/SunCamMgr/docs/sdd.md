# KissCam::SunCamMgr

## Introduction

`KissCam::SunCamMgr` is an active F Prime component that manages an MVP Aerospace KissCAM V2 sun camera over a UART
byte-stream driver (115200 baud, 8N1, set by the deployment). It turns ground commands into the camera's seven-byte
command frames, parses the camera's reply frames, reflects the camera's reported mode, exposure, counters and memory
status in telemetry and events, applies the persisted camera settings (Table 1) at set-up, and delivers each image
readout as one F Prime data product (one slot per product; on our hardware an HD-mode image is also one 640 × 480 slot, §Not supported on our hardware). A camera operation is one `SETUP_CAMERA`, one Table 1 push with its follow-ups, one whole `DOWNLOAD`, or
one other command/reply exchange; at most one operation is in progress at a time. The camera is powered by another component, so the operator calls
`SETUP_CAMERA` once it is on; until then the component is NOT_SETUP.

Device identity: KissCAM V2 Datasheet and User Manual, Revision 1.3 (May 2025; Rev 1.2 of May 2024 is the point of change, Rev 1.1 of April 2023 is superseded). Bench unit on `/dev/ttyUSB0`
(FTDI USB-RS422 cable FTAQMGVF): Ping reply `40 50 00 00 04 01 00 94 d1 0d` = ACK `P`, mode Sleep, 1 valid / 0 invalid
commands, uptime 38097 s — the datasheet's §7.4.1 reply format. At power-on the unit emits exactly 21 unframed ASCII bytes,
`4d 56 50 20 43 61 6d 65 72 61 20 56 32 20 52 53 34 32 32 0d 0a` = "MVP Camera V2 RS422\r\n" (bench capture 2026-11-09, H7): the
product family and interface variant only — no firmware revision is reported in-band (no version field in any reply, no
banner elsewhere). The datasheet's Write Register command is not used.

Revision for hardware (2026-11-09). The approved design met the bench unit and lost on four points; the human ruled
"it must be functional on the hardware we have": (1) no Memory Status before a download, (2) a 0-byte Capture ACK is
nominal, (3) constants set to the measured hardware, (4) unsupported features moved to a table. Bench facts (byte-exact,
bench probe, probes-1..3 / dry-1,2 records): H1 Memory Status (`0x53` and the table's `0x54`) answers error type 1,
unrecognised command; H2 Capture ACK `40 43 <mode> 00 00 0D`, 0 payload bytes, image stored; H3 full line = 2-byte echo +
640 px (LEN 0x0282), preview line = 2 + 64 px (LEN 0x0042), preview image 64 × 48 (lines ≥ 48 return live sensor data,
never an error), full 640 × 480; H4 the camera refuses nothing (Download slot 5 / line 480 / 481, Set Exposure 0, Capture
slot 0, Read Register 0xFFFF all answer normal frames; reads past the end run into the next slot); H5 HD mode changes
only the reply mode byte — an HD Capture writes one 640 × 480 / 64 × 48 slot, slots 2-3 keep their SD content, slot 4 is not
guaranteed; H6 reply mode byte, Ping, Set Mode, Set Exposure, Read Register 0x3012, 115200 8N1 per datasheet; H7 power-on banner
"MVP Camera V2 RS422\r\n" (21 unframed bytes, undocumented). Where the
bench disagrees with the datasheet the bench is the truth; §Not supported on our hardware records each discrepancy.

Datasheet input corrected (phase 2). The design was first written against Rev 1.1; the authoritative input is now Rev 1.3
(Rev 1.2 → 1.3 unchanged in the points below). Rev 1.3 confirms H1 and H2 as documented behaviour: §7.4.3 Memory Status "is
planned future functionality and is not currently implemented" (command table §7.3 still lists 0x54 'S'; the bench answers
unrecognised command to 0x53 and 0x54 alike), and the §7.4.5 Capture response figure is `40 43 MD 00 00 0D` — 0 telemetry
bytes — while the §7.4.5 prose still says "ACK with two telemetry bytes indicating the exposure" (internally inconsistent, so
REQ-013/-033 accept 0 or 2). Rev 1.3 also defines four modes, §6.2/§7.4.2: 0 Sleep, 1 SD, 2 Zoom (centre 640 × 480 crop),
3 HD (1280 × 960 across all four slots, downloaded as four SD images, Capture slot ignored; the Figure 5 caption "1280 x 640"
is a typo); a mode change resets the sensor registers including exposure to defaults (REQ-027's CAMERA_MODE → EXPOSURE push is
spec-backed); the Capture table says Memory Slot 0-3 where the prose and Download say 1-4 (inconsistent, probes-5 decides);
preview is still 80 × 60 / 60 lines (the bench's 64 × 48 stays a hardware discrepancy); error type 0x03 = wrong mode; MRAM
16 Mb. Our earlier "HD" probes sent mode byte 2, which Rev 1.3 defines as Zoom, so H5 is re-measured by probes-5 (phase C):
`Mode` (ZOOM = 2, HD = 3), HD four-slot behaviour and slot indexing follow that result; the HD and Zoom rows of §Not
supported are pending until then.

## Requirements

| ID | Requirement | Verification |
|---|---|---|
| REQ-SunCamMgr-001 | Frames sent and parsed shall comply with Datasheet Rev. 1.3 §7 as corrected by bench facts H1–H7. | Unit Test |
| REQ-SunCamMgr-002 | Camera frames shall be sent only for a Table 2 command or Table 1 push. | Unit Test |
| REQ-SunCamMgr-003 | `SETUP_CAMERA` shall run the Table 2 set-up exchanges in order, then become READY. | Unit Test |
| REQ-SunCamMgr-004 | A failed set-up step shall return the component to NOT_SETUP with a WARNING_HI event. | Unit Test |
| REQ-SunCamMgr-005 | `SETUP_CAMERA` shall be accepted in NOT_SETUP and READY. | Unit Test |
| REQ-SunCamMgr-006 | `SLEEP` and `PING` shall be accepted in NOT_SETUP and READY. | Unit Test |
| REQ-SunCamMgr-007 | `SLEEP` shall command camera mode Sleep and return the component to NOT_SETUP. | Unit Test |
| REQ-SunCamMgr-008 | Table 2 commands marked READY shall be rejected with a WARNING_LO event when not READY. | Unit Test |
| REQ-SunCamMgr-009 | A command except `DOWNLOAD_ABORT` during an operation shall be rejected BUSY with a WARNING_LO event. | Unit Test |
| REQ-SunCamMgr-010 | Table 1 parameters changed while READY shall be pushed when no operation is in progress. | Unit Test |
| REQ-SunCamMgr-011 | A failed parameter push shall return the component to NOT_SETUP with a WARNING_HI event. | Unit Test |
| REQ-SunCamMgr-012 | Exposure shall be read back from register 0x3012 after every exposure push and reported. | Unit Test |
| REQ-SunCamMgr-013 | `CAPTURE` shall send Capture with the commanded slot and test-pattern flag, accepting a 0- or 2-byte ACK. | Unit Test |
| REQ-SunCamMgr-014 | `DOWNLOAD` shall request every line of the commanded slot, FULL_LINES or PREVIEW_LINES, in order. | Unit Test |
| REQ-SunCamMgr-015 | A download shall produce one data product per slot: Table 4 header, then line records. | Unit Test |
| REQ-SunCamMgr-016 | A failed or timed-out line reply shall be retried LINE_RETRIES times before the download fails. | Unit Test |
| REQ-SunCamMgr-017 | A download ended early shall deliver lines received: ACTIVITY_HI event if commanded, WARNING_HI otherwise. | Unit Test |
| REQ-SunCamMgr-018 | No reply within REPLY_TIMEOUT_TICKS `run` ticks shall fail the exchange with a WARNING_HI event. | Unit Test |
| REQ-SunCamMgr-019 | A camera error reply shall raise a WARNING_HI event carrying command, error type and count. | Unit Test |
| REQ-SunCamMgr-020 | Bytes outside a frame and out-of-operation, malformed or wrong-command frames shall be discarded and counted with a WARNING_LO event. | Unit Test |
| REQ-SunCamMgr-021 | While READY, a reply whose mode ≠ commanded mode shall cause NOT_SETUP with a WARNING_HI event. | Unit Test |
| REQ-SunCamMgr-022 | The component shall report the Table 3 telemetry from the source named for each channel. | Unit Test |
| REQ-SunCamMgr-023 | Every command outcome and every Table 1 setting applied shall be reported by an event. | Unit Test |
| REQ-SunCamMgr-024 | A UART send or data-product allocation failure shall end the operation with a WARNING_HI event. | Unit Test |
| REQ-SunCamMgr-025 | Table 2 commands shall run their exchange and report the reply by event and telemetry. | Unit Test |
| REQ-SunCamMgr-026 | `DOWNLOAD_ABORT` with no download in progress shall be rejected with a WARNING_LO event. | Unit Test |
| REQ-SunCamMgr-027 | A CAMERA_MODE push shall be followed by an EXPOSURE push and its readback. | Unit Test |
| REQ-SunCamMgr-028 | An exposure readback differing from the pushed value shall cause NOT_SETUP with a WARNING_HI event. | Unit Test |
| REQ-SunCamMgr-029 | A full queue shall drop the newest input: buffer returned, command answered BUSY, WARNING_HI event. | Unit Test |
| REQ-SunCamMgr-030 | Component state and counter channels shall be written on change and every TLM_PERIOD_TICKS `run` ticks. | Unit Test |
| REQ-SunCamMgr-031 | An out-of-range EXPOSURE shall send no frame; a push is refused, WARNING_LO event, READY kept. | Unit Test |
| REQ-SunCamMgr-032 | Withdrawn — hardware (Introduction H1, H5). | Inspection |
| REQ-SunCamMgr-033 | A Capture outcome shall report the ACK's exposure when present, else the `Exposure` channel value. | Unit Test |

Table 1 — Parameters (persisted camera settings, pushed at set-up and on change)

| Parameter | Type / range | Default | Camera command |
|---|---|---|---|
| CAMERA_MODE | enum SD = 1, HD = 2 (the Set Mode byte, §7.4.2) | SD | Set Mode (`M`) |
| EXPOSURE | U16, EXPOSURE_MIN..EXPOSURE_MAX (2..960) steps of 89.5 µs, checked by the component (§7.4.4) | 672 | Set Exposure (`E`) |

Table 2 — Commands (ground operations)

| Command | Arguments | Needs READY | Camera exchange(s) |
|---|---|---|---|
| SETUP_CAMERA | — | no | set-up steps, in order: Ping MCU, Set Mode, Set Exposure (preceded by the local EXPOSURE range check), Read Register 0x3012, Ping MCU+sensor |
| SLEEP | — | no | Set Mode Sleep |
| PING | type: MCU, MCU_AND_SENSOR | no | Ping |
| MEMORY_STATUS | — | yes | Memory Status (standalone; Rev 1.3 §7.4.3: not implemented — the camera answers UNRECOGNISED_COMMAND → `CameraError`, the L5-2 camera-error stimulus) |
| CAPTURE | slot: SLOT_1..SLOT_4, testPattern: OFF/ON | yes | Capture |
| DOWNLOAD | slot: SLOT_1..SLOT_4, resolution: FULL/PREVIEW | yes | Download × 480 (full) or × 48 (preview) of the commanded slot, no Memory Status |
| DOWNLOAD_ABORT | — | no (accepted only during a download) | none; the download stops after the pending line |
| READ_REGISTER | address U16 | yes | Read Register |

Table 3 — Telemetry

| Channel | Type | Source |
|---|---|---|
| SetupState | enum NOT_SETUP, SETTING_UP, READY | component state |
| CurrentOperation | enum NONE, SETUP_CAMERA, SLEEP, PING, MEMORY_STATUS, CAPTURE, DOWNLOAD, READ_REGISTER, PARAMETER_PUSH | component state (NONE when no operation is in progress) |
| CameraMode | enum SLEEP = 0, SD = 1, HD = 2 (the reply mode byte, §7.2) | mode byte of every reply |
| Exposure | U16 | EXPOSURE value whose Set Exposure was acknowledged; Capture reply exposure |
| ExposureReadback | U16 | Read Register 0x3012 reply |
| CameraUptime | U16 s | Ping reply |
| RegisterValue | {address, value} | READ_REGISTER command reply |
| CameraValidCmds, CameraInvalidCmds | U8 | Ping / error replies |
| MemorySlots | array[4] of {mode (same enum), exposure, uptime} | Memory Status reply (Rev 1.3 §7.4.3 format; never written while Memory Status is unimplemented) |
| LastCameraError | {command, errorType, count} | error reply |
| Exchanges, CameraErrors, Timeouts, DiscardedBytes, MalformedReplies | U32 | component counters |
| ImagesCaptured, LinesDownloaded, ProductsSent, QueueOverflows | U32 | component counters |

Table 4 — Constants and data-product records

| Name | Value | Meaning |
|---|---|---|
| REPLY_TIMEOUT_TICKS | 20 (configurable) | `run` ticks allowed per camera reply (datasheet: 2000 ms) |
| LINE_RETRIES | 3 (configurable) | re-requests of one image line before the download fails |
| TLM_PERIOD_TICKS | 10 (configurable) | `run` ticks between periodic writes of the component state and counter channels (1 s at 10 Hz) |
| EXPOSURE_MIN, EXPOSURE_MAX | 2, 960 | EXPOSURE values the component sends (datasheet §7.4.4) |
| FULL_LINES, PREVIEW_LINES | 480, 48 | Download line requests per image (bench H3; datasheet §7.4.6 says 60 preview lines) |
| FULL_PIXELS, PREVIEW_PIXELS | 640, 64 | pixel bytes per line reply (bench H3; datasheet says 80 preview) |
| ImageHeader record | slot, imageMode, resolution, width, height, exposure, linesExpected | first record of every image product, written when the first line reply arrives; `imageMode` = mode byte of that first reply (every later reply is checked against the commanded mode, REQ-021); `exposure` = the component's last pushed/read-back exposure (the camera reports none); width × height = 640×480 (FULL) / 64×48 (PREVIEW); `linesExpected` = 480 / 48 |
| ImageLine record | U8 array: slot (1 byte), line number (2 bytes, MSB first), then 640 or 64 pixels | one record per line received, camera byte order |

## Design

### Block diagram
```
 drvConnected : Drv.ByteStreamReady ─▶┌─────────────────────────────┐─▶ drvSendOut : Drv.ByteStreamSend
 drvReceiveIn : Drv.ByteStreamData  ─▶│     KissCam.SunCamMgr       │─▶ drvReceiveReturnOut : Fw.BufferSend
 run : Svc.Sched (sync → atomic → runInternal) ─▶│   (active)        │─▶ pingOut : Svc.Ping
 pingIn : Svc.Ping                  ─▶└─────────────────────────────┘─▶ productGetOut, productSendOut (data products)
```
### Ports
| Port | Kind | Type | Purpose |
|---|---|---|---|
| drvConnected | async input, `drop` | Drv.ByteStreamReady | driver ready; handler empty, no state kept |
| drvReceiveIn | async input, `hook` | Drv.ByteStreamData | camera bytes; async (not the sync `ByteStreamDriverClient` import) so framing runs on the component thread |
| drvReceiveReturnOut | output | Fw.BufferSend | return driver receive buffers |
| drvSendOut | output | Drv.ByteStreamSend | send a 7-byte frame from the component's buffer |
| run | sync input | Svc.Sched | rate-group tick, caller's thread: test-and-set the atomic `m_runQueued`; when it was clear, invoke `runInternal` |
| runInternal | internal port, `hook` | — | the tick on the component thread: reply timeouts, deferred parameter push, periodic channels; handler clears the atomic; the queue-full hook clears it and counts the drop (RUN_TICK) — at most one run message is ever queued |
| pingIn / pingOut | async input `hook` / output | Svc.Ping | health ping |
| productGetOut / productSendOut | product get / send | — | allocate and deliver the Image container |

Special ports: cmdIn/cmdRegOut/cmdResponseOut, eventOut/textEventOut, tlmOut, timeGetOut, prmGetOut/prmSetOut. All eight
commands (Table 2) are `async` with queue-full `hook`. Hooks run in the caller's thread and do only this: a command hook
answers `cmdResponse BUSY`, the `drvReceiveIn` hook returns the buffer, the `runInternal` hook clears `m_runQueued`, and every hook increments its source's
mutex-guarded drop count (one per `QueueInput`) — no other member is touched; on its next tick the component thread emits
one `QueueOverflow(source, dropped)` per non-zero source and adds the sum to `QueueOverflows` (REQ-029). Parameters are Table 1; telemetry is Table 3, one channel per FPP row with units in
the doc-comment; data-product records are Table 4 (`ImageHeader` struct, `ImageLine` `U8 array`, container `Image`).

### Events
| Event (args) | Severity | When |
|---|---|---|
| SetupStarted | ACTIVITY_LO | SETUP_CAMERA accepted |
| SetupComplete(mode, exposure) | ACTIVITY_HI | last set-up exchange acknowledged; READY |
| SetupFailed(step, cause) | WARNING_HI | a set-up exchange failed; NOT_SETUP |
| SettingApplied(setting, value) | ACTIVITY_HI | Set Mode / Set Exposure acknowledged |
| SettingRejected(setting, value, min, max) | WARNING_LO | EXPOSURE outside EXPOSURE_MIN..MAX refused before any frame; the applied value stands |
| SettingPushFailed(setting, cause) | WARNING_HI | push outside set-up failed; NOT_SETUP |
| ExposureReadback(pushed, readback) | ACTIVITY_LO | register 0x3012 read after an exposure push |
| SleepEntered | ACTIVITY_HI | Set Mode Sleep acknowledged; NOT_SETUP |
| PingReply(pingType, validCmds, invalidCmds, uptime) | ACTIVITY_LO | Ping acknowledged |
| MemoryStatusReply(slots) | ACTIVITY_LO | Memory Status acknowledged (Rev 1.3 §7.4.3: not implemented, never seen) |
| CaptureComplete(slot, testPattern, exposure, mode) | ACTIVITY_HI | Capture acknowledged (slot as sent; exposure = the ACK's 2-byte value when present, else the last pushed/read-back exposure) |
| DownloadStarted(slot, imageMode, resolution, lines) | ACTIVITY_LO | first line reply received: imageMode = its mode byte, ImageHeader record written (the product was allocated at the command) |
| DownloadComplete(slot, imageMode, resolution, lines, bytes) | ACTIVITY_HI | last line stored, product sent; bytes = image bytes received (lines × 640 or × 64), not serialized record bytes |
| DownloadAborted(slot, linesReceived) | ACTIVITY_HI | DOWNLOAD_ABORT ended the download; lines received delivered |
| DownloadFailed(slot, linesReceived, cause) | WARNING_HI | download ended on a fault (LINE_FAILED, SEND_FAILED, PRODUCT_FULL); lines received delivered; a ModeMismatch during the line phase is reported as LINE_FAILED preceded by the ModeMismatch event |
| LineRetry(line, attempt) | DIAGNOSTIC | a line is re-requested |
| RegisterRead(address, value) | ACTIVITY_HI | Read Register acknowledged |
| CommandRejected(operation, cause) | WARNING_LO | NOT_READY, BUSY, NO_DOWNLOAD; response EXECUTION_ERROR (BUSY for cause BUSY) |
| CameraError(operation, cmdByte, errorType, count) | WARNING_HI | 0xFF reply |
| ReplyTimeout(operation, cmdByte) | WARNING_HI | REPLY_TIMEOUT_TICKS elapsed |
| FrameDiscarded(cause, bytes, head), throttle 10 | WARNING_LO | NO_OPERATION, MALFORMED, WRONG_COMMAND; first 8 bytes kept |
| ModeMismatch(commanded, reported) | WARNING_HI | reply mode ≠ commanded while READY; NOT_SETUP |
| SendFailed(operation, status) | WARNING_HI | driver refused the frame |
| ProductAllocationFailed(bytes) | WARNING_HI | no Image container of the needed size |
| QueueOverflow(source, dropped), throttle 10 | WARNING_HI | drops counted by the hooks since the last tick (COMMAND, UART_RECEIVE, RUN_TICK, HEALTH_PING) |

### Configure
`KissCamConfig` is a `register_fprime_config` module; a deployment overrides `KissCamConfig.fpp` through its config
directory. `run` is driven at 10 Hz so that REPLY_TIMEOUT_TICKS × period = 2 s (datasheet §7); the tick-counted timeout expires between (REPLY_TIMEOUT_TICKS − 1) and REPLY_TIMEOUT_TICKS periods after the send (1.9–2.0 s at 10 Hz); a deployment with another
rate re-sets the constant to keep ≈ 2 s. The deployment opens the UART at 115200 8N1 with 1024-byte receive buffers.
```fpp
constant REPLY_TIMEOUT_TICKS = 20   # run ticks per reply (2 s at 10 Hz)
constant LINE_RETRIES = 3           # re-requests of one line before the download aborts
constant IMAGE_DP_PRIORITY = 10     # default priority of the Image container
constant TLM_PERIOD_TICKS = 10      # run ticks between periodic channel re-writes (1 s at 10 Hz)
constant EXPOSURE_MIN = 2            # EXPOSURE range the component sends (datasheet §7.4.4)
constant EXPOSURE_MAX = 960
instance sunCamMgr: KissCam.SunCamMgr base id 0x... queue size 64 stack size 65536 priority 100
```
Image container size per image: `Header::SIZE + 2·hash + SIZE_OF_ImageHeader_RECORD + lines × SIZE_OF_ImageLine_RECORD
(12 B record overhead with a 64-bit FwSizeType + 3 + pixels)` ≈ 315 KB full (480 × 655 B), ≈ 4 KB preview (48 × 79 B); every image is
one slot whatever the mode, so the data-product buffer manager needs a bin ≥ 320 KB (a 1.3 MB HD bin is no longer needed). Each queued receive
holds one UART pool buffer, so while pool buffers are available UART_RECEIVE cannot by itself overflow a queue sized ≥ pool
depth + pending command, tick and ping messages; the driver's 50 ms null-buffer notices after pool exhaustion, and any other
overflow, fall back to: dropped buffers returned, reply fails as malformed, line retried. Pool depth and queue size are set from
the bench-measured buffers-per-reply N (stage 2) with 2× margin, independent of task priorities (Linux honours them only
with RT scheduling rights); download durations and sequence timeouts are deployment matters (topology SDD).
REPLY_TIMEOUT_TICKS and LINE_RETRIES are compile-time by design: the timeout is the datasheet's fixed 2000 ms, LINE_RETRIES
only bounds an abort the operator can re-command; both are deployment overrides, not flight parameters.

### Behavior
A command is checked against Table 2 and `CurrentOperation`; a refusal emits `CommandRejected`, otherwise it becomes the
operation in progress, its first 7-byte frame leaves on `drvSendOut` from the component's own buffer, and its command
response is deferred to the operation's end (durations and sequence timeouts: topology SDD); SLEEP ends in NOT_SETUP either way and its reply
is exempt from the mode check. Receive buffers with status OP_OK are appended to a component-local 648-byte assembler
(rather than `Svc.FrameAccumulator`, whose `ComDataWithContext` ports would need an adapter and a second topology
component), others are counted as discarded and returned when they carry data (a null notice has nothing to return), and a reply is complete when the length field (excluding the
final 0x0D; image payloads may contain 0x0D) is satisfied. Bytes outside a frame (for example the unframed power-on banner, H7, which arrives whenever the camera is powered while the component is receiving), and a frame whose length field exceeds the assembler or whose last byte is not 0x0D, are skipped up to the next 0x40 or the end of the receive buffer and reported as one `FrameDiscarded(MALFORMED)` carrying their count and first 8 bytes; a reply left incomplete when its exchange times out is discarded `MALFORMED`; a Download reply whose payload is not the 2-byte line echo plus exactly the expected pixel count (640 full, 64 preview) is discarded `MALFORMED` and not stored, so the line fails by the reply timeout and is re-requested (REQ-SunCamMgr-016); and a fragment still in the assembler when the next exchange starts is discarded `NO_OPERATION`; a reply whose payload length, Memory Status mode bytes, error type or mode byte cannot be decoded is discarded the same way and the exchange stays pending. A complete reply is matched to the outstanding command byte and
to every parameter it echoes (Download line number, Read Register address) — a mismatch, or a reply after its exchange
timed out, is discarded (WRONG_COMMAND / NO_OPERATION; replies carry no sequence number, so a late reply to a re-sent
command of the same byte and echo is indistinguishable) — then mode-checked (against the mode just set for a Set Mode reply, otherwise the commanded mode) and decoded into events and channels. A Capture ACK with no payload (Rev 1.3 §7.4.5 response figure `40 43 MD 00 00 0D`, bench H2) is nominal: `CaptureComplete.exposure` is the
component's last pushed or read-back exposure; a 2-byte payload (the §7.4.5 prose) is decoded and reported instead. `run`
ticks count a per-exchange timeout; on expiry or a 0xFF reply the exchange fails and the operation ends per the
requirements (set-up/push → NOT_SETUP; download line → `LineRetry`, then `DownloadFailed`), while a download allocates one
product for the commanded slot at the command (FULL_LINES / PREVIEW_LINES; no Memory Status — Rev 1.3 §7.4.3: not implemented, H1),
writes the `ImageHeader` record and emits `DownloadStarted` when the first line reply arrives (its mode byte is `imageMode`; a later
reply whose mode byte differs is a `ModeMismatch`, REQ-021; the component's last pushed/read-back exposure is `exposure`), adds one `ImageLine` record (slot, line, pixels) per reply and sends the product on completion,
`DownloadAborted` (commanded, ACTIVITY_HI) or `DownloadFailed` (WARNING_HI). EXPOSURE is range-checked by the component:
a value outside EXPOSURE_MIN..EXPOSURE_MAX is refused with one `SettingRejected` and no frame (the refusal clears the dirty
flag, the next PRM_SET re-arms it; the applied value stands, READY is kept), and at set-up it fails the SET_EXPOSURE step
locally — `SettingRejected(EXPOSURE, value, min, max)` then `SetupFailed(SET_EXPOSURE, OUT_OF_RANGE)`, NOT_SETUP — before Set
Exposure is sent. DOWNLOAD answers OK on `DownloadComplete` and on a commanded `DownloadAborted` (the partial product was
delivered), EXECUTION_ERROR on `DownloadFailed`; DOWNLOAD_ABORT answers OK once the download has stopped (also when it stopped on `DownloadFailed`), and a second DOWNLOAD_ABORT while one is pending is refused BUSY. `DownloadComplete.bytes` is the image bytes received: lines × FULL_PIXELS or × PREVIEW_PIXELS (3072 for a complete preview, 307200 for a full image) — the camera-determined quantity, not the serialized record bytes, which depend on `FwSizeType` and the header struct.

Tick path: `run` (sync, rate-group thread) test-and-sets the atomic `m_runQueued` and, when it was clear, invokes
`runInternal`, whose handler and queue-full hook both clear it — the queue never holds more than one tick (the pattern of
`Svc.EventManager`/`Svc.FileManager`). `parameterUpdated` only sets a mutex-guarded dirty flag, one per Table 1 parameter; `SETUP_CAMERA` applies the stored CAMERA_MODE and EXPOSURE and clears both
flags (a parameter changed while NOT_SETUP is never pushed on its own); the `runInternal` handler pushes the dirty parameters while idle (`CAMERA_MODE` then `EXPOSURE` + readback; an EXPOSURE-only change pushes `EXPOSURE` + readback), and on its first tick
the component reports SetupState NOT_SETUP, CurrentOperation NONE and zero counters and re-writes the component state and counter channels (SetupState, CurrentOperation, nine U32 counters) every `TLM_PERIOD_TICKS` ticks (1 Hz at the 10 Hz rate) as well as on change; camera-sourced channels are written on reply only. Replies are parsed by length within the 648-byte assembler; the exposure readback is compared with the value sent.

### Data product
One `Image` container per downloaded image (DpContainer header, then the records; one slot per image in SD or HD mode):
```
 ┌──────────────────────────────┬──────────────────────────────────────────────────┬────────────┐
 │ Fw::DpContainer::Header      │ records                                          │ data hash  │
 │ id, priority, time, procTypes│ ┌──────────────────────────────────────────────┐ │            │
 │ userData, dpState, dataSize  │ │ record id 0  ImageHeader (serialized struct) │ │            │
 │ header hash                  │ │  slot │imageMode│resolution│width│height│…   │ │            │
 │                              │ ├──────────────────────────────────────────────┤ │            │
 │                              │ │ record id 1  ImageLine  U8 array, size + data│ │            │
 │                              │ │  [slot][line MSB][line LSB][pixel 0 … N-1]   │ │            │
 │                              │ │   N = 640 (FULL) or 64 (PREVIEW)             │ │            │
 │                              │ ├──────────────────────────────────────────────┤ │            │
 │                              │ │  … one ImageLine per line received, in the   │ │            │
 │                              │ │  order requested: slot s lines 0..479 (FULL) │ │            │
 │                              │ │  or 0..47 (PREVIEW)                          │ │            │
 │                              │ └──────────────────────────────────────────────┘ │            │
 └──────────────────────────────┴──────────────────────────────────────────────────┴────────────┘
   FULL: 1 + 480 records, ≈ 315 KB (480 × (3 + 640) px/prefix bytes + record overhead + header)
   PREVIEW: 1 + 48 records, ≈ 4 KB (48 × (3 + 64) + record overhead + header)
```
A product that ends early (abort or failure) holds the header and the lines received; `linesExpected` in the header and the
record count tell the ground how much is missing. An HD-mode product has the same shape; only its `imageMode` says HD.

```mermaid
sequenceDiagram
    participant G as Ground
    participant M as SunCamMgr
    participant U as UART driver
    participant D as DpManager
    G->>M: DOWNLOAD(slot, FULL)
    M->>D: productGetOut(Image, size for resolution)
    loop line = 0..479 (FULL) or 0..47 (PREVIEW) (retry ≤ LINE_RETRIES on timeout / error)
        M->>U: drvSendOut "@D s 0 line"
        U-->>M: drvReceiveIn (several buffers) / run ticks
        M->>M: assemble, check ACK/mode/line; first reply → serializeRecord_ImageHeader (imageMode = its mode byte) + DownloadStarted; serializeRecord_ImageLine, return buffers
    end
    M->>D: productSendOut(Image)
    M-->>G: DownloadComplete, cmdResponse OK (DownloadAborted / DownloadFailed on an early end)
```
Out-of-scope failure modes
| Failure | Owner |
|---|---|
| Camera power off / power cycling | the power-control component; operator re-runs SETUP_CAMERA |
| Camera health while idle | operator PING (REQ-002: no autonomous frames); a power-cycle shows as ModeMismatch at the next exchange |
| Serial device missing, wrong baud, driver read errors | Drv.PosixUartDriver and the deployment |
| Data-product storage, cataloguing and downlink | Svc.DpManager / DpWriter / DpCatalog |

### Not supported on our hardware
The bench unit (Introduction, "Revision for hardware") differs from Datasheet Rev 1.3 in these points. Each row is a design
decision: the component is built for the hardware we have, not for the datasheet. Memory Status and the 0-byte Capture ACK
are no longer discrepancies — Rev 1.3 documents both (§7.4.3 "not currently implemented", §7.4.5 response figure) — and are
specified as normal behaviour above; Memory Status stays the standalone `MEMORY_STATUS` command and the L5-2 camera-error stimulus.

| Feature | Datasheet ref | Observed bench behavior (bench probe, probes-1..3 / dry-1,2 records) | Disposition in this design |
|---|---|---|---|
| Preview image 80 × 60 | §7.4.6 (Rev 1.1–1.3) | Preview line = 2-byte echo + 64 px (LEN 0x0042); 48 lines; lines ≥ 48 return live sensor data of the next region, never an error (H3) | Preview = 64 × 48 everywhere (Resolution.PREVIEW, ImageHeader, product sizes, plan PNGs) |
| HD image = four quadrant slots, 1280 × 960 — PENDING probes-5 (phase C) | §6.2, §7.4.2 (Rev 1.3: mode 3 HD; mode 2 is Zoom), Fig. 3, §7.4.5 | Only the reply mode byte changes (02); an HD Capture writes one 640 × 480 / 64 × 48 slot; slots 2-3 keep their SD content; slot 4 not guaranteed after an HD capture (H5) | Four-slot assembly dropped — option (a) below; an HD-mode image is one slot whose header `imageMode` is HD. REQ-032, the unit tests `testDownloadHdImage` / `testDownloadNoImage` (replaced by `testDownloadHdSingleSlot`) and `DownloadFailCause.NO_IMAGE` / `MEMORY_STATUS_FAILED` withdrawn. Operator note: after an HD capture re-capture slot 4 before trusting it. |
| Camera-side parameter validation / error replies for bad slot, line, exposure, register | §7.4.9 (error types 2, 3) | Download slot 5 / line 480 / line 481, Set Exposure 0, Capture slot 0, Read Register 0xFFFF all answer normal frames; reads past the end run linearly into the next slot's memory (H4) | All range checks stay in the component (Slot enum, 480/48 line counts, EXPOSURE_MIN..MAX, REQ-031); `ErrorType.INVALID_PARAMETER` / `WRONG_MODE` kept for decoding (datasheet error types 2, 3) but no test can provoke them; L5-2 uses Memory Status instead of READ_REGISTER 0xFFFF |
| Capture uptime per slot (Memory Status payload) | §7.4.3 | Not available: Memory Status not implemented (Rev 1.3, H1) | `ImageHeader` carries no uptime; `exposure` is the component's last pushed/read-back value |
| Power-on banner (undocumented) | — (not in Rev 1.1, 1.2 or 1.3) | 21 unframed ASCII bytes `4d 56 50 20 43 61 6d 65 72 61 20 56 32 20 52 53 34 32 32 0d 0a` = "MVP Camera V2 RS422\r\n" at camera power-on, then nothing; no firmware revision (H7, bench capture, camera power cycled by the human) | Treated as bytes outside a frame (REQ-020): skipped to the next 0x40, one `FrameDiscarded(MALFORMED, 21, "MVP Came…")`, `DiscardedBytes` += 21, no state change; unit test `testPowerOnBanner` with these exact bytes. No system test: reproducing it needs a camera power cycle, which only the human performs on request (bench rule: no hub power cycling) — recorded as a UT-only waiver in the plan. Identity: product family / interface variant only; firmware unidentifiable in-band |
| Zoom mode (mode 2, centre 640 × 480 crop) — PENDING probes-5 (phase C) | §6.2, §7.4.2 (Rev 1.2+) | Not probed as Zoom: the earlier "HD" probes sent mode byte 2, which Rev 1.3 defines as Zoom (H5 likely measured Zoom) | `Mode` gains ZOOM = 2 and HD becomes 3 if probes-5 confirms; disposition follows the measurement |

Memory Status: SETTLED — KEEP the standalone `MEMORY_STATUS` command. Rev 1.3 §7.4.3 documents it as planned, not implemented;
on this unit it is the only reproducible camera error (H4) and the stimulus of REQ-019 / L5-2.

HD four-slot assembly: option (a), the text of this design — DROP. One code path: the header `width`/`height`/`linesExpected`
are 640/480/480 or 64/48/48 whatever the mode, `Mode.HD` is only a reply mode byte, no quadrant placement on the ground.
Flight code that cannot be exercised on the available hardware is dead code (JPL design principle: no untestable paths).

> **Alternative (b) — not the design text.** Keep the four-quadrant assembly (slots 1-4 in order, 1920 / 240 records,
> 1280×960 / 160×120 header, Memory Status uptime check, REQ-032) behind a compile-time option in `KissCamConfig`
> (`HD_FOUR_SLOT_ASSEMBLY = false`), exercised only by `testDownloadHdImage` / `testDownloadNoImage` against a simulated
> datasheet-conformant camera. Consequences: the 1.3 MB HD product bin and `DownloadFailCause.NO_IMAGE` /
> `MEMORY_STATUS_FAILED` remain; two code paths to maintain; no system test can cover the option on this bench.

L2-10 (HD-mode single-slot preview product): keep or withdraw (human decision at the gate). Recommendation: KEEP. It is the only
system-level evidence that `imageMode = HD` is tagged and decoded on this hardware (REQ-013/-014/-015) and it is hardware-functional (H5).
Consequences of WITHDRAW: HD tagging covered by `testDownloadHdSingleSlot` only; L2-10's PNG and its place in the
`dpMgr.NumDataProducts` order leave the plan.

## Tests

System tests run on the `SunCamRef` deployment (fprime-devices-reference) on the Raspberry Pi 5 bench against the KissCAM V2
bench unit; the plan (`SunCamRef/test/int/plan.json`) is the source of this table. The camera-side fault paths the bench
cannot force (timeouts, failed set-up, send/allocation failure, queue full, stray bytes, mode mismatch) are unit tests.

<!-- fth:tests:begin -->
_Generated by `fth render-sdd-tests` from the plan — do not edit by hand; the package renderer fails when this table and the plan disagree._

| Test | Purpose | Requirements |
|---|---|---|
| L0-1 | Image identity and boot state | REQ-SunCamMgr-002, REQ-SunCamMgr-030 |
| L0-2 | Marker round-trip | REQ-SunCamMgr-030 |
| L2-1 | Set-up from NOT_SETUP and re-entrant from READY | REQ-SunCamMgr-001, REQ-SunCamMgr-003, REQ-SunCamMgr-005, REQ-SunCamMgr-012, REQ-SunCamMgr-022, REQ-SunCamMgr-023 |
| L2-2 | Ping MCU and MCU+sensor | REQ-SunCamMgr-006, REQ-SunCamMgr-022, REQ-SunCamMgr-025 |
| L2-3 | Capture test pattern and live frame | REQ-SunCamMgr-001, REQ-SunCamMgr-013, REQ-SunCamMgr-022, REQ-SunCamMgr-025, REQ-SunCamMgr-033 |
| L2-4 | Preview download → Image data product (decoded) | REQ-SunCamMgr-001, REQ-SunCamMgr-014, REQ-SunCamMgr-015 |
| L2-5 | Full download → Image data product (decoded) | REQ-SunCamMgr-014 |
| L2-6 | EXPOSURE parameter push and readback | REQ-SunCamMgr-010, REQ-SunCamMgr-012, REQ-SunCamMgr-023 |
| L2-7 | Sleep from READY and from NOT_SETUP; READY-only command rejected | REQ-SunCamMgr-006, REQ-SunCamMgr-007, REQ-SunCamMgr-008 |
| L2-8 | Register read of the exposure register | REQ-SunCamMgr-012, REQ-SunCamMgr-025 |
| L2-9 | CAMERA_MODE push re-pushes exposure | REQ-SunCamMgr-010, REQ-SunCamMgr-012, REQ-SunCamMgr-023, REQ-SunCamMgr-027 |
| L2-10 | HD single-slot capture and preview download → 64×48 product tagged HD | REQ-SunCamMgr-013, REQ-SunCamMgr-014, REQ-SunCamMgr-015, REQ-SunCamMgr-033 |
| L3-1 | EXPOSURE persists across a restart | REQ-SunCamMgr-003, REQ-SunCamMgr-005, REQ-SunCamMgr-010, REQ-SunCamMgr-027 |
| L5-1 | Busy rejection and commanded download abort | REQ-SunCamMgr-009, REQ-SunCamMgr-017 |
| L5-2 | Camera error reply: Memory Status is unrecognised on this unit | REQ-SunCamMgr-019 |
| L5-3 | Out-of-range exposure refused locally; set-up fails before Set Exposure | REQ-SunCamMgr-002, REQ-SunCamMgr-004, REQ-SunCamMgr-031 |
| L6-1 | Negative control: READY-only commands refused while NOT_SETUP | REQ-SunCamMgr-002, REQ-SunCamMgr-007, REQ-SunCamMgr-008, REQ-SunCamMgr-026 |
| L6-2 | Negative control: DOWNLOAD_ABORT with no download | REQ-SunCamMgr-026 |
| L7-1 | Final state: test-pattern capture, preview product on disk | REQ-SunCamMgr-001, REQ-SunCamMgr-013, REQ-SunCamMgr-015 |
<!-- fth:tests:end -->

### Unit-test list

| Test | Description | Requirements |
|---|---|---|
| testSetupNominal | Five exchanges in order, READY, SetupComplete, channels reflect mode/exposure | REQ-SunCamMgr-001, REQ-SunCamMgr-003, REQ-SunCamMgr-012, REQ-SunCamMgr-022, REQ-SunCamMgr-023 |
| testSetupFailsEachStep | Timeout or 0xFF at every step, and out-of-range EXPOSURE before Set Exposure → NOT_SETUP, SetupFailed(step, cause) | REQ-SunCamMgr-004, REQ-SunCamMgr-018, REQ-SunCamMgr-019 |
| testSetupReentrant | SETUP_CAMERA while READY and after a failed set-up repeats the full sequence | REQ-SunCamMgr-005 |
| testNoFramesBeforeSetup | No UART frame leaves before SETUP_CAMERA; READY-only commands rejected NOT_READY | REQ-SunCamMgr-002, REQ-SunCamMgr-008 |
| testSleepAndPingNotSetup | SLEEP and PING exchange from NOT_SETUP; SLEEP ends NOT_SETUP on success and failure | REQ-SunCamMgr-006, REQ-SunCamMgr-007 |
| testBusyRejection | Every command during an operation answers BUSY except DOWNLOAD_ABORT | REQ-SunCamMgr-009 |
| testParameterPushIdleOnly | Change during an operation deferred to the next idle READY tick; change while NOT_SETUP applied only by SETUP_CAMERA | REQ-SunCamMgr-010 |
| testParameterPushFailure | Push timeout / error → NOT_SETUP, SettingPushFailed | REQ-SunCamMgr-011 |
| testExposureReadbackMismatch | Readback ≠ pushed value → NOT_SETUP, ExposureReadback with both values | REQ-SunCamMgr-012, REQ-SunCamMgr-028 |
| testCaptureReportsExposure | 0-byte ACK → CaptureComplete(slot, testPattern, last pushed/read-back exposure, mode); 2-byte ACK → the ACK's exposure; ImagesCaptured both times | REQ-SunCamMgr-013, REQ-SunCamMgr-033 |
| testCommandFrameBytes | Every Table 2 command and Table 1 push leaves as the 7-byte §7.4.x frame, byte for byte | REQ-SunCamMgr-001 |
| testChannelsFromReplies | Each Table 3 channel equals its named reply field (uptime, RXV/RXF, MemorySlots, RegisterValue, LastCameraError, counters) | REQ-SunCamMgr-022 |
| testOutcomeEventPerCommand | Every Table 2 command's OK and rejected outcome has its event with the named arguments | REQ-SunCamMgr-023 |
| testDownloadFullAndPreview | No Memory Status frame; 480 / 48 ordered line requests; DownloadStarted and the product header on the first reply (imageMode = its mode byte, exposure = last pushed, 640×480 / 64×48) + one record per line; ProductsSent | REQ-SunCamMgr-014, REQ-SunCamMgr-015 |
| testDownloadHdSingleSlot | Download replies with mode byte HD → one single-slot product, header imageMode = HD, 640×480 / 64×48 (the datasheet's four-slot HD image is §Not supported) | REQ-SunCamMgr-014, REQ-SunCamMgr-015 |
| testDownloadLineRetryAndFail | Line timeout retried LINE_RETRIES times then DownloadFailed LINE_FAILED (WARNING_HI), partial product sent | REQ-SunCamMgr-016, REQ-SunCamMgr-017 |
| testDownloadFailedSendAndProductFull | Download frame refused → DownloadFailed SEND_FAILED; container full → PRODUCT_FULL | REQ-SunCamMgr-017, REQ-SunCamMgr-024 |
| testDownloadAbortCommanded | DOWNLOAD_ABORT during download → DownloadAborted (ACTIVITY_HI), partial product sent; rejected NO_DOWNLOAD when idle | REQ-SunCamMgr-017, REQ-SunCamMgr-026 |
| testReplyTimeoutTicks | Exchange fails after exactly REPLY_TIMEOUT_TICKS run ticks; Timeouts channel | REQ-SunCamMgr-018 |
| testCameraErrorReply | 0xFF reply → CameraError(operation, cmdByte, errorType, count), LastCameraError, CameraErrors | REQ-SunCamMgr-019 |
| testMalformedAndStrayFrames | Bad length, wrong command byte, late reply, non-OK buffer → FrameDiscarded, DiscardedBytes, resync on 0x40 | REQ-SunCamMgr-020 |
| testPowerOnBanner | The 21 power-on banner bytes (H7) idle and in the middle of a Ping exchange → one FrameDiscarded(MALFORMED, 21, head "MVP Came"), DiscardedBytes += 21, no assert, the following framed reply still parsed | REQ-SunCamMgr-020 |
| testSplitAndCrPayloadReply | Reply split across buffers and containing 0x0D parsed by length | REQ-SunCamMgr-001, REQ-SunCamMgr-020 |
| testModeMismatch | Reply mode ≠ commanded mode while READY → ModeMismatch, NOT_SETUP | REQ-SunCamMgr-021 |
| testMemoryStatusAndRegister | MEMORY_STATUS answered per datasheet → MemorySlots; MEMORY_STATUS answered unrecognised command (bench, H1) → CameraError(MEMORY_STATUS, UNRECOGNISED_COMMAND), EXECUTION_ERROR, READY kept; READ_REGISTER → RegisterRead, RegisterValue, address echo checked | REQ-SunCamMgr-025 |
| testModeChangeRepushesExposure | CAMERA_MODE change pushes Set Mode then Set Exposure + readback | REQ-SunCamMgr-027 |
| testSendAndAllocationFailure | drvSendOut failure → SendFailed, operation fails; product allocation failure → ProductAllocationFailed | REQ-SunCamMgr-024 |
| testQueueFullHooks | Overflowing every async input: BUSY response, buffer returned, QueueOverflow per source, QueueOverflows sum | REQ-SunCamMgr-029 |
| testPeriodicChannelWrite | State and counter channels written on change and again after TLM_PERIOD_TICKS idle run ticks | REQ-SunCamMgr-030 |
| testExposureRangeCheck | EXPOSURE 1 and 961 → SettingRejected, no frame, READY kept; 2 and 960 pushed; persisted 1 fails set-up at SET_EXPOSURE | REQ-SunCamMgr-031, REQ-SunCamMgr-004 |
| testRunPortSingleQueuedTick | run invoked N times before dispatch → one runInternal message; hook clears the atomic | REQ-SunCamMgr-029 |
| testHealthPingAndDrvConnected | pingIn answered on pingOut; drvConnected ignored | — (interface) |

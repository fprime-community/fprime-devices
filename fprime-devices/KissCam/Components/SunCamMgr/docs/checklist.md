# F´ (F Prime) Component Development Checklist

**Component Name:** SunCamMgr  
**Component Owner:** _[Owner Name]_  
**Component Contributors:** _[Contributor Names]_  
**Review Date:** 2026-11-16  
**Checklist Version:** 1.2.0

_Pre-filled by fprime-auto-checklist 0.1.0 for `lib/fprime-devices/fprime-devices/KissCam/Components/SunCamMgr` at `a798095e27e6`. 🟢 YES 55, 🟠 NO 13, ⚪ N/A 4, 🔵 REVIEW 0, ⚫ UNCHECKED 1._
_Section fingerprints (inputs each section judges): 1=3a517334b335 2=22356b5a2956 3=9154444e47a8 4=ba333d52c9b8 5=a89190c3a5f8_
_Stage: **closeout** (close-out); sections due: 1, 2, 3, 4, 5_

## Status Legend

| Status | Meaning |
|--------|---------|
| 🟢 YES | Item satisfied -- verified by a tool (or attested by the checked record). |
| 🟠 NO | Not satisfied -- a tool found a violation; see Notes. |
| ⚪ N/A | Not applicable -- the model declares nothing this item concerns. |
| 🔵 REVIEW | Needs a human or LLM judgement; the rule to apply is in Notes. |
| ⚫ UNCHECKED | The deciding tool did not run (missing input, tool or license ack); see Notes. |

**Note:** Any status other than **🟢 YES** requires an explanation in the **Notes** column.

## Definitions

| Term | Definition |
|------|------------|
| SDD | Software Description Document — the design documentation for the component. |
| Block diagram | A diagram illustrating the component's ports (and their directions/types), used to visualize the component's interfaces. |

_clang-tidy: 44 rules run; 0 finding(s) in autocoded files were set aside (autocoder, not component)._

## 1. Requirements

| # | Item | Status | Notes |
|---|------|--------|-------|
| 1.1 | Do requirements exist in the component SDD that define the component's expected operational behaviors? | 🟢 YES |  |
| 1.2 | Do requirements exist in the component SDD that define input data expected to be received and handled by the component? | 🟢 YES |  |
| 1.3 | Do requirements exist in the component SDD that define output data expected to be produced by the component? | 🟢 YES |  |
| 1.4 | Do requirements exist in the component SDD for all relevant memory usage constraints for the component? | 🟢 YES |  |
| 1.5 | Do requirements exist in the component SDD for all relevant hardware I/O usage constraints for the component? | 🟢 YES |  |
| 1.6 | Do requirements exist in the component SDD for all relevant runtime performance constraints for the component? | 🟢 YES |  |
| 1.7 | Does every requirement specify a verification method? | 🟢 YES | R8 pass (36 requirement(s) checked) |
| 1.8 | Are there any relevant external interface or functional requirements documented elsewhere such as ICDs/FDDs? If so, please specify in notes. | 🟢 YES | D9 pass (no External References table; cited inline: datasheet 2000) |

## 2. Design

| # | Item | Status | Notes |
|---|------|--------|-------|
| 2.1 | Is the high level component functionality documented/described in the SDD? | 🟢 YES | SDD pass (tier platinum) |
| 2.2 | Is the component block diagram documented in the SDD and consistent with the FPP model? | 🟢 YES | D9 pass (no External References table; cited inline: datasheet 2000) |
| 2.3 | Is the component port table documented in the SDD and consistent with the FPP model? | 🟢 YES | D7 pass (7 port(s) checked) |
| 2.4 | Are the component input port behaviors described in the SDD? | 🟢 YES | D10 pass (4 input port(s) checked) |
| 2.5 | Are the component input port handlers defined? | 🟢 YES | D1 pass (4 input port(s) checked) |
| 2.6 | Are the component command handlers defined? | 🟢 YES | D2 pass (7 command(s) checked) |
| 2.7 | Are the component sequence diagrams documented? | 🟢 YES | D9 pass (no External References table; cited inline: datasheet 2000) |
| 2.8 | Are component state machines and use-cases described in the SDD? | 🟢 YES | D9 pass (no External References table; cited inline: datasheet 2000) |
| 2.9 | Are the state machine actions defined? | ⚪ N/A | D3: no state machines declared |
| 2.10 | Are component commands, telemetry, events and parameters documented in the SDD (and consistent with the FPP model)? | 🟠 NO | Pre-existing SDD/FPP table name mismatches (main bc9656f); DOWNLOAD row and REQ-037 updated by this change; rest left — minimal change. [D8] |
| 2.11 | Are component data products documented and consistent with the FPP model? | 🟠 NO | Pre-existing (main bc9656f): ImageHeader/ImageLine records absent from the SDD data-product table; not touched by this change. [D8] |
| 2.12 | Are all data product containers assigned a default priority? | 🟢 YES | D4 pass (1 container(s) checked) |
| 2.13 | Does each component port in the FPP model have a known/planned use-case? | 🟢 YES | R6 pass (7 port(s) traced against 36 requirement(s); 7 covered by companion/SDD evidence)<br>D10 pass (4 input port(s) checked) |
| 2.14 | Are port directions and types as expected? | 🟢 YES | D7 pass (7 port(s) checked) |
| 2.15 | Is the component type as expected (passive, queued or active)? | 🟢 YES | D9 pass (no External References table; cited inline: datasheet 2000) |
| 2.16 | Are all the component requirements mapped to design description and/or artifacts? | 🟢 YES | D5 pass (36 requirement(s) checked) |
| 2.17 | Does the component FPP compile and generate auto-code? | 🟢 YES | D6 pass (1 FPP file(s) checked) |

## 3. Implementation

| # | Item | Status | Notes |
|---|------|--------|-------|
| 3.1 | Are all component source dependencies listed in the CMakeLists.txt file for the component? | 🟠 NO | Pre-existing (main bc9656f): Os/Mutex.hpp in SunCamMgr.hpp:11, CMakeLists.txt untouched by this change. [I1] |
| 3.2 | Does the component compile without warnings for all required targets? | 🟢 YES | I2 pass (1 target(s) built) |
| 3.3 | Are all loops properly terminated/bounded? | 🟢 YES | `cpp/fprime/jpl-c/loop-bounds` 0 findings |
| 3.4 | Has gating been provided for any potential cyclic error/status messages (e.g. log messages, events)? | 🟢 YES | `cpp/fprime/unthrottled-cyclic-event` 0 findings |
| 3.5 | Do all switch statements have a break after each case and terminate with a default case? | 🟢 YES | `cpp/fprime/switch-default` 0 findings<br>`cpp/fprime/switch-case-fallthrough` 0 findings |
| 3.6 | Verify all non-empty case statements that fall through to the following case statement are explicitly commented as such | 🟢 YES | `cpp/fprime/switch-case-fallthrough` 0 findings |
| 3.7 | Are all variables locally defined unless global or specific visibility is required? | 🟢 YES | `cpp/fprime/global-mutable-variable` 0 findings |
| 3.8 | Are all class members defined appropriately using private, public and protected keywords? | 🟢 YES | `cpp/fprime/public-data-member` 0 findings |
| 3.9 | Verify that no floating point expressions are tested for equality/inequality | 🟢 YES | `cpp/fprime/float-equality` 0 findings |
| 3.10 | Is the precision of floating point numbers sufficient to ensure accuracy and floating-point equivalence is evaluated with margin (i.e. safe floating-point usage), and is not used in a loop control? | 🟢 YES | `cpp/fprime/float-narrowing` 0 findings<br>`cpp/fprime/float-loop-control` 0 findings<br>`cpp/fprime/float-equality` 0 findings |
| 3.11 | Are all variables initialized before use? | 🟢 YES | `cpp/fprime/uninitialized-member` 0 findings<br>`cpp/uninitialized-local` 0 findings |
| 3.12 | Are the priorities of all data products produced by the component configurable via command(s) or parameter(s)? | 🟢 YES | Audit (a798095e27e6 (closeout stage)): Override: DOWNLOAD's `$priority: FwDpPriorityType` argument sets the Image container priority (SunCamMgr.fpp; REQ-SunCamMgr-037; UT testDownloadPriority; system tests L2-4 (0 → 10), L2-11 (7)). I3 only inspects command/parameter names, not arguments. |
| 3.13 | Are all commands acknowledged with success/failure? | 🟢 YES | `cpp/fprime/command-response` 0 findings |
| 3.14 | Do all validation failure cases for port and command handlers include a return statement to avoid unintended code execution beyond validation failures? | 🟢 YES | `cpp/fprime/validation-failure-return` 0 findings |
| 3.15 | Are all internal port interfaces invoked via the auto-coded internal port invoke() function? | 🟢 YES | `cpp/fprime/internal-port-invoke` 0 findings |
| 3.16 | All pointer arguments to functions are checked to be not nullptr using FW_ASSERT | 🟠 NO | Pre-existing (main bc9656f): SunCamMgr.cpp:686-687 frame parser, outside this change's hunks (217, 246-248). [cpp/fprime/checking-parameter-values] |
| 3.17 | Assumptions on software state are validated using FW_ASSERT. | 🟠 NO | Pre-existing (main bc9656f): SunCamMgr.cpp:133, 197, 623, 1018 — handlers outside this change. [cpp/fprime/jpl-c/use-of-assertions-density] |
| 3.18 | Assertions shall not be used to validate state that could be dependent on hardware or user input. | 🟢 YES | `cpp/fprime/assert-on-untrusted-input` 0 findings |
| 3.19 | Verify all ISR code is free of floating point operations | 🟢 YES | `cpp/fprime/no-float-in-isr` 0 findings |
| 3.20 | Has the ISR code been optimized to minimize length/execution time? | 🟢 YES | `cpp/fprime/isr-complexity` 0 findings |
| 3.21 | Verify any software state that's expected upon receive of an interrupt is configured prior to enabling/setting conditions that would allow the interrupt to trigger. | ⚪ N/A | no interrupt handlers: SunCamMgr is driven by Drv.ByteStreamDriver ports and the scheduler tick (no *_isr/*irq* functions in SunCamMgr.cpp/.hpp) |
| 3.22 | Verify there are no blocking operations in ISRs | 🟢 YES | `cpp/fprime/no-blocking-in-isr` 0 findings |
| 3.23 | Verify any relevant interrupt enables and interrupts are cleared during initialization and re-enabled selectively as needed. | ⚪ N/A | no interrupt enables: no peripheral or ISR configuration in the component (UART owned by the driver component) |
| 3.24 | Are RateGroup schedule calls non-blocking? | 🟢 YES | `cpp/fprime/no-blocking-in-sched-handler` 0 findings |
| 3.25 | Verify no synchronous file access is done within a critical region (e.g. SchedIn) | 🟢 YES | `cpp/fprime/no-file-access-in-critical` 0 findings |
| 3.26 | Verify that all loops and if/else statements are compound statements (i.e. always use brackets {}) | 🟢 YES | `cpp/fprime/compound-statements` 0 findings |
| 3.27 | Verify that all divisions first check for small numbers (including zero) | 🟠 NO | Pre-existing (main bc9656f): SunCamMgr.cpp:541 m_linesPerSlot (set from Resolution at set-up), outside this change. [cpp/fprime/division-guard, cpp/fprime/float-division-guard] |
| 3.28 | Verify all array accesses are checked to be within array index bounds | 🟠 NO | Pre-existing: same findings as 3.16, outside this change's hunks. [cpp/fprime/unchecked-array-index, cpp/fprime/checking-parameter-values] |
| 3.29 | Verify there are no function declarations within a function. | 🟢 YES | `cpp/fprime/no-function-decl-in-function` 0 findings |
| 3.30 | Verify that the three expressions in all for statements only concern flow control | 🟢 YES | `cpp/fprime/for-header-flow-control` 0 findings |
| 3.31 | Verify that there are no dependencies on signed integer overflow behavior | 🟢 YES | `cpp/fprime/signed-overflow` 0 findings |
| 3.32 | Verify return values are used or voided otherwise. | 🟠 NO | Pre-existing (main bc9656f): memset returns in the constructor, SunCamMgr.cpp:115-120, outside this change. [cpp/fprime/jpl-c/checking-return-values] |
| 3.33 | Verify units are incorporated into variable name, where applicable | 🟢 YES |  |
| 3.34 | Verify that all invariant numbers are in enums, static constexpr or #defines. Enums or static constexpr must be used unless #define is only way to specify constant. | 🟠 NO | Pre-existing (main bc9656f): frame byte offsets at SunCamMgr.cpp:488-489, outside this change. [cpp/fprime/magic-numbers, cpp/fprime/define-constant] |
| 3.35 | Verify no use of printf | 🟢 YES | `cpp/fprime/no-printf` 0 findings |
| 3.36 | Does the component use platform independent standard data types (e.g. I32, F32 etc.) rather than primitive data types (e.g. int, float etc.)? | 🟢 YES | `cpp/fprime/jpl-c/basic-int-types` 0 findings |
| 3.37 | Does the component pass static analysis? | 🟠 NO | Pre-existing: I4 medium findings are the 3.16 ones, outside this change's hunks. [I4] |
| 3.38 | Does the implementation address all component requirements and remains consistent with the documented design? | 🟢 YES |  |

## 4. Unit Testing

| # | Item | Status | Notes |
|---|------|--------|-------|
| 4.1 | Have all test output/results been published? | ⚫ UNCHECKED | U1 not run: not done yet: no published unit-test artifacts given (--artifacts-dir) |
| 4.2 | Are all component unit test source dependencies listed in the CMakeLists.txt file for the component? | 🟠 NO | Pre-existing (main bc9656f): Fw/Dp include in test/ut/SunCamMgrTester.cpp:11 predates this change. [U3] |
| 4.3 | Have input data ranges been tested for boundary conditions? | 🟢 YES |  |
| 4.4 | Do the unit tests pass a memory leak check? | 🟠 NO | LeakSanitizer (detect_leaks=1) on fprime-devices_KissCam_Components_SunCamMgr_ut_exe: 34/34 tests pass, direct leaks from Os::Generic::PriorityQueue::create via the autocoded SunCamMgrComponentBase::init — framework/autocode, present on baseline tests, not introduced by this change; the tool looked for a lib/-prefixed executable name. [U4] |
| 4.5 | Is there sufficient code coverage? | 🟢 YES | Audit (a798095e27e6 (closeout stage)): Override: `fprime-util check --coverage` in the development stage (ASAN_OPTIONS=detect_leaks=0) measured lines 98.5% (715/726), functions 98.4%, branches 84.4% — dev-evidence/ut/print-summary.txt + ut-check.log in suncam-v2-2026-11-13.zip (manifest-pinned); the tool's run failed on the LeakSanitizer exit of the autocoded init (4.4), not on coverage. |

## 5. Close-Out

| # | Item | Status | Notes |
|---|------|--------|-------|
| 5.1 | Verify no TBDs, TODOs or printfs in the code (no incomplete code). | 🟢 YES | `cpp/fprime/incomplete-code` 0 findings<br>`cpp/fprime/no-printf` 0 findings |
| 5.2 | Verify that component can be instantiated, connected, and compiled along with the Topology for all applicable targets. | 🟢 YES | C1 pass (1 deployment(s) checked for 'SunCamMgr') |
| 5.3 | Verify that the flight deployment with component instantiation/connections runs successfully on a hardware platform. Indicate hardware platform used for checkout in Notes. | 🟢 YES | C2 pass (record hardware-run.md)<br>Attested by the hardware-run record checked by C2. |
| 5.4 | Update the SDD to accurately reflect the final state of the component. Add any additional design detail, including any known idiosyncrasies. | 🟠 NO | Pre-existing D8 mismatches as 2.10/2.11; the rows this change touched (DOWNLOAD, Image priority, Table 4, §8) reflect the final state. [D7, D8] |
| 5.5 | Validate that CI passes. | ⚪ N/A | Audit (a798095e27e6 (closeout stage)): No CI in this repository: fprime-community/fprime-devices has no .github/workflows and the autobot fork reports 0 check-runs for a798095. |

## Hand-off

Review queue for the **closeout** stage (1 row(s)); record each with `fprime-auto-checklist decide`:

- **4.1** (audit the tool's verdict) — ⚫ UNCHECKED

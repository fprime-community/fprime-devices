# F´ (F Prime) Component Development Checklist

**Component Name:** SunCamMgr  
**Component Owner:** _[Owner Name]_  
**Component Contributors:** _[Contributor Names]_  
**Review Date:** 2026-11-17  
**Checklist Version:** 1.2.0

_Pre-filled by fprime-auto-checklist 0.2.2 for `lib/fprime-devices/fprime-devices/KissCam/Components/SunCamMgr` at `7e85c2694690`. 🟢 YES 68, 🟠 NO 0, ⚪ N/A 5, 🔵 REVIEW 0, ⚫ UNCHECKED 0._
_Section fingerprints (inputs each section judges): 1=abff151eb64d 2=55f7e1a45cda 3=234ef8f66292 4=0f288cab307e 5=1159acdc79ba_
_Stage: **closeout** (close-out); sections due: 1, 2, 3, 4, 5_

## Status Legend

| Status | Meaning |
|--------|---------|
| 🟢 YES | Item satisfied -- verified by a tool (or attested by the checked record). |
| 🟠 NO | Not satisfied -- a tool found a violation; see Notes. |
| ⚪ N/A | Not applicable -- the model declares nothing this item concerns. |
| 🔵 REVIEW | Needs a human or LLM judgement; the rule to apply is in Notes. |
| ⚫ UNCHECKED | The deciding tool did not run (missing input, tool or license ack); see Notes. |

**Note:** Any status other than **🟢 YES** requires an explanation in the **Notes** column. A Status cell reads `<status> @<commit>`: decided by the tool at that commit; `by review` when a reviewer decided a row the tool left open; `overridden by reviewer` when a reviewer replaced the tool's verdict (reason in Notes).

## Definitions

| Term | Definition |
|------|------------|
| SDD | Software Description Document — the design documentation for the component. |
| Block diagram | A diagram illustrating the component's ports (and their directions/types), used to visualize the component's interfaces. |

_clang-tidy: 44 rules run; 0 finding(s) in autocoded files were set aside (autocoder, not component)._

## 1. Requirements

| # | Item | Status | Notes |
|---|------|--------|-------|
| 1.1 | Do requirements exist in the component SDD that define the component's expected operational behaviors? | 🟢 YES @1601e867c6fc by review |  |
| 1.2 | Do requirements exist in the component SDD that define input data expected to be received and handled by the component? | 🟢 YES @1601e867c6fc by review |  |
| 1.3 | Do requirements exist in the component SDD that define output data expected to be produced by the component? | 🟢 YES @1601e867c6fc by review |  |
| 1.4 | Do requirements exist in the component SDD for all relevant memory usage constraints for the component? | 🟢 YES @1601e867c6fc by review |  |
| 1.5 | Do requirements exist in the component SDD for all relevant hardware I/O usage constraints for the component? | 🟢 YES @1601e867c6fc by review |  |
| 1.6 | Do requirements exist in the component SDD for all relevant runtime performance constraints for the component? | 🟢 YES @1601e867c6fc by review |  |
| 1.7 | Does every requirement specify a verification method? | 🟢 YES @7e85c2694690 | R8 pass (36 requirement(s) checked) |
| 1.8 | Are there any relevant external interface or functional requirements documented elsewhere such as ICDs/FDDs? If so, please specify in notes. | 🟢 YES @7e85c2694690 | D9 pass (no External References table; cited inline: datasheet 2000) |

## 2. Design

| # | Item | Status | Notes |
|---|------|--------|-------|
| 2.1 | Is the high level component functionality documented/described in the SDD? | 🟢 YES @7e85c2694690 | SDD pass (tier platinum) |
| 2.2 | Is the component block diagram documented in the SDD and consistent with the FPP model? | 🟢 YES @7e85c2694690 | D9 pass (no External References table; cited inline: datasheet 2000) |
| 2.3 | Is the component port table documented in the SDD and consistent with the FPP model? | 🟢 YES @7e85c2694690 | D7 pass (7 port(s) checked) |
| 2.4 | Are the component input port behaviors described in the SDD? | 🟢 YES @7e85c2694690 | D10 pass (4 input port(s) checked) |
| 2.5 | Are the component input port handlers defined? | 🟢 YES @7e85c2694690 | D1 pass (4 input port(s) checked) |
| 2.6 | Are the component command handlers defined? | 🟢 YES @7e85c2694690 | D2 pass (7 command(s) checked) |
| 2.7 | Are the component sequence diagrams documented? | 🟢 YES @7e85c2694690 | D9 pass (no External References table; cited inline: datasheet 2000) |
| 2.8 | Are component state machines and use-cases described in the SDD? | 🟢 YES @7e85c2694690 | D9 pass (no External References table; cited inline: datasheet 2000) |
| 2.9 | Are the state machine actions defined? | ⚪ N/A @7e85c2694690 | D3: no state machines declared |
| 2.10 | Are component commands, telemetry, events and parameters documented in the SDD (and consistent with the FPP model)? | 🟢 YES @7e85c2694690 | D8 pass (56 dictionary item(s) checked) |
| 2.11 | Are component data products documented and consistent with the FPP model? | 🟢 YES @7e85c2694690 | D8 pass (56 dictionary item(s) checked) |
| 2.12 | Are all data product containers assigned a default priority? | 🟢 YES @7e85c2694690 | D4 pass (1 container(s) checked) |
| 2.13 | Does each component port in the FPP model have a known/planned use-case? | 🟢 YES @7e85c2694690 | R6 pass (7 port(s) traced against 36 requirement(s); 7 covered by companion/SDD evidence)<br>D10 pass (4 input port(s) checked) |
| 2.14 | Are port directions and types as expected? | 🟢 YES @7e85c2694690 | D7 pass (7 port(s) checked) |
| 2.15 | Is the component type as expected (passive, queued or active)? | 🟢 YES @7e85c2694690 | D9 pass (no External References table; cited inline: datasheet 2000) |
| 2.16 | Are all the component requirements mapped to design description and/or artifacts? | 🟢 YES @7e85c2694690 | D5 pass (36 requirement(s) checked) |
| 2.17 | Does the component FPP compile and generate auto-code? | 🟢 YES @7e85c2694690 | D6 pass (1 FPP file(s) checked) |

## 3. Implementation

| # | Item | Status | Notes |
|---|------|--------|-------|
| 3.1 | Are all component source dependencies listed in the CMakeLists.txt file for the component? | 🟢 YES @7e85c2694690 | I1 pass (1 dependency module(s) checked; 1 provided by FPP/implicit framework dependencies) |
| 3.2 | Does the component compile without warnings for all required targets? | 🟢 YES @7e85c2694690 | I2 pass (1 target(s) built) |
| 3.3 | Are all loops properly terminated/bounded? | 🟢 YES @7e85c2694690 | `cpp/fprime/jpl-c/loop-bounds` 0 findings |
| 3.4 | Has gating been provided for any potential cyclic error/status messages (e.g. log messages, events)? | 🟢 YES @7e85c2694690 | `cpp/fprime/unthrottled-cyclic-event` 0 findings |
| 3.5 | Do all switch statements have a break after each case and terminate with a default case? | 🟢 YES @7e85c2694690 | `cpp/fprime/switch-default` 0 findings<br>`cpp/fprime/switch-case-fallthrough` 0 findings |
| 3.6 | Verify all non-empty case statements that fall through to the following case statement are explicitly commented as such | 🟢 YES @7e85c2694690 | `cpp/fprime/switch-case-fallthrough` 0 findings |
| 3.7 | Are all variables locally defined unless global or specific visibility is required? | 🟢 YES @7e85c2694690 | `cpp/fprime/global-mutable-variable` 0 findings |
| 3.8 | Are all class members defined appropriately using private, public and protected keywords? | 🟢 YES @7e85c2694690 | `cpp/fprime/public-data-member` 0 findings |
| 3.9 | Verify that no floating point expressions are tested for equality/inequality | 🟢 YES @7e85c2694690 | `cpp/fprime/float-equality` 0 findings |
| 3.10 | Is the precision of floating point numbers sufficient to ensure accuracy and floating-point equivalence is evaluated with margin (i.e. safe floating-point usage), and is not used in a loop control? | 🟢 YES @7e85c2694690 | `cpp/fprime/float-narrowing` 0 findings<br>`cpp/fprime/float-loop-control` 0 findings<br>`cpp/fprime/float-equality` 0 findings |
| 3.11 | Are all variables initialized before use? | 🟢 YES @7e85c2694690 | `cpp/fprime/uninitialized-member` 0 findings<br>`cpp/uninitialized-local` 0 findings |
| 3.12 | Are the priorities of all data products produced by the component configurable via command(s) or parameter(s)? | 🟢 YES @7e85c2694690 overridden by reviewer | I3: container 'Image' priority is not configurable: no command or parameter mentioning 'priority' found<br>DOWNLOAD's `$priority` argument sets the Image container priority (SunCamMgr.fpp, REQ-SunCamMgr-037; UT testDownloadPriority, system test L2-11); I3 only reads command/parameter names |
| 3.13 | Are all commands acknowledged with success/failure? | 🟢 YES @7e85c2694690 | `cpp/fprime/command-response` 0 findings |
| 3.14 | Do all validation failure cases for port and command handlers include a return statement to avoid unintended code execution beyond validation failures? | 🟢 YES @7e85c2694690 | `cpp/fprime/validation-failure-return` 0 findings |
| 3.15 | Are all internal port interfaces invoked via the auto-coded internal port invoke() function? | 🟢 YES @7e85c2694690 | `cpp/fprime/internal-port-invoke` 0 findings |
| 3.16 | All pointer arguments to functions are checked to be not nullptr using FW_ASSERT | 🟢 YES @7e85c2694690 | `cpp/fprime/checking-parameter-values` 0 findings |
| 3.17 | Assumptions on software state are validated using FW_ASSERT. | 🟢 YES @7e85c2694690 overridden by reviewer | `cpp/fprime/jpl-c/use-of-assertions-density` lib/fprime-devices/fprime-devices/KissCam/Components/SunCamMgr/SunCamMgr.cpp:133 All functions of more than 10 lines should have at least one assertion, unless they handle errors by returning a status enum or bool, or by emitting a WARNING/FATAL event.<br>`cpp/fprime/jpl-c/use-of-assertions-density` lib/fprime-devices/fprime-devices/KissCam/Components/SunCamMgr/SunCamMgr.cpp:197 All functions of more than 10 lines should have at least one assertion, unless they handle errors by returning a status enum or bool, or by emitting a WARNING/FATAL event.<br>`cpp/fprime/jpl-c/use-of-assertions-density` lib/fprime-devices/fprime-devices/KissCam/Components/SunCamMgr/SunCamMgr.cpp:1023 All functions of more than 10 lines should have at least one assertion, unless they handle errors by returning a status enum or bool, or by emitting a WARNING/FATAL event.<br>Remaining density sites (drvReceiveIn_handler, CAPTURE_cmdHandler, writeCounterChannels) carry no software-state assumption to assert: they handle untrusted driver bytes / command arguments with conditionals and write telemetry; every state assumption elsewhere is FW_ASSERTed (strayByte, processReply, handleReply, storeLine, exchangeFailed) |
| 3.18 | Assertions shall not be used to validate state that could be dependent on hardware or user input. | 🟢 YES @7e85c2694690 | `cpp/fprime/assert-on-untrusted-input` 0 findings |
| 3.19 | Verify all ISR code is free of floating point operations | 🟢 YES @7e85c2694690 | `cpp/fprime/no-float-in-isr` 0 findings |
| 3.20 | Has the ISR code been optimized to minimize length/execution time? | 🟢 YES @7e85c2694690 | `cpp/fprime/isr-complexity` 0 findings |
| 3.21 | Verify any software state that's expected upon receive of an interrupt is configured prior to enabling/setting conditions that would allow the interrupt to trigger. | ⚪ N/A @e0e2ffca7e47 by review | no interrupt handlers: SunCamMgr is driven by Drv.ByteStreamDriver ports and the scheduler tick (no *_isr/*irq* functions in SunCamMgr.cpp/.hpp) |
| 3.22 | Verify there are no blocking operations in ISRs | 🟢 YES @7e85c2694690 | `cpp/fprime/no-blocking-in-isr` 0 findings |
| 3.23 | Verify any relevant interrupt enables and interrupts are cleared during initialization and re-enabled selectively as needed. | ⚪ N/A @e0e2ffca7e47 by review | no interrupt enables: no peripheral or ISR configuration in the component (UART owned by the driver component) |
| 3.24 | Are RateGroup schedule calls non-blocking? | 🟢 YES @7e85c2694690 | `cpp/fprime/no-blocking-in-sched-handler` 0 findings |
| 3.25 | Verify no synchronous file access is done within a critical region (e.g. SchedIn) | 🟢 YES @7e85c2694690 | `cpp/fprime/no-file-access-in-critical` 0 findings |
| 3.26 | Verify that all loops and if/else statements are compound statements (i.e. always use brackets {}) | 🟢 YES @7e85c2694690 | `cpp/fprime/compound-statements` 0 findings |
| 3.27 | Verify that all divisions first check for small numbers (including zero) | 🟢 YES @7e85c2694690 | `cpp/fprime/division-guard` 0 findings<br>`cpp/fprime/float-division-guard` 0 findings |
| 3.28 | Verify all array accesses are checked to be within array index bounds | 🟢 YES @7e85c2694690 | `cpp/fprime/unchecked-array-index` 0 findings<br>`cpp/fprime/checking-parameter-values` 0 findings |
| 3.29 | Verify there are no function declarations within a function. | 🟢 YES @7e85c2694690 | `cpp/fprime/no-function-decl-in-function` 0 findings |
| 3.30 | Verify that the three expressions in all for statements only concern flow control | 🟢 YES @7e85c2694690 | `cpp/fprime/for-header-flow-control` 0 findings |
| 3.31 | Verify that there are no dependencies on signed integer overflow behavior | 🟢 YES @7e85c2694690 | `cpp/fprime/signed-overflow` 0 findings |
| 3.32 | Verify return values are used or voided otherwise. | 🟢 YES @7e85c2694690 | `cpp/fprime/jpl-c/checking-return-values` 0 findings |
| 3.33 | Verify units are incorporated into variable name, where applicable | 🟢 YES @e0e2ffca7e47 by review |  |
| 3.34 | Verify that all invariant numbers are in enums, static constexpr or #defines. Enums or static constexpr must be used unless #define is only way to specify constant. | 🟢 YES @7e85c2694690 | `cpp/fprime/magic-numbers` 0 findings<br>`cpp/fprime/define-constant` 0 findings |
| 3.35 | Verify no use of printf | 🟢 YES @7e85c2694690 | `cpp/fprime/no-printf` 0 findings |
| 3.36 | Does the component use platform independent standard data types (e.g. I32, F32 etc.) rather than primitive data types (e.g. int, float etc.)? | 🟢 YES @7e85c2694690 | `cpp/fprime/jpl-c/basic-int-types` 0 findings |
| 3.37 | Does the component pass static analysis? | 🟢 YES @7e85c2694690 | I4 pass (3 finding(s) in module; threshold medium) |
| 3.38 | Does the implementation address all component requirements and remains consistent with the documented design? | 🟢 YES @e0e2ffca7e47 by review |  |

## 4. Unit Testing

| # | Item | Status | Notes |
|---|------|--------|-------|
| 4.1 | Have all test output/results been published? | ⚪ N/A @7e85c2694690 overridden by reviewer | U1 not run: not done yet: no published unit-test artifacts given (--artifacts-dir)<br>The repository has no unit-test artifact publication step (no CI); results are in the campaign's development evidence bundle (dev-evidence-v3, shipped in the run-for-record package) |
| 4.2 | Are all component unit test source dependencies listed in the CMakeLists.txt file for the component? | 🟢 YES @7e85c2694690 | U3 pass (1 dependency module(s) checked) |
| 4.3 | Have input data ranges been tested for boundary conditions? | 🟢 YES @e0e2ffca7e47 by review |  |
| 4.4 | Do the unit tests pass a memory leak check? | 🟢 YES @7e85c2694690 overridden by reviewer | U4: unit-test executable not found after 'fprime-util build --ut'<br>LeakSanitizer clean after `component.deinit()` in the Tester destructor: `ASAN_OPTIONS=detect_leaks=1 fprime-util check` → 34/34 PASS, no sanitizer output (before: 193120 bytes in 136 allocations); U4 could not locate the UT executable in this run |
| 4.5 | Is there sufficient code coverage? | 🟢 YES @7e85c2694690 | U5 pass (line coverage 98.50% (threshold 80.00%)) |

## 5. Close-Out

| # | Item | Status | Notes |
|---|------|--------|-------|
| 5.1 | Verify no TBDs, TODOs or printfs in the code (no incomplete code). | 🟢 YES @7e85c2694690 | `cpp/fprime/incomplete-code` 0 findings<br>`cpp/fprime/no-printf` 0 findings |
| 5.2 | Verify that component can be instantiated, connected, and compiled along with the Topology for all applicable targets. | 🟢 YES @7e85c2694690 | C1 pass (1 deployment(s) checked for 'SunCamMgr') |
| 5.3 | Verify that the flight deployment with component instantiation/connections runs successfully on a hardware platform. Indicate hardware platform used for checkout in Notes. | 🟢 YES @7e85c2694690 | C2 pass (record hardware-run.md)<br>Attested by the hardware-run record checked by C2. |
| 5.4 | Update the SDD to accurately reflect the final state of the component. Add any additional design detail, including any known idiosyncrasies. | 🟢 YES @7e85c2694690 | D7 pass (7 port(s) checked)<br>D8 pass (56 dictionary item(s) checked)<br>D9 pass (no External References table; cited inline: datasheet 2000)<br>SDD pass (tier platinum) |
| 5.5 | Validate that CI passes. | ⚪ N/A @7e85c2694690 overridden by reviewer | C3 not run: not done yet: CI status not consulted (needs the repository, the commit and GITHUB_TOKEN)<br>The repository has no CI configured |

## Hand-off

Nothing to review at the **closeout** stage.

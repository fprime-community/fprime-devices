# F´ (F Prime) Component Development Checklist

**Component Name:** SunCamMgr  
**Component Owner:** _[Owner Name]_  
**Component Contributors:** _[Contributor Names]_  
**Review Date:** 2026-11-17  
**Checklist Version:** 1.2.0

_Pre-filled by fprime-auto-checklist 0.2.0 for `lib/fprime-devices/fprime-devices/KissCam/Components/SunCamMgr` at `508b1a3a39b1`. 🟢 YES 55, 🟠 NO 13, ⚪ N/A 4, 🔵 REVIEW 0, ⚫ UNCHECKED 1._
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
| 1.1 | Do requirements exist in the component SDD that define the component's expected operational behaviors? | 🟢 YES @69c71bf16019 by review |  |
| 1.2 | Do requirements exist in the component SDD that define input data expected to be received and handled by the component? | 🟢 YES @69c71bf16019 by review |  |
| 1.3 | Do requirements exist in the component SDD that define output data expected to be produced by the component? | 🟢 YES @69c71bf16019 by review |  |
| 1.4 | Do requirements exist in the component SDD for all relevant memory usage constraints for the component? | 🟢 YES @69c71bf16019 by review |  |
| 1.5 | Do requirements exist in the component SDD for all relevant hardware I/O usage constraints for the component? | 🟢 YES @69c71bf16019 by review |  |
| 1.6 | Do requirements exist in the component SDD for all relevant runtime performance constraints for the component? | 🟢 YES @69c71bf16019 by review |  |
| 1.7 | Does every requirement specify a verification method? | 🟢 YES @508b1a3a39b1 | R8 pass (36 requirement(s) checked) |
| 1.8 | Are there any relevant external interface or functional requirements documented elsewhere such as ICDs/FDDs? If so, please specify in notes. | 🟢 YES @508b1a3a39b1 | D9 pass (no External References table; cited inline: datasheet 2000) |

## 2. Design

| # | Item | Status | Notes |
|---|------|--------|-------|
| 2.1 | Is the high level component functionality documented/described in the SDD? | 🟢 YES @508b1a3a39b1 | SDD pass (tier platinum) |
| 2.2 | Is the component block diagram documented in the SDD and consistent with the FPP model? | 🟢 YES @508b1a3a39b1 | D9 pass (no External References table; cited inline: datasheet 2000) |
| 2.3 | Is the component port table documented in the SDD and consistent with the FPP model? | 🟢 YES @508b1a3a39b1 | D7 pass (7 port(s) checked) |
| 2.4 | Are the component input port behaviors described in the SDD? | 🟢 YES @508b1a3a39b1 | D10 pass (4 input port(s) checked) |
| 2.5 | Are the component input port handlers defined? | 🟢 YES @508b1a3a39b1 | D1 pass (4 input port(s) checked) |
| 2.6 | Are the component command handlers defined? | 🟢 YES @508b1a3a39b1 | D2 pass (7 command(s) checked) |
| 2.7 | Are the component sequence diagrams documented? | 🟢 YES @508b1a3a39b1 | D9 pass (no External References table; cited inline: datasheet 2000) |
| 2.8 | Are component state machines and use-cases described in the SDD? | 🟢 YES @508b1a3a39b1 | D9 pass (no External References table; cited inline: datasheet 2000) |
| 2.9 | Are the state machine actions defined? | ⚪ N/A @508b1a3a39b1 | D3: no state machines declared |
| 2.10 | Are component commands, telemetry, events and parameters documented in the SDD (and consistent with the FPP model)? | 🟠 NO @508b1a3a39b1 | D8: SDD command table lists 'Set Exposure', which the FPP model does not declare<br>D8: SDD command table lists 'Set Mode', which the FPP model does not declare<br>D8: event 'FrameDiscarded' is missing from the SDD event table<br>D8: event 'QueueOverflow' is missing from the SDD event table<br>D8: SDD event table lists 'FrameDiscarded, throttle 10', which the FPP model does not declare<br>D8: SDD event table lists 'QueueOverflow, throttle 10', which the FPP model does not declare<br>... 2 more |
| 2.11 | Are component data products documented and consistent with the FPP model? | 🟠 NO @508b1a3a39b1 | D8: data product 'ImageHeader' is missing from the SDD data product table<br>D8: data product 'ImageLine' is missing from the SDD data product table |
| 2.12 | Are all data product containers assigned a default priority? | 🟢 YES @508b1a3a39b1 | D4 pass (1 container(s) checked) |
| 2.13 | Does each component port in the FPP model have a known/planned use-case? | 🟢 YES @508b1a3a39b1 | R6 pass (7 port(s) traced against 36 requirement(s); 7 covered by companion/SDD evidence)<br>D10 pass (4 input port(s) checked) |
| 2.14 | Are port directions and types as expected? | 🟢 YES @508b1a3a39b1 | D7 pass (7 port(s) checked) |
| 2.15 | Is the component type as expected (passive, queued or active)? | 🟢 YES @508b1a3a39b1 | D9 pass (no External References table; cited inline: datasheet 2000) |
| 2.16 | Are all the component requirements mapped to design description and/or artifacts? | 🟢 YES @508b1a3a39b1 | D5 pass (36 requirement(s) checked) |
| 2.17 | Does the component FPP compile and generate auto-code? | 🟢 YES @508b1a3a39b1 | D6 pass (1 FPP file(s) checked) |

## 3. Implementation

| # | Item | Status | Notes |
|---|------|--------|-------|
| 3.1 | Are all component source dependencies listed in the CMakeLists.txt file for the component? | 🟠 NO @508b1a3a39b1 | I1: dependency 'Os' (from #include "Os/Mutex.hpp") is not declared in CMakeLists.txt (add it to DEPENDS as Os) |
| 3.2 | Does the component compile without warnings for all required targets? | 🟢 YES @508b1a3a39b1 | I2 pass (1 target(s) built) |
| 3.3 | Are all loops properly terminated/bounded? | 🟢 YES @508b1a3a39b1 | `cpp/fprime/jpl-c/loop-bounds` 0 findings |
| 3.4 | Has gating been provided for any potential cyclic error/status messages (e.g. log messages, events)? | 🟢 YES @508b1a3a39b1 | `cpp/fprime/unthrottled-cyclic-event` 0 findings |
| 3.5 | Do all switch statements have a break after each case and terminate with a default case? | 🟢 YES @508b1a3a39b1 | `cpp/fprime/switch-default` 0 findings<br>`cpp/fprime/switch-case-fallthrough` 0 findings |
| 3.6 | Verify all non-empty case statements that fall through to the following case statement are explicitly commented as such | 🟢 YES @508b1a3a39b1 | `cpp/fprime/switch-case-fallthrough` 0 findings |
| 3.7 | Are all variables locally defined unless global or specific visibility is required? | 🟢 YES @508b1a3a39b1 | `cpp/fprime/global-mutable-variable` 0 findings |
| 3.8 | Are all class members defined appropriately using private, public and protected keywords? | 🟢 YES @508b1a3a39b1 | `cpp/fprime/public-data-member` 0 findings |
| 3.9 | Verify that no floating point expressions are tested for equality/inequality | 🟢 YES @508b1a3a39b1 | `cpp/fprime/float-equality` 0 findings |
| 3.10 | Is the precision of floating point numbers sufficient to ensure accuracy and floating-point equivalence is evaluated with margin (i.e. safe floating-point usage), and is not used in a loop control? | 🟢 YES @508b1a3a39b1 | `cpp/fprime/float-narrowing` 0 findings<br>`cpp/fprime/float-loop-control` 0 findings<br>`cpp/fprime/float-equality` 0 findings |
| 3.11 | Are all variables initialized before use? | 🟢 YES @508b1a3a39b1 | `cpp/fprime/uninitialized-member` 0 findings<br>`cpp/uninitialized-local` 0 findings |
| 3.12 | Are the priorities of all data products produced by the component configurable via command(s) or parameter(s)? | 🟢 YES @a798095e27e6 overridden by reviewer | I3: container 'Image' priority is not configurable: no command or parameter mentioning 'priority' found<br>Override: DOWNLOAD's `$priority: FwDpPriorityType` argument sets the Image container priority (SunCamMgr.fpp; REQ-SunCamMgr-037; UT testDownloadPriority; system tests L2-4 (0 → 10), L2-11 (7)). I3 only inspects command/parameter names, not arguments. |
| 3.13 | Are all commands acknowledged with success/failure? | 🟢 YES @508b1a3a39b1 | `cpp/fprime/command-response` 0 findings |
| 3.14 | Do all validation failure cases for port and command handlers include a return statement to avoid unintended code execution beyond validation failures? | 🟢 YES @508b1a3a39b1 | `cpp/fprime/validation-failure-return` 0 findings |
| 3.15 | Are all internal port interfaces invoked via the auto-coded internal port invoke() function? | 🟢 YES @508b1a3a39b1 | `cpp/fprime/internal-port-invoke` 0 findings |
| 3.16 | All pointer arguments to functions are checked to be not nullptr using FW_ASSERT | 🟠 NO @508b1a3a39b1 | `cpp/fprime/checking-parameter-values` lib/fprime-devices/fprime-devices/KissCam/Components/SunCamMgr/SunCamMgr.cpp:686 Parameter frame is dereferenced without first being checked (e.g. with FW_ASSERT or a condition).<br>`cpp/fprime/checking-parameter-values` lib/fprime-devices/fprime-devices/KissCam/Components/SunCamMgr/SunCamMgr.cpp:687 Parameter frame is dereferenced without first being checked (e.g. with FW_ASSERT or a condition).<br>`cpp/fprime/checking-parameter-values` lib/fprime-devices/fprime-devices/KissCam/Components/SunCamMgr/SunCamMgr.cpp:688 Parameter frame is dereferenced without first being checked (e.g. with FW_ASSERT or a condition).<br>`cpp/fprime/checking-parameter-values` lib/fprime-devices/fprime-devices/KissCam/Components/SunCamMgr/SunCamMgr.cpp:772 Parameter payload is dereferenced without first being checked (e.g. with FW_ASSERT or a condition).<br>`cpp/fprime/checking-parameter-values` lib/fprime-devices/fprime-devices/KissCam/Components/SunCamMgr/SunCamMgr.cpp:773 Parameter payload is dereferenced without first being checked (e.g. with FW_ASSERT or a condition).<br>`cpp/fprime/checking-parameter-values` lib/fprime-devices/fprime-devices/KissCam/Components/SunCamMgr/SunCamMgr.cpp:774 Parameter payload is dereferenced without first being checked (e.g. with FW_ASSERT or a condition).<br>... 5 more |
| 3.17 | Assumptions on software state are validated using FW_ASSERT. | 🟠 NO @508b1a3a39b1 | `cpp/fprime/jpl-c/use-of-assertions-density` lib/fprime-devices/fprime-devices/KissCam/Components/SunCamMgr/SunCamMgr.cpp:133 All functions of more than 10 lines should have at least one assertion, unless they handle errors by returning a status enum or bool, or by emitting a WARNING/FATAL event.<br>`cpp/fprime/jpl-c/use-of-assertions-density` lib/fprime-devices/fprime-devices/KissCam/Components/SunCamMgr/SunCamMgr.cpp:197 All functions of more than 10 lines should have at least one assertion, unless they handle errors by returning a status enum or bool, or by emitting a WARNING/FATAL event.<br>`cpp/fprime/jpl-c/use-of-assertions-density` lib/fprime-devices/fprime-devices/KissCam/Components/SunCamMgr/SunCamMgr.cpp:623 All functions of more than 10 lines should have at least one assertion, unless they handle errors by returning a status enum or bool, or by emitting a WARNING/FATAL event.<br>`cpp/fprime/jpl-c/use-of-assertions-density` lib/fprime-devices/fprime-devices/KissCam/Components/SunCamMgr/SunCamMgr.cpp:1018 All functions of more than 10 lines should have at least one assertion, unless they handle errors by returning a status enum or bool, or by emitting a WARNING/FATAL event. |
| 3.18 | Assertions shall not be used to validate state that could be dependent on hardware or user input. | 🟢 YES @508b1a3a39b1 | `cpp/fprime/assert-on-untrusted-input` 0 findings |
| 3.19 | Verify all ISR code is free of floating point operations | 🟢 YES @508b1a3a39b1 | `cpp/fprime/no-float-in-isr` 0 findings |
| 3.20 | Has the ISR code been optimized to minimize length/execution time? | 🟢 YES @508b1a3a39b1 | `cpp/fprime/isr-complexity` 0 findings |
| 3.21 | Verify any software state that's expected upon receive of an interrupt is configured prior to enabling/setting conditions that would allow the interrupt to trigger. | ⚪ N/A @564b2a9110ce by review | no interrupt handlers: SunCamMgr is driven by Drv.ByteStreamDriver ports and the scheduler tick (no *_isr/*irq* functions in SunCamMgr.cpp/.hpp) |
| 3.22 | Verify there are no blocking operations in ISRs | 🟢 YES @508b1a3a39b1 | `cpp/fprime/no-blocking-in-isr` 0 findings |
| 3.23 | Verify any relevant interrupt enables and interrupts are cleared during initialization and re-enabled selectively as needed. | ⚪ N/A @564b2a9110ce by review | no interrupt enables: no peripheral or ISR configuration in the component (UART owned by the driver component) |
| 3.24 | Are RateGroup schedule calls non-blocking? | 🟢 YES @508b1a3a39b1 | `cpp/fprime/no-blocking-in-sched-handler` 0 findings |
| 3.25 | Verify no synchronous file access is done within a critical region (e.g. SchedIn) | 🟢 YES @508b1a3a39b1 | `cpp/fprime/no-file-access-in-critical` 0 findings |
| 3.26 | Verify that all loops and if/else statements are compound statements (i.e. always use brackets {}) | 🟢 YES @508b1a3a39b1 | `cpp/fprime/compound-statements` 0 findings |
| 3.27 | Verify that all divisions first check for small numbers (including zero) | 🟠 NO @508b1a3a39b1 | `cpp/fprime/division-guard` lib/fprime-devices/fprime-devices/KissCam/Components/SunCamMgr/SunCamMgr.cpp:541 The divisor m_linesPerSlot is not checked against zero before this % operation.<br>`cpp/fprime/float-division-guard` 0 findings |
| 3.28 | Verify all array accesses are checked to be within array index bounds | 🟠 NO @508b1a3a39b1 | `cpp/fprime/unchecked-array-index` 0 findings<br>`cpp/fprime/checking-parameter-values` lib/fprime-devices/fprime-devices/KissCam/Components/SunCamMgr/SunCamMgr.cpp:686 Parameter frame is dereferenced without first being checked (e.g. with FW_ASSERT or a condition).<br>`cpp/fprime/checking-parameter-values` lib/fprime-devices/fprime-devices/KissCam/Components/SunCamMgr/SunCamMgr.cpp:687 Parameter frame is dereferenced without first being checked (e.g. with FW_ASSERT or a condition).<br>`cpp/fprime/checking-parameter-values` lib/fprime-devices/fprime-devices/KissCam/Components/SunCamMgr/SunCamMgr.cpp:688 Parameter frame is dereferenced without first being checked (e.g. with FW_ASSERT or a condition).<br>`cpp/fprime/checking-parameter-values` lib/fprime-devices/fprime-devices/KissCam/Components/SunCamMgr/SunCamMgr.cpp:772 Parameter payload is dereferenced without first being checked (e.g. with FW_ASSERT or a condition).<br>`cpp/fprime/checking-parameter-values` lib/fprime-devices/fprime-devices/KissCam/Components/SunCamMgr/SunCamMgr.cpp:773 Parameter payload is dereferenced without first being checked (e.g. with FW_ASSERT or a condition).<br>... 6 more |
| 3.29 | Verify there are no function declarations within a function. | 🟢 YES @508b1a3a39b1 | `cpp/fprime/no-function-decl-in-function` 0 findings |
| 3.30 | Verify that the three expressions in all for statements only concern flow control | 🟢 YES @508b1a3a39b1 | `cpp/fprime/for-header-flow-control` 0 findings |
| 3.31 | Verify that there are no dependencies on signed integer overflow behavior | 🟢 YES @508b1a3a39b1 | `cpp/fprime/signed-overflow` 0 findings |
| 3.32 | Verify return values are used or voided otherwise. | 🟠 NO @508b1a3a39b1 | `cpp/fprime/jpl-c/checking-return-values` lib/fprime-devices/fprime-devices/KissCam/Components/SunCamMgr/SunCamMgr.cpp:115 The return value of non-void function memset is not checked.<br>`cpp/fprime/jpl-c/checking-return-values` lib/fprime-devices/fprime-devices/KissCam/Components/SunCamMgr/SunCamMgr.cpp:116 The return value of non-void function memset is not checked.<br>`cpp/fprime/jpl-c/checking-return-values` lib/fprime-devices/fprime-devices/KissCam/Components/SunCamMgr/SunCamMgr.cpp:117 The return value of non-void function memset is not checked.<br>`cpp/fprime/jpl-c/checking-return-values` lib/fprime-devices/fprime-devices/KissCam/Components/SunCamMgr/SunCamMgr.cpp:118 The return value of non-void function memset is not checked.<br>`cpp/fprime/jpl-c/checking-return-values` lib/fprime-devices/fprime-devices/KissCam/Components/SunCamMgr/SunCamMgr.cpp:119 The return value of non-void function memset is not checked.<br>`cpp/fprime/jpl-c/checking-return-values` lib/fprime-devices/fprime-devices/KissCam/Components/SunCamMgr/SunCamMgr.cpp:120 The return value of non-void function memset is not checked.<br>... 5 more |
| 3.33 | Verify units are incorporated into variable name, where applicable | 🟢 YES @564b2a9110ce by review |  |
| 3.34 | Verify that all invariant numbers are in enums, static constexpr or #defines. Enums or static constexpr must be used unless #define is only way to specify constant. | 🟠 NO @508b1a3a39b1 | `cpp/fprime/magic-numbers` lib/fprime-devices/fprime-devices/KissCam/Components/SunCamMgr/SunCamMgr.cpp:488 Magic number 5; consider a named constant (enum, static constexpr, or #define).<br>`cpp/fprime/magic-numbers` lib/fprime-devices/fprime-devices/KissCam/Components/SunCamMgr/SunCamMgr.cpp:489 Magic number 6; consider a named constant (enum, static constexpr, or #define).<br>`cpp/fprime/define-constant` 0 findings |
| 3.35 | Verify no use of printf | 🟢 YES @508b1a3a39b1 | `cpp/fprime/no-printf` 0 findings |
| 3.36 | Does the component use platform independent standard data types (e.g. I32, F32 etc.) rather than primitive data types (e.g. int, float etc.)? | 🟢 YES @508b1a3a39b1 | `cpp/fprime/jpl-c/basic-int-types` 0 findings |
| 3.37 | Does the component pass static analysis? | 🟠 NO @508b1a3a39b1 | I4: lib/fprime-devices/fprime-devices/KissCam/Components/SunCamMgr/SunCamMgr.cpp:686: [medium] cpp/fprime/checking-parameter-values: Parameter frame is dereferenced without first being checked (e.g. with FW_ASSERT or a condition).<br>I4: lib/fprime-devices/fprime-devices/KissCam/Components/SunCamMgr/SunCamMgr.cpp:687: [medium] cpp/fprime/checking-parameter-values: Parameter frame is dereferenced without first being checked (e.g. with FW_ASSERT or a condition).<br>I4: lib/fprime-devices/fprime-devices/KissCam/Components/SunCamMgr/SunCamMgr.cpp:688: [medium] cpp/fprime/checking-parameter-values: Parameter frame is dereferenced without first being checked (e.g. with FW_ASSERT or a condition).<br>I4: lib/fprime-devices/fprime-devices/KissCam/Components/SunCamMgr/SunCamMgr.cpp:772: [medium] cpp/fprime/checking-parameter-values: Parameter payload is dereferenced without first being checked (e.g. with FW_ASSERT or a condition).<br>I4: lib/fprime-devices/fprime-devices/KissCam/Components/SunCamMgr/SunCamMgr.cpp:773: [medium] cpp/fprime/checking-parameter-values: Parameter payload is dereferenced without first being checked (e.g. with FW_ASSERT or a condition).<br>I4: lib/fprime-devices/fprime-devices/KissCam/Components/SunCamMgr/SunCamMgr.cpp:774: [medium] cpp/fprime/checking-parameter-values: Parameter payload is dereferenced without first being checked (e.g. with FW_ASSERT or a condition).<br>... 17 more |
| 3.38 | Does the implementation address all component requirements and remains consistent with the documented design? | 🟢 YES @564b2a9110ce by review |  |

## 4. Unit Testing

| # | Item | Status | Notes |
|---|------|--------|-------|
| 4.1 | Have all test output/results been published? | ⚫ UNCHECKED @508b1a3a39b1 | U1 not run: not done yet: no published unit-test artifacts given (--artifacts-dir) |
| 4.2 | Are all component unit test source dependencies listed in the CMakeLists.txt file for the component? | 🟠 NO @508b1a3a39b1 | U3: unit-test dependency 'Fw/Dp' (from #include "Fw/Dp/DpContainer.hpp") is not declared in the module's CMake files (add it to UT_DEPENDS/DEPENDS) |
| 4.3 | Have input data ranges been tested for boundary conditions? | 🟢 YES @564b2a9110ce by review |  |
| 4.4 | Do the unit tests pass a memory leak check? | 🟠 NO @508b1a3a39b1 | U4: unit-test executable not found after 'fprime-util build --ut' |
| 4.5 | Is there sufficient code coverage? | 🟢 YES @a798095e27e6 overridden by reviewer | U5: 'fprime-util check --coverage' failed: itComponents() build-fprime-automatic-native-ut/fprime-devices/KissCam/Components/SunCamMgr/SunCamMgrTesterHelpers.cpp:125 1:     #8 0x63ff98cabe5d in KissCam::SunCamMgrTester::SunCamMgrTester() lib/fprime-devices/fprime-devices/KissCam/Components/SunCamMgr/test/ut/SunCamMgrTester.cpp:47 1:     #9 0x63ff98c8051d in SunCamMgr_testModeChangeRepushesExposure_Test::TestBody() lib/fprime-devices/fprime-devices/KissCam/Components/SunCamMgr/test/ut/SunCamMgrTestMain.cpp:42 1:     #10 0x63ff9931067f in void testing::internal::HandleSehExceptionsInMethodIfSupported<testing::Test, void>(testing::Test*, void (testing::Test::*)(), char const*) lib/fprime/googletest/googletest/src/gtest.cc:2599 1:     #11 0x63ff992e60e7 in void testing::internal::HandleExceptionsInMethodIfSupported<testing::Test, void>(testing::Test*, void (testing::Test::*)(), char const*) lib/fprime/googletest/googletest/src/gtest.cc:2635 1:     #12 0x63ff9923c54b in testing::Test::Run() lib/fprime/googletest/googletest/src/gtest.cc:2674 1:     #13 0x63ff99240157 in testing::TestInfo::Run() lib/fprime/googletest/googletest/src/gtest.cc:2853 1:     #14 0x63ff99244972 in testing::TestSuite::Run() lib/fprime/googletest/googletest/src/gtest.cc:3012 1:     #15 0x63ff9928a61a in testing::internal::UnitTestImpl::RunAllTests() lib/fprime/googletest/googletest/src/gtest.cc:5870 1:     #16 0x63ff99317aca in bool testing::internal::HandleSehExceptionsInMethodIfSupported<testing::internal::UnitTestImpl, bool>(testing::internal::UnitTestImpl*, bool (testing::internal::UnitTestImpl::*)(), char const*) lib/fprime/googletest/googletest/src/gtest.cc:2599 1:     #17 0x63ff992ebfc1 in bool testing::internal::HandleExceptionsInMethodIfSupported<testing::internal::UnitTestImpl, bool>(testing::internal::UnitTestImpl*, bool (testing::internal::UnitTestImpl::*)(), char const*) lib/fprime/googletest/googletest/src/gtest.cc:2635 1:     #18 0x63ff9927d6af in testing::UnitTest::Run() lib/fprime/googletest/googletest/src/gtest.cc:5444 1:     #19 0x63ff98c92d76 in RUN_ALL_TESTS() lib/fprime/googletest/googletest/include/gtest/gtest.h:2293 1:     #20 0x63ff98c8222e in main lib/fprime-devices/fprime-devices/KissCam/Components/SunCamMgr/test/ut/SunCamMgrTestMain.cpp:52 1:     #21 0x7bf4b3229d8f  (/lib/x86_64-linux-gnu/libc.so.6+0x29d8f) (BuildId: 12b87db2df813df96f66d6f02dd18ea5c8b7cf8d) 1:  1: SUMMARY: AddressSanitizer: 193120 byte(s) leaked in 136 allocation(s). 1/1 Test #1: fprime-devices_KissCam_Components_SunCamMgr_ut_exe ...***Failed    1.11 sec  0% tests passed, 1 tests failed out of 1  Total Test time (real) =   1.16 sec  The following tests FAILED: 	  1 - fprime-devices_KissCam_Components_SunCamMgr_ut_exe (Failed) [INFO] Running CTest with context: ['fprime-devices_KissCam_Components_SunCamMgr_ut_exe'] Errors while running CTest Output from these tests are in: build-fprime-automatic-native-ut/fprime-devices/KissCam/Components/SunCamMgr/Testing/Temporary/LastTest.log Use "--rerun-failed --output-on-failure" to re-run the failed cases verbosely. [ERROR] CalledProcessError: Command '['ctest', '--test-dir', 'build-fprime-automatic-native-ut/fprime-devices/KissCam/Components/SunCamMgr', '--no-tests=error', '-V', '-R', '^(fprime-devices_KissCam_Components_SunCamMgr_ut_exe)$']' returned non-zero exit status 8. <br>Override: `fprime-util check --coverage` in the development stage (ASAN_OPTIONS=detect_leaks=0) measured lines 98.5% (715/726), functions 98.4%, branches 84.4% — dev-evidence/ut/print-summary.txt + ut-check.log in suncam-v2-2026-11-13.zip (manifest-pinned); the tool's run failed on the LeakSanitizer exit of the autocoded init (4.4), not on coverage. |

## 5. Close-Out

| # | Item | Status | Notes |
|---|------|--------|-------|
| 5.1 | Verify no TBDs, TODOs or printfs in the code (no incomplete code). | 🟢 YES @508b1a3a39b1 | `cpp/fprime/incomplete-code` 0 findings<br>`cpp/fprime/no-printf` 0 findings |
| 5.2 | Verify that component can be instantiated, connected, and compiled along with the Topology for all applicable targets. | 🟢 YES @508b1a3a39b1 | C1 pass (1 deployment(s) checked for 'SunCamMgr') |
| 5.3 | Verify that the flight deployment with component instantiation/connections runs successfully on a hardware platform. Indicate hardware platform used for checkout in Notes. | 🟢 YES @508b1a3a39b1 | C2 pass (record hardware-run.md)<br>Attested by the hardware-run record checked by C2. |
| 5.4 | Update the SDD to accurately reflect the final state of the component. Add any additional design detail, including any known idiosyncrasies. | 🟠 NO @508b1a3a39b1 | D7 pass (7 port(s) checked)<br>D8: SDD command table lists 'Set Exposure', which the FPP model does not declare<br>D8: SDD command table lists 'Set Mode', which the FPP model does not declare<br>D8: event 'FrameDiscarded' is missing from the SDD event table<br>D8: event 'QueueOverflow' is missing from the SDD event table<br>D8: SDD event table lists 'FrameDiscarded, throttle 10', which the FPP model does not declare<br>... 7 more |
| 5.5 | Validate that CI passes. | ⚪ N/A @a798095e27e6 overridden by reviewer | C3: no check runs reported for 508b1a3a39b1a9e0a0ca863a064ec99802758eb9<br>No CI in this repository: fprime-community/fprime-devices has no .github/workflows and the autobot fork reports 0 check-runs for a798095. |

## Hand-off

Review queue for the **closeout** stage (14 row(s)); record each with `fprime-auto-checklist decide`:

- **2.10** (read the tool's finding; override only if it is wrong) — 🟠 NO
- **2.11** (read the tool's finding; override only if it is wrong) — 🟠 NO
- **3.1** (read the tool's finding; override only if it is wrong) — 🟠 NO
- **3.16** (read the tool's finding; override only if it is wrong) — 🟠 NO
- **3.17** (read the tool's finding; override only if it is wrong) — 🟠 NO
- **3.27** (read the tool's finding; override only if it is wrong) — 🟠 NO
- **3.28** (read the tool's finding; override only if it is wrong) — 🟠 NO
- **3.32** (read the tool's finding; override only if it is wrong) — 🟠 NO
- **3.34** (read the tool's finding; override only if it is wrong) — 🟠 NO
- **3.37** (read the tool's finding; override only if it is wrong) — 🟠 NO
- **4.1** (read the tool's finding; override only if it is wrong) — ⚫ UNCHECKED
- **4.2** (read the tool's finding; override only if it is wrong) — 🟠 NO
- **4.4** (read the tool's finding; override only if it is wrong) — 🟠 NO
- **5.4** (read the tool's finding; override only if it is wrong) — 🟠 NO

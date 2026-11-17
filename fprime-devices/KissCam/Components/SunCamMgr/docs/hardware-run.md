# Hardware run — SunCamMgr

| Field | Value |
|---|---|
| Platform | Raspberry Pi 5 (Linux aarch64), KissCAM on FTDI USB-RS422 (`/dev/ttyUSB0`, 115200 8N1) |
| Deployment | SunCamRef (fprime-community/fprime-devices-reference `234a75d543e899cb4b8e6fbde9cd8b968500893b`) |
| Commit | b0446d2bf9bb99f1271f19fb66ba3bab2279794f |
| Date | 2026-11-17 |
| Result | PASS |
| Tester | Devin clean runner (session fb3c56a9) for M Starch |

Run-for-record `rd549fcc` (run under fth v0.1.22; rows 5–7 re-issued under v0.1.25 and packaged by v0.1.26 after harness fixes, disclosed in the package index; manifest `d549fccb`, seal-id `e0c212cb`, record `7ce456b3`): 17 PASS, 2 expected FAIL (drawn planted faults L2-6, L7-1, detected), 0 real FAIL, 0 UNVERIFIED, 1 DEFERRED (L8-1, human power cycle, by ruling). Package `suncam-v3-2026-11-17.zip` sha256 `b8b6f4818aa5df8c28df6cf69db6cfe92c513370c192aafb88a589fe04e076a6`.

module KissCam {
    @ Compile-time configuration of the KissCAM V2 manager (SDD Table 4). Timeouts are counts of
    @ SunCamMgr `run` ticks; the deployment chooses the rate-group period.
    module KissCamConfig {
        @ `run` ticks allowed for one camera reply before the exchange fails (datasheet: 2000 ms)
        constant REPLY_TIMEOUT_TICKS = 20
        @ Re-requests of one image line (after a failed or timed-out reply) before the download aborts
        constant LINE_RETRIES = 3
        @ Default priority of the Image data-product container
        constant IMAGE_DP_PRIORITY = 10
        @ EXPOSURE range accepted by the component before any frame is sent (datasheet §7.4.4)
        constant EXPOSURE_MIN = 2
        constant EXPOSURE_MAX = 960
        @ run ticks between periodic re-writes of the component-owned channels (1 s at 10 Hz)
        constant TLM_PERIOD_TICKS = 10
    }
}

# Deployment-overridable configuration of the KissCam library (SDD Table 4). A deployment overrides
# KissCamConfig.fpp through its own register_fprime_config(... CONFIGURATION_OVERRIDES ...).
register_fprime_config(
    AUTOCODER_INPUTS
        "${CMAKE_CURRENT_LIST_DIR}/KissCamConfig.fpp"
    INTERFACE
)

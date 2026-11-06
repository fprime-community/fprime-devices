# Deployment-overridable configuration of the Fram library (SDD Table 1). A deployment
# overrides these files through its own register_fprime_config(... CONFIGURATION_OVERRIDES ...).
register_fprime_config(
    AUTOCODER_INPUTS
        "${CMAKE_CURRENT_LIST_DIR}/FramConfig.fpp"
    HEADERS
        "${CMAKE_CURRENT_LIST_DIR}/FramConfig.hpp"
    INTERFACE
)

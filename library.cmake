# fprime-devices: reusable F Prime device drivers (memories, peripherals) that attach to a
# platform's bus drivers. Each device directory holds its Types, Ports, Components and a
# Subtopology that instantiates the device alongside the bus driver it is bound to.
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/fprime-devices/")

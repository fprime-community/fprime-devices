# fprime-devices

Reusable [F Prime](https://github.com/nasa/fprime) device libraries from the fprime-community,
laid out like [fprime-sensors](https://github.com/fprime-community/fprime-sensors): one directory
per device under `fprime-devices/`, each with its types, ports, component(s) and a subtopology that
instantiates the component and its bus driver, plus a `docs/sdd.md`.

Consume this repository as a git submodule and add it to the deployment's `settings.ini`
`library_locations`; `library.cmake` registers the device directories with the F Prime build.

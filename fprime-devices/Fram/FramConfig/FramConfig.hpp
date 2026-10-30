// ======================================================================
// \title  FramConfig.hpp
// \brief  Expected device identity of the F-RAM (SDD Table 1, EXPECTED_DEVICE_ID)
// ======================================================================
#ifndef Fram_FramConfig_hpp
#define Fram_FramConfig_hpp

#include <Fw/FPrimeBasicTypes.hpp>
#include "FramConfig/FppConstantsAc.hpp"

namespace Fram {
namespace FramConfig {

//! Expected RDID reply in wire order (first byte received first). Deployment-configurable: override this header
//! (CONFIGURATION_OVERRIDES) for the fitted CY15B102QN family member.
//! 00 2A C2 7F 7F 7F 7F 7F 7F = CY15B102QN-50SXI (Infineon 002-26764 Rev. *D); 60 2A C2 7F x6 = CY15B102QN-50SXE
//! (002-19073).
static constexpr U8 EXPECTED_DEVICE_ID[Fram::FramConfig::DeviceIdSize] = {0x00, 0x2A, 0xC2, 0x7F, 0x7F,
                                                                          0x7F, 0x7F, 0x7F, 0x7F};

}  // namespace FramConfig
}  // namespace Fram

#endif  // Fram_FramConfig_hpp

#ifndef __VERS_H__
#define __VERS_H__
/**
 * @brief This is the Smartfin FW codebase
 * 
 * DATE     WHO DESCRIPTION
 * ----------------------------------------------------------------------------
 * 06/11/22 NH  v2.0.0.6
 *              - Bug fix for smartfin-fw2#27 - adding w/d trip to upload
 *              - Updating versioning
 *              - Bug fix for smartfin-fw2#33
 *              - Bug fix for smartfin-fw2#30 - adding CLI monitoring of water
 *                  detect status
 * 06/03/21 NH  v2.0.0.5
 *              - Bug fixes for smartfin-fw2#11, smartfin-fw2#14, smartfin-fw2#23
 *              - Bug fixes for smartfin-fw2#4
 *              - Fixed GPS status LED during deploy
 * 05/28/21 NH  v2.0.0.4
 *              - Bug fixes for smartfin-fw2#10, smartfin-fw2#15, smartfin-fw2#2
 *              - Fixing compiler warnings
 * 
 */

#include "product.hpp"

#define FW_MAJOR_VERSION    2
#define FW_MINOR_VERSION    0
#define FW_BUILD_NUM        9
#define FW_BRANCH           "3Gfix"

#if PRODUCT_VERSION_USE_HEX == 1
#define PRODUCT_VERSION_VALUE (FW_MAJOR_VERSION << 13 | FW_MINOR_VERSION << 6 | FW_BUILD_NUM)
#else
#define PRODUCT_VERSION_VALUE (FW_MAJOR_VERSION * 10000 + FW_MINOR_VERSION * 100 + FW_BUILD_NUM)
#endif

void VERS_printBanner(void);
const char* VERS_getBuildDate(void);
const char* VERS_getBuildTime(void);
#endif
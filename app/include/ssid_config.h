//
// Why this file?
//
// We all need to add our personal SSID/passphrase to each ESP project but we
// do not want that information pushed to Github.
//
// First tell git to ignore changes to this file:
//
// git update-index --assume-unchanged app/include/ssid_config.h 
//
// Then, enter your SSID and passphrase below and it will never be committed
// to Github.
//
// For reference, see
//   https://www.kernel.org/pub/software/scm/git/docs/git-update-index.html
//

#ifndef __SSID_CONFIG_H__
#define __SSID_CONFIG_H__

#define SSID_NAME "ZTE_5560"
#define SSID_PASS "espressif"

#endif // __SSID_CONFIG_H__

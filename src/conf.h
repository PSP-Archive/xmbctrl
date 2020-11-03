#ifndef __CONF_H__
#define __CONF_H__

#define CONF_PATH "ms0:/seplugins/xmbctrl.conf"

typedef struct _XMBConfig
{
		int magic;
		s16 password_lock;
		s16 block_normal;
		s16 spoof_mac;
} XMBConfig;

/**
 * Gets the SE configuration.
 * Avoid using this function, it may corrupt your program.
 * Use sctrlXMBGetCongiEx function instead.
 *
 * @param config - pointer to a XMBConfig structure that receives the SE configuration
 * @returns 0 on success
*/
int sctrlXMBGetConfig(XMBConfig *configX);

/**
 * Gets the SE configuration
 *
 * @param config - pointer to a XMBConfig structure that receives the SE configuration
 * @param size - The size of the structure
 * @returns 0 on success
*/
int sctrlXMBGetConfigEx(XMBConfig *configX, int size);

/**
 * Sets the SE configuration
 * This function can corrupt the configuration in flash, use
 * sctrlSESetConfigEx instead.
 *
 * @param config - pointer to a XMBConfig structure that has the SE configuration to set
 * @returns 0 on success
*/
int sctrlXMBSetConfig(XMBConfig *configX);

/**
 * Sets the SE configuration
 *
 * @param config - pointer to a XMBConfig structure that has the SE configuration to set
 * @param size - the size of the structure
 * @returns 0 on success
*/
int sctrlXMBSetConfigEx(XMBConfig *configX, int size);

void load_default_conf_x(XMBConfig *configX);

int SetConfig_x(XMBConfig *configX);
int GetConfig_x(XMBConfig *configX);

#endif
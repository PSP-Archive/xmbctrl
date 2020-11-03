/*
 * This file is part of PRO CFW.

 * PRO CFW is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * PRO CFW is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PRO CFW. If not, see <http://www.gnu.org/licenses/ .
 * I will mod it to use with my TN's xmbctrl mod for PRO //djmati11
 */

#include <pspsdk.h>
#include <pspkernel.h>

#include <string.h>
#include "main.h"
#include "utils.h"
#include "conf.h"
#include <systemctrl.h>


#define CONFIG_MAGIC_x 0x53984745

static inline u32 get_conf_magic_x(void)
{
        u32 version;

        version = CONFIG_MAGIC_x;

        return version;
}

int GetConfig_x(XMBConfig *configX)
{
        SceUID fd;
        u32 k1;
   
      //  k1 = pspSdkSetK1(0);
        memset(configX, 0, sizeof(*configX));
        fd = sceIoOpen(CONF_PATH, PSP_O_RDONLY, 0644);

        if (fd < 0) {
       //         pspSdkSetK1(k1);

                return -1;
        }

        if (sceIoRead(fd, configX, sizeof(*configX)) != sizeof(*configX)) {
                sceIoClose(fd);
       //         pspSdkSetK1(k1);

                return -2;
        }

        if(configX->magic != get_conf_magic_x()) {
                sceIoClose(fd);
      //          pspSdkSetK1(k1);
                
                return -3;
        }

        sceIoClose(fd);
      //  pspSdkSetK1(k1);

        return 0;
}

int SetConfig_x(XMBConfig *configX)
{
        u32 k1;
        SceUID fd;
//
     //   k1 = pspSdkSetK1(0);
        sceIoRemove(CONF_PATH);
        fd = sceIoOpen(CONF_PATH, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);

        if (fd < 0) {
      //          pspSdkSetK1(k1);

                return -1;
        }

        configX->magic = get_conf_magic_x();

        if (sceIoWrite(fd, configX, sizeof(*configX)) != sizeof(*configX)) {
                sceIoClose(fd);
          //      pspSdkSetK1(k1);

                return -1;
        }

        sceIoClose(fd);
     //   pspSdkSetK1(k1);

        return 0;
}

int sctrlXMBSetConfigEx(XMBConfig *configX, int size)
{
        u32 k1;
        SceUID fd;
        int written;
   
       // k1 = pspSdkSetK1(0);
        fd = sceIoOpen(CONF_PATH, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);

        if (fd < 0) {
               // pspSdkSetK1(k1);

                return -1;
        }

        configX->magic = get_conf_magic_x();

        written = sceIoWrite(fd, configX, size);

        if (written != size) {
                sceIoClose(fd);
               // pspSdkSetK1(k1);

                return -1;
        }

        sceIoClose(fd);
       // pspSdkSetK1(k1);

        return 0;
}

int sctrlXMBGetConfigEx(XMBConfig *configX, int size)
{
        int read;
        u32 k1;
        SceUID fd;

        read = -1;
       // k1 = pspSdkSetK1(0);
        memset(configX, 0, size);
        fd = sceIoOpen(CONF_PATH, PSP_O_RDONLY, 0666);

        if (fd > 0) {
                read = sceIoRead(fd, configX, size);
                sceIoClose(fd);
         //       pspSdkSetK1(k1);

                if(read != size) {
                        return -2;
                }

                if(configX->magic != get_conf_magic_x()) {
                        return -3;
                }
                
                return 0;
        }

     //   pspSdkSetK1(k1);

        return -1;
}

int sctrlXMBSetConfig(XMBConfig *configX)
{
        return sctrlXMBSetConfigEx(configX, sizeof(*configX));
}

int sctrlXMBGetConfig(XMBConfig *configX)
{
        return sctrlXMBGetConfigEx(configX, sizeof(*configX));
}

void load_default_conf_x(XMBConfig *configX)
{
        memset(configX, 0, sizeof(*configX));
        configX->magic = get_conf_magic_x();
        configX->password_lock = 0;
		configX->block_normal = 1;
		configX->spoof_mac = 0;
}
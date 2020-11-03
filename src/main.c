/*
	6.39 TN-A, XmbControl
	Copyright (C) 2011, Total_Noob

	main.c: XmbControl main code
	
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <pspsdk.h>
#include <pspkernel.h>
#include <psputility_sysparam.h>
#include <pspctrl.h>
#include <systemctrl.h>
#include <systemctrl_se.h>
#include <vshctrl.h>
#include <kubridge.h>

#include "main.h"
#include "en_tnsettings.h"
#include "utils.h"
#include "conf.h"

#if !defined(CONFIG_620) && !defined(CONFIG_635) && !defined(CONFIG_639)
#error You have to build with CONFIG_XXX=1
#endif

PSP_MODULE_INFO("XmbControl", 0x0007, 1, 0);

typedef struct
{
	char *name;
	char *items[3];
	char *options[20];
	char *xmbctrl_options[3];
	char *buttonassign;
	char *speed[9];
	char *usbdevice[6];
	char *regions[14];
	char *normal_driver;
	char *drivers[3];
	char *msspeed[8];
} StringContainer;

StringContainer string;

char *dr_normal[4];

#define N_STRINGS ((sizeof(string) / sizeof(char **)))

typedef struct
{
	int mode;
	int negative;
	char *item;
} GetItem;

GetItem GetItemes[] =
{
	{ 1, 0, "msg_cpuclockxmb" },
	{ 1, 0, "msg_cpuclockgame" },
	{ 2, 0, "msg_usbdevice" },
	{ 3, 0, "msg_fakeregion" },
	{ 5, 0, "msg_umdmode" },
	{ 6, 0, "msg_msspeedup" },
	{ 0, 0, "msg_xmbplugin" },
	{ 0, 0, "msg_gameplugin" },
	{ 0, 0, "msg_popplugin" },
	{ 0, 0, "msg_skipgameboot" },
	{ 0, 0, "msg_hiddenmac" },
	{ 0, 0, "msg_proupdate" },
	{ 0, 0, "msg_hidepic" },
	{ 0, 0, "msg_usbcharge" },
	{ 0, 0, "msg_noanalog" },
	{ 0, 0, "msg_nodrm" },
	{ 0, 0, "msg_hidecfw" },
	{ 0, 0, "msg_useversion" },
	{ 0, 0, "msg_protflash" },
	{ 0, 0, "msg_slimcolors" },
};
GetItem GetItemes2[] =
{
	{ 0, 0, "msg_password" },
	{ 0, 0, "msg_blocknormal" },
	{ 0, 0, "msg_spoofmac" },
};
#define N_ITEMS (sizeof(GetItemes) / sizeof(GetItem))
#define N_ITEMS2 (sizeof(GetItemes2) / sizeof(GetItem))

char *button_options[] = { "O", "X" };

char *items[] =
{
	"msgtop_sysconf_configuration",
	"msgtop_sysconf_plugins",
	"msgtop_sysconf_xmbctrlprefs",
};

enum PluginMode
{
	PluginModeVSH = 0,
	PluginModeGAME = 1,
	PluginModePOPS
};

char *plugin_texts_ms0[] =
{
	"ms0:/seplugins/vsh.txt",
	"ms0:/seplugins/game.txt",
	"ms0:/seplugins/pops.txt"
};

char *plugin_texts_ef0[] =
{
	"ef0:/seplugins/vsh.txt",
	"ef0:/seplugins/game.txt",
	"ef0:/seplugins/pops.txt"
};

#define MAX_PLUGINS_CONST 100

typedef struct
{
	char *path;
	char *plugin;
	int activated;
	int mode;
} Plugin;

Plugin *table = NULL;
int count = 0;

int (* AddVshItem)(void *a0, int topitem, SceVshItem *item);
SceSysconfItem *(*GetSysconfItem)(void *a0, void *a1);
int (* ExecuteAction)(int action, int action_arg);
int (* UnloadModule)(int skip);
int (* OnXmbPush)(void *arg0, void *arg1);
int (* OnXmbContextMenu)(void *arg0, void *arg1);

void (* LoadStartAuth)();
int (* auth_handler)(int a0);
void (* OnRetry)();

void (* AddSysconfItem)(u32 *option, SceSysconfItem **item);
void (* OnInitMenuPspConfig)();

u32 sysconf_unk, sysconf_option;

int is_tn_config = 0;
int unload = 0;

u32 backup[4];
int context_mode = 0;

char *ver_info = NULL;
char *mac_info = NULL;

char user_buffer[128];

STMOD_HANDLER previous;
SEConfig config;
XMBConfig Xconfig;

u32 psp_fw_version;
int psp_model;

int startup = 1;

SceContextItem *context;
SceVshItem *new_item;

SceContextItem *context2;
SceVshItem *new_item2;
void *xmb_arg0, *xmb_arg1;

int cpu_list[] = { 0, 20, 75, 100, 133, 222, 266, 300, 333 };
int bus_list[] = { 0, 10, 37, 50, 66, 111, 133, 150, 166 };

#define N_CPUS (sizeof(cpu_list) / sizeof(int))
#define N_BUSS (sizeof(bus_list) / sizeof(int))


void ClearCaches()
{
	sceKernelDcacheWritebackAll();
	kuKernelIcacheInvalidateAll();
}

__attribute__((noinline)) int GetCPUSpeed(int cpu)
{
	int i;
	for(i = 0; i < N_CPUS; i++) if(cpu_list[i] == cpu) return i;
	return 0;
}

__attribute__((noinline)) int GetBUSSpeed(int bus)
{
	int i;
	for(i = 0; i < N_BUSS; i++) if(bus_list[i] == bus) return i;
	return 0;
}


int readPlugins(int mode, int cur_n, Plugin *plugin_table)
{
	char temp_path[64];
	int res = 0, i = cur_n;
	
	SceUID fd = sceIoOpen(plugin_texts_ms0[mode], PSP_O_RDONLY, 0);
	if(fd < 0)
	{
		fd = sceIoOpen(plugin_texts_ef0[mode], PSP_O_RDONLY, 0);
		if(fd < 0) return i;
	}

	char *buffer = (char *)sce_paf_private_malloc(1024);
	int size = sceIoRead(fd, buffer, 1024);
	char *p = buffer;

	do
	{
		sce_paf_private_memset(temp_path, 0, sizeof(temp_path));

		res = GetPlugin(p, size, temp_path, &plugin_table[i].activated);
		if(res > 0)
		{
			if(plugin_table[i].path) sce_paf_private_free(plugin_table[i].path);
			plugin_table[i].path = (char *)sce_paf_private_malloc(sce_paf_private_strlen(temp_path) + 1);
			sce_paf_private_strcpy(plugin_table[i].path, temp_path);

			if(plugin_table[i].plugin) sce_paf_private_free(plugin_table[i].plugin);
			table[i].plugin = (char *)sce_paf_private_malloc(64);
			sce_paf_private_sprintf(table[i].plugin, "plugin_%08X", (u32)i);

			plugin_table[i].mode = mode;

			size -= res;
			p += res;
			i++;
		} 
	} while(res > 0);

	sce_paf_private_free(buffer);
	sceIoClose(fd);

	return i;
}

void writePlugins(int mode, Plugin *plugin_table, int count)
{	
	SceUID fd = sceIoOpen(plugin_texts_ms0[mode], PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
	if(fd < 0) fd = sceIoOpen(plugin_texts_ef0[mode], PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);

	if(fd >= 0)
	{
		int i;
		for(i = 0; i < count; i++)
		{
			if(plugin_table[i].mode == mode)
			{
				sceIoWrite(fd, plugin_table[i].path, sce_paf_private_strlen(plugin_table[i].path));
				sceIoWrite(fd, " ", sizeof(char));

				sceIoWrite(fd, (plugin_table[i].activated == 1 ? "1" : "0"), sizeof(char));

				if(i != (count - 1)) sceIoWrite(fd, "\r\n", 2 * sizeof(char));
			}
		}
		
		sceIoClose(fd);
	}
}

int readPluginTable()
{
	if(!table)
	{
		table = sce_paf_private_malloc(MAX_PLUGINS_CONST * sizeof(Plugin));
		sce_paf_private_memset(table, 0, MAX_PLUGINS_CONST * sizeof(Plugin));
	}

	count = readPlugins(PluginModeVSH, 0, table);
	count = readPlugins(PluginModeGAME, count, table);
	count = readPlugins(PluginModePOPS, count, table);

	if(count <= 0)
	{
		if(table)
		{
			sce_paf_private_free(table);
			table = NULL;
		}

		return 0;
	}

	return 1;
}

int LoadTextLanguage(int new_id)
{
	static char *language[] = { "ja", "en", "fr", "es", "de", "it", "nl", "pt", "ru", "ko", "ch1", "ch2" };

	int id;
	sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_LANGUAGE, &id);

	if(new_id >= 0)
	{
		if(new_id == id) return 0;
		id = new_id;
	}

	char file[64];
	sce_paf_private_sprintf(file, "ms0:/seplugins/%s_prosettings.txt", language[id]);

	SceUID fd = sceIoOpen(file, PSP_O_RDONLY, 0);
	if(fd < 0)
	{
		sce_paf_private_sprintf(file, "ef0:/seplugins/%s_prosettings.txt", language[id]);
		fd = sceIoOpen(file, PSP_O_RDONLY, 0);
	}

	if(fd >= 0)
	{
		/* Skip UTF8 magic */
		u32 magic;
		sceIoRead(fd, &magic, sizeof(magic));
		sceIoLseek(fd, (magic & 0xFFFFFF) == 0xBFBBEF ? 3 : 0, PSP_SEEK_SET);
	}

	char line[128];
	
	
	int i;
	int j = 0;
	for(i = 0; i < N_STRINGS; i++)
	{
		sce_paf_private_memset(line, 0, sizeof(line));

		if(&((char **)&string)[i] >= &string.speed[1] && &((char **)&string)[i] <= &string.speed[8])
		{
			sce_paf_private_sprintf(line, "%d/%d", cpu_list[&((char **)&string)[i] - &*string.speed], bus_list[&((char **)&string)[i] - &*string.speed]);
		}
		else if(&((char **)&string)[i] >= &string.usbdevice[1] && &((char **)&string)[i] <= &string.usbdevice[4])
		{
			sce_paf_private_sprintf(line, "Flash %d", &((char **)&string)[i] - &*string.usbdevice - 1);
		}
		else
		{
			if(fd >= 0)
			{
				ReadLine(fd, line);
			}
			else
			{
				sce_paf_private_strcpy(line, en_tnsettings[j]);
				j++;
			}
		}

		/* Free buffer if already exist */
		if(((char **)&string)[i]) sce_paf_private_free(((char **)&string)[i]);

		/* Allocate buffer */
		((char **)&string)[i] = sce_paf_private_malloc(sce_paf_private_strlen(line) + 1);

		/* Copy to buffer */
		sce_paf_private_strcpy(((char **)&string)[i], line);
	}

	if(fd >= 0) sceIoClose(fd);

	//if (!Xconfig.block_normal) {
		dr_normal[0] = string.normal_driver;
		dr_normal[1] = string.drivers[0];
		dr_normal[2] = string.drivers[1];
		dr_normal[3] = string.drivers[2];
	//}
	return 1;
}

int AddVshItemPatched(void *a0, int topitem, SceVshItem *item)
{
	if(sce_paf_private_strcmp(item->text, "msgtop_sysconf_console") == 0)
	{
		if(Xconfig.password_lock) LoadStartAuth();
		else startup = 0;

		LoadTextLanguage(-1);

		new_item = (SceVshItem *)sce_paf_private_malloc(sizeof(SceVshItem));
		sce_paf_private_memcpy(new_item, item, sizeof(SceVshItem));

		new_item->id = 47; //information board id
		new_item->action_arg = sysconf_tnconfig_action_arg;
		new_item->play_sound = 0;
		//sce_paf_private_strcpy(new_item->image, imgur);
		sce_paf_private_strcpy(new_item->text, "msgtop_sysconf_tnconfig");

		context = (SceContextItem *)sce_paf_private_malloc((3 * sizeof(SceContextItem)) + 1);

		AddVshItem(a0, topitem, new_item);
	} 
	
	
	return AddVshItem(a0, topitem, item);
}

int OnXmbPushPatched(void *arg0, void *arg1)
{
	xmb_arg0 = arg0;
	xmb_arg1 = arg1;
	return OnXmbPush(arg0, arg1);
}

int OnXmbContextMenuPatched(void *arg0, void *arg1)
{
	new_item->context = NULL;
	return OnXmbContextMenu(arg0, arg1);
}

int ExecuteActionPatched(int action, int action_arg)
{
	int old_is_tn_config = is_tn_config;

	if(action == sysconf_console_action)
	{
		if(action_arg == sysconf_tnconfig_action_arg)
		{
			sctrlSEGetConfig(&config);
			sctrlXMBGetConfig(&Xconfig);
			int n = readPluginTable() + 2;

			sce_paf_private_memset(context, 0, (3 * sizeof(SceContextItem)) + 1);

			int i;
			for(i = 0; i < n; i++)
			{
				sce_paf_private_strcpy(context[i].text, items[i]);
				context[i].play_sound = 1;
				context[i].action = 0x80000;
				context[i].action_arg = i + 1;
			}

			new_item->context = context;

			OnXmbContextMenu(xmb_arg0, xmb_arg1);
			return 0;
		}
		
		is_tn_config = 0;
	}
	else if(action == 0x80000)
	{
		is_tn_config = action_arg;
		action = sysconf_console_action;
		action_arg = sysconf_console_action_arg;
	}

	if(old_is_tn_config != is_tn_config)
	{
		/* Reset variables */
		sce_paf_private_memset(backup, 0, sizeof(backup));
		context_mode = 0;

		/* Unload sysconf */
		unload = 1;
	}

	return ExecuteAction(action, action_arg);
}

int UnloadModulePatched(int skip)
{
	if(unload)
	{
		skip = -1;
		unload = 0;
	}
	return UnloadModule(skip);
}

void AddSysconfContextItem(char *text, char *subtitle, char *regkey)
{
	SceSysconfItem *item = (SceSysconfItem *)sce_paf_private_malloc(sizeof(SceSysconfItem));

	item->id = 5;
	item->unk = (u32 *)sysconf_unk;
	item->regkey = regkey;
	item->text = text;
	item->subtitle = subtitle;
	item->page = "page_psp_config_umd_autoboot";

	((u32 *)sysconf_option)[2] = 1;

	AddSysconfItem((u32 *)sysconf_option, &item);
}

void OnInitMenuPspConfigPatched()
{
	if(is_tn_config == 1)
	{
		if(((u32 *)sysconf_option)[2] == 0)
		{
			int i;
			for(i = 0; i < N_ITEMS; i++)
			{
				AddSysconfContextItem(GetItemes[i].item, NULL, GetItemes[i].item);
			}

			AddSysconfContextItem("msg_buttonassign", NULL, "/CONFIG/SYSTEM/XMB/button_assign");
		}
	}
	else if(is_tn_config == 2)
	{
		if(((u32 *)sysconf_option)[2] == 0)
		{
			int i;
			for(i = 0; i < count; i++)
			{
				char *subtitle = NULL;
				switch(table[i].mode)
				{
					case PluginModeVSH:
						subtitle = "msg_pluginmode_vsh";
						break;

					case PluginModeGAME:
						subtitle = "msg_pluginmode_game";
						break;

					case PluginModePOPS:
						subtitle = "msg_pluginmode_pops";
						break;
				}

				AddSysconfContextItem(table[i].plugin, subtitle, table[i].plugin);
			}
		}
	}
	else if(is_tn_config == 3)
	{
		if(((u32 *)sysconf_option)[2] == 0)
		{
			int i;
			for(i = 0; i < N_ITEMS2; i++)
			{
				AddSysconfContextItem(GetItemes2[i].item, NULL, GetItemes2[i].item);
			}

			//AddSysconfContextItem("msg_buttonassign", NULL, "/CONFIG/SYSTEM/XMB/button_assign");
		}
	}
	else
	{
		OnInitMenuPspConfig();
	}
}

SceSysconfItem *GetSysconfItemPatched(void *a0, void *a1)
{
	SceSysconfItem *item = GetSysconfItem(a0, a1);

	if(is_tn_config == 1)
	{
		int i;
		for(i = 0; i < N_ITEMS; i++)
		{	
			if(sce_paf_private_strcmp(item->text, "msg_umdmode") == 0)
			{
				if (Xconfig.block_normal == 1) 
				{
					context_mode = 5;
				}
				else
				{
					context_mode = 7;
				}
			}
			else if(sce_paf_private_strcmp(item->text, GetItemes[i].item) == 0)
			{
				context_mode = GetItemes[i].mode;
			}
		}

		if(sce_paf_private_strcmp(item->text, "msg_buttonassign") == 0)
		{
			context_mode = 4;
		}
	}
	else if(is_tn_config == 3)
	{
		int i;
		for(i = 0; i < N_ITEMS2; i++)
		{
			if(sce_paf_private_strcmp(item->text, GetItemes2[i].item) == 0)
			{
				context_mode = GetItemes2[i].mode;
			}
		}

		//if(sce_paf_private_strcmp(item->text, "msg_buttonassign") == 0)
		//{
		//	context_mode = 4;
		//}
	}
	return item;
}

wchar_t *scePafGetTextPatched(void *a0, char *name)
{
	if(name)
	{
		if(is_tn_config == 1)
		{
			int i;
			for(i = 0; i < N_ITEMS; i++)
			{
				if(sce_paf_private_strcmp(name, GetItemes[i].item) == 0)
				{
					utf8_to_unicode((wchar_t *)user_buffer, string.options[i]);
					return (wchar_t *)user_buffer;
				}
			}
		}
		else if(is_tn_config == 2)
		{
			if(sce_paf_private_strncmp(name, "plugin_", 7) == 0)
			{
				u32 i = sce_paf_private_strtoul(name + 7, NULL, 16);

				char file[64];
				sce_paf_private_strcpy(file, table[i].path);

				char *p = sce_paf_private_strrchr(table[i].path, '/');
				if(p)
				{
					char *p2 = sce_paf_private_strchr(p + 1, '.');
					if(p2)
					{
						int len = (int)(p2 - (p + 1));
						sce_paf_private_strncpy(file, p + 1, len);
						file[len] = '\0';
					}
				}

				utf8_to_unicode((wchar_t *)user_buffer, file);
				return (wchar_t *)user_buffer;
			}
		}
		else if(is_tn_config == 3)
		{
			int i;
			for(i = 0; i < N_ITEMS2; i++)
			{
				if(sce_paf_private_strcmp(name, GetItemes2[i].item) == 0)
				{
					utf8_to_unicode((wchar_t *)user_buffer, string.xmbctrl_options[i]);
					return (wchar_t *)user_buffer;
				}
			}
		}
		if(sce_paf_private_strcmp(name, "msgtop_sysconf_tnconfig") == 0)
		{
			utf8_to_unicode((wchar_t *)user_buffer, string.name);
			return (wchar_t *)user_buffer;
		}
		else if(sce_paf_private_strcmp(name, "msgtop_sysconf_configuration") == 0)
		{
			utf8_to_unicode((wchar_t *)user_buffer, string.items[0]);
			return (wchar_t *)user_buffer;
		}
		else if(sce_paf_private_strcmp(name, "msgtop_sysconf_plugins") == 0)
		{
			utf8_to_unicode((wchar_t *)user_buffer, string.items[1]);
			return (wchar_t *)user_buffer;
		}
		else if(sce_paf_private_strcmp(name, "msgtop_sysconf_xmbctrlprefs") == 0)
		{
			utf8_to_unicode((wchar_t *)user_buffer, string.items[2]);
			return (wchar_t *)user_buffer;
		}
		else if(sce_paf_private_strcmp(name, "msg_buttonassign") == 0)
		{
			utf8_to_unicode((wchar_t *)user_buffer, string.buttonassign);
			return (wchar_t *)user_buffer;
		}
		else if(sce_paf_private_strcmp(name, "msg_pluginmode_vsh") == 0)
		{
			return L"[VSH]";
		}
		else if(sce_paf_private_strcmp(name, "msg_pluginmode_game") == 0)
		{
			return L"[GAME]";
		}
		else if(sce_paf_private_strcmp(name, "msg_pluginmode_pops") == 0)
		{
			return L"[POPS]";
		}
	}

	wchar_t *res = scePafGetText(a0, name);


	return res;
}

int vshGetRegistryValuePatched(u32 *option, char *name, void *arg2, int size, int *value)
{
	if(name)
	{
		if(is_tn_config == 1)
		{
			s16 lulz = Xconfig.block_normal ? (config.umdmode - 1) : config.umdmode;
			if (Xconfig.block_normal == 1) {
				GetItemes[4].mode = 5;
			} else {
				GetItemes[4].mode = 7;
			}
			s16 configs[] =
			{
				GetCPUSpeed(config.vshcpuspeed),
				GetCPUSpeed(config.umdisocpuspeed),
				config.usbdevice,
				config.fakeregion,
				//config.umdmode,
				lulz,
				config.msspeed,
				config.plugvsh,
				config.pluggame,
				config.plugpop,
				config.skipgameboot,
				config.machidden,
				config.useownupdate,
				config.hidepic,
				config.usbcharge,
				config.noanalog,
				config.usenodrm,
				config.hide_cfw_dirs,
				config.useversion,
				config.flashprot,
				config.slimcolor,
			};

			int i;
			for(i = 0; i < N_ITEMS; i++)
			{
				/*if(sce_paf_private_strcmp(name, "msg_umdmode") == 0) 
				{
					if (Xconfig.block_normal == 1) {
						context_mode = 5;
						
					} else {
						context_mode = 7;
						
					}
					
					*value = configs[i];
					return 0;
				}*/
//				else
				if(sce_paf_private_strcmp(name, GetItemes[i].item) == 0)
				{
					context_mode = GetItemes[i].mode;
					*value = configs[i];
					return 0;
				}
			}

			if(sce_paf_private_strcmp(name, "/CONFIG/SYSTEM/XMB/button_assign") == 0)
			{
				context_mode = 4;
			}
			
		}
		else if(is_tn_config == 2)
		{
			if(sce_paf_private_strncmp(name, "plugin_", 7) == 0)
			{
				u32 i = sce_paf_private_strtoul(name + 7, NULL, 16);
				*value = table[i].activated;
				return 0;
			}
		}
		else if(is_tn_config == 3)
		{
			s16 configs[] =
			{
				Xconfig.password_lock,
				Xconfig.block_normal,
				Xconfig.spoof_mac,
			};

			int i;
			for(i = 0; i < N_ITEMS2; i++)
			{
				if(sce_paf_private_strcmp(name, GetItemes2[i].item) == 0)
				{
					context_mode = GetItemes2[i].mode;
					*value = configs[i];
					return 0;
				}
				
			}

			
			
		}
	}

	return vshGetRegistryValue(option, name, arg2, size, value);
}

int vshSetRegistryValuePatched(u32 *option, char *name, int size, int *value)
{
	if(name)
	{
		if(is_tn_config == 1)
		{
			static s16 *configs[] =
			{
				&config.vshcpuspeed,
				&config.umdisocpuspeed,
				&config.usbdevice,
				&config.fakeregion,
				&config.umdmode,
				&config.msspeed,
				&config.plugvsh,
				&config.pluggame,
				&config.plugpop,
				&config.skipgameboot,
				&config.machidden,
				&config.useownupdate,
				&config.hidepic,
				&config.usbcharge,
				&config.noanalog,
				&config.usenodrm,
				&config.hide_cfw_dirs,
				&config.useversion,
				&config.flashprot,
				&config.slimcolor,
			};

			int i;
			for(i = 0; i < N_ITEMS; i++)
			{
				if(sce_paf_private_strcmp(name, GetItemes[i].item) == 0)
				{
					if(sce_paf_private_strcmp(name, "msg_cpuclockxmb") == 0)
					{
						*configs[i] = cpu_list[*value];
						config.vshbusspeed = bus_list[*value];
					}
					else if(sce_paf_private_strcmp(name, "msg_cpuclockgame") == 0)
					{
						*configs[i] = cpu_list[*value];
						config.umdisobusspeed = bus_list[*value];
					}
					else if((Xconfig.block_normal) && (sce_paf_private_strcmp(name, "msg_umdmode") == 0)) 
					{
						*configs[i] = *value + 1;
					}
					else if((Xconfig.spoof_mac) && (sce_paf_private_strcmp(name, "msg_hiddenmac") == 0))
					{
						*configs[i] = *value;
						Xconfig.spoof_mac = 0;
						sctrlXMBSetConfig(&Xconfig);
					}
					else
					{
						*configs[i] = GetItemes[i].negative ? !(*value) : *value;
					}

					sctrlSESetConfig(&config);
					vctrlVSHExitVSHMenu(&config, 0, 0);
					return 0;
				}
			}
			
		}
		else if(is_tn_config == 2)
		{
			if(sce_paf_private_strncmp(name, "plugin_", 7) == 0)
			{
				u32 i = sce_paf_private_strtoul(name + 7, NULL, 16);
				table[i].activated = *value;
				writePlugins(table[i].mode, table, count);
				return 0;
			}
		}
		else if(is_tn_config == 3)
		{
			static s16 *configs[] =
			{
				&Xconfig.password_lock,
				&Xconfig.block_normal,
				&Xconfig.spoof_mac,
			};

			int i;
			for(i = 0; i < N_ITEMS2; i++)
			{
				if(sce_paf_private_strcmp(name, GetItemes2[i].item) == 0)
				{
					*configs[i] = GetItemes2[i].negative ? !(*value) : *value;
					sctrlXMBSetConfig(&Xconfig);
					if (sce_paf_private_strcmp(name, "msg_blocknormal") == 0) {
						if (Xconfig.block_normal) {
							if (config.umdmode == 0) {
								config.umdmode = 3;
								sctrlSESetConfig(&config);
								vctrlVSHExitVSHMenu(&config, 0, 0);
							}
						}
					}
					else if (sce_paf_private_strcmp(name, "msg_spoofmac") == 0) {
						if (Xconfig.spoof_mac) {
							if (config.machidden == 1) {
								config.machidden = 0;
								sctrlSESetConfig(&config);
								vctrlVSHExitVSHMenu(&config, 0, 0);
							}
						}
					}
					//vctrlVSHExitVSHMenu(&Xconfig, 0, 0);
					return 0;
				}
			}
			
		}

		if(sce_paf_private_strcmp(name, "/CONFIG/SYSTEM/XMB/language") == 0)
		{
			LoadTextLanguage(*value);
		}
	}

	return vshSetRegistryValue(option, name, size, value);
}

void HijackContext(SceRcoEntry *src, char **options, int n)
{
	SceRcoEntry *plane = (SceRcoEntry *)((u32)src + src->first_child);
	SceRcoEntry *mlist = (SceRcoEntry *)((u32)plane + plane->first_child);
	u32 *mlist_param = (u32 *)((u32)mlist + mlist->param);

	/* Backup */
	if(backup[0] == 0 && backup[1] == 0 && backup[2] == 0 && backup[3] == 0)
	{
		backup[0] = mlist->first_child;
		backup[1] = mlist->child_count;
		backup[2] = mlist_param[16];
		backup[3] = mlist_param[18];
	}

	if(context_mode)
	{
		SceRcoEntry *base = (SceRcoEntry *)((u32)mlist + mlist->first_child);

		SceRcoEntry *item = (SceRcoEntry *)sce_paf_private_malloc(base->next_entry * n);
		u32 *item_param = (u32 *)((u32)item + base->param);

		mlist->first_child = (u32)item - (u32)mlist;
		mlist->child_count = n;
		mlist_param[16] = 13;
		mlist_param[18] = 6;

		int i;
		for(i = 0; i < n; i++)
		{
			sce_paf_private_memcpy(item, base, base->next_entry);

			item_param[0] = 0xDEAD;
			item_param[1] = (u32)options[i];

			if(i != 0) item->prev_entry = item->next_entry;
			if(i == n - 1) item->next_entry = 0;

			item = (SceRcoEntry *)((u32)item + base->next_entry);
			item_param = (u32 *)((u32)item + base->param);
		}
	}
	else
	{
		/* Restore */
		mlist->first_child = backup[0];
		mlist->child_count = backup[1];
		mlist_param[16] = backup[2];
		mlist_param[18] = backup[3];
	}

	sceKernelDcacheWritebackAll();
}

int PAF_Resource_GetPageNodeByID_Patched(void *resource, char *name, SceRcoEntry **child)
{
	int res = PAF_Resource_GetPageNodeByID(resource, name, child);

	if(name)
	{
		if(is_tn_config == 1)
		{
			if(sce_paf_private_strcmp(name, "page_psp_config_umd_autoboot") == 0)
			{
				switch(context_mode)
				{
					case 0:
						HijackContext(*child, NULL, 0);
						break;

					case 1:
						HijackContext(*child, string.speed, sizeof(string.speed) / sizeof(char *));
						break;

					case 2:
						HijackContext(*child, string.usbdevice, sizeof(string.usbdevice) / sizeof(char *));
						break;

					case 3:
						HijackContext(*child, string.regions, sizeof(string.regions) / sizeof(char *));
						break;

					case 4:
						HijackContext(*child, button_options, sizeof(button_options) / sizeof(char *));
						break;
						
					case 5:
						HijackContext(*child, string.drivers, sizeof(string.drivers) / sizeof(char *));
						break;
							
					case 6:
						HijackContext(*child, string.msspeed, sizeof(string.msspeed) / sizeof(char *));
						break; 
						
					case 7:
						HijackContext(*child, dr_normal, sizeof(dr_normal) / sizeof(char *));
						break;
				}
			}
		}
	}

	return res;
}

int PAF_Resource_ResolveRefWString_Patched(void *resource, u32 *data, int *a2, char **string, int *t0)
{
	if(data[0] == 0xDEAD)
	{
		utf8_to_unicode((wchar_t *)user_buffer, (char *)data[1]);
		*(wchar_t **)string = (wchar_t *)user_buffer;
		return 0;
	}

	return PAF_Resource_ResolveRefWString(resource, data, a2, string, t0);
}

int sceDisplaySetHoldModePatched(int status)
{
	if(config.skipgameboot) return 0;
	return sceDisplaySetHoldMode(status);
}

int auth_handler_new(int a0)
{
	startup = a0;
	return auth_handler(a0);
}

int OnInitAuthPatched(void *a0, int (* handler)(), void *a2, void *a3, int (* OnInitAuth)())
{
	return OnInitAuth(a0, startup ? auth_handler_new : handler, a2, a3);
}

int sceVshCommonGuiBottomDialogPatched(void *a0, void *a1, void *a2, int (* cancel_handler)(), void *t0, void *t1, int (* handler)(), void *t3)
{
	return sceVshCommonGuiBottomDialog(a0, a1, a2, startup ? OnRetry : cancel_handler, t0, t1, handler, t3);
}


void PatchVshMain(u32 text_addr)
{
#ifdef CONFIG_639
	if(psp_fw_version == FW_639) {
		AddVshItem = (void *)text_addr + 0x00022608;
		ExecuteAction = (void *)text_addr + 0x000169A8;
		UnloadModule = (void *)text_addr + 0x00016D9C;
		OnXmbPush = (void *)text_addr + 0x000168EC;
		OnXmbContextMenu = (void *)text_addr + 0x000163A0;
		LoadStartAuth = (void *)text_addr + 0x5DA0;
		auth_handler = (void *)text_addr + 0x1A2EC;

		/* Allow old SFOs */
		_sw(0, text_addr + 0x11FD8);
		_sw(0, text_addr + 0x11FE0);
		_sw(0, text_addr + 0x12230);
		
		MAKE_CALL(text_addr + 0xCA08, sceDisplaySetHoldModePatched);

		MAKE_CALL(text_addr + 0x00020EBC, AddVshItemPatched);
		MAKE_CALL(text_addr + 0x00016984, ExecuteActionPatched);
		MAKE_CALL(text_addr + 0x00030828, ExecuteActionPatched);
		MAKE_CALL(text_addr + 0x00016B7C, UnloadModulePatched);

		_sw(0x8C48000C, text_addr + 0x5ED4); //lw $t0, 12($v0)
		MAKE_CALL(text_addr + 0x5ED8, OnInitAuthPatched);
		
		REDIRECT_FUNCTION(text_addr + 0x0003F264, scePafGetTextPatched);

		_sw((u32)OnXmbPushPatched, text_addr + 0x00052E34);
		_sw((u32)OnXmbContextMenuPatched, text_addr + 0x00052E40);
	}
#endif

#ifdef CONFIG_635
	if(psp_fw_version == FW_635) {
		AddVshItem = (void *)text_addr + 0x00022608;
		ExecuteAction = (void *)text_addr + 0x000169A8;
		UnloadModule = (void *)text_addr + 0x00016D9C;
		OnXmbPush = (void *)text_addr + 0x000168EC;
		OnXmbContextMenu = (void *)text_addr + 0x000163A0;
		LoadStartAuth = (void *)text_addr + 0x5DA0;
		auth_handler = (void *)text_addr + 0x1A2EC;
		
		/* Allow old SFOs */
		_sw(0, text_addr + 0x11FD8);
		_sw(0, text_addr + 0x11FE0);
		_sw(0, text_addr + 0x12230);
				
		MAKE_CALL(text_addr + 0xCA08, sceDisplaySetHoldModePatched);

		MAKE_CALL(text_addr + 0x00020EBC, AddVshItemPatched);
		MAKE_CALL(text_addr + 0x00016984, ExecuteActionPatched);
		MAKE_CALL(text_addr + 0x00030828, ExecuteActionPatched);
		MAKE_CALL(text_addr + 0x00016B7C, UnloadModulePatched);

		_sw(0x8C48000C, text_addr + 0x5ED4); //lw $t0, 12($v0)
		MAKE_CALL(text_addr + 0x5ED8, OnInitAuthPatched);
		
		REDIRECT_FUNCTION(text_addr + 0x0003F264, scePafGetTextPatched);

		_sw((u32)OnXmbPushPatched, text_addr + 0x00052E3C);
		_sw((u32)OnXmbContextMenuPatched, text_addr + 0x00052E48);
	}
#endif

#ifdef CONFIG_620
   	if (psp_fw_version == FW_620) {
		AddVshItem = (void *)text_addr + 0x21E18;
		ExecuteAction = (void *)text_addr + 0x16340;
		UnloadModule = (void *)text_addr + 0x16734;
		OnXmbContextMenu = (void *)text_addr + 0x15D38;
		OnXmbPush = (void *)text_addr + 0x16284;
		LoadStartAuth = (void *)text_addr + 0x5D10;
		auth_handler = (void *)text_addr + 0x19C40;

		/* Allow old SFOs */
		_sw(0, text_addr + 0x11A70);
		_sw(0, text_addr + 0x11A78);
		_sw(0, text_addr + 0x11D84);

		MAKE_CALL(text_addr + 0xC7A8, sceDisplaySetHoldModePatched);

		MAKE_CALL(text_addr + 0x206F8, AddVshItemPatched);
		MAKE_CALL(text_addr + 0x1631C, ExecuteActionPatched);
		MAKE_CALL(text_addr + 0x2FF8C, ExecuteActionPatched);
		MAKE_CALL(text_addr + 0x16514, UnloadModulePatched);

		_sw(0x8C48000C, text_addr + 0x5E44); //lw $t0, 12($v0)
		MAKE_CALL(text_addr + 0x5E48, OnInitAuthPatched);

		REDIRECT_FUNCTION(text_addr + 0x3ECB0, scePafGetTextPatched);

		_sw((u32)OnXmbPushPatched, text_addr + 0x51EBC);
		_sw((u32)OnXmbContextMenuPatched, text_addr + 0x51EC8);
	}
#endif

	ClearCaches();
}

void PatchSysconfPlugin(u32 text_addr)
{
#ifdef CONFIG_639
	if(psp_fw_version == FW_639) {
		AddSysconfItem = (void *)text_addr + 0x0002828C;
		GetSysconfItem = (void *)text_addr + 0x00023854;
		OnInitMenuPspConfig = (void *)text_addr + 0x0001CC08;

		sysconf_unk = text_addr + 0x000330F0;
		sysconf_option = text_addr + 0x000335BC;

		/* Allows more than 18 items */
		_sh(0xFF, text_addr + 0x000029AC);

		MAKE_CALL(text_addr + 0x00001714, vshGetRegistryValuePatched);
		MAKE_CALL(text_addr + 0x00001738, vshSetRegistryValuePatched);

		MAKE_CALL(text_addr + 0x00002A28, GetSysconfItemPatched);

		REDIRECT_FUNCTION(text_addr + 0x000299E0, PAF_Resource_GetPageNodeByID_Patched);
		REDIRECT_FUNCTION(text_addr + 0x00029A98, PAF_Resource_ResolveRefWString_Patched);
		REDIRECT_FUNCTION(text_addr + 0x00029708, scePafGetTextPatched);

		_sw((u32)OnInitMenuPspConfigPatched, text_addr + 0x00030424);
		
		if (ver_info)
		{
			_sw(0x3C020000 | ((int)ver_info >> 16), text_addr + 0x18F3C);
			_sw(0x34420000 | ((int)ver_info & 0xFFFF), text_addr + 0x18F3C + 4);
		}

		if (mac_info)
		{
			_sw(0x3C060000 | ((int)mac_info >> 16), text_addr + 0x18C6C);
			_sw(0x24C60000 | ((int)mac_info & 0xFFFF), text_addr + 0x18C6C + 4);
		}		

	} 
#endif

#ifdef CONFIG_635
	if(psp_fw_version == FW_635) {
		AddSysconfItem = (void *)text_addr + 0x0002828C;
		GetSysconfItem = (void *)text_addr + 0x00023854;
		OnInitMenuPspConfig = (void *)text_addr + 0x0001CC08;

		sysconf_unk = text_addr + 0x000330F0;
		sysconf_option = text_addr + 0x000335BC;

		/* Allows more than 18 items */
		_sh(0xFF, text_addr + 0x000029AC);

		MAKE_CALL(text_addr + 0x00001714, vshGetRegistryValuePatched);
		MAKE_CALL(text_addr + 0x00001738, vshSetRegistryValuePatched);

		MAKE_CALL(text_addr + 0x00002A28, GetSysconfItemPatched);

		REDIRECT_FUNCTION(text_addr + 0x000299E0, PAF_Resource_GetPageNodeByID_Patched);
		REDIRECT_FUNCTION(text_addr + 0x00029A98, PAF_Resource_ResolveRefWString_Patched);
		REDIRECT_FUNCTION(text_addr + 0x00029708, scePafGetTextPatched);

		_sw((u32)OnInitMenuPspConfigPatched, text_addr + 0x00030424);	
		
		if (ver_info)
		{
			_sw(0x3C020000 | ((int)ver_info >> 16), text_addr + 0x18F3C);
			_sw(0x34420000 | ((int)ver_info & 0xFFFF), text_addr + 0x18F3C + 4);
		}

		if (mac_info)
		{
			_sw(0x3C060000 | ((int)mac_info >> 16), text_addr + 0x18C6C);
			_sw(0x24C60000 | ((int)mac_info & 0xFFFF), text_addr + 0x18C6C + 4);
		}	
	} 
#endif

#ifdef CONFIG_620
	if (psp_fw_version == FW_620) {
		AddSysconfItem = (void *)text_addr + 0x00027918;
		GetSysconfItem = (void *)text_addr + 0x00022F54;
		OnInitMenuPspConfig = (void *)text_addr + 0x0001C3A0;

		sysconf_unk = text_addr + 0x00032608;
		sysconf_option = text_addr + 0x00032ADC;

		MAKE_CALL(text_addr + 0x000016C8, vshGetRegistryValuePatched);
		MAKE_CALL(text_addr + 0x000016EC, vshSetRegistryValuePatched);

		MAKE_CALL(text_addr + 0x00002934, GetSysconfItemPatched);

		REDIRECT_FUNCTION(text_addr + 0x00028DA4, PAF_Resource_GetPageNodeByID_Patched);
		REDIRECT_FUNCTION(text_addr + 0x00028FFC, PAF_Resource_ResolveRefWString_Patched);
		REDIRECT_FUNCTION(text_addr + 0x0002901C, scePafGetTextPatched);

		_sw((u32)OnInitMenuPspConfigPatched, text_addr + 0x0002FA24);

		//y-y-yes it's finally for 6.20
		_sh(0xFF, text_addr + 0x28B8);
		
		if (ver_info)
		{
			_sw(0x3C020000 | ((int)ver_info >> 16), text_addr + 0x18920);
			_sw(0x34420000 | ((int)ver_info & 0xFFFF), text_addr + 0x18920 + 4);
		}

		if (mac_info)
		{
			_sw(0x3C060000 | ((int)mac_info >> 16), text_addr + 0x18650);
			_sw(0x24C60000 | ((int)mac_info & 0xFFFF), text_addr + 0x18650 + 4);
		}	
	}
#endif

	ClearCaches();
}

void PatchMsVideoMainPlugin(u32 text_addr, u32 text_size)
{

#ifdef CONFIG_639
	if(psp_fw_version == FW_639) {
		/* Patch bitrate limit (increase to 16384+2) */
		_sh(0x4003, text_addr + 0x3D324);
		_sh(0x4003, text_addr + 0x3D75C);
		_sh(0x4003, text_addr + 0x431F8);
	
		/* Patch resolution limit to (130560) pixels (480x272) */
		int i;
		for(i = 0; i < text_size; i += 4)
		{
			u32 addr = text_addr + i;
			if((_lw(addr) & 0x34000000) == 0x34000000 && (_lw(addr) & 0xFFFF) == 0x2C00)
			{
				_sh(0xFE00, addr);
			}
		}
	} 
#endif

#ifdef CONFIG_635
	if(psp_fw_version == FW_635) {
		/* Patch bitrate limit (increase to 16384+2) */
		_sh(0x4003, text_addr + 0x3D324);
		_sh(0x4003, text_addr + 0x3D75C);
		_sh(0x4003, text_addr + 0x431F8);
	
		/* Patch resolution limit to (130560) pixels (480x272) */
		int i;
		for(i = 0; i < text_size; i += 4)
		{
			u32 addr = text_addr + i;
			if((_lw(addr) & 0x34000000) == 0x34000000 && (_lw(addr) & 0xFFFF) == 0x2C00)
			{
				_sh(0xFE00, addr);
			}
		}
	} 
#endif

#ifdef CONFIG_620
	if (psp_fw_version == FW_620) {
		/* Patch resolution limit to (130560) pixels (480x272) */
		_sh(0xFE00, text_addr + 0x3AB2C);
		_sh(0xFE00, text_addr + 0x3ABB4);
		_sh(0xFE00, text_addr + 0x3D3AC);
		_sh(0xFE00, text_addr + 0x3D608);
		_sh(0xFE00, text_addr + 0x43B98);
		_sh(0xFE00, text_addr + 0x73A84);
		_sh(0xFE00, text_addr + 0x880A0);
	
		/* Patch bitrate limit (increase to 16384+2) */
		_sh(0x4003, text_addr + 0x3D324);
		_sh(0x4003, text_addr + 0x3D36C);
		_sh(0x4003, text_addr + 0x42C40);
	}
#endif
	

	ClearCaches();
}

void PatchAuthPlugin(u32 text_addr)
{

#ifdef CONFIG_639
	if(psp_fw_version == FW_639) {
		OnRetry = (void *)text_addr + 0x5C8;
		MAKE_CALL(text_addr + 0x13A0, sceVshCommonGuiBottomDialogPatched);
		ClearCaches();
	} 
#endif

#ifdef CONFIG_635
	if(psp_fw_version == FW_635) {
		OnRetry = (void *)text_addr + 0x5C8;
		MAKE_CALL(text_addr + 0x13A0, sceVshCommonGuiBottomDialogPatched);
		ClearCaches();	
	} 
#endif

#ifdef CONFIG_620
	if (psp_fw_version == FW_620) {
		OnRetry = (void *)text_addr + 0x5C8;
		MAKE_CALL(text_addr + 0x13B4, sceVshCommonGuiBottomDialogPatched);
		ClearCaches();
	}
#endif
	
}


int OnModuleStart(SceModule2 *mod)
{
	char *modname = mod->modname;
	u32 text_addr = mod->text_addr;

	if(sce_paf_private_strcmp(modname, "vsh_module") == 0) 
	{
		PatchVshMain(text_addr);
	}
	else if(sce_paf_private_strcmp(modname, "sceVshAuthPlugin_Module") == 0)
	{
		PatchAuthPlugin(text_addr);
	}
	else if(sce_paf_private_strcmp(modname, "sysconf_plugin_module") == 0) 
	{
		PatchSysconfPlugin(text_addr);
	}
	else if(sce_paf_private_strcmp(modname, "msvideo_main_plugin_module") == 0)
	{
		PatchMsVideoMainPlugin(text_addr, mod->text_size);
	}

	if(!previous)
		return 0;

	return previous(mod);
}
	int getSpoof(char *file, int mode)
	{	
		int i;
		char *global;
		u16 isunicode = 0;
	
		SceIoStat stat;
		memset(&stat, 0, sizeof(SceIoStat));
	
		if (sceIoGetstat(file, &stat) < 0)
			return -1;

		SceUID fd = sceIoOpen(file, PSP_O_RDONLY, 0777);
	
		if (fd < 0)
			return -1;
	
		sceIoRead(fd, &isunicode, sizeof(u16));
	
		if (isunicode != 0xFEFF)
		{
			isunicode = 0;
			stat.st_size = (stat.st_size * 2) + 2;
			sceIoLseek32(fd, 0, PSP_SEEK_SET);
		}

		//	SceUID block_id = sceKernelAllocPartitionMemory(2, "", PSP_SMEM_Low, stat.st_size, NULL);
		SceUID block_id = sceKernelAllocPartitionMemory(2, "", 1, stat.st_size, NULL);

		if (block_id < 0)
			return -1;

		global = sceKernelGetBlockHeadAddr(block_id);
		memset(global, 0, stat.st_size);
	
		if (isunicode)
		{
			sceIoRead(fd, global, stat.st_size - 2);
		}
	
		else
		{
			for (i = 0; i < (stat.st_size / 2); i++)
			{
				sceIoRead(fd, global + (i * 2), 1);
			}
		}

		if (!mode)
			ver_info = global; //lazy

		else
			mac_info = global;

		sceIoClose(fd);
		return 0;
	}
int module_start(SceSize args, void *argp)
{
	psp_fw_version = sceKernelDevkitVersion();
	psp_model = kuKernelGetModel();
	int ret;
        
	ret = GetConfig_x(&Xconfig);

	if (ret != 0) {
			load_default_conf_x(&Xconfig);
			SetConfig_x(&Xconfig);
	} else {
			//printk("Loading config OK\n");
	}
	sctrlSEGetConfig(&config);
	//sctrlXMBGetConfig(&Xconfig);
	
	/*if (Xconfig.spoof_ver == 1) {
		int fd = getSpoof("ms0:/spver.txt", 0);
		if(fd < 0)
		{
			fd = getSpoof("ef0:/spver.txt", 0);
		}
		
		if(fd < 0)
		{
			//ver_info = "";
		} 
		
	}*/
	if (Xconfig.spoof_mac == 1) {
		int fd = getSpoof("ms0:/spmac.txt", 1);
		if(fd < 0)
		{
			fd = getSpoof("ef0:/spmac.txt", 1);
		}
		
		if(fd < 0)
		{
			//mac_info = "";
		}
		
	}
	previous = sctrlHENSetStartModuleHandler(OnModuleStart);
	return 0;
}

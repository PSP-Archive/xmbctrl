/*
	6.39 TN-A, XmbControl
	Copyright (C) 2011, Total_Noob

	main.h: XmbControl main header file
	
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

#ifndef __MAIN_H__
#define __MAIN_H__

#define MAKE_CALL(a, f) _sw(0x0C000000 | (((u32)(f) >> 2) & 0x03FFFFFF), a);
#define REDIRECT_FUNCTION(a, f) _sw(0x08000000 | (((u32)(f) >> 2) & 0x03FFFFFF), a); _sw(0, a + 4);
#define MAKE_DUMMY_FUNCTION0(a) _sw(0x03E00008, a); _sw(0x00001021, a + 4);

#define sysconf_console_id 4
#define sysconf_console_action 2
#define sysconf_console_action_arg 2

#define sysconf_tnconfig_action_arg 0x1000
#define sysconf_tnconfig_resetvsh 0x1001
#define sysconf_tnconfig_resetconf 0x1002

typedef struct
{
	char text[48];
	int play_sound;
	int action;
	int action_arg;
} SceContextItem; //60

typedef struct
{
	int id; //0
	int relocate; //4
	int action; //8
	int action_arg; //C
	SceContextItem *context; //10
	char *subtitle; //14
	int unk; //18
	char play_sound; //1C
	char memstick; //1D
	char umd_icon; //1E
	char image[4]; //1F
	char image_shadow[4]; //23
	char image_glow[4]; //27
	char text[0x25]; //2B
} SceVshItem; //50

typedef struct
{
	void *unk; //0
	int id; //4
	char *regkey; //8
	char *text; //C
	char *subtitle; //10
	char *page; //14
} SceSysconfItem; //18

typedef struct
{
	u8 id; //0
	u8 type; //1
	u16 unk1; //2
	u32 label; //4
	u32 param; //8
	u32 first_child; //C
	int child_count; //10
	u32 next_entry; // 14
	u32 prev_entry; //18
	u32 parent; //1C
	u32 unknown[2]; //20
} SceRcoEntry; //24

int sce_paf_private_wcslen(wchar_t *);
int sce_paf_private_sprintf(char *, const char *, ...);
void *sce_paf_private_memcpy(void *, void *, int);
void *sce_paf_private_memset(void *, char, int);
int sce_paf_private_strlen(char *);
char *sce_paf_private_strcpy(char *, const char *);
char *sce_paf_private_strncpy(char *, const char *, int);
int sce_paf_private_strcmp(const char *, const char *);
int sce_paf_private_strncmp(const char *, const char *, int);
char *sce_paf_private_strchr(const char *, int);
char *sce_paf_private_strrchr(const char *, int);
char* sce_paf_private_strpbrk(const char *, const char *);
int sce_paf_private_strtoul(const char *, char **, int);
void *sce_paf_private_malloc(int);
void sce_paf_private_free(void *);

wchar_t *scePafGetText(void *, char *);
int PAF_Resource_GetPageNodeByID(void *, char *, SceRcoEntry **);
int PAF_Resource_ResolveRefWString(void *, u32 *, int *, char **, int *);

int vshGetRegistryValue(u32 *, char *, void *, int , int *);
int vshSetRegistryValue(u32 *, char *, int , int *);

int sceVshCommonGuiBottomDialog(void *a0, void *a1, void *a2, int (* cancel_handler)(), void *t0, void *t1, int (* handler)(), void *t3);

#endif

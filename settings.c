/*
 * Copyright (C) 2012 Michael L. Schaecher
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/reboot.h>
#include <reboot/reboot.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <sys/wait.h>
#include <sys/limits.h>
#include <dirent.h>
#include <sys/stat.h>

#include <signal.h>
#include <sys/wait.h>

#include "bootloader.h"
#include "common.h"
#include "cutils/properties.h"
#include "firmware.h"
#include "install.h"
#include "minui/minui.h"
#include "minzip/DirUtil.h"
#include "roots.h"
#include "recovery_ui.h"

#include "extendedcommands.h"
#include "settings.h"
#include "nandroid.h"
#include "mounts.h"
#include "flashutils/flashutils.h"
#include "edify/expr.h"
#include <libgen.h>
#include "mtdutils/mtdutils.h"
#include "bmlutils/bmlutils.h"

#define ABS_MT_POSITION_X 0x35  /* Center X ellipse position */
#define ABS_MT_POSITION_Y 0x36  /* Center Y ellipse position */

int signature_check_enabled = 1;
int script_assert_enabled = 1;

void
toggle_signature_check()
{
    signature_check_enabled = !signature_check_enabled;
    ui_print("Signature Check: %s\n", signature_check_enabled ? "Enabled" : "Disabled");
}

void toggle_script_asserts()
{
    script_assert_enabled = !script_assert_enabled;
    ui_print("Script Asserts: %s\n", script_assert_enabled ? "Enabled" : "Disabled");
}

static void partition_sdcard(const char* volume) {

    static char* ext_sizes[] = { "205M",
                                 "410M",
                                 "819M",
                                 "1638M",
                                 "3276M",
                                 NULL };

    static char* swap_sizes[] = { "0M",
                                  "32M",
                                  "64M",
                                  "128M",
                                  "256M",
                                  NULL };

    static char* ext_types[] = { "ext2",
                                  "ext3",
                                  "ext4",
                                  NULL };

    static char* ext_headers[] = { "Chose Partition Size", "", NULL };
    static char* swap_headers[] = { "Chose Swap Size (optional)", "", NULL };
    static char* type_headers[] = { "Chose File System", "", NULL };

    int ext_size = get_menu_selection(ext_headers, ext_sizes, 0, 0);
    if (ext_size == GO_BACK)
        return;

    int swap_size = get_menu_selection(swap_headers, swap_sizes, 0, 0);
    if (swap_size == GO_BACK)
        return;

    int ext_type = get_menu_selection(type_headers, ext_types, 0, 0);
    if (ext_type == GO_BACK)
        return;

    char sddevice[256];
    Volume *vol = volume_for_path(volume);
    strcpy(sddevice, vol->device);
    // we only want the mmcblk, not the partition
    sddevice[strlen("/dev/block/mmcblkX")] = NULL;
    char cmd[PATH_MAX];
    setenv("SDPATH", sddevice, 1);
    sprintf(cmd, "sdparted -es %s -ss %s -efs %s -s", ext_sizes[ext_size], swap_sizes[swap_size], ext_types[ext_type]);
    ui_print("Creating %s %s partition.\n -- Please wait...\n\n", ext_sizes[ext_size], ext_types[ext_type]);
    if (0 == __system(cmd))
        ui_print("Done!\n");
    else
        ui_print("An error occured while\npartitioning your SD Card,\nplease see /tmp/recovery.log\nfor more details.\n");
}

static void convert_partition(const char* volume) {

    char backup_path[PATH_MAX];
    time_t t = time(NULL);
    struct tm *tmp = localtime(&t);
    if (tmp == NULL)
    {
        struct timeval tp;
        gettimeofday(&tp, NULL);
        sprintf(backup_path, "/sdcard/clockworkmod/backup/%d", tp.tv_sec);
    } else {
        strftime(backup_path, sizeof(backup_path), "/sdcard/clockworkmod/backup/%F.%H.%M.%S", tmp);
    }

    static char* ext_upgrader[] = { "ext3",
                                   "ext4",
                                   NULL };

    static char* ext_headers[] = { "Convert Partition To", "", NULL };

    int ext_upgrade = get_menu_selection(ext_headers, ext_upgrader, 0, 0);
    if (ext_upgrade == GO_BACK)
        return;

    ui_print("Backing and fromating sd-ext\nbefore converting file sysetm.\n\n");
    nandroid_backup(backup_path);
    format_volume("/sd-ext");
    char sddevice[256];
    Volume *vol = volume_for_path(volume);
    strcpy(sddevice, vol->device);
    // To upgarde the sd-ext partition we need to mount the path first.
    sddevice[strlen("/dev/block/mmcblk0p2")] = NULL;
    char cmd[PATH_MAX];
    setenv("EXTPATH", sddevice, 1);
    sprintf(cmd, "sdparted -ufs %s -s", ext_upgrader[ext_upgrade]);
    ui_print("Converting Partition to %s\n -- Please wait...\n\n", ext_upgrader[ext_upgrade]);
    if (0 == __system(cmd)) {
        ui_print("Done!\nRestoring sd-ext backup\n\n");
        nandroid_restore(backup_path, 0, 0, 0, 0, 1, 0);
    } else {
        ui_print("An error occured while\nconvert partition,\nplease see recovery.log\nfor more details.\n");
    }
}

void show_advance_menu() {

	static char* headers[] = { "Advance Menu",
                                "",
                                NULL
    };

	#define OPTIONS_ITEM_UTILITIES		0
	#define OPTIONS_ITEM_SETTINGS		1
	#define OPTIONS_ITEM_DEBUGGING		2
                               
	static char* list[4];
	list[0] = "Utlities";
	list[1] = "Settings";
	list[2] = "Debugging";
	list[3] = NULL;

	for (;;) {

		int chosen_menu = get_menu_selection(headers, list, 0, 0);
		if (chosen_menu == GO_BACK)
		    break;
		switch (chosen_menu) {
		    case OPTIONS_ITEM_UTILITIES:
		    {
		        static char* utility_headers[3];
		        utility_headers[0] = "Utlities Menu";
		        utility_headers[1] = "\n";
		        utility_headers[2] = NULL;
		                        
		        static char* utility_list[4];
		        utility_list[0] = "Partition Manager";
		        utility_list[1] = "Mounts and Storage";
		        utility_list[2] = "Clear Screen";
		        utility_list[3] = NULL;

		        int chosen_utility = get_menu_selection(utility_headers, utility_list, 0, 0);
		        switch (chosen_utility) {
		                 case 0:
                                 {
                                     static char* partition_headers[3];
                                     partition_headers[0] = "Partition Manager";
                                     partition_headers[1] = "\n";
                                     partition_headers[2] = NULL;

                                     static char* partition_list[3];
                                     partition_list[0] = "Partition SDCard";
                                     partition_list[1] = "Convert Partition";
                                     partition_list[2] = NULL;

                                     int chosen_partition = get_menu_selection(partition_headers, partition_list, 0, 0);
                                     switch (chosen_partition) {
                                              case 0:
                                                  partition_sdcard("/sdcard");
		                                  break;
                                              case 1:
                                                  convert_partition("/sd-ext");
                                                  break;
                                     }
                                     break;
                                 }
		                 case 1:
		                     show_partition_menu();
		                     break;
		                 case 2:
		                     ui_print("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
		                     break;
		        }
		        break;
		    }
		    case OPTIONS_ITEM_SETTINGS:
		    {
		        static char* settings_headers[3];
		        settings_headers[0] = "Settings Menu";
		        settings_headers[1] = "\n";
		        settings_headers[2] = NULL;
		                        
		        static char* settings_list[3];
		        settings_list[0] = "Set Signature Check";
		        settings_list[1] = "Set Script Asserts";
		        settings_list[2] = NULL;            

		        int chosen_settings = get_menu_selection(settings_headers, settings_list, 0, 0);
		        switch (chosen_settings) {
                                 case 0:
		                     toggle_signature_check();
		                     break;
		                 case 1:
		                     toggle_script_asserts();
		                     break;
		        }
		        break;
		    }
		    case OPTIONS_ITEM_DEBUGGING:
		    {
		        static char* debug_headers[3];
		        debug_headers[0] = "Debug Menu";
		        debug_headers[1] = "\n";
		        debug_headers[2] = NULL;

		        static char* debug_list[5];
		        debug_list[0] = "Print Recovery log";
		        debug_list[1] = "Report Errors";
		        debug_list[2] = "Run Key Test";
		        debug_list[3] = "Qiuck Fix";
		        debug_list[4] = NULL;

		        int chosen_debug = get_menu_selection(debug_headers, debug_list, 0, 0);
		        switch (chosen_debug) {
		                 case 0:
		                 {
		                     ui_printlogtail(12);
		                     break;
		                 }
		                 case 1:
		                     handle_failure(1);
		                     break;
	                         case 2:
                                {
                                    ui_print("Outputting key codes.\n");
                                    ui_print("Go back to end debugging.\n");
                                    struct keyStruct{
				                    	int code;
				                    	int x;
				                    	int y;
				                    }*key;
                                    int action;
                                    do
                                    {
                                        key = ui_wait_key();
	                    				if(key->code == ABS_MT_POSITION_X)
	                    				{
	                    			        action = device_handle_mouse(key, 1);
	                    					ui_print("Touch: X: %d\tY: %d\n", key->x, key->y);
	                    				}
	                    				else
	                    				{
	                    			        action = device_handle_key(key->code, 1);
	                    					ui_print("Key: %x\n", key->code);
	                    				}
                                    }
                                    while (action != GO_BACK);
                                    break;
                                 }
		                 case 3:
                                 {
                                     static char* fixes_headers[3];
                                     fixes_headers[0] = "Qiuck Fixes";
                                     fixes_headers[1] = "\n";
                                     fixes_headers[2] = NULL;

                                     static char* fixes_list[3];
                                     fixes_list[0] = "Fix Premissions";
                                     fixes_list[1] = "Fix Recvoery Boot-loop";
                                     fixes_list[2] = NULL;

                                     int chosen_fixes = get_menu_selection(fixes_headers, fixes_list, 0, 0);
                                     switch (chosen_fixes) {
                                              case GO_BACK:
                                                  continue;
                                              case 0:
                                              {
                                                  ensure_path_mounted("/system");
		                                  ensure_path_mounted("/data");
		                                  ui_print("Fixing permissions...\n");
		                                  __system("fix_permissions");
		                                  ui_print("Done!\n");
		                                  break;
                                              }
                                              case 1:
			                      {
				                  format_volume("misc:");
				                  format_volume("persist:");
				                  reboot(RB_AUTOBOOT);
				                  break;
		                              }
                                     }
                                     break;
                                 }
		        }
		        break;
	    }
	}
    }
}

void handle_failure(int ret)
{
    if (ret == 0)
        return;
    if (0 != ensure_path_mounted("/sdcard"))
        return;
    mkdir("/sdcard/clockworkmod", S_IRWXU);
    __system("cp /tmp/recovery.log /sdcard/clockworkmod/recovery.log");
    ui_print("/tmp/recovery.log was copied to\n/sdcard/clockworkmod/recovery.log.\nPlease visit ZenGarden home\npage to report error.\n\n\n\n");
}

int verify_root_and_recovery() {
    if (ensure_path_mounted("/system") != 0)
        return 0;

    int ret = 0;
    struct stat st;
    if (0 == lstat("/system/etc/install-recovery.sh", &st)) {
        if (st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) {
            ui_show_text(1);
            ret = 1;
            if (confirm_selection("ROM may flash stock recovery on boot. Fix?", "Yes - Disable recovery flash")) {
                __system("chmod -x /system/etc/install-recovery.sh");
            }
        }
    }

    if (0 == lstat("/system/bin/su", &st)) {
        if (S_ISREG(st.st_mode)) {
            if ((st.st_mode & (S_ISUID | S_ISGID)) != (S_ISUID | S_ISGID)) {
                ui_show_text(1);
                ret = 1;
                if (confirm_selection("Root access possibly lost. Fix?", "Yes - Fix root (/system/bin/su)")) {
                    __system("chmod 6755 /system/bin/su");
                }
            }
        }
    }

    if (0 == lstat("/system/xbin/su", &st)) {
        if (S_ISREG(st.st_mode)) {
            if ((st.st_mode & (S_ISUID | S_ISGID)) != (S_ISUID | S_ISGID)) {
                ui_show_text(1);
                ret = 1;
                if (confirm_selection("Root access possibly lost. Fix?", "Yes - Fix root (/system/xbin/su)")) {
                    __system("chmod 6755 /system/xbin/su");
                }
            }
        }
    }

    ensure_path_unmounted("/system");
    return ret;
}


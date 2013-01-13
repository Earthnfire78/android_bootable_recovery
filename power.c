/*
 * Copyright (C) 2012 Drew Walton & Nathan Bass
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

void show_power_menu() {

	static char* headers[] = { "Power Options",
                                "",
                                NULL
    };

	#define POWER_OPTIONS_ITEM_REBOOT	0
	#define POWER_OPTIONS_ITEM_REBOOT2	1
	#define POWER_OPTIONS_ITEM_REBOOT3	2

	static char* list[4];
	list[0] = "Boot Android";
        list[1] = "Reboot Recovery";
	list[2] = "Reboot Bootloader";
	list[3] = NULL;
	for (;;) {

		int chosen_item = get_menu_selection(headers, list, 0, 0);
		if (chosen_item == GO_BACK)
		    break;
		switch (chosen_item) {
			case POWER_OPTIONS_ITEM_REBOOT:
				reboot_wrapper(NULL);
				break;
			case POWER_OPTIONS_ITEM_REBOOT2:
				reboot_wrapper("recovery");
				break;
			case POWER_OPTIONS_ITEM_REBOOT3:
				reboot_wrapper("bootloader");
				break;
		}
	}
}

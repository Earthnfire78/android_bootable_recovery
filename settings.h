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

extern int signature_check_enabled;
extern int script_assert_enabled;

void toggle_signature_check();

void toggle_script_asserts();

void show_power_menu();

void show_mount_usb_storage_menu();

void show_partition_menu();

void show_advanced_menu();

#define NANDROID_BACKUP_FORMAT_FILE "/sdcard/zenrecovery/settings/.default_backup_format"
#define NANDROID_BACKUP_FORMAT_TAR 0
#define NANDROID_BACKUP_FORMAT_DUP 1

#define SIGNATURE_CHECK_FILE "/sdcard/zenrecovery/settings/.signature_check"
#define SIGNATURE_CHECK_ENABLED  1
#define SIGNATURE_CHECK_DISABLED 0

void show_mount_usb_storage_menu();

void handle_failure(int ret);

int verify_root_and_recovery();





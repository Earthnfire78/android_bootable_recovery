int 
install_zip(const char* packagefilepath);

int
install_zip_reboot(const char* packagefilepath);

int
install_gapps_reboot(const char* packagefilepath);

void
show_install_update_menu();

void
free_string_array(char** array);

void
show_choose_gapps_menu(const char *mount_point);

void
show_choose_zip_reboot_menu(const char *mount_point);

void
show_choose_zip_menu(const char *mount_point);

int
__system(const char *command);

int format_unknown_device(const char *device, const char* path, const char *fs_type);

void 
show_nandroid_restore_menu(const char* path);

void
show_nandroid_verify_menu(const char* path);

void
show_nandroid_delete_menu(const char* path);

int
confirm_selection(const char* title, const char* confirm);

void
show_nandroid_advanced_restore_menu(const char* path);

void 
show_nandroid_menu();

void
write_fstab_root(char *path, FILE *file);

void create_fstab();

int bml_check_volume(const char *path);

void process_volumes();

int
is_path_mounted(const char* path);

int has_datadata();

int
volume_main(int argc, char **argv);

int extendedcommand_file_exists();

#define UPDATING_FORMAT_DATA "rm -r /data/backup /data/boot-cache /data/dalvik-cache /data/local/tmp /data/lost+found /sd-ext/dalvik-cache /sd-ext/data"
#define UPDATING_FORMAT_CASHE "rm -r /data/dalvik-cache /sd-ext/dalvik-cache"



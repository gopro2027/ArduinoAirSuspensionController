// #include "image_tile.h"
// lv_obj_t *img = NULL;

// #define MAX_FILENAME_LENGTH 50
// #define MAX_FILE_COUNT 100

// char bin_filenames[MAX_FILE_COUNT][MAX_FILENAME_LENGTH];
// int bin_file_count = 0;

// int is_bin_file(const char *filename)
// {
//     const char *ext = strrchr(filename, '.');
//     return (ext != NULL && strcmp(ext, ".bin") == 0);
// }

// void read_bin_files(const char *dir_path)
// {
//     lv_fs_dir_t dir;
//     lv_fs_res_t res = lv_fs_dir_open(&dir, dir_path);
//     if (res != LV_FS_RES_OK)
//     {
//         return;
//     }
//     char filename[MAX_FILENAME_LENGTH];
//     while (lv_fs_dir_read(&dir, filename) == LV_FS_RES_OK && filename[0] != '\0')
//     {
//         // printf("Checking file: %s\n", filename);
//         if (is_bin_file(filename))
//         {
//             if (bin_file_count < MAX_FILE_COUNT)
//             {
//                 strncpy(bin_filenames[bin_file_count], filename, MAX_FILENAME_LENGTH - 1);
//                 bin_filenames[bin_file_count][MAX_FILENAME_LENGTH - 1] = '\0';
//                 bin_file_count++;
//             }
//             else
//             {
//                 break;
//             }
//         }
//     }
//     lv_fs_dir_close(&dir);
// }

// void print_bin_filenames()
// {
//     for (int i = 0; i < bin_file_count; i++)
//     {
//         printf("Found .bin file: %s \r\n", bin_filenames[i]);
//     }
// }

// static void img_callback(lv_event_t *e)
// {
//     char str_buf[20];
//     static uint16_t img_index = 0;
//     lv_event_code_t code = lv_event_get_code(e);
//     if (code == LV_EVENT_SHORT_CLICKED)
//     {
//         if (++img_index >= bin_file_count)
//         {
//             img_index = 0;
//         }
//         sprintf(str_buf, "D:/images/%s", bin_filenames[img_index]);
//         lv_img_set_src(img, str_buf);
//     }
// }

// void clicked_event_cb(lv_event_t *e)
// {
//     lv_event_code_t code = lv_event_get_code(e);
//     if (code == LV_EVENT_SHORT_CLICKED)
//     {
//         printf("LV_EVENT_SHORT_CLICKED\n");
//     }
// }
// void image_tile_init(lv_obj_t *parent)
// {
//     char str_buf[20];
//     img = lv_img_create(parent);
//     lv_obj_set_size(img, lv_pct(100), lv_pct(100));
//     lv_obj_add_flag(img, LV_OBJ_FLAG_CLICKABLE);
//     lv_obj_add_event_cb(img, img_callback, LV_EVENT_SHORT_CLICKED, NULL);
//     read_bin_files("D:/images");
//     print_bin_filenames();
//     sprintf(str_buf, "D:/images/%s", bin_filenames[0]);
//     lv_img_set_src(img, str_buf);
// }
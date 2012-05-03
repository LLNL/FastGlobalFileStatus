
typedef struct _matched_entry_t {
  char * mount_point_path;
  int score;
} matched_entry_t;

extern "C" int g_fs_status_initialize();

extern "C" int  g_fs_status_provide_best_fs(unsigned long bytes, 
					unsigned long bytes_to_free,
					matched_entry_t ** matched_array,
					int *array_size);

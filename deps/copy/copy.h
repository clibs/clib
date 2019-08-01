#ifndef COPY_DIR_H
#define COPY_DIR_H


/**
 *  Copies one file
 *
 *  @example copy_file("./dir/file.txt", "./target_dir/file.txt");
 *           "./target_dir/file.txt" will be created if it doesn't exist
 *
 *  @return 0 on success, -1 otherwise
 */
int copy_file(char *from_path, char *to_path);

/**
 * Copies directory recursively to the given target
 *
 * @example copy_dir("./dir/sub_dir", "./target_dir/sub_dir")
 *          "target_dir/sub_dir" will be created if it doesn't exist
 *
 * @return 0 on success, -1 otherwise
 */
int copy_dir(char *dir, char *target_dir);


#endif

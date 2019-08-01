#include <stdio.h>
#include "fs/fs.h"
#include "tinydir/tinydir.h"
#include "copy.h"


#define is_dot_file(file) 0 == strcmp(".", file.name) || 0 == strcmp("..", file.name)
#define check_err(x) if (0 != (err = x)) break;


int copy_file(char *from, char *to)
{
    char *content = fs_read(from);
    if (!content) {
        return -1;
    }
    fs_write(to, content);
    free(content);

    return 0;
}

static void check_dir(char *dir)
{
    if (0 != fs_exists(dir)) {
        fs_mkdir(dir, 0700);
    }
}

static int copy(tinydir_file file, char *target_dir)
{
    char target_path[strlen(target_dir) + strlen(file.name) + 2];

    sprintf(target_path, "%s/%s", target_dir, file.name);

    if (file.is_dir) {
        return copy_dir(file.path, target_path);
    }

    return copy_file(file.path, target_path);
}

int copy_dir(char *dir_path, char *target_dir)
{
    int err = 0;
    tinydir_dir dir;
    tinydir_file file;
    tinydir_open(&dir, dir_path);
    check_dir(target_dir);

    while (dir.has_next) {
        check_err(tinydir_readfile(&dir, &file));

        if (is_dot_file(file)) {
            goto next;
        }
        check_err(copy(file, target_dir));

        next:
        check_err(tinydir_next(&dir));
    }
    tinydir_close(&dir);

    return err;
}

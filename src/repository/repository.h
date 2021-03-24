//
// repository.h
//
// Copyright (c) 2021 Elbert van de Put
// MIT licensed
//
#ifndef CLIB_SRC_REPOSITORY_REPOSITORY_H
#define CLIB_SRC_REPOSITORY_REPOSITORY_H

#include <clib-secrets.h>
#include <http-get/http-get.h>

typedef struct repository_file_t* repository_file_handle_t;

/**
 * Initialize with secrets to enable authentication.
 */
void repository_init(clib_secrets_t secrets);

/**
 * Start download of package manifest for the package with url.
 * The file will be stored at destination_path.
 * This function starts a thread to dowload the file, the thread can be joined with `repository_file_finish_download`.
 *
 * @return a handle on success and NULL on failure.
 */
http_get_response_t* repository_fetch_package_manifest(const char*package_url, const char* slug, const char* version);

/**
 * Start download of a file for the package with url.
 * The file will be stored at destination_path.
 * This function starts a thread to dowload the file, the thread can be joined with `repository_file_finish_download`.
 *
 * @return a handle on success and NULL on failure.
 */
repository_file_handle_t repository_download_package_file(const char*package_url, const char* slug, const char* version, const char *file_path, const char* destination_path);

/**
 * Waits until the download is finished.
 * @param file
 */
void repository_file_finish_download(repository_file_handle_t file);

/**
 * Free the memory held by the file.
 * @param file
 */
void repository_file_free(repository_file_handle_t file);

#endif//CLIB_SRC_REPOSITORY_REPOSITORY_H

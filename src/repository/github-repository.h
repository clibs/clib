//
// github-repository.h
//
// Copyright (c) 2021 Elbert van de Put
// MIT licensed
//
#ifndef CLIB_SRC_REPOSITORY_GITHUB_REPOSITORY_H
#define CLIB_SRC_REPOSITORY_GITHUB_REPOSITORY_H

char* github_repository_get_url_for_file(const char* hostname, const char*package_id, const char* version, const char *file, const char* secret);

#endif//CLIB_SRC_REPOSITORY_GITHUB_REPOSITORY_H

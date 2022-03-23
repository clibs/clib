//
// gitlab-repository.h
//
// Copyright (c) 2021 Elbert van de Put
// MIT licensed
//
#ifndef CLIB_SRC_REPOSITORY_GITLAB_REPOSITORY_H
#define CLIB_SRC_REPOSITORY_GITLAB_REPOSITORY_H

char* gitlab_repository_get_url_for_file(const char*package_url, const char* slug, const char* version, const char *file, const char* secret);

#endif//CLIB_SRC_REPOSITORY_GITLAB_REPOSITORY_H

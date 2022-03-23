//
// clib-package-installer.h
//
// Copyright (c) 2021 Clib authors
// MIT licensed
//
#include "clib-package.h"
#include <registry-manager.h>

#ifndef CLIB_SRC_COMMON_CLIB_PACKAGE_INSTALLER_H
#define CLIB_SRC_COMMON_CLIB_PACKAGE_INSTALLER_H

void clib_package_installer_init(registries_t registries, clib_secrets_t secrets);

int clib_package_install(clib_package_t *pkg, const char *dir, int verbose);

int clib_package_install_executable(clib_package_t *pkg, const char *dir, int verbose);

int clib_package_install_dependencies(clib_package_t *pkg, const char *dir, int verbose);

int clib_package_install_development(clib_package_t *pkg, const char *dir, int verbose);

#endif

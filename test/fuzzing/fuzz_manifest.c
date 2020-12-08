#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "../../src/common/clib-package.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if(size<3){
            return 0;
    }
    char filename[256];
    sprintf(filename, "libfuzzer.json");

    FILE *fp = fopen(filename, "wb");
    if (!fp)
            return 0;
    fwrite(data, size, 1, fp);
    fclose(fp);
    
    clib_package_t *pkg = 
	    clib_package_load_from_manifest(filename, 0);
    if(pkg) {
	    clib_package_free(pkg);
    }
    unlink(filename);
    return 0;
}

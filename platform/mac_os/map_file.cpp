extern "C" {
#include "../platform.h"
}

#include "../error.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

void *map_file_to_memory(const char *name, size_t *sz) {
    if (!name || !sz) {
        return NULL;
    }

    int fd = open(name, O_RDONLY);
    if (fd == -1) {
        display_error("Failed to open file: %s %s", name, strerror(errno));
        return NULL;
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        display_error("Failed to fstat file: %s %s", name, strerror(errno));
        close(fd);
        return NULL;
    }
    *sz = static_cast<size_t>(st.st_size);

    void *mapped = mmap(NULL, *sz, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped == MAP_FAILED) {
        display_error("Failed to map file: %s %s", name, strerror(errno));
        close(fd);
        return NULL;
    }

    close(fd);
    return mapped;
}

void unmap_file(void *ptr, size_t sz) {
    if (!ptr || sz == 0) {
        return;
    }

    if (munmap(ptr, sz) == -1) {
        display_error("Failed to unmap file: %s", strerror(errno));
    }
}

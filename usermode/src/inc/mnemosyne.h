#ifndef _MNEMOSYNE_H

# define MNEMOSYNE_PERSISTENT __attribute__ ((section("PERSISTENT")))
# define MNEMOSYNE_ATOMIC __tm_atomic

# ifdef __cplusplus
extern "C" {
# endif

#include <sys/types.h>

void *mnemosyne_segment_create(void *start, size_t length, int prot, int flags);
int mnemosyne_segment_destroy(void *start, size_t length);
void *mnemosyne_malloc(size_t bytes);
void mnemosyne_free(void *ptr);

# ifdef __cplusplus
}
# endif

#endif /* _MNEMOSYNE_H */

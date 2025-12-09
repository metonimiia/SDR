#ifndef SYMBOL_SYNC_H
#define SYMBOL_SYNC_H

#include <stdint.h>

typedef struct {
    int *sym_indices;
    int count;
} SymbolSyncResult;

SymbolSyncResult symbol_sync(
    const float *I,
    const float *Q,
    int length,
    int Nsps
);

#endif

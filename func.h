#ifndef FUNC_H
#define FUNC_H

int* to_bpsk(int *array, int length);
int* upsampling(int *IQ, int length, int sample);
int* convolve_filter(int array[], int mask[], int len, int len_mask);

#endif

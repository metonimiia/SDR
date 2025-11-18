#include "func.h"
#include <stdlib.h>

int* to_bpsk(int *array, int length){
    int *IQ = (int*)malloc(length * 2 * sizeof(int));
    for(int i = 0; i < length; i++){
        IQ[2*i]   = (array[i] == 0) ? 1 : -1;
        IQ[2*i+1] = 0;
    }
    return IQ;
}

int* upsampling(int *IQ, int length, int sample) {
    int symbols = length / 2;
    int up_len = symbols * sample;
    int *tx_buff = (int*)malloc(up_len * sizeof(int));

    int k = 0;
    for (int i = 0; i < length; i += 2) {
        tx_buff[k++] = IQ[i];
        for(int j = 0; j < sample - 1; j++)
            tx_buff[k++] = 0;
    }
    return tx_buff;
}

int* convolve_filter(int array[], int mask[], int len, int len_mask) {
    int output_len = len - len_mask + 1;
    int *output = (int*)malloc(output_len * sizeof(int));

    for (int i = 0; i < output_len; i++) {
        int s = 0;
        for (int j = 0; j < len_mask; j++)
            s += array[i + j] * mask[j];
        output[i] = s;
    }
    return output;
}

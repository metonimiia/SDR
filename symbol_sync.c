#include "symbol_sync.h"
#include <stdlib.h>
#include <math.h>
#include <stdio.h>


#define BnTs 0.01

SymbolSyncResult symbol_sync(
    const float *I, const float *Q, int len, int Nsps)
{
    SymbolSyncResult result;
    result.sym_indices = malloc(sizeof(int) * (len / Nsps + 10));
    result.count = 0;

    double K1 = (4 * BnTs) / (1 + 2 * BnTs + pow(BnTs, 2));
    double K2 = (4 * pow(BnTs, 2)) / (1 + 2 * BnTs + pow(BnTs, 2));

    double p1 = 0.0;     // Пропорциональная ветвь
    double p2 = 0.0;     // Интегральная ветвь (дробная задержка)

    int offset = 0;      
    int n = offset;

    while (n + 2 * Nsps < len)
    {
      
        float eI = (I[n + Nsps + Nsps] - I[n + Nsps]) * I[n + Nsps / 2];
        float eQ = (Q[n + Nsps + Nsps] - Q[n + Nsps]) * Q[n + Nsps / 2];
        float e = eI + eQ;

        p1 = e * K1;
        p2 = p2 + p1 + e * K2;

        
        while (p2 > 1.0) p2 -= 1.0;
        while (p2 < 0.0) p2 += 1.0;

       
        offset = (int)round(p2 * Nsps);

        int sym_index = n + offset;
        if (sym_index < len)
        {
            result.sym_indices[result.count++] = sym_index;
        }

       
        n += Nsps;
    }

    return result;
}

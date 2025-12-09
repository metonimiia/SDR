import numpy as np
import matplotlib.pyplot as plt
import sys

def gardner_ted(I, Q, n, Nsps, shift):
    r0 = I[n + shift]
    r1 = I[n + shift + Nsps//2]
    r2 = I[n + shift + Nsps]
    
    i0 = Q[n + shift]
    i1 = Q[n + shift + Nsps//2]
    i2 = Q[n + shift + Nsps]
    
    e = (r2 - r0) * r1 + (i2 - i0) * i1
    return e

def main():
    if len(sys.argv) != 2:
        print(f"malo argv")
        return
    
    filename = sys.argv[1]
    
    rx = np.fromfile(filename, dtype=np.int16)
    real = rx[0::2]  # I
    imag = rx[1::2]  # Q
    
    start = 5000
    end = 20000
    if len(real) > end:
        real = real[start:end]
        imag = imag[start:end]
    
    p1, p2 = 0, 0
    BnTs = 0.01
    Nsps = 10
    Kp = 0.02 #0.002
    zeta = np.sqrt(2) / 2
    
    theta = (BnTs / Nsps) / (zeta + 1/(4*zeta))
    K1 = (-4*zeta*theta) / ((1 + 2*zeta*theta + theta**2) * Kp)
    K2 = (-4*theta**2) / ((1 + 2*zeta*theta + theta**2) * Kp)
    
    print(f"Параметры:")
    print(f"K1 = {K1:.6f}")
    print(f"K2 = {K2:.6f}")
    print(f"theta = {theta:.6f}")
    print(f"p1 = {p1}, p2 = {p2}")
    print(f"BnTs = {BnTs}, Nsps = {Nsps}, Kp = {Kp}, zeta = {zeta:.6f}\n")
    
   
    h = np.ones(Nsps)
    conv_real = np.convolve(real, h, mode='same')
    conv_imag = np.convolve(imag, h, mode='same')
    
   
    conv_real = conv_real / np.max(np.abs(conv_real))
    conv_imag = conv_imag / np.max(np.abs(conv_imag))
    
    
    print("Анализ зависимости TED от offset:")
    offsets = np.arange(-Nsps//2, Nsps//2 + 1)
    ted_values = []
    
    n = Nsps * 10 
    
    for offset in offsets:
        e = gardner_ted(conv_real, conv_imag, n, Nsps, offset)
        ted_values.append(e)
        print(f"  offset={offset:3d}, e={e:8.8f}")
    
    min_index = np.argmin(np.abs(ted_values))
    min_offset = offsets[min_index]
    min_ted = ted_values[min_index]
    
    print(f"\nМинимальный TED при offset = {min_offset}, e = {min_ted:.8f}")
    print("\nРабота контура синхронизации с NCC:")
    shift = 0
    e_values = []
    r = [0]  # NCC
    ncc_indices = []
    r_values = [] 
    num_symbols = min(100, (len(conv_real) - 3*Nsps) // Nsps)
    
    shift = 0  # начальное смещение
    r = [0]    # NCC счетчик
    ncc_indices = []
    e_values = []

    for k in range(num_symbols):
        n = k * Nsps

        e = gardner_ted(conv_real, conv_imag, n, Nsps, shift)

        # Фильтр контура
        p1 = e * K1
        p2 = p2 + p1 + e * K2

        # Ограничение p2
        while p2 > 1:
            p2 -= 2
        while p2 < -1:
            p2 += 1

        # Коррекция смещения в целых отсчетах
        delta_shift = int(round(p2 * Nsps))
        shift += delta_shift

        # NCC счетчик
        r_new = (r[-1] + 1 + delta_shift) % Nsps
        if r_new < r[-1]:
            ncc_indices.append(n + r_new)  # команда на взятие отсчета
        r.append(r_new)

        e_values.append(e)

        print(f"Символ {k}: e={e:.9f}, shift={shift:.12f}, r={r_new:.2f}, NCC взятие={r_new < r[-1]}")

    

    print("\nNCC команды на выборку (символы и r):")
    for idx, r_val in zip(ncc_indices, r_values):
        print(f"  Символ idx={idx}, r={r_val:.2f}")

    print("\nИндексы выборки по NCC:", ncc_indices)
    
    # График S-кривой
    plt.figure(figsize=(8, 6))
    plt.plot(offsets, ted_values, 'bo-', linewidth=2, markersize=8)
    plt.plot(min_offset, min_ted, 'ro', markersize=8, label=f'Min TED (offset={min_offset})')
    plt.axhline(y=0, color='k', linestyle='-', alpha=0.3)
    plt.axvline(x=0, color='k', linestyle='-', alpha=0.3)
    plt.grid(True, alpha=0.3)
    plt.xlabel('Offset (смещение в отсчетах)')
    plt.ylabel('TED output (e)')
    plt.title('S-кривая детектора временной ошибки Гарднера')
    plt.legend()
    plt.show()
    
    # График ошибки во времени
    if e_values:
        plt.figure(figsize=(8, 4))
        plt.plot(range(len(e_values)), e_values, 'g-', linewidth=2, marker='o')
        plt.axhline(y=0, color='k', linestyle='-', alpha=0.3)
        plt.grid(True, alpha=0.3)
        plt.xlabel('Номер символа')
        plt.ylabel('Ошибка TED')
        plt.title('Ошибка TED во времени')
        plt.show()

if __name__ == '__main__':
    main()

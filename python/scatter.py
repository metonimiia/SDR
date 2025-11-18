import numpy as np
import math
import matplotlib.pyplot as plt

# Открываем файл для чтения
name = "/home/plutoSDR/dev/txdata.pcm"

data = []
imag = []
real = []
count = []
counter = 0
absIQ = []
with open(name, "rb") as f:
    index = 2
    while (byte := f.read(2)):
        I = 0
        Q = 0
        if(index %2 == 0):
            Q = int.from_bytes(byte, byteorder='little', signed=True)
            real.append(Q)
            counter += 1
            count.append(counter)
        else:
            I = int.from_bytes(byte, byteorder='little', signed=True)
            imag.append(I)
        
        index += 1
    for i in range(len(imag)):
        abs = math.sqrt(imag[i]**2 + real[i]**2)
        absIQ.append(abs)

# График I и Q компонент во времени
plt.figure(1)
plt.plot(count, imag, color='red', label='I (In-phase)', alpha=0.7)
plt.plot(count, real, color='blue', label='Q (Quadrature)', alpha=0.7)
plt.xlabel('Отсчеты')
plt.ylabel('Амплитуда')
plt.title('I и Q компоненты сигнала')
plt.legend()
plt.grid(True, alpha=0.3)
plt.show()

# График амплитуды сигнала
plt.figure(2)
plt.plot(count, absIQ, color='purple', label='Амплитуда |IQ|')
plt.xlabel('Отсчеты')
plt.ylabel('Амплитуда')
plt.title('Амплитуда сигнала')
plt.legend()
plt.grid(True, alpha=0.3)
plt.show()

# Диаграмма созвездия
plt.figure(3)
plt.scatter(real, imag, color='green', alpha=0.6, s=10)
plt.xlabel('Q компонента')
plt.ylabel('I компонента')
plt.title('Диаграмма созвездия (Constellation Diagram)')
plt.grid(True, alpha=0.3)
plt.axhline(y=0, color='k', linestyle='-', alpha=0.3)
plt.axvline(x=0, color='k', linestyle='-', alpha=0.3)
plt.axis('equal')

# Обрезаем по оси Y - задаем фиксированные пределы
plt.ylim(-1500, 1500)  # Замените значения на нужные вам
plt.show()

name2 = np.convolve(real,np.ones(10))
plt.figure(4)
plt.plot(name2)
plt.show()






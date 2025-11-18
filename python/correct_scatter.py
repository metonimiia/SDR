import numpy as np
import matplotlib.pyplot as plt

# Путь к файлу
name = "/home/plutoSDR/dev/txdata.pcm"

real = []
imag = []

with open(name, "rb") as f:
    index = 0
    temp_real = []
    temp_imag = []
    while (byte := f.read(2)):
        value = int.from_bytes(byte, byteorder="little", signed=True)
        if index % 2 == 0:  # Чередуем I Q
            temp_real.append(value)
        else:
            temp_imag.append(value)
        index += 1

# Берём только пары с обоими компонентами
length = min(len(temp_real), len(temp_imag))
for i in range(length):
    if abs(temp_real[i]) > 100 and abs(temp_imag[i]) > 100:
        real.append(temp_real[i])
        imag.append(temp_imag[i])

# Проверка, есть ли точки для построения
if real and imag:
    plt.figure(figsize=(6, 6))
    plt.scatter(real, imag, s=10, alpha=0.6)
    plt.xlabel("I")
    plt.ylabel("Q")
    plt.title("IQ Scatter Plot")
    plt.grid(True)
    plt.show()
else:
    print("Нет точек для отображения графика")

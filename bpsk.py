import numpy as np
import matplotlib.pyplot as plt

count_bits = 10 #количество бит
bits = np.random.randint(0,2,count_bits) #задаем 0 и 1
print("Биты: ",bits)

bits_am = np.where(bits == 0,1,-1) #если 1 то становится -1, если 0 то становится 1
print("Биты после мапера: ",bits_am)

plt.figure(figsize=(8,6))
plt.subplot(2,1,1)
plt.step(range(count_bits),bits)
plt.title("Изначальная генерация бит")
plt.grid()
plt.subplot(2,1,2)
plt.step(range(count_bits),bits_am)
plt.title("Биты после маппера")
plt.grid()
plt.show()

plt.figure()
plt.scatter(bits_am,np.zeros_like(bits_am))
plt.title("Созвездие BPSK")
plt.xlabel("I")
plt.ylabel("Q")
plt.grid()
plt.show()

dt = 0.001 
f = 2 #частота несущей
T_bit = 1 #длительность бита

t = np.arange(0, T_bit, dt)
signal_cos = np.cos(2*np.pi*f*t)
plt.figure()
plt.plot(t, signal_cos)
plt.title("Сигнал несущей")
plt.xlabel("Время")
plt.ylabel("Амплитуда")
plt.grid()
plt.show()

t_total = np.arange(0, count_bits*T_bit, dt)
signal = np.zeros_like(t_total)

# Формирование BPSK-сигнала
for i, b in enumerate(bits):
    t = np.arange(i*T_bit, (i+1)*T_bit, dt)
    phase = 0 if b == 0 else np.pi
    signal[i*len(t):(i+1)*len(t)] = np.cos(2*np.pi*f*t + phase)

plt.figure(figsize=(10,4))
plt.plot(t_total, signal)
plt.title("BPSK сигнал")
plt.xlabel("Время")
plt.ylabel("Амплитуда")
plt.grid()
plt.show()




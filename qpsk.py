import numpy as np
import matplotlib.pyplot as plt

count_bits = 10 
bits = np.random.randint(0, 2, count_bits)
print("Биты:", bits)

symbols = bits.reshape(-1, 2)
print("Символы (по 2 бита):")
print(symbols)

I = np.where(symbols[:, 0] == 0, 1, -1)
Q = np.where(symbols[:, 1] == 0, 1, -1)
print("I-компоненты:", I)
print("Q-компоненты:", Q)

plt.figure(figsize=(10,6))
plt.subplot(3,1,1)
plt.step(range(count_bits), bits)
plt.title("Исходные биты")
plt.grid(True)

plt.subplot(3,1,2)
plt.step(range(len(I)), I)
plt.title("I компонента")
plt.grid()

plt.subplot(3,1,3)
plt.step(range(len(Q)), Q)
plt.title("Q компонента")
plt.grid()
plt.tight_layout()
plt.show()

#Созвездие QPSK
plt.figure()
plt.scatter(I, Q)
plt.title("Созвездие QPSK")
plt.xlabel("I")
plt.ylabel("Q")
plt.grid()
plt.axis('equal')
plt.show()

dt = 0.001
f = 2
T_sym = 1
t = np.arange(0, T_sym, dt)

component_I = np.cos(2 * np.pi * f * t)
component_Q = np.sin(2 * np.pi * f * t)

plt.figure(figsize=(10,4))
plt.plot(t, component_I, label='I = cos(2πft)')
plt.plot(t, component_Q, label='Q = sin(2πft)')
plt.title("Несущие для QPSK (I и Q)")
plt.xlabel("Время")
plt.ylabel("Амплитуда")
plt.legend()
plt.grid(True)
plt.show()

#Формирование QPSK сигнала
t_total = np.arange(0, len(symbols)*T_sym, dt)
signal = np.zeros_like(t_total)
I_sig = np.zeros_like(t_total)
Q_sig = np.zeros_like(t_total)

for i in range(len(symbols)):
    t_sym = np.arange(i*T_sym, (i+1)*T_sym, dt)
    I_part = I[i] * np.cos(2*np.pi*f*t_sym)
    Q_part = Q[i] * np.sin(2*np.pi*f*t_sym)
    I_sig[i*len(t_sym):(i+1)*len(t_sym)] = I_part
    Q_sig[i*len(t_sym):(i+1)*len(t_sym)] = Q_part
    signal[i*len(t_sym):(i+1)*len(t_sym)] = I_part - Q_part  # итоговый сигнал

plt.figure(figsize=(10,8))
plt.subplot(3,1,1)
plt.plot(t_total, I_sig)
plt.title("I компонента")
plt.grid()

plt.subplot(3,1,2)
plt.plot(t_total, Q_sig)
plt.title("Q компонента")
plt.grid()

plt.subplot(3,1,3)
plt.plot(t_total, signal)
plt.title("QPSK сигнал ")
plt.xlabel("Время")
plt.ylabel("Амплитуда")
plt.show()



import numpy as np
import matplotlib.pyplot as plt
import math

name = "rxdata.pcm"

data = []
imag = []
real = []
count = []
absIQ = []
counter = 0
with open(name, "rb") as f:
    index = 0
    while (byte := f.read(2)):
        if(index % 2 == 0):
            Q = int.from_bytes(byte, byteorder='little', signed=True)
            real.append(Q)
            counter += 1
            count.append(counter)
        else:
            I = int.from_bytes(byte, byteorder='little', signed=True)
            imag.append(I)
        index += 1
for i in range(len(imag)):
    abs = math.sqrt(imag[i] * imag[i] + real[i] * real[i])
    absIQ.append(abs)


plt.figure(1)

plt.plot(count,(imag),'r-')
plt.plot(count,(real), 'b-')
plt.show()

plt.figure(2)
plt.plot(count,(absIQ), 'g-')
plt.show()  

filter = np.ones(10)

print(filter)

name2 = np.convolve(real, filter)

output_filename = "real_part    _filtered.pcm"
with open(output_filename, "wb") as f:
    for value in name2:
        int_value = int(value)
        int_value = max(min(int_value, 32767), -32768)
        f.write(int_value.to_bytes(2, byteorder='little', signed=True))

print(f"Filtered real part saved to {output_filename}")

plt.figure(3)
plt.plot(name2)
plt.show()  

threshold = 100  
signal_present = np.abs(real) > threshold

if np.any(signal_present):
    start_idx = np.argmax(signal_present)
    end_idx = len(real) - np.argmax(signal_present[::-1])
    
    real_data = real[2165:3121] #start 2160
    imag_data = imag[start_idx:end_idx]
    
    print(f"Data range: {start_idx} to {end_idx} (total: {end_idx - start_idx} samples)")
else:
    real_data = real
    imag_data = imag
    print("No significant signal found, showing all data")

step = 10
real_10 = real_data[::step]

plt.figure(figsize=(8, 8))
plt.scatter(real_10, real_10, alpha=0.5, s=5)
plt.title(f'Constellation (every {step}th point)')
plt.show()

sync_name = "/home/plutoSDR/dev/python/real_part_filtered.pcm"
sync_symbols = []

with open(sync_name, "rb") as f:
    while (byte := f.read(2)):
        value = int.from_bytes(byte, byteorder='little', signed=True)
        sync_symbols.append(value)

print(f"Read {len(sync_symbols)} synchronized symbols")

plt.figure(figsize=(12, 5))
plt.subplot(1, 2, 1)
plt.plot(sync_symbols, 'bo-', markersize=3, alpha=0.7)
plt.title('Synchronized Symbols (Time Domain)')
plt.xlabel('Symbol Index')
plt.ylabel('Amplitude')
plt.grid(True)

plt.subplot(1, 2, 2)
plt.scatter(sync_symbols, np.zeros_like(sync_symbols), alpha=0.7, s=30)
plt.title('Synchronized Constellation')
plt.xlabel('In-phase')
plt.ylabel('Quadrature')
plt.grid(True)
plt.axvline(x=0, color='r', linestyle='--', alpha=0.5)

plt.tight_layout()
plt.show()
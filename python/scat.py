import numpy as np
import matplotlib.pyplot as plt

name = "../symb_after_rx.pcm"

I = []
Q = []


with open(name, "rb") as f:
    while True:

        i_bytes = f.read(2)
        q_bytes = f.read(2)
        if not i_bytes or not q_bytes:
            break
            
        I.append(int.from_bytes(i_bytes, byteorder='little', signed=True))
        Q.append(int.from_bytes(q_bytes, byteorder='little', signed=True))

plt.figure(figsize=(8, 8))
plt.scatter(I, Q, alpha=0.5, s=1)
plt.axhline(y=0, color='k', linestyle='-')
plt.axvline(x=0, color='k', linestyle='-')
plt.grid(True)
plt.xlabel('I')
plt.ylabel('Q')
plt.title('Constellation Diagram')
plt.show()
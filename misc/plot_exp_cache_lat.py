#!/bin/env python

import matplotlib.pyplot as plt
import matplotlib.ticker as mticker

# Read data from file
x: list[int] = []
y: list[float] = []
with open("data.csv", "r") as file:
    for line in file:
        if line.strip():
            x_val, y_val = line.strip().split(",")
            x.append(int(x_val))
            y.append(float(y_val))

y_lbl = [2**x for x in range(0, 10)]

# Plot
plt.figure(figsize=(12, 6))
plt.plot(x, y, marker="o")

plt.xscale("log", base=2)
plt.yscale("log", base=2)

plt.ylim(0, 512)

plt.yticks(y_lbl)
plt.ylim(-0.1, 513)
plt.gca().yaxis.set_major_formatter(mticker.ScalarFormatter())

plt.title("Memory Access Latency")
plt.xlabel("Latency [Cycles]")
plt.ylabel("Accessed Bytes")
plt.grid(True)
plt.show()

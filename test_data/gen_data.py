import math
import random

def const(x):
    return x


for i in range(200):
    x = [math.sin(t + random.random()) for t in range(20000)]
    with open(f"sine_{i:03}.txt", 'w') as f:
        for item in x:
            f.write("%f\n" % item)
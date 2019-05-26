import math
import random
import numpy

NUM_FILES=200
NUM_POINTS=20000

def const(x):
    return x

for i in range(NUM_FILES):
    # x = [math.sin(t + random.random()) for t in range(20000)]
    # x = [2 * (random.random() - 1) for t in range(NUM_POINTS)]
    x = numpy.random.normal(size=NUM_POINTS)
    y = [x[t] + 0.015 * math.sin(2 * math.pi * t * 100 / NUM_POINTS) for t in range(NUM_POINTS)]
    with open(f"sine_{i:03}.txt", 'w') as f:
        for item in y:
            f.write("%2.15f\n" % item)
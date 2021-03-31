# Semestral work from NI-PDD

* [courses](https://courses.fit.cvut.cz/NI-PDP/)

## Performace

## Summary
| File  | Cost | Ref seq [s] | HW1 seq [s] | HW1 task [s] | Ref seq [call] | HW1 seq [call] | HW1 task [call] |
|-------|------|-------------|-------------|--------------|----------------|----------------|-----------------|
| saj1  | 5    | 0           | 0           | 0            | 750            | 10             | 1e+3            |
| saj2  | 14   | 0.6         | 1.9         | 0.6          | 14e+6          | 2e+6           | 14e+6           |
| saj3  | 12   | 0.01        | 0           | 0            | 300e+3         | 12e+3          | 1e+5            |
| saj4  | 14   | 0.4         | 1           | 0.3          | 8e+6           | 1e+6           | 7e+6            |
| saj5  | 12   | 12          | 28.2        | 0.5          | 193e+6         | 23e+6          | 12e+6           |
| saj6  | 16   | 31          | 529.1       | 1.3          | 502e+6         | 565e+6         | 24+e6           |
| saj7  | 16   | 67          | 141         | 0            | 1.2e+9         | 133e+6         | 1e+6            |
| saj8  | 20   | 1170        | 22750.8     | 323.8        | 19e+9          | 27e+9          | 8e+9            |
| saj9  | 26   | 6.3         | 0.9         | 1.7          | 89e+6          | 6e+5           | 42e+6           |
| saj10 | 20   | 36          | 61.9        | 21.6         | 460e+6         | 45e+6          | 462e+6          |
| saj11 | 18   | 0.2         | 0.6         | 0.5          | 3e+6           | 4e+5           | 11e+6           |
| saj12 | 22   | 72          | 249.2       | 109.2        | 1.2e+9         | 243e+6         | 2e+9            |

## Task parallelism – threshold

* hardware used was HW1
* measurements are in seconds
* T is depth threshold, after which only sequential solving is used


| File  | T=2 | T=3  | T=4 | T=5  | 🔥 T=6  | T=99999 |
|-------|--------------|--------------|--------------|--------------|--------------|------------------|
| saj1  | 0            | 0            | 0            | 0            | 0            | 0                |
| saj2  | 0.8          | 0.5          | 0.5          | 0.6          | 0.5          | 0.7              |
| saj3  | 0            | 0            | 0            | 0            | 0            | 0                |
| saj4  | 0.3          | 0.3          | 0.3          | 0.3          | 0.3          | 0.3              |
| saj5  | 0            | 0.1          | 0            | 0            | 0            | 0                |
| saj6  | 1            | 1            | 0.9          | 0.9          | 0.9          | 1.3              |
| saj7  | 0            | 0.5          | 0            | 0.1          | 0            | 0                |
| saj8  | 37           | 125.8        | 32.8         | 31.3         | 30.8         | 40               |
| saj9  | 0.2          | 0.2          | 0.4          | 2.3          | 0.5          | 1.3              |
| saj10 | 23.2         | 20.3         | 17.6         | 17.5         | 17.2         | 21.6             |
| saj11 | 0.2          | 0.3          | 0.7          | 0.2          | 0.3          | 0.4              |
| saj12 | 102.7        | 129.5        | 200          | 151.5        | 139.9        | 220              |


## Data parallelism – scheduling

* hardware used was HW1
* measurements are in seconds
* E is number of epochs in BFS

| File  | dynamic, E=2 | dynamic(10), E=2 | 🔥 dynamic, E=3 | dynamic(10), E=3 | dynamic, E=4 | dynamic(10), E=4 | auto, E=3 | auto, E=4 | guided, E=3 | guided, E=4 |
|-------|--------------|------------------|--------------|------------------|--------------|------------------|-----------|-----------|-------------|-------------|
| saj8  | 50.2         | 105.1            | 37.8         | 56.1             | 7475.6       | 37.8             | 69.2      | 138       | 70.4        | 111.2       |
| saj10 | 32           | 48.3             | 25           | 30.5             | 19.4         | 27.6             | 42        | 41        | 41.9        | 45.1        |
| saj12 | 211.7        | 313.6            | 110.6        | 100.9            | 106.2        | 206.2            | 218.7     | 210.3     | 226         | 224.5       |

### Hardware

#### HW1
*  AMD Ryzen 7 PRO 4750U, 8 physical 16 virtual cores,1397.271 MHz
* compile with ``g++ (GCC) 10.2.1 20201125 (Red Hat 10.2.1-9)``
	* seq ``g++ --std=c++11 -O3 -funroll-loops``
	* task: ``g++ -Wall --std=c++11 -O3 -funroll-loops -fopenmp `` 



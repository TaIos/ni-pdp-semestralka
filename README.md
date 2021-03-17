# Semestral work from NI-PDD

* [courses](https://courses.fit.cvut.cz/NI-PDP/)

## Performace

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

### Hardware

#### HW1
*  AMD Ryzen 7 PRO 4750U, 8 physical 16 virtual cores,1397.271 MHz
* compile with ``g++ (GCC) 10.2.1 20201125 (Red Hat 10.2.1-9)``
	* seq ``g++ --std=c++11 -O3 -funroll-loops``
	* task: ``g++ -Wall --std=c++11 -O3 -funroll-loops -fopenmp `` 



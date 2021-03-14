# Semestrálka z NI-PDD

* [courses](https://courses.fit.cvut.cz/NI-PDP/)

## Měření výkonnosti

### Sekvenční řešení [notebook]

| Soubor    | Čas [s] | Cena | Počet volání   |
|-----------|---------|------|----------------|
| saj1.txt  | 0       | 5    | 10             |
| saj2.txt  | 1.9     | 14   | 2 690 561      |
| saj3.txt  | 0       | 12   | 12 822         |
| saj4.txt  | 1       | 14   | 1 550 605      |
| saj5.txt  | 28.2    | 12   | 23 176 549     |
| saj6.txt  | 529.1   | 16   | 565 523 051    |
| saj7.txt  | 141     | 16   | 133 923 075    |
| saj8.txt  | 22750.8 | 20   | 27 443 340 794 |
| saj9.tx   | 0.9     | 26   | 654 089        |
| saj10.txt | 61.9    | 20   | 45 402 144     |
| saj11.txt | 0.6     | 18   | 473 844        |
| saj12.txt | 249.2   | 22   | 243 181 271    |

* procesor AMD Ryzen 7 PRO 4750U, 8 physical 16 virtual cores,1397.271 MHz
* kompilováno ``g++ -Wall -pedantic -Wextra --std=c++11 -O3 -funroll-loops -o run.out``

### Sekvenční řešení [STAR]

### Taskový paralelismus [notebook]

| Soubor    | Čas [s] | Cena | Počet volání  |
|-----------|---------|------|---------------|
| saj1.txt  | 0       | 5    | 1 298         |
| saj2.txt  | 0.6     | 14   | 14 296 421    |
| saj3.txt  | 0       | 12   | 99 645        |
| saj4.txt  | 0.3     | 14   | 7 933 139     |
| saj5.txt  | 0.5     | 12   | 12 354 598    |
| saj6.txt  | 1.3     | 16   | 24 878 740    |
| saj7.txt  | 0       | 16   | 1 120 215     |
| saj8.txt  | 323.8   | 20   | 7 992 754 319 |
| saj9.tx   | 1.7     | 26   | 42 199 937    |
| saj10.txt | 21.6    | 20   | 462 731 436   |
| saj11.txt | 0.5     | 18   | 11 517 097    |
| saj12.txt | 109.2   | 22   | 2 599 289 591 |

* procesor AMD Ryzen 7 PRO 4750U, 8 physical 16 virtual cores,1397.271 MHz
* kompilováno ``g++ -Wall -pedantic -Wextra --std=c++11 -O3 -funroll-loops -fopenmp -o run.out`` 

### Taskový paralelismus [STAR]

| Soubor    | Čas [s] | Cena | Počet volání |
|-----------|---------|------|--------------|
| saj1.txt  |         |      |              |
| saj2.txt  |         |      |              |
| saj3.txt  |         |      |              |
| saj4.txt  |         |      |              |
| saj5.txt  |         |      |              |
| saj6.txt  |         |      |              |
| saj7.txt  |         |      |              |
| saj8.txt  |         |      |              |
| saj9.tx   |         |      |              |
| saj10.txt |         |      |              |
| saj11.txt |         |      |              |
| saj12.txt |         |      |              |

### Datový paralelismus [notebook]

| Soubor    | Čas [s] | Cena | Počet volání |
|-----------|---------|------|--------------|
| saj1.txt  |         |      |              |
| saj2.txt  |         |      |              |
| saj3.txt  |         |      |              |
| saj4.txt  |         |      |              |
| saj5.txt  |         |      |              |
| saj6.txt  |         |      |              |
| saj7.txt  |         |      |              |
| saj8.txt  |         |      |              |
| saj9.tx   |         |      |              |
| saj10.txt |         |      |              |
| saj11.txt |         |      |              |
| saj12.txt |         |      |              |

### Datový paralelismus [STAR]

| Soubor    | Čas [s] | Cena | Počet volání |
|-----------|---------|------|--------------|
| saj1.txt  |         |      |              |
| saj2.txt  |         |      |              |
| saj3.txt  |         |      |              |
| saj4.txt  |         |      |              |
| saj5.txt  |         |      |              |
| saj6.txt  |         |      |              |
| saj7.txt  |         |      |              |
| saj8.txt  |         |      |              |
| saj9.tx   |         |      |              |
| saj10.txt |         |      |              |
| saj11.txt |         |      |              |
| saj12.txt |         |      |              |

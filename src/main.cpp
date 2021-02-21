#include <iostream>
#include <cstring>

using namespace std;

class Grid {
private:
    char *data;
    int size;
    int row_len;
public:

    Grid(int k) {
        size = k * k;
        row_len = k;
        data = new char[size];
    };

    Grid(const Grid &oth) {
        data = new char[oth.size];
        memcpy(data, oth.data, oth.size);
        size = oth.size;
        row_len = oth.row_len;
    };

    ~Grid() {
        delete[] data;
    }

    friend ostream &operator<<(ostream &os, const Grid &g) {
        for (int i = 0; i < g.size; i++) {
            os << g.data[i];
            if (i % g.row_len) os << " | ";
            else os << endl;
        }
        return os << endl;
    }


};

int main() {
    std::cout << "Hello, World!" << std::endl;
    return 0;
}

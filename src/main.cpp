#include <iostream>
#include <cstring>
#include <fstream>

using namespace std;

class Grid {
private:
    char *data = nullptr;
    int size;
    int row_len;
    string filename;

    // PDD specific
    int k; // přirozené číslo, k>5, reprezentující délku strany šachovnice S o velikosti kxk
    int q; // přirozené číslo q<k^2/2 reprezentující počet rozmístěných figurek na šachovnici S

public:
    Grid(const string &filename) {
        ifstream ifs(filename);
        ifs >> k;
        ifs >> q;
        size = k * k;
        row_len = k;
        this->filename = filename;
        data = new char[size];
        memset(data, 'X', size);

        char c;
        char *head = data;
        while (ifs.get(c) && head < data + size) {
            if (c != '\n' && c != '\r') {
                *(head++) = c;
            }
        }
        ifs.close();
    };

    Grid(const Grid &oth) {
        data = new char[oth.size];
        memcpy(data, oth.data, oth.size);
        size = oth.size;
        row_len = oth.row_len;
    };

    ~Grid() {
        if (data) delete[] data;
    }

    friend ostream &operator<<(ostream &os, const Grid &g) {
        os << g.filename << endl;
        os << "k=" << g.k << ", q=" << g.q << endl;
        for (int i = 0; i < g.size; i++) {
            os << g.data[i];
            if ((i + 1) % g.row_len) os << " | ";
            else os << endl;
        }
        return os << endl;
    }


};

int main(int argc, char **argv) {

    for (int i = 1; i < argc; i++) {
        string filename = argv[i];
        cout << Grid(filename);
    }

    return 0;
}

#include <iostream>
#include <cstring>
#include <fstream>

#define MAX_DEPTH_REACHED  -1
#define HORSE  'J'
#define BISHOP 'S'
#define PAWN 'P'

using namespace std;

class Grid {
public:
    char *data = nullptr;
    int size;
    int row_len;
    string filename;

    // PDD specific
    int k; // přirozené číslo, k>5, reprezentující délku strany šachovnice S o velikosti kxk
    int max_depth; // přirozené číslo max_depth<k^2/2 reprezentující počet rozmístěných figurek na šachovnici S
    int hp; // horse position
    int bp; // bishop position
    int npwn; // number of pawns

    Grid(const string &filename) {

        ifstream ifs(filename);
        ifs >> k;
        ifs >> max_depth;
        size = k * k;
        row_len = k;
        this->filename = filename;
        data = new char[size];
        memset(data, 'X', size);

        npwn = 0;
        char c;
        char *head = data;
        while (ifs.get(c) && head < data + size) {
            if (c != '\n' && c != '\r') {
                if (c == BISHOP) bp = int(head - data);
                if (c == HORSE) hp = int(head - data);
                if (c == PAWN) npwn++;
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
        os << "k=" << g.k << ", max_depth=" << g.max_depth << endl;
        os << "Kůň na indexu " << g.hp << ", střelec na indexu " << g.bp << endl;
        os << "Počet pěšáků " << g.npwn << endl;
        for (int i = 0; i < g.size; i++) {
            os << g.data[i];
            if ((i + 1) % g.row_len) os << " | ";
            else os << endl;
        }
        return os << endl;
    }


};


struct Solution {
    double time;
    long cost;
    long ncalls;

    Solution(double time, long cost, long ncalls) : time(time), cost(cost), ncalls(ncalls) {}

};

Solution bb_dfs_base(Grid grid) {

    return Solution(0, 0, 0);
}

long bb_dfs(Grid g, long depth, char move) {
    if (depth > g.max_depth) return MAX_DEPTH_REACHED;
    if (move == HORSE) {

    } else if (move == BISHOP) {

    }
    return 0;
}

int main(int argc, char **argv) {

    for (int i = 1; i < argc; i++) {
        string filename = argv[i];
        cout << Grid(filename);
    }

    return 0;
}

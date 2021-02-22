#include <iostream>
#include <cstring>
#include <fstream>
#include <vector>
#include <algorithm>

#define MAX_DEPTH_REACHED  -1
#define BETTER_SOLUTION_EXISTS  -2
#define HORSE  'J'
#define BISHOP 'S'
#define PAWN 'P'
#define EMPTY 'X'

using namespace std;

class Grid {
public:
    char *data = nullptr;
    int size;
    int row_len;
    string filename;

    const static char INVALID_AT = '\0';

    // PDD specific
    int k; // přirozené číslo, k>5, reprezentující délku strany šachovnice S o velikosti kxk
    int max_depth; // přirozené číslo max_depth<k^2/2 reprezentující počet rozmístěných figurek na šachovnici S
    int hp; // for_horse position
    int bp; // for_bishop position
    int npwn; // number of pawns

    Grid(const string &filename) {

        ifstream ifs(filename);
        ifs >> k;
        ifs >> max_depth;
        size = k * k;
        row_len = k;
        this->filename = filename;
        data = new char[size];
        memset(data, EMPTY, size);

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

    char at(int row, int col) const {
        if (row < 0 || col < 0 || row * row_len + col >= size) return INVALID_AT;
        return data[row * row_len + col];
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

class Eval {
public:
    static int for_horse(const Grid &g, int idx) {
        if (g.data[idx] == PAWN) return 2;
        return 0;
    };

    static int for_bishop(const Grid &g, int row, int col) {
        if (g.at(row, col) == PAWN) return 2;
        char c;

        // DIAG DOWN RIGHT
        for (int i = 1, c = g.at(row + i, col + i); c != Grid::INVALID_AT; i++) { if (c == PAWN) return 1; }

        // DIAG DOWN LEFT
        for (int i = 1, c = g.at(row + i, col - i); c != Grid::INVALID_AT; i++) { if (c == PAWN) return 1; }

        // DIAG UP RIGHT
        for (int i = 1, c = g.at(row - i, col + i); c != Grid::INVALID_AT; i++) { if (c == PAWN) return 1; }

        // DIAG UP LEFT
        for (int i = 1, c = g.at(row - i, col - i); c != Grid::INVALID_AT; i++) { if (c == PAWN) return 1; }

        return 0;
    }

};

class Next {
public:
    static vector<int> for_horse(const Grid &g) {
        vector<pair<int, int>> candidates(8);
        int idx;

        // UP-RIGHT


//        sort(candidates.begin(), candidates.end(), greater<>());
        return vector<int>();

    };

    static vector<int> for_bishop(const Grid &g) {
        vector<int> candidates(g.k * 2 - 2);

//        sort(candidates.begin(), candidates.end(), greater<>());
        return candidates;
    };
};


void bb_dfs(Grid g, long depth, char play, long &best) {
    if (depth > g.max_depth || depth >= best) return;
    if (g.npwn == 0) {
        best = depth;
        return;
    }
    if (play == HORSE) {
        // TODO
    } else if (play == BISHOP) {
        // TODO
    }
}

int main(int argc, char **argv) {

    return 0;

    for (int i = 1; i < argc; i++) {
        string filename = argv[i];
        cout << Grid(filename);
    }

    return 0;
}

#include <iostream>
#include <cstring>
#include <fstream>
#include <vector>
#include <limits>
#include <chrono>
#include <algorithm>
#include <omp.h>
#include "mpi.h"

// chess pieces
#define HORSE  'J'
#define BISHOP 'S'
#define PAWN 'P'
#define EMPTY '-'

/**
 * MPI message TAGs
 */
enum MessageTag {
    DONE = 0, // work is done
    WORK = 1, // work to be done
    FINISHED = 2, // there is no more work
    UPDATE = 3 // update on the bestPathLen solution found by slave on it's instance
};


// relative mapping for all possible horse movements
// [ROW, COL]
const int HORSE_CAND[8][2] = {
        {-2, -1},
        {-2, 1},

        {-1, 2},
        {1,  2},

        {2,  1},
        {2,  -1},

        {1,  -2},
        {-1, -2}
};

#define EPOCH_CNT 3

using namespace std;

void ensureBufferSize(char **buf, int &bufLen, int bufLenNeeded) {
    if (bufLen >= bufLenNeeded) return;
    delete[] *buf;
    *buf = new char[bufLenNeeded];
    bufLen = bufLenNeeded;
}

class ChessBoard {
private:
    char *grid = nullptr;
    int size;
    int rowLen;
    int pawnCnt;
    int minDepth;

    // PDP hint heuristic
    int maxDepth;

    void setAt(int row, int col, char value) {
        grid[row * rowLen + col] = value;
    }

    class ChessMove {
    private:
        int row;
        int col;
        bool tookPawn;
    public:
        ChessMove(int row, int col, bool tookPawn) : row(row), col(col), tookPawn(tookPawn) {}

        ChessMove() = default;

        friend ostream &operator<<(ostream &os, const ChessMove &m) {
            os << m.row << "," << m.col;
            if (m.tookPawn) os << " *";
            return os;
        }

        void serializeToBuffer(char *buf, int bufLen, int &written) {
            char *head = buf;

            memcpy(head, &row, sizeof(row));
            head += sizeof(row);

            memcpy(head, &col, sizeof(col));
            head += sizeof(col);

            short tookPawnShort = tookPawn;
            memcpy(head, &tookPawnShort, sizeof(tookPawnShort));
            head += sizeof(tookPawnShort);

            written = head - buf;
        }

        static ChessMove deserializeFromBuffer(char *buf, int bufLen, int &read) {
            char *head = buf;

            int row;
            memcpy(&row, head, sizeof(row));
            head += sizeof(row);

            int col;
            memcpy(&col, head, sizeof(col));
            head += sizeof(col);

            short tookPawnShort;
            memcpy(&tookPawnShort, head, sizeof(tookPawnShort));
            bool tookPawn = tookPawnShort;
            head += sizeof(tookPawnShort);

            read = head - buf;
            return ChessMove(row, col, tookPawn);
        }
    };

    class ChessPiece {
    private:
        int row;
        int col;
        char type;

    public:
        ChessPiece() = default;

        ChessPiece(int row, int col, char type) : row(row), col(col), type(type) {}

        void serializeToBuffer(char *buf, int bufLen, int &written) {
            char *head = buf;

            memcpy(head, &row, sizeof(row));
            head += sizeof(row);

            memcpy(head, &col, sizeof(col));
            head += sizeof(col);

            *(head++) = type;

            written = head - buf;
        }

        static ChessPiece deserializeFromBuffer(char *buf, int bufLen, int &read) {
            char *head = buf;

            int row;
            memcpy(&row, head, sizeof(row));
            head += sizeof(row);

            int col;
            memcpy(&col, head, sizeof(col));
            head += sizeof(col);

            char type = *(head++);

            read = head - buf;
            return ChessPiece(row, col, type);
        }

        int getRow() const {
            return row;
        }

        int getCol() const {
            return col;
        }

        char getType() const {
            return type;
        }

        void setRow(int row) {
            ChessPiece::row = row;
        }

        void setCol(int col) {
            ChessPiece::col = col;
        }

    };

    ChessPiece bishop;
    ChessPiece horse;
    vector<ChessMove> moveLog;

    void logMovePiece(int row, int col) {
        bool tookPawn = at(row, col) == PAWN;
        moveLog.emplace_back(ChessMove(row, col, tookPawn));
    }

    void movePiece(ChessPiece &p, int row, int col) {
        logMovePiece(row, col);
        if (at(row, col) == PAWN) pawnCnt--;
        setAt(row, col, p.getType());
        setAt(p.getRow(), p.getCol(), EMPTY);
        p.setRow(row);
        p.setCol(col);
    }

public:
    ChessBoard(char *grid, int size, int rowLen, int pawnCnt, int minDepth, int maxDepth, const ChessPiece &bishop,
               const ChessPiece &horse, const vector<ChessMove> &moveLog) : grid(grid), size(size), rowLen(rowLen),
                                                                            pawnCnt(pawnCnt), minDepth(minDepth),
                                                                            maxDepth(maxDepth), bishop(bishop),
                                                                            horse(horse), moveLog(moveLog) {}

public:

    // returned when accessing invalid position in chess board
    const static char INVALID_AT = '\0';

    ChessBoard(const string &filename) {
        ifstream ifs(filename);
        ifs >> rowLen;
        ifs >> maxDepth;
        size = rowLen * rowLen;
        pawnCnt = 0;
        grid = new char[size];
        memset(grid, EMPTY, size);

        char c;
        char *head = grid;
        while (ifs.get(c) && head < grid + size) {
            if (c != '\n' && c != '\r') {
                int idx = int(head - grid);
                int row = int(idx / rowLen);
                int col = idx % rowLen;
                if (c == BISHOP) bishop = ChessPiece(row, col, BISHOP);
                if (c == HORSE) horse = ChessPiece(row, col, HORSE);
                if (c == PAWN) pawnCnt++;
                *(head++) = c;
            }
        }
        ifs.close();
        minDepth = pawnCnt;
    };

    ChessBoard(const ChessBoard &oth) {
        grid = new char[oth.size];
        memcpy(grid, oth.grid, oth.size);
        size = oth.size;
        rowLen = oth.rowLen;
        bishop = oth.bishop;
        horse = oth.horse;
        pawnCnt = oth.pawnCnt;
        minDepth = oth.minDepth;
        maxDepth = oth.maxDepth;
        moveLog = oth.moveLog;
    };

    void serializeToBuffer(char *buf, int bufLen, int &written) {
        char *head = buf;
        int cnt;

        memcpy(head, &size, sizeof(size));
        head += sizeof(size);

        memcpy(head, &rowLen, sizeof(rowLen));
        head += sizeof(rowLen);

        memcpy(head, &pawnCnt, sizeof(pawnCnt));
        head += sizeof(pawnCnt);

        memcpy(head, &minDepth, sizeof(minDepth));
        head += sizeof(minDepth);

        memcpy(head, &maxDepth, sizeof(maxDepth));
        head += sizeof(maxDepth);

        memcpy(head, grid, size);
        head += size;

        bishop.serializeToBuffer(head, bufLen - (head - buf), cnt);
        head += cnt;

        horse.serializeToBuffer(head, bufLen - (head - buf), cnt);
        head += cnt;

        int moveLogSize = moveLog.size();
        memcpy(head, &moveLogSize, sizeof(moveLogSize));
        head += sizeof(moveLogSize);

        for (int i = 0; i < moveLogSize; i++) {
            moveLog[i].serializeToBuffer(head, bufLen - (head - buf), cnt);
            head += cnt;
        }

        written = head - buf;
    }

    static ChessBoard deserializeFromBuffer(char *buf, int bufLen) {
        int read;
        return deserializeFromBuffer(buf, bufLen, read);
    }

    static ChessBoard deserializeFromBuffer(char *buf, int bufLen, int &read) {
        char *head = buf;
        int cnt;

        int size;
        memcpy(&size, head, sizeof(size));
        head += sizeof(size);

        int rowLen;
        memcpy(&rowLen, head, sizeof(rowLen));
        head += sizeof(rowLen);

        int pawnCnt;
        memcpy(&pawnCnt, head, sizeof(pawnCnt));
        head += sizeof(pawnCnt);

        int minDepth;
        memcpy(&minDepth, head, sizeof(minDepth));
        head += sizeof(minDepth);

        int maxDepth;
        memcpy(&maxDepth, head, sizeof(maxDepth));
        head += sizeof(maxDepth);

        char *grid = new char[size];
        memcpy(grid, head, size);
        head += size;

        ChessPiece bishop = ChessPiece::deserializeFromBuffer(head, bufLen - (head - buf), cnt);
        head += cnt;
        ChessPiece horse = ChessPiece::deserializeFromBuffer(head, bufLen - (head - buf), cnt);
        head += cnt;

        int moveLogSize;
        memcpy(&moveLogSize, head, sizeof(moveLogSize));
        head += sizeof(moveLogSize);

        cout << "==============================" << endl;
        cout << "size=" << size << endl;
        cout << "rowLen=" << rowLen << endl;
        cout << "panwCnt=" << pawnCnt << endl;
        cout << "minDepth=" << minDepth << endl;
        cout << "maxDepth=" << maxDepth << endl;
        cout << "Bishop=" << bishop.getCol() << "," << bishop.getRow() << ", type=" << bishop.getType() << endl;
        cout << "Horse=" << horse.getCol() << ", " << horse.getRow() << ", type=" << horse.getType() << endl;
        cout << "==============================" << endl << endl;

        vector<ChessMove> moveLog(moveLogSize);
        for (int i = 0; i < moveLogSize; i++) {
            moveLog[i] = ChessMove::deserializeFromBuffer(head, bufLen - (head - buf), cnt);
            head += cnt;
        }

        read = head - buf;
        return ChessBoard(grid, size, rowLen, pawnCnt, minDepth, maxDepth, bishop, horse, moveLog);
    }

    ChessBoard &operator=(const ChessBoard &oth) {
        memcpy(grid, oth.grid, oth.size);
        size = oth.size;
        rowLen = oth.rowLen;
        bishop = oth.bishop;
        horse = oth.horse;
        pawnCnt = oth.pawnCnt;
        minDepth = oth.minDepth;
        maxDepth = oth.maxDepth;
        moveLog = oth.moveLog;
        return *this;
    }

    ~ChessBoard() {
        delete[] grid;
    }

    char at(int row, int col) const {
        if (row < 0 || col < 0 || row >= rowLen || col >= rowLen) return INVALID_AT;
        return grid[row * rowLen + col];
    };

    void moveBishop(int row, int col) {
        movePiece(bishop, row, col);
    }

    void moveHorse(int row, int col) {
        movePiece(horse, row, col);
    }

    int getPawnCnt() const {
        return pawnCnt;
    }

    int getMaxDepth() const {
        return maxDepth;
    }

    const ChessPiece &getBishop() const {
        return bishop;
    }

    const ChessPiece &getHorse() const {
        return horse;
    }

    int getRowLen() const {
        return rowLen;
    }

    const vector<ChessMove> &getMoveLog() const {
        return moveLog;
    }

    friend ostream &operator<<(ostream &os, const ChessBoard &g) {
        os << "Délka strany šachovnice: " << g.rowLen << endl;
        os << "minimální hloubka: " << g.minDepth << ", maximální hloubka: " << g.maxDepth << endl;
        os << "Kůň na (" << g.horse.getRow() << "," << g.horse.getCol() << ")" << endl;
        os << "Střelec na (" << g.bishop.getRow() << "," << g.bishop.getCol() << ")" << endl;
        os << "Počet pěšáků " << g.pawnCnt << endl;
        for (int i = 0; i < g.size; i++) {
            os << g.grid[i];
            if ((i + 1) % g.rowLen) os << " | ";
            else os << endl;
        }
        return os;
    }

    int getMinDepth() const {
        return minDepth;
    }

    int getPathLen() const {
        return moveLog.size();
    }
};

struct Instance {
    ChessBoard board;
    int depth;
    char play;

    Instance(const ChessBoard &board, int depth, char play) : board(board), depth(depth), play(play) {}

    void serializeToBuffer(char *buf, int bufLen, int &written) {
        char *head = buf;
        int cnt;

        memcpy(head, &depth, sizeof(depth));
        head += sizeof(depth);

        *(head++) = play;

        board.serializeToBuffer(head, bufLen - (head - buf), cnt);
        head += cnt;

        written = head - buf;
    }

    static Instance deserializeFromBuffer(char *buf, int bufLen) {
        int read;
        return deserializeFromBuffer(buf, bufLen, read);
    }

    static Instance deserializeFromBuffer(char *buf, int bufLen, int &read) {
        char *head = buf;
        int cnt;

        int depth;
        memcpy(&depth, head, sizeof(depth));
        head += sizeof(depth);

        char play = *(head++);

        ChessBoard board = ChessBoard::deserializeFromBuffer(head, bufLen - (head - buf), cnt);
        head += cnt;

        read = head - buf;
        return Instance(board, depth, play);
    }
};

class EvalPosition {
public:
    static int for_horse(const ChessBoard &g, int row, int col) {
        // take pawn
        if (g.at(row, col) == PAWN) return 3;

        // take pawn next move
        for (const auto &cand : HORSE_CAND) {
            if (g.at(row + cand[0], col + cand[1]) == PAWN)
                return 2;
        }

        // one square away from pawn
        if (
                g.at(row + 1, col + 1) == PAWN ||
                g.at(row + 1, col - 1) == PAWN ||
                g.at(row + 1, col) == PAWN ||
                g.at(row - 1, col - 1) == PAWN ||
                g.at(row - 1, col + 1) == PAWN ||
                g.at(row - 1, col) == PAWN ||
                g.at(row, col + 1) == PAWN ||
                g.at(row, col - 1) == PAWN
                )
            return 1;

        return 0;
    };

    static int for_bishop(const ChessBoard &g, int row, int col) {
        if (g.at(row, col) == PAWN) return 2;
        char c;

        // DIAG DOWN RIGHT
        for (int i = 1;; i++) {
            c = g.at(row + i, col + i);
            if (c == ChessBoard::INVALID_AT || c == HORSE) break;
            if (c == PAWN) return 1;
        }

        // DIAG DOWN LEFT
        for (int i = 1;; i++) {
            c = g.at(row + i, col - i);
            if (c == ChessBoard::INVALID_AT || c == HORSE) break;
            if (c == PAWN) return 1;
        }

        // DIAG UP RIGHT
        for (int i = 1;; i++) {
            c = g.at(row - i, col + i);
            if (c == ChessBoard::INVALID_AT || c == HORSE) break;
            if (c == PAWN) return 1;
        }

        // DIAG UP LEFT
        for (int i = 1;; i++) {
            c = g.at(row - i, col - i);
            if (c == ChessBoard::INVALID_AT || c == HORSE) break;
            if (c == PAWN) return 1;
        }

        return 0;
    }


};

class NextPossibleMoves {
public:

    struct NextMove {
        int row;
        int col;
        int cost;

        NextMove() = default;

        NextMove(int row, int col, int cost) : row(row), col(col), cost(cost) {}

        static bool add_bishop_if_possible(int row, int col, const ChessBoard &g, vector<NextMove> &moves) {
            char c = g.at(row, col);
            if (c == HORSE || c == ChessBoard::INVALID_AT) {
                return false;
            }
            if (c == PAWN) {
                moves.emplace_back(row, col, EvalPosition::for_bishop(g, row, col));
                return false;
            }
            if (c == EMPTY) {
                moves.emplace_back(row, col, EvalPosition::for_bishop(g, row, col));
                return true;
            }
            return false;
        }

        static bool add_horse_if_possible(int row, int col, const ChessBoard &g, vector<NextMove> &moves) {
            char c = g.at(row, col);
            if (c == EMPTY || c == PAWN) {
                moves.emplace_back(row, col, EvalPosition::for_horse(g, row, col));
                return true;
            }
            return false;
        }

        struct comparator {
            bool operator()(const NextMove &a, const NextMove &b) const {
                return a.cost > b.cost;
            }
        };

    };

    static vector<NextMove> for_horse(const ChessBoard &g) {
        int row = g.getHorse().getRow();
        int col = g.getHorse().getCol();
        vector<NextMove> moves = vector<NextMove>();
        moves.reserve(8);
        for (const auto &cand : HORSE_CAND) {
            NextMove::add_horse_if_possible(cand[0] + row, cand[1] + col, g, moves);
        }

        sort(moves.begin(), moves.end(), NextPossibleMoves::NextMove::comparator());
        return moves;
    };

    static vector<NextMove> for_bishop(const ChessBoard &g) {
        vector<NextMove> moves = vector<NextMove>();
        moves.reserve(2 * g.getRowLen() - 2);
        int row = g.getBishop().getRow();
        int col = g.getBishop().getCol();

        // DIAG UP RIGHT
        for (int i = 1;; i++) {
            if (!NextMove::add_bishop_if_possible(row - i, col + i, g, moves)) break;
        }

        // DIAG UP LEFT
        for (int i = 1;; i++) {
            if (!NextMove::add_bishop_if_possible(row - i, col - i, g, moves)) break;
        }

        // DIAG DOWN RIGHT
        for (int i = 1;; i++) {
            if (!NextMove::add_bishop_if_possible(row + i, col + i, g, moves)) break;
        }

        // DIAG DOWN LEFT
        for (int i = 1;; i++) {
            if (!NextMove::add_bishop_if_possible(row + i, col - i, g, moves)) break;
        }

        sort(moves.begin(), moves.end(), NextPossibleMoves::NextMove::comparator());
        return moves;
    };

};

// return true if there is better board available
bool betterBoardExists(Instance *ins, long bestPathLen) {
    return
            ins->depth + ins->board.getPawnCnt() >= bestPathLen || // solution with lower cost already exists
            ins->depth + ins->board.getPawnCnt() > ins->board.getMaxDepth() ||
            // max depth would be reached if each play would remove figure
            bestPathLen == ins->board.getMinDepth(); // optimum was reached
}

void bbDfsSeq(Instance *ins, ChessBoard &bestBoard, long &bestPathLen, long &counter) {
    if (!betterBoardExists(ins, bestPathLen)) {
        if (ins->board.getPawnCnt() == 0) {
#pragma omp critical
            {
                if (!betterBoardExists(ins, bestPathLen)) {
                    bestPathLen = ins->depth;
                    bestBoard = ins->board;
                }
            }
        } else if (ins->play == HORSE) {
            for (const auto &m : NextPossibleMoves::for_horse(ins->board)) {
                ChessBoard cpy(ins->board);
                cpy.moveHorse(m.row, m.col);
                bbDfsSeq(new Instance(cpy, ins->depth + 1, BISHOP), bestBoard, bestPathLen, counter);
            }
        } else if (ins->play == BISHOP) {
            for (const auto &m : NextPossibleMoves::for_bishop(ins->board)) {
                ChessBoard cpy(ins->board);
                cpy.moveBishop(m.row, m.col);
                bbDfsSeq(new Instance(cpy, ins->depth + 1, HORSE), bestBoard, bestPathLen, counter);
            }
        }
    }
    delete ins;
#pragma omp atomic update
    counter++;
}

vector<Instance *> generateInstancesFrom(const Instance &initInstance) {
    vector<Instance *> instances = vector<Instance *>();
    instances.emplace_back(new Instance(initInstance));

    for (int i = 0; i < EPOCH_CNT; i++) {
        vector<Instance *> instancesNext = vector<Instance *>();
        for (const auto &ins : instances) {
            if (ins->play == HORSE) {
                for (const auto &m : NextPossibleMoves::for_horse(ins->board)) {
                    ChessBoard cpy(ins->board);
                    cpy.moveHorse(m.row, m.col);
                    instancesNext.emplace_back(new Instance(cpy, ins->depth + 1, BISHOP));
                }
            } else if (ins->play == BISHOP) {
                for (const auto &m : NextPossibleMoves::for_bishop(ins->board)) {
                    ChessBoard cpy(ins->board);
                    cpy.moveBishop(m.row, m.col);
                    instancesNext.emplace_back(new Instance(cpy, ins->depth + 1, HORSE));
                }
            }
        }
        instances = instancesNext;
    }
    cout << "Vygenerováno " << instances.size() << " instancí pro počet epoch " << EPOCH_CNT << "." << endl << endl;
    return instances;
}

ChessBoard bbDfsDataPar(const Instance &startInstance, long &bestPathLen, long &counter) {
    vector<Instance *> instances = generateInstancesFrom(startInstance);
    ChessBoard bestBoard(startInstance.board);
#pragma omp parallel for shared(instances, bestBoard, bestPathLen, counter) schedule(dynamic) default(none)
    for (unsigned long i = 0; i < instances.size(); i++) {
        bbDfsSeq(instances[i], bestBoard, bestPathLen, counter);
    }
    return bestBoard;
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    int myRank, processCount;
    MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
    MPI_Comm_size(MPI_COMM_WORLD, &processCount);

    const int bufLen = 1000000;
    char buf[bufLen];

    if (myRank == 0) { // master process
        Instance startInstance(ChessBoard(argv[1]), 0, HORSE);
        ChessBoard bestBoard(startInstance.board);
        int bestPathLen = numeric_limits<int>::max();
        cout << startInstance.board << endl;
        vector<Instance *> insList = generateInstancesFrom(startInstance);
        size_t insHead = 0;
        int runningSlaves = 0;
        int msgLen = -1;

        cout << "Počet slave procesů: " << processCount - 1 << endl;

        // send initial work to each slave
        for (int i = 1; i < processCount && insHead < insList.size(); i++) {
            insList[insHead]->serializeToBuffer(buf, bufLen, msgLen);
            cout << myRank << ": Posílam první instanci (" << msgLen << " bajtů) procesu číslo " << i << " ---> ";
            MPI_Send(buf, msgLen, MPI_CHAR, i, MessageTag::WORK, MPI_COMM_WORLD);
            cout << "OK" << endl;
            runningSlaves++;
            insHead++;
        }

        // check for finished work from slaves
        int flag;
        MPI_Status status;
        while (true) {
            MPI_Iprobe(MPI_ANY_SOURCE, MessageTag::DONE, MPI_COMM_WORLD, &flag, &status);
            if (flag) {
                // receive & deserialize solution board
                MPI_Get_count(&status, MPI_CHAR, &msgLen);
                MPI_Recv(&buf[0], msgLen, MPI_CHAR, status.MPI_SOURCE, status.MPI_TAG, MPI_COMM_WORLD,
                         MPI_STATUS_IGNORE);
                ChessBoard receivedBoard = ChessBoard::deserializeFromBuffer(buf, msgLen);

                // update best solution
                if (receivedBoard.getPathLen() < bestPathLen) {
                    bestBoard = receivedBoard;
                    bestPathLen = receivedBoard.getPathLen();
                    for (int i = 1; i < processCount; i++) {
                        if (i != status.MPI_SOURCE) {
                            MPI_Send(&bestPathLen, 1, MPI_INT, i, MessageTag::UPDATE, MPI_COMM_WORLD);
                        }
                    }
                }

                // send next work
                // or
                // send finish flag if there is no more work
                if (insHead < insList.size()) {
                    insList[insHead]->serializeToBuffer(buf, bufLen, msgLen);
                    MPI_Send(buf, msgLen, MPI_CHAR, status.MPI_SOURCE, MessageTag::WORK, MPI_COMM_WORLD);
                    insHead++;
                } else {
                    MPI_Send(&bestPathLen, 1, MPI_INT, status.MPI_SOURCE, MessageTag::FINISHED,
                             MPI_COMM_WORLD);
                    runningSlaves--;
                }

                if (runningSlaves == 0) break;
            }
        }

        /*
        long counter = 0;

        cout << startingBoard << endl;
        auto start = chrono::high_resolution_clock::now();
        bbDfsDataPar(new ChessBoard(filename), bestPathLenGlobal, &startingBoard, counter);
        auto stop = chrono::high_resolution_clock::now();

        cout << "Cena\tPočet volání\tČas [ms]" << endl;
        cout << bestPathLenGlobal << "\t" << counter << "\t\t"
             << std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() << endl << endl;

        cout << "Tahy" << endl;
        for (const auto &move : startingBoard.getMoveLog()) {
            cout << move << endl;
        }
         */

        cout << bestBoard << endl;

        // cleanup
        for (const auto &ins : insList) delete ins;
    } else { // slave process
        int flag;
        MPI_Status status;
        long bestPathLenSlave = numeric_limits<int>::max();
        long counterSlave = 0;
        int msgLen = -1;

        cout << myRank << ": Čekám na první zprávu" << endl;

        // TODO run one thread that will check for messages with results
        while (true) {
            MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
            if (flag) {
                if (status.MPI_TAG == MessageTag::WORK) {
                    // receive & deserializeFromBuffer message
                    MPI_Get_count(&status, MPI_CHAR, &msgLen);
                    cout << myRank << ": " << "Dostal jsem novou práci (" << msgLen << " bajtů)" << endl;
                    MPI_Recv(buf, msgLen, MPI_CHAR, status.MPI_SOURCE, status.MPI_TAG, MPI_COMM_WORLD,
                             MPI_STATUS_IGNORE);
                    Instance receivedInstance = Instance::deserializeFromBuffer(buf, msgLen);
                    cout << myRank << ": " << "Úspěšně jsem deserializoval novou práci" << endl;

                    // run
                    ChessBoard bestBoard = bbDfsDataPar(receivedInstance, bestPathLenSlave, counterSlave);

                    // send result
                    bestBoard.serializeToBuffer(buf, bufLen, msgLen);
                    MPI_Send(buf, msgLen, MPI_CHAR, 0, MessageTag::DONE, MPI_COMM_WORLD);
                } else if (status.MPI_TAG == MessageTag::FINISHED) {
                    break;
                }
            }
        }
    }

    MPI_Finalize();
    return 0;
}

#ifndef POEMBLOCK_H
#define POEMBLOCK_H

#include <vector>
#include <string>
using namespace std;

// 古诗文
class poem {
public:
    wstring title;
    vector<wstring> lines;
    poem(const wstring& t, const vector<wstring>& l);
};

// 单个活字块
class characterblock {
public:
    wchar_t character;
    characterblock(wchar_t c);
};


// 活字版
class movabletypeboard {
public:
    struct move {
        string type;
        int a, b, c, d;
        move(const string& t, int a, int b, int c = 0, int d = 0);
    };

private:
    int size;
    vector<vector<characterblock>> grid;
    vector<characterblock> originalorder;
    vector<move> shufflemoves;

public:
    movabletypeboard(const poem& p, int boardsize);
    void shuffle();
    void shuffle(int count);
    bool swaprows(int r1, int r2);
    bool swapcols(int c1, int c2);
    bool swapcross(int x1, int y1, int x2, int y2);
    bool isvalidcrosscenter(int x, int y);
    bool issolved();
    void display();
    void displayoriginal();
    void reset();
    vector<move> getshufflemoves() const;
};

// 加载诗文
vector<poem> loadpoems(const string& filename);

//账号功能
extern string current_user;
void load_users();
void save_users();
bool register_user();
bool login_user();
bool change_username();
bool change_password();
void logout_user();
#endif

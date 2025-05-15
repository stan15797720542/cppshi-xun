#include "poemblock.h"
#include <iostream>
#include <fstream>
#include <ctime>
#include <algorithm>
#include <codecvt>
#include <locale>
#include <fstream>
#include <unordered_map>

using namespace std;

poem::poem(const wstring& t, const vector<wstring>& l) : title(t), lines(l) {}

characterblock::characterblock(wchar_t c) : character(c) {}

movabletypeboard::move::move(const string& t, int a, int b, int c, int d) : type(t), a(a), b(b), c(c), d(d) {}

movabletypeboard::movabletypeboard(const poem& p, int boardsize) : size(boardsize) {
    vector<wchar_t> chars;
    for (const wstring& line : p.lines) {
        for (wchar_t ch : line) {
            if (ch != L' ' && ch != L'\n' && ch != L'\r')
                chars.push_back(ch);
        }
    }
    while (chars.size() < size * size)
        chars.push_back(L'　');

    grid.resize(size, vector<characterblock>(size, characterblock(L'　')));
    int idx = 0;
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            grid[i][j] = characterblock(chars[idx]);
            originalorder.push_back(characterblock(chars[idx]));
            ++idx;
        }
    }

}

void movabletypeboard::shuffle(int count) {
    int done = 0;
    while (done < count) {
        int op = rand() % 3;
        bool success = false;

        if (op == 0) {
            int r1 = rand() % size;
            int r2 = rand() % size;
            if (r1 != r2) success = swaprows(r1, r2);
        }
        else if (op == 1) {
            int c1 = rand() % size;
            int c2 = rand() % size;
            if (c1 != c2) success = swapcols(c1, c2);
        }
        else {
            int x1 = rand() % size;
            int y1 = rand() % size;
            int x2 = rand() % size;
            int y2 = rand() % size;
            if (isvalidcrosscenter(x1, y1) && isvalidcrosscenter(x2, y2))
                success = swapcross(x1, y1, x2, y2);
        }

        if (success) ++done;
    }
}

bool movabletypeboard::swaprows(int r1, int r2) {
    if (r1 < 0 || r2 < 0 || r1 >= size || r2 >= size) return false;
    for (int j = 0; j < size; ++j) {
        swap(grid[r1][j], grid[r2][j]);
    }
    shufflemoves.push_back(move("row", r1, r2));
    return true;
}

bool movabletypeboard::swapcols(int c1, int c2) {
    if (c1 < 0 || c2 < 0 || c1 >= size || c2 >= size) return false;
    for (int i = 0; i < size; ++i) {
        swap(grid[i][c1], grid[i][c2]);
    }
    shufflemoves.push_back(move("col", c1, c2));
    return true;
}

bool movabletypeboard::swapcross(int x1, int y1, int x2, int y2) {
    if (!isvalidcrosscenter(x1, y1) || !isvalidcrosscenter(x2, y2)) return false;
    vector<pair<int, int>> offset = { {0,0}, {-1,0}, {1,0}, {0,-1}, {0,1} };
    for (size_t i = 0; i < offset.size(); ++i) {
        int dx = offset[i].first;
        int dy = offset[i].second;
        swap(grid[x1 + dx][y1 + dy], grid[x2 + dx][y2 + dy]);
    }
    shufflemoves.push_back(move("cross", x1, y1, x2, y2));
    return true;
}

bool movabletypeboard::isvalidcrosscenter(int x, int y) {
    return x - 1 >= 0 && x + 1 < size && y - 1 >= 0 && y + 1 < size;
}

bool movabletypeboard::issolved() {
    int idx = 0;
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            if (grid[i][j].character != originalorder[idx++].character)
                return false;
        }
    }
    return true;
}

void movabletypeboard::display() {
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            wcout << grid[i][j].character << L" ";
        }
        wcout << endl;
    }
}

vector<poem> loadpoems(const string& filename) {
    vector<poem> poems;
    wifstream infile(filename);
    infile.imbue(locale(locale(), new codecvt_utf8<wchar_t>));
    if (!infile) {
        wcerr << L"无法打开文件: " << filename.c_str() << endl;
        return poems;
    }
    wstring line;
    while (getline(infile, line)) {
        size_t pos = line.find(L'#');
        if (pos != wstring::npos) {
            wstring title = line.substr(0, pos);
            wstring content = line.substr(pos + 1);
            vector<wstring> lines;
            int rowlen = 6;
            for (size_t i = 0; i < content.size(); i += rowlen) {
                lines.push_back(content.substr(i, rowlen));
            }
            poems.push_back(poem(title, lines));
        }
    }
    infile.close();
    return poems;
}

void movabletypeboard::displayoriginal() {
    int idx = 0;
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            wcout << originalorder[idx++].character << L" ";
        }
        wcout << endl;
    }
}
void movabletypeboard::reset() {
    int idx = 0;
    for (int i = 0; i < size; ++i)
        for (int j = 0; j < size; ++j)
            grid[i][j] = originalorder[idx++];
}

vector<movabletypeboard::move> movabletypeboard::getshufflemoves() const {
    return shufflemoves;
}

unordered_map<string, string> users;
string current_user = "";

void load_users() {
    users.clear();
    ifstream fin("users.txt");
    string line;
    while (getline(fin, line)) {
        auto pos = line.find(':');
        if (pos != string::npos) {
            string u = line.substr(0, pos);
            string p = line.substr(pos + 1);
            users[u] = p;
        }
    }
}

void save_users() {
    ofstream fout("users.txt");
    for (auto& kv : users) {
        fout << kv.first << ":" << kv.second << "\n";
    }
}

bool register_user() {
    cout << "—— 注册 ——\n用户名：";
    string u; cin >> u;
    if (users.count(u)) {
        cout << "用户名已存在！\n";
        return false;
    }
    cout << "密码：";
    string p; cin >> p;
    users[u] = p;
    save_users();
    cout << "注册成功！请登录。\n";
    return true;
}

bool login_user() {
    cout << "—— 登录 ——\n用户名：";
    string u; cin >> u;
    cout << "密码：";
    string p; cin >> p;
    auto it = users.find(u);
    if (it != users.end() && it->second == p) {
        current_user = u;
        cout << "登录成功，欢迎 " << u << "！\n";
        return true;
    }
    cout << "用户名或密码错误！\n";
    return false;
}

bool change_username() {
    cout << "—— 修改用户名 ——\n新用户名：";
    string nu; cin >> nu;
    if (nu.empty() || users.count(nu)) {
        cout << "用户名为空或已存在，取消操作。\n";
        return false;
    }
    users[nu] = users[current_user];
    users.erase(current_user);
    current_user = nu;
    save_users();
    cout << "用户名修改成功，新用户名：" << nu << "\n";
    return true;
}

bool change_password() {
    cout << "—— 修改密码 ——\n请输入旧密码：";
    string oldp; cin >> oldp;
    if (users[current_user] != oldp) {
        cout << "旧密码不正确，取消操作。\n";
        return false;
    }
    cout << "请输入新密码：";
    string newp; cin >> newp;
    users[current_user] = newp;
    save_users();
    cout << "密码修改成功。\n";
    return true;
}

void logout_user() {
    if (current_user.empty()) return;
    users.erase(current_user);
    save_users();
    cout << "账户已注销并删除： " << current_user << "\n";
    current_user.clear();
}

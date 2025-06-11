#include "poemblock.h"
#include <string>
#include <locale>
#include <codecvt>

// --- 全局变量定义 ---
map<wstring, User> users;
wstring current_user;
map<wstring, adventureprogress> user_progress;
AppState currentState = AppState::LOGIN;

// --- 构造函数实现 ---
poem::poem(const wstring& t, const vector<wstring>& l, const string& c) : title(t), lines(l), original_content(c) {}

characterblock::characterblock(wchar_t c) : character(c) {}

movabletypeboard::move::move(const string& t, int a, int b, int c, int d) : type(t), a(a), b(b), c(c), d(d) {}
// --- 工具函数实现 ---
wstring s2ws(const string& str) {
    if (str.empty()) return wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

string ws2s(const wstring& wstr) {
    if (wstr.empty()) return string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}
// --- UI 核心实现 ---
void drawButton(const Button& btn) {
    if (!btn.isVisible) return;
    // 绘制按钮背景
    setfillcolor(btn.color);
    solidrectangle(btn.x, btn.y, btn.x + btn.width, btn.y + btn.height);

    LOGFONT oldfont;
    gettextstyle(&oldfont);

    int sizeToUse = (btn.fontSize > 0) ? btn.fontSize : 20;

    settextstyle(sizeToUse, 0, L"微软雅黑"); 

    setbkmode(TRANSPARENT);
    settextcolor(BLACK);
    RECT r = { btn.x, btn.y, btn.x + btn.width, btn.y + btn.height };
    drawtext(btn.text.c_str(), &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // 恢复之前的字体设置
    settextstyle(&oldfont);
}

bool isMouseOnButton(const MOUSEMSG& msg, const Button& btn) {
    if (!btn.isVisible) return false;
    return msg.x >= btn.x && msg.x <= btn.x + btn.width &&
        msg.y >= btn.y && msg.y <= btn.y + btn.height;
}
// --- 数据 I/O 实现 ---
void load_users() {
    users.clear();
    ifstream fin("users.txt");
    if (!fin.is_open()) return;
    string line;
    while (getline(fin, line)) {
        if (line.empty()) continue;
        istringstream iss(line);
        string name, pwd;
        User u;
        iss >> name >> pwd;
        u.password = pwd;
        for (int d = 1; d <= 3; ++d) iss >> u.best_single[d];
        for (int d = 1; d <= 3; ++d) iss >> u.best_multi[d];
        users[s2ws(name)] = u;
    }
    fin.close();
}

void save_users() {
    ofstream fout("users.txt", ios::trunc);
    if (!fout.is_open()) return;
    for (auto& p : users) {
        const wstring& name_w = p.first;
        const User& u = p.second;
        string name = ws2s(name_w); 
        fout << name << " " << u.password;

        for (int d = 1; d <= 3; ++d) {
            try {
                fout << " " << u.best_single.at(d);
            }
            catch (const std::out_of_range& e) {
                fout << " " << 0;
                cerr << "Key " << d << " not found in best_single for user " << name << endl;
            }
        }

        for (int d = 1; d <= 3; ++d) {
            try {
                fout << " " << u.best_multi.at(d);
            }
            catch (const std::out_of_range& e) {
                fout << " " << 0;
                cerr << "Key " << d << " not found in best_multi for user " << name << endl;
            }
        }

        fout << "\n";
    }
    fout.close();
}

void load_progress() {
    user_progress.clear();
    ifstream fin("progress.txt");
    if (!fin.is_open()) return;
    string line;
    while (getline(fin, line)) {
        if (line.empty()) continue;
        istringstream iss(line);
        string name;
        adventureprogress prog;
        iss >> name >> prog.libai_level >> prog.dufu_level >> prog.sushi_level >> prog.zhuzibaijia_level;
        iss >> prog.libai_score >> prog.dufu_score >> prog.sushi_score >> prog.zhuzibaijia_score;
        string scores_data;
        if (getline(iss, scores_data) && !scores_data.empty()) {
            istringstream scores_stream(scores_data);
            string chapter_scores;
            while (getline(scores_stream, chapter_scores, ';')) {
                size_t colon_pos = chapter_scores.find(':');
                if (colon_pos != string::npos) {
                    string chapter = chapter_scores.substr(0, colon_pos);
                    string scores = chapter_scores.substr(colon_pos + 1);
                    vector<int> chapter_best_scores;
                    istringstream score_stream(scores);
                    string score;
                    while (getline(score_stream, score, ',')) {
                        if (!score.empty()) chapter_best_scores.push_back(stoi(score));
                    }
                    prog.best_scores[chapter] = chapter_best_scores;
                }
            }
        }
        user_progress[s2ws(name)] = prog;  // FIX: 转换为宽字符串存储
    }
    fin.close();
}

void save_progress() {
    ofstream fout("progress.txt", ios::trunc);
    if (!fout.is_open()) return;
    for (auto& p : user_progress) {
        const wstring& name_w = p.first;
        const adventureprogress& prog = p.second;
        string name = ws2s(name_w); 
        fout << name << " " << prog.libai_level << " " << prog.dufu_level << " " << prog.sushi_level << " " << prog.zhuzibaijia_level << " " << prog.libai_score << " " << prog.dufu_score << " " << prog.sushi_score << " " << prog.zhuzibaijia_score;
        for (auto& chapter_pair : prog.best_scores) {
            fout << " " << chapter_pair.first << ":";
            for (size_t i = 0; i < chapter_pair.second.size(); ++i) {
                if (i > 0) fout << ",";
                fout << chapter_pair.second[i];
            }
            fout << ";";
        }
        fout << "\n";
    }
    fout.close();
}

vector<poem> loadpoems_gui(const string& filename) {
    vector<poem> poems;
    wifstream file_in(filename); // FIX: Use wifstream for wide character files
    if (!file_in.is_open()) {
        MessageBox(GetHWnd(), (L"无法打开题库文件: " + s2ws(filename)).c_str(), L"错误", MB_OK | MB_ICONERROR);
        return poems;
    }
    // FIX: Apply UTF-8 facet to handle encoding correctly
    file_in.imbue(locale(locale::empty(), new codecvt_utf8<wchar_t>));

    wstring line_ws;
    while (getline(file_in, line_ws)) {
        if (line_ws.empty()) continue;
        size_t pos_ws = line_ws.find(L'#');
        if (pos_ws != wstring::npos) {
            wstring title_ws = line_ws.substr(0, pos_ws);
            wstring content_ws = line_ws.substr(pos_ws + 1);

            vector<wstring> lines_ws;
            wstringstream wss(content_ws);
            wstring segment;
            // Split content by Chinese punctuation
            while (getline(wss, segment, L'，') || getline(wss, segment, L'。') || getline(wss, segment, L'！') || getline(wss, segment, L'？')) {
                if (!segment.empty()) lines_ws.push_back(segment);
            }
            // Add the last part if exists
            if (!wss.eof() && !segment.empty()) {
                lines_ws.push_back(segment);
            }

            poems.push_back(poem(title_ws, lines_ws, ws2s(line_ws)));
        }
    }
    file_in.close();
    return poems;
}

void add_poem_to_file_w(const string& filename, const wstring& title, const wstring& content) {
    if (title.find(L'#') != wstring::npos || content.find(L'#') != wstring::npos) {
        MessageBox(GetHWnd(), L"标题或内容不能包含'#'字符。", L"错误", MB_OK | MB_ICONERROR);
        return;
    }

    // 使用 wofstream 并附加 app 模式
    wofstream outfile(filename, ios::app);
    if (!outfile) {
        MessageBox(GetHWnd(), L"无法写入题库文件。", L"错误", MB_OK | MB_ICONERROR);
        return;
    }
    // 同样应用 UTF-8 编码
    outfile.imbue(locale(locale::empty(), new codecvt_utf8<wchar_t>));

    outfile.seekp(0, ios::end);
    if (outfile.tellp() != 0) {
        outfile << endl;
    }
    outfile << title << L"#" << content;
    outfile.close();
    MessageBox(GetHWnd(), L"诗文添加成功！", L"成功", MB_OK);
}

void delete_poem_from_file_w(const string& filename, const wstring& title_to_delete) {
    vector<wstring> all_lines;
    wifstream infile(filename);
    if (!infile) {
        MessageBox(GetHWnd(), L"无法打开题库文件。", L"错误", MB_OK | MB_ICONERROR);
        return;
    }
    infile.imbue(locale(locale::empty(), new codecvt_utf8<wchar_t>));

    wstring line;
    bool found = false;
    while (getline(infile, line)) {
        if (line.empty()) continue;
        if (line.substr(0, line.find(L'#')) != title_to_delete) {
            all_lines.push_back(line);
        }
        else {
            found = true;
        }
    }
    infile.close();

    if (!found) {
        MessageBox(GetHWnd(), L"未找到该标题的诗文。", L"提示", MB_OK);
        return;
    }

    wofstream outfile(filename, ios::trunc);
    outfile.imbue(locale(locale::empty(), new codecvt_utf8<wchar_t>));
    for (size_t i = 0; i < all_lines.size(); ++i) {
        outfile << all_lines[i] << (i == all_lines.size() - 1 ? L"" : L"\n");
    }
    outfile.close();
    MessageBox(GetHWnd(), L"删除成功！", L"成功", MB_OK);
}
// --- MovableTypeBoard 类实现 ---
movabletypeboard::movabletypeboard(const poem& p, int boardsize) : size(boardsize) {
    vector<wchar_t> chars;
    for (const wstring& line : p.lines) {
        for (wchar_t ch : line) {
            if (ch != L' ' && ch != L'\n' && ch != L'\r' && ch != L'，' && ch != L'。' && ch != L'！' && ch != L'？')
                chars.push_back(ch);
        }
    }
    while (chars.size() < (size_t)size * size) {
        chars.push_back(L'　');
    }

    // CRITICAL FIX: Store the original order *before* shuffling the characters for the grid.
    originalorder.reserve(chars.size());
    for (wchar_t ch : chars) {
        originalorder.push_back(characterblock(ch));
    }

    grid.resize(size, vector<characterblock>(size, characterblock(L' ')));
    int idx = 0;
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            grid[i][j] = originalorder[idx++]; // Initially, grid is same as original
        }
    }
}

void movabletypeboard::shuffle(int count) {
    random_device rd;
    mt19937 g(rd());
    shuffle_moves.clear();
    int done = 0;

    while (done < count) {
        int op = uniform_int_distribution<>(0, 2)(g);
        bool success = false;

        if (op == 0) { // 交换行
            int r1 = uniform_int_distribution<>(0, size - 1)(g);
            int r2 = uniform_int_distribution<>(0, size - 1)(g);
            if (r1 != r2) {
                success = swaprows(r1, r2);
            }
        }
        else if (op == 1) { // 交换列
            int c1 = uniform_int_distribution<>(0, size - 1)(g);
            int c2 = uniform_int_distribution<>(0, size - 1)(g);
            if (c1 != c2) {
                success = swapcols(c1, c2);
            }
        }
        else { // 十字交换
            int x1 = uniform_int_distribution<>(0, size - 1)(g);
            int y1 = uniform_int_distribution<>(0, size - 1)(g);
            int x2 = uniform_int_distribution<>(0, size - 1)(g);
            int y2 = uniform_int_distribution<>(0, size - 1)(g);
            if (isvalidcrosscenter(x1, y1) && isvalidcrosscenter(x2, y2)) {
                success = swapcross(x1, y1, x2, y2);
            }
        }

        if (success) ++done;
    }
}

bool movabletypeboard::swaprows(int r1, int r2) {
    if (r1 < 0 || r2 < 0 || r1 >= size || r2 >= size || r1 == r2) return false;

    for (int j = 0; j < size; ++j) {
        swap(grid[r1][j], grid[r2][j]);
    }
    shuffle_moves.emplace_back("row", r1, r2);
    return true;
}

bool movabletypeboard::swapcols(int c1, int c2) {
    if (c1 < 0 || c2 < 0 || c1 >= size || c2 >= size || c1 == c2) return false;

    for (int i = 0; i < size; ++i) {
        swap(grid[i][c1], grid[i][c2]);
    }
    shuffle_moves.emplace_back("col", c1, c2);
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
    shuffle_moves.emplace_back("cross", x1, y1, x2, y2);
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

void movabletypeboard::reset() {
    int idx = 0;
    for (int i = 0; i < size; ++i)
        for (int j = 0; j < size; ++j)
            grid[i][j] = originalorder[idx++];
}

void movabletypeboard::display(int grid_x, int grid_y, int cell_size, const vector<pair<int, int>>& selections) {
    settextstyle((int)(cell_size * 0.8), 0, L"楷体");
    setbkmode(TRANSPARENT);
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            int x = grid_x + j * cell_size;
            int y = grid_y + i * cell_size;
            bool is_selected = false;
            for (const auto& sel : selections) {
                if (sel.first == i && sel.second == j) {
                    is_selected = true;
                    break;
                }
            }
            if (is_selected) {
                setlinecolor(RED);
                setfillcolor(YELLOW);
            }
            else {
                setlinecolor(BLACK);
                setfillcolor(RGB(245, 245, 220));
            }
            fillrectangle(x, y, x + cell_size, y + cell_size);
            wchar_t char_str[2] = { grid[i][j].character, L'\0' };
            RECT r = { x, y, x + cell_size, y + cell_size };
            drawtext(char_str, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
    }
    setlinecolor(BLACK);
}

void movabletypeboard::displayOriginal(int grid_x, int grid_y, int cell_size) {
    settextstyle((int)(cell_size * 0.8), 0, L"楷体");
    setbkmode(TRANSPARENT);
    setlinecolor(BLACK);
    int idx = 0;
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            int x = grid_x + j * cell_size;
            int y = grid_y + i * cell_size;

            setfillcolor(LIGHTGREEN); // 用绿色背景表示正确答案
            solidrectangle(x, y, x + cell_size, y + cell_size);

            wchar_t char_str[2] = { originalorder[idx++].character, L'\0' };
            RECT r = { x, y, x + cell_size, y + cell_size };
            drawtext(char_str, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
    }
}
// --- 后端游戏逻辑实现 ---
void update_ranking_single(const wstring& username, int difficulty, long long time_used) {
    if (users.find(username) == users.end()) return;
    User& u = users.at(username);
    int time_to_store = static_cast<int>(time_used);

    if (u.best_single.at(difficulty) == 0 || time_to_store < u.best_single.at(difficulty)) {
        u.best_single[difficulty] = time_to_store;
        save_users();
    }
}

int get_shuffle_count_by_difficulty(int difficulty) {
    if (difficulty == 1) return 5;
    if (difficulty == 2) return 10;
    if (difficulty == 3) return 15;
    return 5;
}

int get_shuffle_count(int level) {
    if (level <= 6) return (3 + level - 1);
    return min(20, 10 + (level - 6));
}

int calculate_score(int steps, long long time_used, int shuffle_count, int board_size) {
    int base_score = board_size * board_size * 10;
    int ideal_steps = shuffle_count;
    int step_penalty = max(0, steps - ideal_steps) * 5;
    int ideal_time = board_size * board_size + shuffle_count * 2;
    int time_penalty = max(0, (int)time_used - ideal_time) * 2;
    return max(50, base_score - step_penalty - time_penalty);
}

int get_unlock_requirement(const string& chapter, int current_poems_count) {
    map<string, int> base_reqs = { {"dufu", 800}, {"sushi", 1200}, {"zhuzibaijia", 1600} };
    if (base_reqs.find(chapter) == base_reqs.end()) return 0;
    int base_req = base_reqs[chapter];
    int poems_factor = max(6, current_poems_count);
    return (base_req * 6) / poems_factor;
}

bool is_chapter_unlocked(const string& chapter, const adventureprogress& prog) {
    if (chapter == "libai") return true;
    if (chapter == "dufu") return prog.libai_score >= get_unlock_requirement("dufu", loadpoems_gui("libai.txt").size());
    if (chapter == "sushi") return prog.dufu_score >= get_unlock_requirement("sushi", loadpoems_gui("dufu.txt").size());
    if (chapter == "zhuzibaijia") return prog.sushi_score >= get_unlock_requirement("zhuzibaijia", loadpoems_gui("sushi.txt").size());
    return false;
}

void runLoginScreen() {
    wchar_t username_w[50] = L"";
    wchar_t password_w[50] = L"";
    Button loginBtn = { 300, 350, 200, 50, L"登录", GREEN };
    Button registerBtn = { 300, 420, 200, 50, L"去注册", LIGHTBLUE };
    Button exitBtn = { 680, 520, 100, 40, L"退出", RED };

    while (currentState == AppState::LOGIN) {
        BeginBatchDraw();
        cleardevice(); setbkcolor(RGB(240, 240, 240));
        settextstyle(40, 0, L"微软雅黑"); outtextxy(WINDOW_WIDTH / 2 - 120, 80, L"活字排诗 - 登录");
        settextstyle(20, 0, L"微软雅黑"); outtextxy(250, 200, L"用户名:"); outtextxy(250, 250, L"密码:");
        rectangle(350, 195, 550, 225); rectangle(350, 245, 550, 275);
        outtextxy(355, 200, username_w);  // FIX: 直接使用宽字符数组
        wstring pass_display(wcslen(password_w), L'*');  // FIX: 使用wcslen
        outtextxy(355, 250, pass_display.c_str());
        drawButton(loginBtn); drawButton(registerBtn); drawButton(exitBtn);
        EndBatchDraw();

        MOUSEMSG msg;
        if (MouseHit()) {
            msg = GetMouseMsg();
            if (msg.uMsg == WM_LBUTTONDOWN) {
                if (isMouseOnButton(msg, loginBtn)) {
                    InputBox(username_w, 49, L"请输入用户名");
                    InputBox(password_w, 49, L"请输入密码", L"登录", L"", 0, 0, true);
                    wstring u(username_w);
                    string p = ws2s(wstring(password_w));
                    if (users.count(u) && users.at(u).password == p) {
                        current_user = u;
                        // Initialize progress if first time login
                        if (user_progress.find(current_user) == user_progress.end()) {
                            user_progress[current_user] = adventureprogress();
                        }
                        MessageBox(GetHWnd(), L"登录成功！", L"提示", MB_OK);
                        currentState = AppState::MAIN_MENU;
                    }
                    else {
                        MessageBox(GetHWnd(), L"用户名或密码错误！", L"错误", MB_OK | MB_ICONERROR);
                    }
                }
                else if (isMouseOnButton(msg, registerBtn)) { currentState = AppState::REGISTER; }
                else if (isMouseOnButton(msg, exitBtn)) { currentState = AppState::EXIT; }
            }
        }
        Sleep(1);
    }
}

void runRegisterScreen() {
    wchar_t username_s[50] = L"", password_s[50] = L"", confirm_s[50] = L"";
    Button registerBtn = { 300, 400, 200, 50, L"确认注册", GREEN };
    Button backBtn = { 300, 470, 200, 50, L"返回登录", LIGHTGRAY };

    while (currentState == AppState::REGISTER) {
        BeginBatchDraw();
        cleardevice();
        setbkcolor(RGB(240, 240, 240));
        settextstyle(40, 0, L"微软雅黑");
        outtextxy(WINDOW_WIDTH / 2 - 140, 80, L"新用户注册");
        settextstyle(20, 0, L"微软雅黑");
        outtextxy(200, 200, L"新用户名:");
        rectangle(320, 195, 520, 225);
        outtextxy(200, 250, L"输入密码:");
        rectangle(320, 245, 520, 275);
        outtextxy(200, 300, L"确认密码:");
        rectangle(320, 295, 520, 325);
        drawButton(registerBtn);
        drawButton(backBtn);
        EndBatchDraw();

        MOUSEMSG msg;
        if (MouseHit()) {
            msg = GetMouseMsg();
            if (msg.uMsg == WM_LBUTTONDOWN) {
                if (isMouseOnButton(msg, registerBtn)) {
                    InputBox(username_s, 49, L"请输入新用户名");
                    InputBox(password_s, 49, L"请输入密码", L"", L"", 0, 0, true);
                    InputBox(confirm_s, 49, L"请确认密码", L"", L"", 0, 0, true);

                    string u = ws2s(username_s), p1 = ws2s(password_s), p2 = ws2s(confirm_s);

                    if (u.empty() || p1.empty()) {
                        MessageBox(GetHWnd(), L"用户名和密码不能为空！", L"错误", MB_OK);
                        continue;
                    }
                    wstring wu = s2ws(u); 
                    if (users.count(wu)) {
                        MessageBox(GetHWnd(), L"用户名已存在！", L"错误", MB_OK);
                        continue;
                    }
                    
                    if (p1 != p2) {
                        MessageBox(GetHWnd(), L"两次密码输入不一致！", L"错误", MB_OK);
                        continue;
                    }

                    User newUser;
                    newUser.password = p1;
                    users[wu] = newUser;
                    save_users();
                    MessageBox(GetHWnd(), L"注册成功！请返回登录。", L"成功", MB_OK);
                    currentState = AppState::LOGIN;
                }
                else if (isMouseOnButton(msg, backBtn)) {
                    currentState = AppState::LOGIN;
                }
            }
        }
        Sleep(1);
    }
}

void runMainMenu() {
    int btn_width = 200;
    int btn_x = WINDOW_WIDTH / 2 - btn_width / 2;

    Button accountBtn = { btn_x, 200, btn_width, 50, L"账户管理", LIGHTGREEN };
    Button rankBtn = { btn_x, 270, btn_width, 50, L"查看排名", LIGHTGREEN };
    Button libBtn = { btn_x, 340, btn_width, 50, L"管理题库", LIGHTGREEN };
    Button gameBtn = { btn_x, 410, btn_width, 50, L"开始游戏", LIGHTCYAN };
    Button logoutBtn = { btn_x, 480, btn_width, 50, L"退出登录", LIGHTGRAY };

    while (currentState == AppState::MAIN_MENU) {
        BeginBatchDraw();
        cleardevice();
        settextstyle(40, 0, L"微软雅黑"); outtextxy(WINDOW_WIDTH / 2 - 100, 50, L"主菜单");
        wstring welcome_msg = L"欢迎, " + current_user;
        settextstyle(20, 0, L"微软雅黑"); outtextxy(50, 120, welcome_msg.c_str());
        drawButton(accountBtn); drawButton(rankBtn); drawButton(libBtn); drawButton(gameBtn); drawButton(logoutBtn);
        EndBatchDraw();

        MOUSEMSG msg;
        if (MouseHit()) {
            msg = GetMouseMsg();
            if (msg.uMsg == WM_LBUTTONDOWN) {
                if (isMouseOnButton(msg, gameBtn)) currentState = AppState::GAME_MODE_SELECT;
                else if (isMouseOnButton(msg, logoutBtn)) { current_user.clear(); currentState = AppState::LOGIN; }
                else if (isMouseOnButton(msg, accountBtn)) currentState = AppState::ACCOUNT_MANAGE;
                else if (isMouseOnButton(msg, rankBtn)) currentState = AppState::RANKING;
                else if (isMouseOnButton(msg, libBtn)) currentState = AppState::POEM_LIB_MENU;
            }
        }
        Sleep(1);
    }
}

void runAccountManageScreen() {
    Button changeUserBtn = { 300, 200, 200, 50, L"修改用户名", LIGHTBLUE };
    Button changePassBtn = { 300, 270, 200, 50, L"修改密码", LIGHTBLUE };
    Button deleteAccountBtn = { 300, 360, 200, 50, L"注销并删除账户", RED };
    Button backBtn = { 50, 500, 150, 40, L"返回主菜单", LIGHTGRAY };

    while (currentState == AppState::ACCOUNT_MANAGE) {
        BeginBatchDraw();
        cleardevice();
        settextstyle(40, 0, L"微软雅黑");
        outtextxy(WINDOW_WIDTH / 2 - 140, 100, L"账户管理");

        drawButton(changeUserBtn);
        drawButton(changePassBtn);
        drawButton(deleteAccountBtn); 
        drawButton(backBtn);

        EndBatchDraw();

        MOUSEMSG msg;
        if (MouseHit()) {
            msg = GetMouseMsg();
            if (msg.uMsg == WM_LBUTTONDOWN) {
                if (isMouseOnButton(msg, changeUserBtn)) {
                    wchar_t newUser_s[50] = L"";
                    if (InputBox(newUser_s, 49, L"请输入新的用户名：")) {
                        string nu = ws2s(newUser_s);
                        if (nu.empty() || users.count(s2ws(nu))) {
                            MessageBox(GetHWnd(), L"用户名为空或已存在！", L"错误", MB_OK | MB_ICONERROR);
                        }
                        else {
                            users[s2ws(nu)] = users[current_user];
                            users.erase(current_user);
                            current_user = s2ws(nu);
                            save_users();
                            MessageBox(GetHWnd(), (L"用户名修改成功，新用户名为：" + s2ws(nu)).c_str(), L"成功", MB_OK);
                        }
                    }
                }
                else if (isMouseOnButton(msg, changePassBtn)) {
                    wchar_t old_p[50] = L"", new_p[50] = L"";
                    if (InputBox(old_p, 49, L"请输入旧密码：", L"", L"", 0, 0, true)) {
                        if (users[current_user].password != ws2s(old_p)) {
                            MessageBox(GetHWnd(), L"旧密码不正确！", L"错误", MB_OK | MB_ICONERROR);
                        }
                        else {
                            if (InputBox(new_p, 49, L"请输入新密码：", L"", L"", 0, 0, true)) {
                                users[current_user].password = ws2s(new_p);
                                save_users();
                                MessageBox(GetHWnd(), L"密码修改成功！", L"成功", MB_OK);
                            }
                        }
                    }
                }
                else if (isMouseOnButton(msg, deleteAccountBtn)) {

                    if (MessageBox(GetHWnd(),
                        L"警告：此操作将永久删除您的所有数据（包括排名和闯关进度），且无法恢复！\n您确定要继续吗？",
                        L"确认删除账户",
                        MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) == IDYES)
                    {
                        wchar_t password_check[50] = L"";
                        if (InputBox(password_check, 49, L"为确认是您本人操作，请输入您的登录密码：", L"身份验证", L"", 0, 0, true)) {

                            string current_password = users[current_user].password;
                            string input_password = ws2s(password_check);

                            if (current_password == input_password) {
                                // 从用户数据中移除
                                users.erase(current_user);

                                // 从进度数据中移除
                                if (user_progress.count(current_user)) {
                                    user_progress.erase(current_user);
                                }

                                // 保存修改后的文件
                                save_users();
                                save_progress();

                                MessageBox(GetHWnd(), L"您的账户已成功删除。", L"操作完成", MB_OK);

                                current_user.clear(); // 清空当前用户
                                currentState = AppState::LOGIN; // 状态切换到登录界面
                            }
                            else {
                                // 密码错误
                                MessageBox(GetHWnd(), L"密码错误，操作已取消。", L"验证失败", MB_OK | MB_ICONERROR);
                            }
                        }
                    }
                }
                else if (isMouseOnButton(msg, backBtn)) {
                    currentState = AppState::MAIN_MENU;
                }
            }
        }
        Sleep(1);
    }
}

void runRankingScreen() {
    Button backBtn = { 50, 500, 150, 40, L"返回主菜单", LIGHTGRAY };
    const User& u = users[current_user];

    while (currentState == AppState::RANKING) {
        BeginBatchDraw();
        cleardevice();
        settextstyle(40, 0, L"微软雅黑");

        wstring title = L"玩家 " + current_user + L" 的排名";
        outtextxy(WINDOW_WIDTH / 2 - 250, 50, title.c_str());

        settextstyle(20, 0, L"微软雅黑");

        auto draw_table = [&](int y_offset, const wstring& mode_title, const map<int, int>& scores) {
            outtextxy(150, y_offset, mode_title.c_str());
            rectangle(150, y_offset + 30, 650, y_offset + 100);
            line(150, y_offset + 65, 650, y_offset + 65);
            outtextxy(170, y_offset + 35, L"难度");
            outtextxy(320, y_offset + 35, L"简单");
            outtextxy(420, y_offset + 35, L"中等");
            outtextxy(520, y_offset + 35, L"困难");
            outtextxy(170, y_offset + 70, L"用时");

            auto getScore = [&scores](int difficulty) -> int {
                auto it = scores.find(difficulty);
                return (it != scores.end()) ? it->second : 0;
                };

            wstring s1 = getScore(1) == 0 ? L"--" : std::to_wstring(getScore(1));
            wstring s2 = getScore(2) == 0 ? L"--" : std::to_wstring(getScore(2));
            wstring s3 = getScore(3) == 0 ? L"--" : std::to_wstring(getScore(3));

            outtextxy(330, y_offset + 70, s1.c_str());
            outtextxy(430, y_offset + 70, s2.c_str());
            outtextxy(530, y_offset + 70, s3.c_str());
            };

        draw_table(150, L"单人模式最佳用时(秒)", u.best_single);
        draw_table(300, L"多人模式最佳用时(秒)", u.best_multi); // 现在可以正确显示多人数据
        drawButton(backBtn);
        EndBatchDraw();

        MOUSEMSG msg;
        if (MouseHit() && (msg = GetMouseMsg()).uMsg == WM_LBUTTONDOWN && isMouseOnButton(msg, backBtn)) {
            currentState = AppState::MAIN_MENU;
        }
        Sleep(1);
    }
}

void runPoemLibMenu() {
    // 自由模式按钮
    Button viewFreeBtn = { 100, 150, 250, 50, L"查看/删除 自由模式 题库", LIGHTBLUE,12 };
    Button addFreeBtn = { 100, 220, 250, 50, L"添加 自由模式 诗文", LIGHTBLUE ,16};

    // 闯关模式按钮
    Button viewAdventureBtn = { 450, 150, 250, 50, L"查看/删除 闯关模式 题库", LIGHTCYAN,12 };
    Button addAdventureBtn = { 450, 220, 250, 50, L"添加 闯关模式 诗文", LIGHTCYAN,16 };

    Button backBtn = { 50, 500, 150, 40, L"返回主菜单", LIGHTGRAY };

    while (currentState == AppState::POEM_LIB_MENU) {
        BeginBatchDraw();
        cleardevice();
        settextstyle(40, 0, L"微软雅黑"); outtextxy(WINDOW_WIDTH / 2 - 140, 80, L"题库管理");
        drawButton(viewFreeBtn);
        drawButton(addFreeBtn);
        drawButton(viewAdventureBtn);
        drawButton(addAdventureBtn);
        drawButton(backBtn);
        EndBatchDraw();

        MOUSEMSG msg;
        if (MouseHit() && (msg = GetMouseMsg()).uMsg == WM_LBUTTONDOWN) {
            if (isMouseOnButton(msg, backBtn)) {
                currentState = AppState::MAIN_MENU;
            }
            // 自由模式
            else if (isMouseOnButton(msg, viewFreeBtn)) {
                runViewPoemsScreen("poems.txt"); 
            }
            else if (isMouseOnButton(msg, addFreeBtn)) {
                runAddPoemScreen("poems.txt");
            }
            // 闯关模式
            else if (isMouseOnButton(msg, viewAdventureBtn)) {
                runChapterSelectScreen(AppState::VIEW_POEMS); // 先选择章节再查看
            }
            else if (isMouseOnButton(msg, addAdventureBtn)) {
                runChapterSelectScreen(AppState::ADD_POEM); // 先选择章节再添加
            }
        }
        Sleep(1);
    }
}

void runChapterSelectScreen(AppState intended_action) {
    // 根据传入的操作，设置界面标题
    wstring title;
    if (intended_action == AppState::VIEW_POEMS) title = L"请选择要查看/删除的章节";
    else if (intended_action == AppState::ADD_POEM) title = L"请选择要添加诗文的章节";
    else return; // 非法操作

    vector<pair<wstring, string>> chapters = {
        {L"李白 章节", "libai.txt"},
        {L"杜甫 章节", "dufu.txt"},
        {L"苏轼 章节", "sushi.txt"},
        {L"诸子百家 章节", "zhuzibaijia.txt"}
    };
    vector<Button> chapterBtns;
    for (size_t i = 0; i < chapters.size(); ++i) {
        chapterBtns.push_back({ 250, 150 + static_cast<int>(i) * 70, 300, 60, chapters[i].first, LIGHTCYAN });
    }
    Button backBtn = { 50, 520, 100, 40, L"返回", LIGHTGRAY };

    AppState tempState = AppState::ADVENTURE_CHAPTER_VIEW;
    while (tempState == AppState::ADVENTURE_CHAPTER_VIEW) {
        BeginBatchDraw();
        cleardevice();
        settextstyle(30, 0, L"微软雅黑");
        outtextxy(WINDOW_WIDTH / 2 - 200, 80, title.c_str());

        for (const auto& btn : chapterBtns) drawButton(btn);
        drawButton(backBtn);
        EndBatchDraw();

        MOUSEMSG msg;
        if (MouseHit() && (msg = GetMouseMsg()).uMsg == WM_LBUTTONDOWN) {
            if (isMouseOnButton(msg, backBtn)) {
                tempState = AppState::EXIT; // 退出此界面循环
                break;
            }

            for (size_t i = 0; i < chapterBtns.size(); ++i) {
                if (isMouseOnButton(msg, chapterBtns[i])) {
                    string filename = chapters[i].second;
                    // 根据传入的操作，执行对应的功能
                    if (intended_action == AppState::VIEW_POEMS) {
                        runViewPoemsScreen(filename);
                    }
                    else if (intended_action == AppState::ADD_POEM) {
                        runAddPoemScreen(filename);
                    }
                    // 退出此界面循环，返回题库主菜单
                    tempState = AppState::EXIT;
                    break;
                }
            }
        }
        Sleep(1);
    }
    currentState = AppState::POEM_LIB_MENU;
}

void runViewPoemsScreen(const string& filename) {
    int page = 0;
    const int items_per_page = 10;

    Button backBtn = { 50, 520, 100, 40, L"返回", LIGHTGRAY };
    Button prevBtn = { 450, 520, 100, 40, L"上一页", LIGHTBLUE };
    Button nextBtn = { 570, 520, 100, 40, L"下一页", LIGHTBLUE };
    // 删除按钮现在将一直可见
    Button delBtn = { 690, 520, 100, 40, L"删除诗文", RED };

    AppState tempState = AppState::VIEW_POEMS;
    while (tempState == AppState::VIEW_POEMS) {
        // 每次循环重新加载，以反映删除操作
        vector<poem> poems = loadpoems_gui(filename);

        BeginBatchDraw();
        cleardevice();
        wstring title = L"题库列表: " + s2ws(filename);
        settextstyle(40, 0, L"微软雅黑"); outtextxy(50, 30, title.c_str());
        settextstyle(16, 0, L"宋体");

        int start_index = page * items_per_page;
        for (int i = 0; i < items_per_page && (static_cast<size_t>(start_index) + i) < poems.size(); ++i) {
            int current_index = start_index + i;
            wstring text = to_wstring(current_index + 1) + L". " + poems[current_index].title;
            outtextxy(100, 80 + i * 40, text.c_str());
        }

        prevBtn.isVisible = (page > 0);
        nextBtn.isVisible = ((page + 1) * items_per_page < poems.size());

        drawButton(backBtn);
        drawButton(prevBtn);
        drawButton(nextBtn);
        drawButton(delBtn); // 绘制删除按钮
        EndBatchDraw();

        MOUSEMSG msg;
        if (MouseHit() && (msg = GetMouseMsg()).uMsg == WM_LBUTTONDOWN) {
            if (isMouseOnButton(msg, backBtn)) {
                tempState = AppState::EXIT;
            }
            else if (isMouseOnButton(msg, prevBtn) && prevBtn.isVisible) {
                page--;
            }
            else if (isMouseOnButton(msg, nextBtn) && nextBtn.isVisible) {
                page++;
            }
            else if (isMouseOnButton(msg, delBtn)) {
                runDeletePoemScreen(filename);
                // 删除后，循环会自动重新加载并刷新列表
            }
        }
        Sleep(1);
    }
    // 退出后，返回到题库管理主菜单
    currentState = AppState::POEM_LIB_MENU;
}

void runAddPoemScreen(const string& filename) {
    wchar_t title_w[100];
    wchar_t content_w[1024];
    wstring prompt_title = L"为 " + s2ws(filename) + L" 添加新诗文";
    MessageBox(GetHWnd(), prompt_title.c_str(), L"添加诗文", MB_OK);

    if (InputBox(title_w, 99, L"请输入诗文标题 (不能为空)")) {
        wstring title_str(title_w);
        if (title_str.empty()) {
            MessageBox(GetHWnd(), L"标题不能为空！", L"错误", MB_OK | MB_ICONERROR);
        }
        else {
            if (InputBox(content_w, 1023, L"请输入诗文正文 (不能为空)")) {
                wstring content_str(content_w);
                if (content_str.empty()) {
                    MessageBox(GetHWnd(), L"内容不能为空！", L"错误", MB_OK | MB_ICONERROR);
                }
                else {
                    add_poem_to_file_w(filename, title_str, content_str);
                }
            }
        }
    }
    currentState = AppState::POEM_LIB_MENU;
}

void runDeletePoemScreen(const string& filename) {
    wchar_t title_w[100];
    wstring prompt_title = L"从 " + s2ws(filename) + L" 删除诗文";
    MessageBox(GetHWnd(), prompt_title.c_str(), L"删除诗文", MB_OK);

    if (InputBox(title_w, 99, L"请输入要删除的完整诗文标题")) {
        wstring title_str(title_w);
        if (title_str.empty()) {
            MessageBox(GetHWnd(), L"标题不能为空！", L"错误", MB_OK | MB_ICONERROR);
        }
        else {
            delete_poem_from_file_w(filename, title_str);
        }
    }
    // 返回后，runViewPoemsScreen 会重新加载并刷新列表
}

void runGameModeSelectScreen() {
    Button singleBtn = { 300, 180, 200, 60, L"单人模式", LIGHTBLUE };
    Button multiBtn = { 300, 260, 200, 60, L"多人模式", LIGHTGREEN }; 
    Button adventureBtn = { 300, 340, 200, 60, L"闯关模式", LIGHTCYAN };
    Button backBtn = { 50, 500, 150, 40, L"返回主菜单", LIGHTGRAY };

    while (currentState == AppState::GAME_MODE_SELECT) {
        BeginBatchDraw();
        cleardevice();
        settextstyle(40, 0, L"微软雅黑"); outtextxy(WINDOW_WIDTH / 2 - 160, 100, L"选择游戏模式");
        drawButton(singleBtn);
        drawButton(multiBtn); 
        drawButton(adventureBtn);
        drawButton(backBtn);
        EndBatchDraw();

        MOUSEMSG msg;
        if (MouseHit() && (msg = GetMouseMsg()).uMsg == WM_LBUTTONDOWN) {
            if (isMouseOnButton(msg, singleBtn)) currentState = AppState::SINGLE_PLAYER_SETUP;
            else if (isMouseOnButton(msg, multiBtn)) currentState = AppState::MULTIPLAYER_SETUP; // 链接到新状态
            else if (isMouseOnButton(msg, adventureBtn)) currentState = AppState::ADVENTURE_MENU;
            else if (isMouseOnButton(msg, backBtn)) currentState = AppState::MAIN_MENU;
        }
        Sleep(1);
    }
}

void runSinglePlayerSetup() {
    vector<poem> poems = loadpoems_gui("poems.txt");
    if (poems.empty()) {
        MessageBox(GetHWnd(), L"自由题库为空，无法开始游戏！", L"错误", MB_OK | MB_ICONERROR);
        currentState = AppState::GAME_MODE_SELECT;
        return;
    }
    int selected_poem_idx = -1;
    int selected_difficulty = 1;
    int selected_board_size = 6;
    int page = 0;
    const int items_per_page = 8;

    bool timer_enabled = false;
    int time_limit_seconds = 180; // 默认3分钟

    // --- 界面按钮 ---
    Button backBtn = { 50, 520, 100, 40, L"返回", LIGHTGRAY };
    Button prevBtn = { 300, 520, 100, 40, L"上一页", LIGHTBLUE };
    Button nextBtn = { 420, 520, 100, 40, L"下一页", LIGHTBLUE };
    Button startBtn = { 650, 520, 100, 40, L"开始游戏", GREEN };
    vector<Button> difficultyBtns = {
        {550, 160, 80, 40, L"简单", YELLOW},
        {550, 210, 80, 40, L"中等", LIGHTGRAY},
        {550, 260, 80, 40, L"困难", LIGHTGRAY}
    };
    vector<Button> sizeBtns = {
    {650, 160, 80, 40, L"6x6", YELLOW}, // 默认选中6x6
    {650, 210, 80, 40, L"7x7", LIGHTGRAY},
    {650, 260, 80, 40, L"8x8", LIGHTGRAY},
    {650, 310, 80, 40, L"9x9", LIGHTGRAY} // 如果需要9x9
    };
    Button timerToggleBtn = { 550, 350, 150, 40, L"启用限时: 关", RED };
    Button setTimeBtn = { 550, 400, 150, 40, L"设置时间", LIGHTGRAY };


    while (currentState == AppState::SINGLE_PLAYER_SETUP) {
        BeginBatchDraw();
        cleardevice();
        settextstyle(30, 0, L"微软雅黑"); outtextxy(50, 30, L"单人模式设置");
        settextstyle(20, 0, L"微软雅黑"); outtextxy(50, 80, L"选择诗文:");

        vector<Button> poemBtns;
        int start_index = page * items_per_page;
        for (int i = 0; i < items_per_page && (static_cast<size_t>(start_index) + i) < poems.size(); ++i) {
            int current_index = start_index + i;
            Button pBtn = { 50, 120 + i * 45, 450, 40, poems[current_index].title, (current_index == selected_poem_idx ? YELLOW : LIGHTGRAY) };
            poemBtns.push_back(pBtn);
            drawButton(pBtn);
        }

        // 难度选择
        outtextxy(550, 120, L"选择难度:");
        for (const auto& btn : difficultyBtns) drawButton(btn);
        outtextxy(650, 120, L"选择大小:");
        for (const auto& btn : sizeBtns) drawButton(btn);
        outtextxy(550, 310, L"限时挑战:");
        timerToggleBtn.text = timer_enabled ? L"启用限时: 开" : L"启用限时: 关";
        timerToggleBtn.color = timer_enabled ? GREEN : RED;
        drawButton(timerToggleBtn);

        // 仅在启用限时后才显示设置按钮和时间
        setTimeBtn.isVisible = timer_enabled;
        drawButton(setTimeBtn);
        if (timer_enabled) {
            wstring time_text = L"当前限时: " + to_wstring(time_limit_seconds) + L" 秒";
            outtextxy(550, 450, time_text.c_str());
        }

        // 绘制底部按钮
        prevBtn.isVisible = (page > 0);
        nextBtn.isVisible = ((page + 1) * items_per_page < poems.size());
        startBtn.isVisible = (selected_poem_idx != -1);
        drawButton(backBtn); drawButton(prevBtn); drawButton(nextBtn); drawButton(startBtn);
        EndBatchDraw();

        MOUSEMSG msg;
        if (MouseHit() && (msg = GetMouseMsg()).uMsg == WM_LBUTTONDOWN) {
            if (isMouseOnButton(msg, backBtn)) { currentState = AppState::GAME_MODE_SELECT; break; }
            if (isMouseOnButton(msg, prevBtn) && prevBtn.isVisible) page--;
            if (isMouseOnButton(msg, nextBtn) && nextBtn.isVisible) page++;

            if (isMouseOnButton(msg, timerToggleBtn)) {
                timer_enabled = !timer_enabled;
            }
            if (timer_enabled && isMouseOnButton(msg, setTimeBtn)) {
                wchar_t time_input[10] = L"";
                if (InputBox(time_input, 9, L"请输入限时秒数（如180）")) {
                    try {
                        time_limit_seconds = stoi(wstring(time_input));
                        if (time_limit_seconds <= 0) {
                            time_limit_seconds = 180; // 输入无效则恢复默认
                            MessageBox(GetHWnd(), L"输入无效，已重置为180秒。", L"提示", MB_OK);
                        }
                    }
                    catch (...) {
                        time_limit_seconds = 180; // 转换失败也恢复默认
                        MessageBox(GetHWnd(), L"输入格式错误，已重置为180秒。", L"提示", MB_OK);
                    }
                }
            }

            if (isMouseOnButton(msg, startBtn) && startBtn.isVisible) {
                const poem& selected_poem = poems[selected_poem_idx];
                int char_count = 0;
                for (const wstring& line : selected_poem.lines) {
                    for (wchar_t ch : line) {
                        if (ch != L' ' && ch != L'\n' && ch != L'\r' && ch != L'，' && ch != L'。' && ch != L'！' && ch != L'？')
                            char_count++;
                    }
                }

                if (char_count < selected_board_size * selected_board_size) {
                    wstring errmsg = L"您选择的诗文《" + selected_poem.title + L"》长度不足以填满 "
                        + to_wstring(selected_board_size) + L"x" + to_wstring(selected_board_size)
                        + L" 的棋盘。\n请选择一首更长的诗或一个更小的棋盘。";
                    MessageBox(GetHWnd(), errmsg.c_str(), L"错误", MB_OK | MB_ICONERROR);
                    continue; // 中断本次点击，让用户重新选择
                }
                //传递计时器参数
                playSinglePlayerGame(poems[selected_poem_idx], selected_difficulty, selected_board_size, timer_enabled, time_limit_seconds);
                break;
            }
            for (size_t i = 0; i < poemBtns.size(); ++i) {
                if (isMouseOnButton(msg, poemBtns[i])) { selected_poem_idx = start_index + i; }
            }
            for (size_t i = 0; i < sizeBtns.size(); ++i) {
                if (isMouseOnButton(msg, sizeBtns[i])) {
                    selected_board_size = 6 + i; // 6x6是第一个，所以是6+索引
                    for (auto& btn : sizeBtns) btn.color = LIGHTGRAY; // 全部变灰
                    sizeBtns[i].color = YELLOW; // 选中的变黄
                }
            }
            for (size_t i = 0; i < difficultyBtns.size(); ++i) {
                if (isMouseOnButton(msg, difficultyBtns[i])) {
                    selected_difficulty = i + 1;
                    for (auto& btn : difficultyBtns) btn.color = LIGHTGRAY;
                    difficultyBtns[i].color = YELLOW;
                }
            }
        }
        Sleep(1);
    }
}

void showHintScreen(movabletypeboard& board) {
    Button backBtn = { 650, 520, 120, 40, L"返回游戏", LIGHTGREEN };

    bool viewing_hint = true;
    while (viewing_hint) {
        BeginBatchDraw();
        cleardevice();

        // 显示提示标题
        settextstyle(25, 0, L"微软雅黑");
        outtextxy(50, 30, L"提示 - 原始活字版");

        settextstyle(16, 0, L"宋体");
        outtextxy(50, 70, L"这是正确的活字版排列，请参考后返回游戏继续。");

        // 显示原始活字版
        board.displayOriginal(50, 100, 40);

        drawButton(backBtn);
        EndBatchDraw();

        if (MouseHit()) {
            MOUSEMSG msg = GetMouseMsg();
            if (msg.uMsg == WM_LBUTTONDOWN && isMouseOnButton(msg, backBtn)) {
                viewing_hint = false;
            }
        }
        Sleep(1);
    }
}

void showGiveUpScreen(movabletypeboard& board) {
    Button backBtn = { 650, 520, 120, 40, L"返回菜单", LIGHTGRAY };

    // 获取打乱步骤并反向作为答案
    vector<movabletypeboard::move> moves = board.get_shuffle_moves();
    std::reverse(moves.begin(), moves.end());

    bool viewing_answer = true;
    while (viewing_answer) {
        BeginBatchDraw();
        cleardevice();

        // 显示标题
        settextstyle(25, 0, L"微软雅黑");
        outtextxy(50, 30, L"游戏结束 - 正确答案");

        // 显示原始活字版
        settextstyle(18, 0, L"微软雅黑");
        outtextxy(50, 70, L"正确的活字版：");
        board.displayOriginal(50, 100, 35);

        // 显示解答步骤
        outtextxy(400, 70, L"复原步骤：");
        settextstyle(14, 0, L"宋体");

        int max_steps = min((int)moves.size(), 15); // 最多显示15步
        for (int i = 0; i < max_steps; ++i) {
            const auto& move = moves[i];
            wstring step_text = to_wstring(i + 1) + L". ";

            if (move.type == "row") {
                step_text += L"交换第 " + to_wstring(move.a) + L" 行和第 " + to_wstring(move.b) + L" 行";
            }
            else if (move.type == "col") {
                step_text += L"交换第 " + to_wstring(move.a) + L" 列和第 " + to_wstring(move.b) + L" 列";
            }
            else if (move.type == "cross") {
                step_text += L"交换中心点(" + to_wstring(move.a) + L"," + to_wstring(move.b) +
                    L")和(" + to_wstring(move.c) + L"," + to_wstring(move.d) + L")的十字";
            }
            else if (move.type == "reverse") {
                step_text += L"翻转(" + to_wstring(move.a) + L"," + to_wstring(move.b) +
                    L")到(" + to_wstring(move.c) + L"," + to_wstring(move.d) + L")";
            }

            outtextxy(400, 100 + i * 20, step_text.c_str());
        }

        if (moves.size() > 15) {
            outtextxy(400, 100 + 15 * 20, L"... (还有更多步骤)");
        }

        drawButton(backBtn);
        EndBatchDraw();

        if (MouseHit()) {
            MOUSEMSG msg = GetMouseMsg();
            if (msg.uMsg == WM_LBUTTONDOWN && isMouseOnButton(msg, backBtn)) {
                viewing_answer = false;
            }
        }
        Sleep(1);
    }
}

void playSinglePlayerGame(const poem& p, int difficulty, int boardsize, bool timer_enabled, int time_limit_seconds) {
    movabletypeboard board(p, boardsize);
    board.shuffle(get_shuffle_count_by_difficulty(difficulty));
    const int grid_display_width = 540;
    int cell_size = 55 - boardsize * 2;
    int grid_x = (WINDOW_WIDTH / 2) - (boardsize * cell_size / 2); // 水平居中
    int grid_y = 100;

    enum class SwapMode { ROW_COL, CROSS_SWAP };
    SwapMode current_swap_mode = SwapMode::ROW_COL;
    vector<pair<int, int>> selections;

    int button_area_x = grid_x + boardsize * cell_size + 50; 

    Button modeBtn = { button_area_x, grid_y + 50,  200, 50, L"模式: 行/列交换", LIGHTBLUE ,16};
    Button hintBtn = { button_area_x, grid_y + 120, 200, 50, L"提示", YELLOW };
    Button quitBtn = { button_area_x, grid_y + 190, 200, 50, L"放弃", LIGHTGRAY };

    auto start_time = chrono::steady_clock::now();
    bool game_running = true;

    while (game_running) {
        BeginBatchDraw();
        cleardevice();
        settextstyle(25, 0, L"微软雅黑");
        outtextxy(50, 30, (L"单人模式 - " + p.title).c_str());

        auto now = chrono::steady_clock::now();
        auto duration = chrono::duration_cast<chrono::seconds>(now - start_time);
        if (timer_enabled) {
            long long time_left = time_limit_seconds - duration.count();
            wstring time_str = L"剩余时间: " + to_wstring(time_left) + L" s";
            if (time_left <= 10) settextcolor(RED);
            outtextxy(550, 30, time_str.c_str());
            settextcolor(BLACK);
            if (time_left <= 0) {
                MessageBox(GetHWnd(), L"时间到！挑战失败！", L"时间耗尽", MB_OK | MB_ICONEXCLAMATION);
                showGiveUpScreen(board);
                game_running = false;
            }
        }
        else {
            wstring time_str = L"用时: " + to_wstring(duration.count()) + L" s";
            outtextxy(600, 30, time_str.c_str());
        }

        settextstyle(16, 0, L"宋体");
        if (current_swap_mode == SwapMode::ROW_COL) {
            modeBtn.text = L"模式: 行/列交换";
            outtextxy(grid_x, 80, L"操作: 点击两个同行或同列的格子来交换。");
        }
        else {
            modeBtn.text = L"模式: 十字交换";
            outtextxy(grid_x, 80, L"操作: 点击两个格子交换其十字区域。");
        }

        board.display(grid_x, grid_y, cell_size, selections);

        drawButton(hintBtn);
        drawButton(quitBtn);
        drawButton(modeBtn);
        EndBatchDraw();

        if (!game_running) {
            Sleep(1);
            continue;
        }

        MOUSEMSG msg;
        if (MouseHit() && (msg = GetMouseMsg()).uMsg == WM_LBUTTONDOWN) {
            if (isMouseOnButton(msg, hintBtn)) {
                showHintScreen(board);
            }
            else if (isMouseOnButton(msg, quitBtn)) {
                if (MessageBox(GetHWnd(), L"确定要放弃本局游戏吗？", L"确认", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                    showGiveUpScreen(board);
                    game_running = false;
                }
            }
            else if (isMouseOnButton(msg, modeBtn)) {
                current_swap_mode = (current_swap_mode == SwapMode::ROW_COL) ? SwapMode::CROSS_SWAP : SwapMode::ROW_COL;
                selections.clear();
            }
            else {
                int col = (msg.x - grid_x) / cell_size;
                int row = (msg.y - grid_y) / cell_size;
                if (row >= 0 && row < boardsize && col >= 0 && col < boardsize) {
                    bool already_selected = false;
                    for (const auto& sel : selections)
                        if (sel.first == row && sel.second == col)
                            already_selected = true;

                    if (!already_selected) {
                        selections.push_back({ row, col });
                    }

                    if (selections.size() == 2) {
                        pair<int, int> p1 = selections[0];
                        pair<int, int> p2 = selections[1];

                        if (current_swap_mode == SwapMode::ROW_COL) {
                            if (p1.first == p2.first && p1.second != p2.second) {
                                board.swapcols(p1.second, p2.second);
                            }
                            else if (p1.second == p2.second && p1.first != p2.first) {
                                board.swaprows(p1.first, p2.first);
                            }
                        }
                        else if (current_swap_mode == SwapMode::CROSS_SWAP) {
                            if (!board.swapcross(p1.first, p1.second, p2.first, p2.second)) {
                                MessageBox(GetHWnd(), L"十字交换失败！\n请确保两个点都是有效的十字中心。", L"操作无效", MB_OK | MB_ICONWARNING);
                            }
                        }
                        selections.clear();
                    }
                }
            }
        }

        if (game_running && board.issolved()) {
            if (!timer_enabled || (timer_enabled && duration.count() <= time_limit_seconds)) {
                update_ranking_single(current_user, difficulty, duration.count());
            }
            wstring success_msg = L"恭喜！你用 " + to_wstring(duration.count()) + L" 秒完成了挑战！";
            MessageBox(GetHWnd(), success_msg.c_str(), L"成功", MB_OK);
            game_running = false;
        }
        Sleep(1);
    }
    currentState = AppState::GAME_MODE_SELECT;
}

void update_ranking_multi(const wstring& username, int difficulty, int time_used) {
    if (users.find(username) == users.end()) return;
    User& u = users.at(username);
    if (u.best_multi.at(difficulty) == 0 || time_used < u.best_multi.at(difficulty)) {
        u.best_multi[difficulty] = time_used;
        save_users();
    }
}

void runMultiplayerSetup() {
    int selected_difficulty = 1;
    vector<Button> difficultyBtns = {
        {550, 200, 150, 40, L"简单", YELLOW},
        {550, 250, 150, 40, L"中等", LIGHTGRAY},
        {550, 300, 150, 40, L"困难", LIGHTGRAY}
    };
    Button startBtn = { 300, 400, 200, 60, L"开始挑战", GREEN };
    Button backBtn = { 50, 520, 150, 40, L"返回", LIGHTGRAY };

    while (currentState == AppState::MULTIPLAYER_SETUP) {
        BeginBatchDraw();
        cleardevice();
        settextstyle(40, 0, L"微软雅黑");
        outtextxy(WINDOW_WIDTH / 2 - 160, 80, L"多人模式挑战");

        settextstyle(20, 0, L"宋体");
        outtextxy(100, 180, L"规则说明:");
        outtextxy(120, 220, L"1. 游戏共 4 轮，题板大小会递增。");
        outtextxy(120, 250, L"2. 总挑战限时为 20 分钟 (1200秒)。");
        outtextxy(120, 280, L"3. 请在右侧选择本次挑战的统一难度。");

        outtextxy(550, 160, L"选择难度:");
        for (const auto& btn : difficultyBtns) drawButton(btn);

        drawButton(startBtn);
        drawButton(backBtn);
        EndBatchDraw();

        MOUSEMSG msg;
        if (MouseHit() && (msg = GetMouseMsg()).uMsg == WM_LBUTTONDOWN) {
            for (size_t i = 0; i < difficultyBtns.size(); ++i) {
                if (isMouseOnButton(msg, difficultyBtns[i])) {
                    selected_difficulty = i + 1;
                    for (auto& btn : difficultyBtns) btn.color = LIGHTGRAY;
                    difficultyBtns[i].color = YELLOW;
                }
            }
            if (isMouseOnButton(msg, startBtn)) {
                playMultiplayerGame(selected_difficulty);; // 开始核心游戏流程
                break;
            }
            if (isMouseOnButton(msg, backBtn)) {
                currentState = AppState::GAME_MODE_SELECT;
                break;
            }
        }
        Sleep(1);
    }
}
// 多人游戏核心流程控制 
void playMultiplayerGame(int difficulty) {
    const int TOTAL_ROUNDS = 4;
    long long total_time_limit = 1200; // 20分钟
    int rounds_completed = 0;
    auto total_start_time = chrono::steady_clock::now();

    for (int i = 0; i < TOTAL_ROUNDS; ++i) {
        long long time_elapsed = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - total_start_time).count();
        if (total_time_limit - time_elapsed <= 0) {
            MessageBox(GetHWnd(), L"总时间已耗尽！", L"挑战结束", MB_OK);
            break;
        }

        bool round_won = playSingleMultiplayerRound(i + 1, 6 + i, total_start_time, difficulty);
        if (round_won) {
            rounds_completed++;
        }
        // 按您的要求，即使本轮放弃，也继续下一轮
    }

    long long total_time_used = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - total_start_time).count();
    runMultiplayerSummaryScreen(rounds_completed, total_time_used);
}
//单轮多人游戏
bool playSingleMultiplayerRound(int round_num, int board_size, chrono::steady_clock::time_point total_start_time, int difficulty) {
   
    cleardevice();
    settextstyle(30, 0, L"微软雅黑");
    wstring round_info = L"第 " + to_wstring(round_num) + L" / 4 轮";
    wstring board_info = L"题板大小: " + to_wstring(board_size) + L"x" + to_wstring(board_size);
    outtextxy(WINDOW_WIDTH / 2 - 100, 200, round_info.c_str());
    outtextxy(WINDOW_WIDTH / 2 - 120, 250, board_info.c_str());
    outtextxy(WINDOW_WIDTH / 2 - 150, 350, L"（点击鼠标左键开始本轮）");
    FlushMouseMsgBuffer();
    while (true) {
        if (MouseHit() && GetMouseMsg().uMsg == WM_LBUTTONDOWN) break;
        Sleep(50);
    }

    vector<poem> poems = loadpoems_gui("poems.txt");
    if (poems.empty()) {
        MessageBox(GetHWnd(), L"自由题库为空，无法开始游戏！", L"错误", MB_OK | MB_ICONERROR);
        return false;
    }
    int poem_idx = rand() % poems.size();

    movabletypeboard board(poems[poem_idx], board_size);
    board.shuffle(get_shuffle_count_by_difficulty(difficulty));

    int cell_size = 55 - board_size * 2;
    int grid_x = (WINDOW_WIDTH - board_size * cell_size) / 2;
    int grid_y = 100;

    enum class SwapMode { ROW_COL, CROSS_SWAP };
    SwapMode current_swap_mode = SwapMode::ROW_COL;
    vector<pair<int, int>> selections;

    Button quitBtn = { 680, 520, 100, 40, L"放弃本轮", LIGHTGRAY };
    Button hintBtn = { 680, 460, 100, 40, L"提示", GREEN };
    Button modeBtn = { 50, 520, 200, 40, L"模式: 行/列交换", LIGHTBLUE };

    auto round_start_time = chrono::steady_clock::now();
    bool game_running = true;
    bool hint_used = false;

    while (game_running) {
        BeginBatchDraw();
        cleardevice();
        settextstyle(20, 0, L"微软雅黑");
        outtextxy(50, 30, (L"第" + to_wstring(round_num) + L"轮 - " + poems[poem_idx].title).c_str());
        long long current_total_elapsed = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - total_start_time).count();
        wstring time_str = L"总剩余时间: " + to_wstring(1200 - current_total_elapsed) + L" s";
        outtextxy(550, 30, time_str.c_str());

        settextstyle(16, 0, L"宋体");
        if (current_swap_mode == SwapMode::ROW_COL) {
            modeBtn.text = L"模式: 行/列交换";
            outtextxy(grid_x, 80, L"操作: 点击两个同行或同列的格子来交换。");
        }
        else {
            modeBtn.text = L"模式: 十字交换";
            outtextxy(grid_x, 80, L"操作: 点击两个格子交换其十字区域。");
        }

        board.display(grid_x, grid_y, cell_size, selections);
        drawButton(quitBtn); drawButton(hintBtn); drawButton(modeBtn);
        EndBatchDraw();

        if (1200 - current_total_elapsed <= 0) { return false; }

        if (MouseHit()) {
            MOUSEMSG msg = GetMouseMsg();
            if (msg.uMsg == WM_LBUTTONDOWN) {
                if (isMouseOnButton(msg, quitBtn)) {
                    if (MessageBox(GetHWnd(), L"确定要放弃本轮吗？\n将显示完整答案，本轮判负。", L"确认放弃", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                        showGiveUpScreen(board);
                        return false;
                    }
                }
                else if (isMouseOnButton(msg, hintBtn)) {
                    showHintScreen(board);
                    hint_used = true;
                }
                else if (isMouseOnButton(msg, modeBtn)) {
                    current_swap_mode = (current_swap_mode == SwapMode::ROW_COL) ? SwapMode::CROSS_SWAP : SwapMode::ROW_COL;
                    selections.clear();
                }
                else {
                    int col = (msg.x - grid_x) / cell_size;
                    int row = (msg.y - grid_y) / cell_size;
                    if (row >= 0 && row < board_size && col >= 0 && col < board_size) {
                        bool already_selected = false;
                        for (const auto& sel : selections) if (sel.first == row && sel.second == col) already_selected = true;
                        if (!already_selected) selections.push_back({ row, col });

                        if (selections.size() == 2) {
                            pair<int, int> p1 = selections[0];
                            pair<int, int> p2 = selections[1];

                            if (current_swap_mode == SwapMode::ROW_COL) {
                                if (p1.first == p2.first && p1.second != p2.second) {
                                    board.swapcols(p1.second, p2.second);
                                }
                                else if (p1.second == p2.second && p1.first != p2.first) {
                                    board.swaprows(p1.first, p2.first);
                                }
                            }
                            else if (current_swap_mode == SwapMode::CROSS_SWAP) {
                                if (!board.swapcross(p1.first, p1.second, p2.first, p2.second)) {
                                    MessageBox(GetHWnd(), L"十字交换失败！\n请确保两个点都是有效的十字中心。", L"操作无效", MB_OK | MB_ICONWARNING);
                                }
                            }
                            selections.clear();
                        }
                    }
                }
            }
        }

        if (board.issolved()) {
            if (hint_used) {
                MessageBox(GetHWnd(), L"虽然完成了，但由于您查看了提示，本轮不计分。", L"提示", MB_OK);
                return false;
            }
            auto round_end_time = chrono::steady_clock::now();
            long long round_duration = chrono::duration_cast<chrono::seconds>(round_end_time - round_start_time).count();
            update_ranking_multi(current_user, difficulty, static_cast<int>(round_duration));
            MessageBox(GetHWnd(), L"恭喜！本轮完成！", L"成功", MB_OK);
            return true;
        }
        Sleep(1);
    }
    return false;
}

void runMultiplayerSummaryScreen(int rounds_completed, long long total_time_used) {
    currentState = AppState::MULTIPLAYER_SUMMARY;
    Button backBtn = { 300, 400, 200, 60, L"返回游戏模式", LIGHTGRAY };

    while (currentState == AppState::MULTIPLAYER_SUMMARY) {
        BeginBatchDraw();
        cleardevice();
        settextstyle(40, 0, L"微软雅黑");
        outtextxy(WINDOW_WIDTH / 2 - 160, 150, L"多人模式挑战结束");

        settextstyle(25, 0, L"宋体");
        wstring completed_text = L"完成轮数: " + to_wstring(rounds_completed) + L" / 4";
        wstring time_text = L"总用时: " + to_wstring(total_time_used) + L" 秒";
        outtextxy(WINDOW_WIDTH / 2 - 150, 250, completed_text.c_str());
        outtextxy(WINDOW_WIDTH / 2 - 150, 300, time_text.c_str());

        drawButton(backBtn);
        EndBatchDraw();

        if (MouseHit()) {
            MOUSEMSG msg = GetMouseMsg();
            if (msg.uMsg == WM_LBUTTONDOWN && isMouseOnButton(msg, backBtn)) {
                currentState = AppState::GAME_MODE_SELECT;
            }
        }
        Sleep(1);
    }
}

void runAdventureMenuScreen() {
    Button backBtn = { 50, 520, 150, 40, L"返回游戏模式", LIGHTGRAY };

    vector<tuple<string, wstring, int>> chapters = {
        {"libai", L"壹 · 李白诗篇", 6},
        {"dufu", L"贰 · 杜甫诗篇", 7},
        {"sushi", L"叁 · 苏轼诗篇", 8},
        {"zhuzibaijia", L"肆 · 诸子百家", 9}
    };
    vector<Button> chapterBtns;
    for (size_t i = 0; i < chapters.size(); ++i) {
        chapterBtns.push_back({ 100, 150 + (int)i * 80, 600, 70, get<1>(chapters[i]), LIGHTGRAY });
    }

    while (currentState == AppState::ADVENTURE_MENU) {
        BeginBatchDraw();
        cleardevice();
        settextstyle(40, 0, L"微软雅黑"); outtextxy(WINDOW_WIDTH / 2 - 140, 50, L"活字排诗 · 闯关");

        adventureprogress& prog = user_progress.at(current_user);
        map<string, int> scores = { {"libai", prog.libai_score}, {"dufu", prog.dufu_score}, {"sushi", prog.sushi_score}, {"zhuzibaijia", prog.zhuzibaijia_score} };
        map<string, int> levels = { {"libai", prog.libai_level}, {"dufu", prog.dufu_level}, {"sushi", prog.sushi_level}, {"zhuzibaijia", prog.zhuzibaijia_level} };

        for (size_t i = 0; i < chapterBtns.size(); ++i) {
            const auto& chap_id = get<0>(chapters[i]);
            bool unlocked = is_chapter_unlocked(chap_id, prog);
            chapterBtns[i].color = unlocked ? LIGHTCYAN : DARKGRAY;
            drawButton(chapterBtns[i]);

            settextstyle(16, 0, L"宋体");
            wstring status_text;
            if (unlocked) {
                status_text = L"进度: " + to_wstring(levels.at(chap_id)) + L" 关 | 总分: " + to_wstring(scores.at(chap_id));
            }
            else {
                string prev_chap_id = (i > 0) ? get<0>(chapters[i - 1]) : "";
                if (!prev_chap_id.empty()) {
                    int req = get_unlock_requirement(chap_id, loadpoems_gui(prev_chap_id + ".txt").size());
                    status_text = L"未解锁 (需要前一章总分达到 " + to_wstring(req) + L")";
                }
            }
            outtextxy(chapterBtns[i].x + 20, chapterBtns[i].y + 45, status_text.c_str());
        }

        drawButton(backBtn);
        EndBatchDraw();

        MOUSEMSG msg;
        if (MouseHit() && (msg = GetMouseMsg()).uMsg == WM_LBUTTONDOWN) {
            if (isMouseOnButton(msg, backBtn)) { currentState = AppState::GAME_MODE_SELECT; break; }
            for (size_t i = 0; i < chapterBtns.size(); ++i) {
                if (isMouseOnButton(msg, chapterBtns[i])) {
                    if (is_chapter_unlocked(get<0>(chapters[i]), prog)) {
                        runAdventureChapterScreen(get<0>(chapters[i]), get<1>(chapters[i]), get<2>(chapters[i]));
                        // After chapter screen, loop will break naturally
                    }
                    else {
                        MessageBox(GetHWnd(), L"此章节尚未解锁！", L"提示", MB_OK);
                    }
                }
            }
        }
        Sleep(1);
    }
}

void runAdventureChapterScreen(const string& chapter_id, const wstring& chapter_name, int board_size) {
    vector<poem> poems = loadpoems_gui(chapter_id + ".txt");
    if (poems.empty()) {
        MessageBox(GetHWnd(), (L"无法加载章节题库: " + s2ws(chapter_id) + L".txt").c_str(), L"错误", MB_OK | MB_ICONERROR);
        currentState = AppState::ADVENTURE_MENU;
        return;
    }

    Button backBtn = { 50, 520, 100, 40, L"返回", LIGHTGRAY };

    // Set state for this screen's loop
    currentState = AppState::ADVENTURE_CHAPTER_VIEW;
    while (currentState == AppState::ADVENTURE_CHAPTER_VIEW) {
        BeginBatchDraw();
        cleardevice();
        settextstyle(30, 0, L"微软雅黑"); outtextxy(50, 30, chapter_name.c_str());

        adventureprogress& prog = user_progress.at(current_user);
        map<string, int> levels_completed_map = { {"libai", prog.libai_level}, {"dufu", prog.dufu_level}, {"sushi", prog.sushi_level}, {"zhuzibaijia", prog.zhuzibaijia_level} };
        int levels_completed = levels_completed_map.at(chapter_id);

        vector<Button> levelBtns;
        for (int i = 0; i < (int)poems.size(); ++i) {
            COLORREF color = DARKGRAY;
            wstring level_text = L"第 " + to_wstring(i + 1) + L" 关: " + poems[i].title;

            if (i < levels_completed) { // Completed level
                color = GREEN;
                if (prog.best_scores.count(chapter_id) && i < (int)prog.best_scores.at(chapter_id).size()) {
                    level_text += L" (最高分: " + to_wstring(prog.best_scores.at(chapter_id)[i]) + L")";
                }
            }
            else if (i == levels_completed) { // Next challenge
                color = YELLOW;
                level_text += L" (新挑战)";
            } // Else, it's locked

            Button lBtn = { 100, 80 + i * 50, 600, 45, level_text, color };
            lBtn.isVisible = (i <= levels_completed); // Only show completed and next level
            levelBtns.push_back(lBtn);
            drawButton(lBtn);
        }

        drawButton(backBtn);
        EndBatchDraw();

        MOUSEMSG msg;
        if (MouseHit() && (msg = GetMouseMsg()).uMsg == WM_LBUTTONDOWN) {
            if (isMouseOnButton(msg, backBtn)) { currentState = AppState::ADVENTURE_MENU; break; }
            for (size_t i = 0; i < levelBtns.size(); ++i) {
                if (levelBtns[i].isVisible && isMouseOnButton(msg, levelBtns[i])) {
                    playAdventureGame(poems[i], chapter_id, i + 1, board_size);
                    // After game, break this loop to reload progress data on chapter screen
                    currentState = AppState::ADVENTURE_CHAPTER_VIEW; // Stay to reload
                }
            }
        }
        Sleep(1);
    }
    // After loop finishes (e.g., by pressing back), go back to main adventure menu
    currentState = AppState::ADVENTURE_MENU;
}

void playAdventureGame(const poem& p, const string& chapter_id, int level, int boardsize) {
    movabletypeboard board(p, boardsize);
    int shuffle_c = get_shuffle_count(level);
    board.shuffle(shuffle_c);

    int cell_size = 55 - boardsize * 2;
    int grid_x = (WINDOW_WIDTH - boardsize * cell_size) / 2;
    int grid_y = 80;

    enum class SwapMode { ROW_COL, CROSS_SWAP };
    SwapMode current_swap_mode = SwapMode::ROW_COL;
    vector<pair<int, int>> selections;

    Button quitBtn = { 680, 520, 100, 40, L"放弃", LIGHTGRAY };
    Button hintBtn = { 680, 460, 100, 40, L"提示", GREEN };
    Button modeBtn = { 50, 520, 200, 40, L"模式: 行/列交换", LIGHTBLUE };

    int steps_used = 0;
    auto start_time = chrono::steady_clock::now();
    bool game_running = true;
    bool hint_used = false;

    while (game_running) {
        BeginBatchDraw();
        cleardevice();
        settextstyle(20, 0, L"微软雅黑");
        wstring title = L"第 " + to_wstring(level) + L" 关 - " + p.title;
        outtextxy(50, 30, title.c_str());
        auto now = chrono::steady_clock::now();
        auto duration = chrono::duration_cast<chrono::seconds>(now - start_time).count();
        wstring time_str = L"用时: " + to_wstring(duration) + L" s | 步数: " + to_wstring(steps_used);
        outtextxy(500, 30, time_str.c_str());

        settextstyle(16, 0, L"宋体");
        if (current_swap_mode == SwapMode::ROW_COL) {
            modeBtn.text = L"模式: 行/列交换";
            outtextxy(grid_x, 65, L"操作: 点击两个同行或同列的格子来交换。");
        }
        else {
            modeBtn.text = L"模式: 十字交换";
            outtextxy(grid_x, 65, L"操作: 点击两个格子交换其十字区域。");
        }

        board.display(grid_x, grid_y, cell_size, selections);
        drawButton(quitBtn); drawButton(hintBtn); drawButton(modeBtn);
        EndBatchDraw();

        MOUSEMSG msg;
        if (MouseHit() && (msg = GetMouseMsg()).uMsg == WM_LBUTTONDOWN) {
            if (isMouseOnButton(msg, quitBtn)) {
                if (MessageBox(GetHWnd(), L"放弃挑战将不会获得任何分数，确定吗？", L"确认", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                    showGiveUpScreen(board);
                    game_running = false;
                }
            }
            else if (isMouseOnButton(msg, hintBtn)) {
                showHintScreen(board);
                hint_used = true;
            }
            else if (isMouseOnButton(msg, modeBtn)) {
                current_swap_mode = (current_swap_mode == SwapMode::ROW_COL) ? SwapMode::CROSS_SWAP : SwapMode::ROW_COL;
                selections.clear();
            }
            else {
                int col = (msg.x - grid_x) / cell_size;
                int row = (msg.y - grid_y) / cell_size;
                if (row >= 0 && row < boardsize && col >= 0 && col < boardsize) {
                    bool already_selected = false;
                    for (const auto& sel : selections) if (sel.first == row && sel.second == col) already_selected = true;
                    if (!already_selected) selections.push_back({ row, col });

                    if (selections.size() == 2) {
                        pair<int, int> p1 = selections[0];
                        pair<int, int> p2 = selections[1];
                        bool swapped = false;

                        if (current_swap_mode == SwapMode::ROW_COL) {
                            if (p1.first == p2.first && p1.second != p2.second) {
                                board.swapcols(p1.second, p2.second);
                                swapped = true;
                            }
                            else if (p1.second == p2.second && p1.first != p2.first) {
                                board.swaprows(p1.first, p2.first);
                                swapped = true;
                            }
                        }
                        else if (current_swap_mode == SwapMode::CROSS_SWAP) {
                            if (!board.swapcross(p1.first, p1.second, p2.first, p2.second)) {
                                MessageBox(GetHWnd(), L"十字交换失败！\n请确保两个点都是有效的十字中心。", L"操作无效", MB_OK | MB_ICONWARNING);
                            }
                            else {
                                swapped = true;
                            }
                        }
                        if (swapped) steps_used++;
                        selections.clear();
                    }
                }
            }
        }

        if (game_running && board.issolved()) {
            if (hint_used) {
                MessageBox(GetHWnd(), L"虽然完成了，但由于您查看了提示，本关卡不计分。", L"提示", MB_OK);
            }
            else {
                int final_score = calculate_score(steps_used, duration, shuffle_c, boardsize);
                wstring success_msg = L"恭喜过关！\n用时: " + to_wstring(duration) + L"s\n步数: " + to_wstring(steps_used) + L"步\n获得分数: " + to_wstring(final_score) + L"分";
                MessageBox(GetHWnd(), success_msg.c_str(), L"挑战成功", MB_OK);
                adventureprogress& prog = user_progress.at(current_user);
                if (prog.best_scores.find(chapter_id) == prog.best_scores.end() || (int)prog.best_scores.at(chapter_id).size() < level) {
                    prog.best_scores[chapter_id].resize(level, 0);
                }
                int old_best = prog.best_scores.at(chapter_id)[level - 1];
                if (final_score > old_best) {
                    prog.best_scores.at(chapter_id)[level - 1] = final_score;
                    if (chapter_id == "libai") prog.libai_score += (final_score - old_best);
                    else if (chapter_id == "dufu") prog.dufu_score += (final_score - old_best);
                    else if (chapter_id == "sushi") prog.sushi_score += (final_score - old_best);
                    else if (chapter_id == "zhuzibaijia") prog.zhuzibaijia_score += (final_score - old_best);
                }
                map<string, int*> level_ptr_map = { {"libai", &prog.libai_level}, {"dufu", &prog.dufu_level}, {"sushi", &prog.sushi_level}, {"zhuzibaijia", &prog.zhuzibaijia_level} };
                if (level > *level_ptr_map.at(chapter_id)) {
                    (*level_ptr_map.at(chapter_id))++;
                }
                save_progress();
            }
            game_running = false;
        }
        Sleep(1);
    }
}

int main() {
    initgraph(WINDOW_WIDTH, WINDOW_HEIGHT);
    setlocale(LC_ALL, ""); 
    srand((unsigned)time(NULL));

    load_users();
    load_progress();

    while (currentState != AppState::EXIT) {
        switch (currentState) {
        case AppState::LOGIN: runLoginScreen(); break;
        case AppState::REGISTER: runRegisterScreen(); break;
        case AppState::MAIN_MENU: runMainMenu(); break;
        case AppState::ACCOUNT_MANAGE: runAccountManageScreen(); break;
        case AppState::RANKING: runRankingScreen(); break;
        case AppState::POEM_LIB_MENU: runPoemLibMenu(); break;
        case AppState::GAME_MODE_SELECT: runGameModeSelectScreen(); break;
        case AppState::SINGLE_PLAYER_SETUP: runSinglePlayerSetup(); break;
        case AppState::MULTIPLAYER_SETUP: runMultiplayerSetup(); break;
        case AppState::MULTIPLAYER_SUMMARY: runMultiplayerSummaryScreen(0, 0); break;
        case AppState::ADVENTURE_MENU: runAdventureMenuScreen(); break;
        default:
            currentState = AppState::LOGIN;
            break;
        }
    }
    closegraph();
    return 0;
}

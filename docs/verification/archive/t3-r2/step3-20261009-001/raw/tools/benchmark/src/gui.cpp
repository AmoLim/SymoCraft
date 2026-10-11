#include "gui.h"
#include "benchmark/runner.h"
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlobj.h>
#include <atomic>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <thread>

namespace {
    enum Id { Game = 100, BrowseGame, Output, BrowseOutput, Profile, Focus, Label, Power, Gpu, Clocks,
        Start, Cancel, Retry, Open, ExportZip, ProgressBar, Status, Results };
    std::wstring Text(HWND window)
    {
        std::wstring text(GetWindowTextLengthW(window) + 1, L'\0');
        text.resize(GetWindowTextW(window, text.data(), static_cast<int>(text.size()))); return text;
    }
    std::wstring Phase(const std::string& phase)
    {
        if (phase == "initializing") return L"准备场景";
        if (phase == "warmup") return L"预热";
        if (phase == "sampling") return L"采样";
        if (phase == "exporting" || phase == "finished") return L"校验结果";
        if (phase == "passed") return L"通过";
        if (phase == "cancelled") return L"已取消";
        if (phase == "failed") return L"失败";
        return L"待运行";
    }
    class App {
    public:
        HINSTANCE instance;
        HWND window = nullptr;
        HFONT font = nullptr, title_font = nullptr;
        int dpi = 96;
        std::jthread worker;
        std::mutex mutex;
        Benchmark::Progress snapshot;
        Benchmark::Outcome outcome;
        Benchmark::Config last_config;
        std::string error;
        Benchmark::fs::path exported;
        std::atomic_bool finished = false;
        bool busy = false, exporting = false, close_pending = false, has_session = false;

        explicit App(HINSTANCE value) : instance(value) {}
        ~App()
        {
            if (worker.joinable()) { worker.request_stop(); worker.join(); }
            if (font) DeleteObject(font);
            if (title_font) DeleteObject(title_font);
        }
        App(const App&) = delete;
        App& operator=(const App&) = delete;
        int Px(int value) const { return MulDiv(value, dpi, 96); }
        HWND Control(int id) const { return GetDlgItem(window, id); }
        HWND Add(const wchar_t* klass, const wchar_t* text, DWORD style, int id, int x, int y, int w, int h)
        {
            const auto control = CreateWindowExW(klass == std::wstring(L"EDIT") ? WS_EX_CLIENTEDGE : 0,
                klass, text, WS_CHILD | WS_VISIBLE | style, Px(x), Px(y), Px(w), Px(h), window,
                reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), instance, nullptr);
            if (!control) throw std::runtime_error("Cannot create control");
            SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE); return control;
        }
        void Combo(int id, int x, int y, int w, std::initializer_list<const wchar_t*> choices)
        {
            auto combo = Add(L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_TABSTOP | WS_VSCROLL, id, x, y, w, 200);
            for (const auto choice : choices) SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(choice));
            SendMessageW(combo, CB_SETCURSEL, 0, 0);
        }
        void Init()
        {
            dpi = static_cast<int>(GetDpiForWindow(window));
            font = CreateFontW(-Px(16), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Microsoft YaHei UI");
            title_font = CreateFontW(-Px(24), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Microsoft YaHei UI");
            auto title = Add(L"STATIC", L"SymoCraft Benchmark", 0, -1, 24, 18, 810, 36);
            SendMessageW(title, WM_SETFONT, reinterpret_cast<WPARAM>(title_font), TRUE);
            Add(L"STATIC", L"游戏程序", 0, -1, 24, 72, 100, 24);
            Add(L"EDIT", (Benchmark::ExecutablePath().parent_path() / "SymoCraft.exe").c_str(), WS_TABSTOP | ES_AUTOHSCROLL, Game, 124, 68, 620, 30);
            Add(L"BUTTON", L"选择…", WS_TABSTOP, BrowseGame, 754, 68, 96, 30);
            Add(L"STATIC", L"结果位置", 0, -1, 24, 113, 100, 24);
            Add(L"EDIT", Benchmark::DefaultOutputParent().c_str(), WS_TABSTOP | ES_AUTOHSCROLL, Output, 124, 109, 620, 30);
            Add(L"BUTTON", L"选择…", WS_TABSTOP, BrowseOutput, 754, 109, 96, 30);
            Add(L"STATIC", L"采样协议", 0, -1, 24, 158, 100, 24);
            Combo(Profile, 124, 154, 275, {L"正式基线 · 36 分钟 / 9 轮", L"快速检查 · 1 分钟 / 3 轮"});
            Add(L"STATIC", L"焦点策略", 0, -1, 444, 158, 100, 24);
            Combo(Focus, 548, 154, 302, {L"允许失焦（不支持最小化）", L"严格前台"});
            Add(L"STATIC", L"机器代号", 0, -1, 24, 203, 100, 24);
            Add(L"EDIT", L"unknown", WS_TABSTOP | ES_AUTOHSCROLL, Label, 124, 199, 275, 30);
            SendMessageW(Control(Label), EM_SETLIMITTEXT, 80, 0);
            Add(L"STATIC", L"电源模式", 0, -1, 444, 203, 100, 24);
            Combo(Power, 548, 199, 302, {L"未记录", L"接通电源 / 平衡", L"接通电源 / 高性能", L"电池供电", L"自定义"});
            Add(L"STATIC", L"显卡模式", 0, -1, 24, 248, 100, 24);
            Combo(Gpu, 124, 244, 275, {L"未记录", L"独显直连", L"混合显卡", L"仅集成显卡"});
            Add(L"STATIC", L"频率设置", 0, -1, 444, 248, 100, 24);
            Combo(Clocks, 548, 244, 302, {L"未记录", L"默认 / 无超频降压", L"超频", L"降压", L"其他自定义"});
            Add(L"BUTTON", L"开始采样", WS_TABSTOP | BS_DEFPUSHBUTTON, Start, 24, 294, 126, 36);
            Add(L"BUTTON", L"取消", WS_TABSTOP, Cancel, 162, 294, 90, 36);
            Add(L"BUTTON", L"重试失败轮次", WS_TABSTOP, Retry, 264, 294, 150, 36);
            Add(L"BUTTON", L"打开结果", WS_TABSTOP, Open, 602, 294, 116, 36);
            Add(L"BUTTON", L"导出 ZIP", WS_TABSTOP, ExportZip, 730, 294, 120, 36);
            Add(L"STATIC", L"就绪", SS_LEFT, Status, 24, 346, 826, 46);
            auto bar = Add(PROGRESS_CLASSW, L"", PBS_SMOOTH, ProgressBar, 24, 401, 826, 12);
            SendMessageW(bar, PBM_SETRANGE32, 0, 1000);
            auto list = Add(WC_LISTVIEWW, L"", LVS_REPORT | LVS_SINGLESEL | WS_BORDER | WS_TABSTOP, Results, 24, 431, 826, 195);
            ListView_SetExtendedListViewStyle(list, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_LABELTIP);
            const wchar_t* names[]{L"场景 / 轮次", L"状态", L"P95 (ms)", L"平均 FPS", L"失焦 (s)", L"实际 GPU"};
            const int widths[]{120, 90, 100, 100, 100, 290};
            for (int i = 0; i < 6; ++i) {
                LVCOLUMNW column{}; column.mask = LVCF_TEXT | LVCF_WIDTH;
                column.pszText = const_cast<wchar_t*>(names[i]); column.cx = Px(widths[i]); ListView_InsertColumn(list, i, &column);
            }
            SetTimer(window, 1, 250, nullptr); EnableControls();
        }
        void EnableControls()
        {
            for (int id : {Game, BrowseGame, Output, BrowseOutput, Profile, Focus, Label, Power, Gpu, Clocks, Start}) EnableWindow(Control(id), !busy);
            EnableWindow(Control(Cancel), busy);
            EnableWindow(Control(Retry), !busy && has_session && !outcome.passed);
            EnableWindow(Control(Open), !busy && has_session);
            EnableWindow(Control(ExportZip), !busy && has_session);
        }
        void Browse(bool game)
        {
            if (game) {
                wchar_t file[32768]{};
                OPENFILENAMEW dialog{sizeof(dialog)}; dialog.hwndOwner = window; dialog.lpstrFile = file; dialog.nMaxFile = 32768;
                dialog.lpstrFilter = L"Executable (*.exe)\0*.exe\0"; dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
                if (GetOpenFileNameW(&dialog)) SetWindowTextW(Control(Game), file);
            } else {
                IFileDialog* dialog = nullptr;
                if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog)))) throw std::runtime_error("Cannot create directory picker");
                struct Release { IFileDialog* p; ~Release() { p->Release(); } } release{dialog};
                dialog->SetOptions(FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
                if (SUCCEEDED(dialog->Show(window))) {
                    IShellItem* item = nullptr;
                    if (SUCCEEDED(dialog->GetResult(&item))) {
                        PWSTR path = nullptr;
                        if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path))) { SetWindowTextW(Control(Output), path); CoTaskMemFree(path); }
                        item->Release();
                    }
                }
            }
        }
        void Begin(bool resume, bool export_zip)
        {
            if (busy) return;
            if (worker.joinable()) worker.join();
            if (!resume && !export_zip) {
                Benchmark::Config config;
                config.game = Benchmark::fs::absolute(Text(Control(Game)));
                const auto parent = Text(Control(Output));
                if (parent.empty()) throw std::runtime_error("Result location is empty");
                config.output = Benchmark::fs::absolute(parent) / ("run-" + Benchmark::Timestamp());
                config.quick = SendMessageW(Control(Profile), CB_GETCURSEL, 0, 0) == 1;
                config.focus_policy = SendMessageW(Control(Focus), CB_GETCURSEL, 0, 0) == 0 ? "allow-unfocused" : "strict";
                config.machine_label = Benchmark::Utf8(Text(Control(Label)));
                config.power_mode = Benchmark::Utf8(Text(Control(Power)));
                config.gpu_mode = Benchmark::Utf8(Text(Control(Gpu)));
                config.clocks = Benchmark::Utf8(Text(Control(Clocks)));
                last_config = config; has_session = false; outcome = {};
                ListView_DeleteAllItems(Control(Results));
            }
            finished = false; busy = true; exporting = export_zip; error.clear(); exported.clear(); snapshot = {};
            EnableControls();
            SetWindowTextW(Control(Status), export_zip ? L"正在导出…" : L"正在准备…");
            const auto config = last_config;
            worker = std::jthread([this, config, resume, export_zip](std::stop_token stop) {
                try {
                    if (export_zip) exported = Benchmark::Export(config, stop);
                    else outcome = Benchmark::Run(config, stop, [this](const Benchmark::Progress& update) {
                        std::lock_guard guard(mutex); snapshot = update;
                    }, resume);
                } catch (const std::exception& e) { error = e.what(); }
                finished.store(true, std::memory_order_release);
            });
        }
        void FillResults()
        {
            ListView_DeleteAllItems(Control(Results));
            if (!has_session) return;
            const auto session = Benchmark::ReadYaml(last_config.output / "session.yaml");
            const auto plan = Benchmark::Plan(last_config.quick);
            int row = 0;
            for (const auto& round : plan) {
                const auto record = session["rounds"][row];
                std::wstring title = Benchmark::Wide(round.scene) + L" / " + std::to_wstring(round.repeat);
                LVITEMW item{}; item.mask = LVIF_TEXT; item.iItem = row; item.pszText = title.data(); ListView_InsertItem(Control(Results), &item);
                auto cell = [&](int column, std::wstring text) { ListView_SetItemText(Control(Results), row, column, text.data()); };
                cell(1, Phase(record["state"].as<std::string>("idle")));
                if (record["passed"].as<bool>()) {
                    const auto attempt = record["attempts"].as<unsigned>();
                    const auto name = round.scene + "-" + std::to_string(round.repeat) + "-attempt-" + std::to_string(attempt);
                    const auto result = Benchmark::ReadYaml(last_config.output / name / "result.yaml");
                    auto number = [](double value) { std::wostringstream text; text << std::fixed << std::setprecision(2) << value; return text.str(); };
                    cell(2, number(result["frame_ms"]["p95"].as<double>())); cell(3, number(result["throughput_fps"].as<double>()));
                    cell(4, number(result["focus"]["sample_unfocused_seconds_estimate"].as<double>())); cell(5, Benchmark::Wide(result["gl_renderer"].as<std::string>()));
                }
                ++row;
            }
        }
        void Tick()
        {
            if (!busy) return;
            if (finished.load(std::memory_order_acquire)) {
                worker.join(); busy = false;
                has_session = Benchmark::fs::is_regular_file(last_config.output / "session.yaml");
                EnableControls(); FillResults();
                std::wstring text;
                if (!error.empty()) text = L"操作失败：" + Benchmark::Wide(error);
                else if (exporting) text = L"已导出：" + exported.wstring();
                else if (outcome.passed) text = last_config.quick ? L"快速检查通过（非正式基线）" : L"采样完成，详见 session.yaml 的连续性与环境记录";
                else text = outcome.cancelled ? L"已取消，保留本轮诊断文件。" : L"本轮未通过，后续轮次未启动。详情见结果目录。";
                SetWindowTextW(Control(Status), text.c_str());
                if (!exporting && outcome.passed) SendMessageW(Control(ProgressBar), PBM_SETPOS, 1000, 0);
                if (close_pending) DestroyWindow(window);
                return;
            }
            if (exporting) return;
            Benchmark::Progress progress;
            { std::lock_guard guard(mutex); progress = snapshot; }
            std::wostringstream text;
            text << L"第 " << progress.round << L" / " << progress.total << L" 轮 · " << Phase(progress.phase)
                 << L" · 本轮约 " << static_cast<int>(progress.elapsed) << L" / " << static_cast<int>(progress.duration) << L" 秒\r\n"
                 << Benchmark::Wide(progress.message);
            SetWindowTextW(Control(Status), text.str().c_str());
            const double fraction = progress.duration > 0 ? progress.elapsed / progress.duration : 0;
            const int value = progress.total > 0 ? static_cast<int>(1000 * ((progress.round > 0 ? progress.round - 1 : 0) + fraction) / progress.total) : 0;
            SendMessageW(Control(ProgressBar), PBM_SETPOS, value, 0);
        }
        LRESULT Message(UINT message, WPARAM wparam, LPARAM lparam)
        {
            switch (message) {
            case WM_CREATE: Init(); return 0;
            case WM_CTLCOLORSTATIC:
                SetBkColor(reinterpret_cast<HDC>(wparam), GetSysColor(COLOR_WINDOW));
                return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_WINDOW));
            case WM_COMMAND:
                switch (LOWORD(wparam)) {
                case BrowseGame: Browse(true); break;
                case BrowseOutput: Browse(false); break;
                case Start: Begin(false, false); break;
                case Retry: Begin(true, false); break;
                case ExportZip: Begin(false, true); break;
                case Cancel: if (busy) { worker.request_stop(); SetWindowTextW(Control(Status), L"正在停止并保留诊断…"); } break;
                case Open: if (has_session) ShellExecuteW(window, L"open", last_config.output.c_str(), nullptr, nullptr, SW_SHOWNORMAL); break;
                }
                return 0;
            case WM_TIMER: Tick(); return 0;
            case WM_CLOSE:
                if (busy) { close_pending = true; worker.request_stop(); SetWindowTextW(Control(Status), L"正在停止并退出…"); }
                else DestroyWindow(window);
                return 0;
            case WM_DESTROY: KillTimer(window, 1); PostQuitMessage(0); return 0;
            }
            return DefWindowProcW(window, message, wparam, lparam);
        }
        static LRESULT CALLBACK Procedure(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
        {
            auto* self = reinterpret_cast<App*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (message == WM_NCCREATE) {
                self = static_cast<App*>(reinterpret_cast<CREATESTRUCTW*>(lparam)->lpCreateParams);
                self->window = hwnd; SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            }
            if (!self) return DefWindowProcW(hwnd, message, wparam, lparam);
            if (message == WM_NCDESTROY) {
                SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
                self->window = nullptr;
                return DefWindowProcW(hwnd, message, wparam, lparam);
            }
            try { return self->Message(message, wparam, lparam); }
            catch (const std::exception& error) {
                MessageBoxW(hwnd, Benchmark::Wide(error.what()).c_str(), L"SymoCraft Benchmark", MB_OK | MB_ICONERROR);
                return message == WM_CREATE ? -1 : 0;
            }
        }
    };
}
int RunGui(HINSTANCE instance, int show)
{
    const auto com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    struct Com { bool initialized; ~Com() { if (initialized) CoUninitialize(); } } cleanup{SUCCEEDED(com)};
    INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_PROGRESS_CLASS | ICC_LISTVIEW_CLASSES}; InitCommonControlsEx(&controls);
    App app(instance);
    WNDCLASSW klass{}; klass.hInstance = instance; klass.lpfnWndProc = App::Procedure; klass.lpszClassName = L"SymoCraftBenchmarkRunner";
    klass.hCursor = LoadCursorW(nullptr, IDC_ARROW); klass.hIcon = LoadIconW(nullptr, IDI_APPLICATION); klass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    if (!RegisterClassW(&klass)) return 1;
    const auto dpi = GetDpiForSystem();
    RECT rectangle{0, 0, MulDiv(874, dpi, 96), MulDiv(650, dpi, 96)};
    const DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    AdjustWindowRectExForDpi(&rectangle, style, FALSE, 0, dpi);
    auto window = CreateWindowExW(0, klass.lpszClassName, L"SymoCraft Benchmark", style, CW_USEDEFAULT, CW_USEDEFAULT,
        rectangle.right - rectangle.left, rectangle.bottom - rectangle.top, nullptr, nullptr, instance, &app);
    if (!window) { UnregisterClassW(klass.lpszClassName, instance); return 1; }
    ShowWindow(window, show); UpdateWindow(window);
    MSG message{}; int status = 0;
    while ((status = GetMessageW(&message, nullptr, 0, 0)) > 0) {
        if (!IsDialogMessageW(window, &message)) { TranslateMessage(&message); DispatchMessageW(&message); }
    }
    if (status < 0 && app.window) {
        app.worker.request_stop();
        if (app.worker.joinable()) app.worker.join();
        DestroyWindow(app.window);
    }
    UnregisterClassW(klass.lpszClassName, instance);
    return status < 0 ? 1 : static_cast<int>(message.wParam);
}

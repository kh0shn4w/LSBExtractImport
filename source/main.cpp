#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <windows.h>
#include <conio.h>
#include "SubtitleFile.h"
#include "ConsoleUtils.h"

std::string CenterText(const std::string& text, int consoleWidth) {
    int textLength = static_cast<int>(text.length());
    int padding = (consoleWidth > textLength) ? (consoleWidth - textLength) / 2 : 0;
    return std::string(padding, ' ') + text;
}

int GetConsoleWidth() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    return csbi.srWindow.Right - csbi.srWindow.Left + 1;
}

void CenterConsoleWindow() {
    HWND hWnd = GetConsoleWindow();
    if (hWnd != NULL) {
        RECT rect;
        GetWindowRect(hWnd, &rect);
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;
        int x = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
        int y = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;
        SetWindowPos(hWnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

void PrintHeader() {
    ConsoleUtils::ClearConsole();
    int consoleWidth = GetConsoleWidth();
    ConsoleUtils::PrintColored(CenterText("==============================", consoleWidth), ConsoleUtils::Color::CYAN);
    ConsoleUtils::PrintColored(CenterText("LiS BtS Unpacker", consoleWidth), ConsoleUtils::Color::WHITE);
    ConsoleUtils::PrintColored(CenterText("Ameer Xoshnaw - GamesinKurdish", consoleWidth), ConsoleUtils::Color::CYAN);
    ConsoleUtils::PrintColored(CenterText("==============================", consoleWidth), ConsoleUtils::Color::CYAN);
    std::cout << std::endl;
}

std::vector<std::string> FindLsbFiles(const std::string& directory) {
    std::vector<std::string> lsb_files;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
        if (entry.path().extension() == ".lsb") {
            lsb_files.push_back(entry.path().string());
        }
    }
    return lsb_files;
}

std::vector<std::string> FindTxtFiles(const std::string& directory) {
    std::vector<std::string> txt_files;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
        if (entry.path().extension() == ".txt") {
            txt_files.push_back(entry.path().string());
        }
    }
    return txt_files;
}

std::vector<std::string> FindBkpFiles(const std::string& directory) {
    std::vector<std::string> bkp_files;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
        if (entry.path().extension() == ".bkp") {
            bkp_files.push_back(entry.path().string());
        }
    }
    return bkp_files;
}

void ConvertLsbToTxt(const std::vector<std::string>& lsb_files, bool isUnix) {
    SubtitleFile subtitle;
    for (const auto& file : lsb_files) {
        ConsoleUtils::PrintColored("Processing: " + file, ConsoleUtils::Color::CYAN);
        subtitle.ReadLsbFile(file);
        if (subtitle.badLSB) {
            ConsoleUtils::PrintColored("Error: File is corrupted or invalid: " + file, ConsoleUtils::Color::RED);
            continue;
        }
        subtitle.WriteTxtFile(file, isUnix);
        ConsoleUtils::PrintColored("Converted to TXT: " + std::filesystem::path(file).replace_extension(".txt").string(), ConsoleUtils::Color::CYAN);
        //ShellExecuteA(nullptr, "open", "notepad.exe", std::filesystem::path(file).replace_extension(".txt").string().c_str(), nullptr, SW_SHOW);
    }
}

void ConvertTxtToLsb(const std::string& directory) {
    std::vector<std::string> txt_files = FindTxtFiles(directory);
    if (txt_files.empty()) {
        ConsoleUtils::PrintColored("No .txt files found!", ConsoleUtils::Color::RED);
        return;
    }

    SubtitleFile subtitle;
    for (const auto& file : txt_files) {
        ConsoleUtils::PrintColored("Importing: " + file, ConsoleUtils::Color::CYAN);
        subtitle.ReadTxtFile(file);
        subtitle.WriteLsbFile(file);
        ConsoleUtils::PrintColored("Converted to LSB: " + std::filesystem::path(file).replace_extension(".lsb").string(), ConsoleUtils::Color::CYAN);
        std::filesystem::remove(file);
        ConsoleUtils::PrintColored("Deleted TXT: " + file, ConsoleUtils::Color::WHITE);
    }

    std::vector<std::string> bkp_files = FindBkpFiles(directory);
    for (const auto& bkp_file : bkp_files) {
        std::filesystem::remove(bkp_file);
        ConsoleUtils::PrintColored("Deleted BKP: " + bkp_file, ConsoleUtils::Color::WHITE);
    }
}

int DisplayMenu() {
    int selected = 1;
    bool done = false;
    int consoleWidth = GetConsoleWidth();
    while (!done) {
        PrintHeader();
        ConsoleUtils::PrintColored(CenterText("Select an option (use UP/DOWN arrows, press ENTER):", consoleWidth), ConsoleUtils::Color::WHITE);
        if (selected == 1) {
            ConsoleUtils::PrintColored(CenterText("> 1. Export LSB to TXT", consoleWidth), ConsoleUtils::Color::CYAN);
            ConsoleUtils::PrintColored(CenterText("  2. Import TXT to LSB", consoleWidth), ConsoleUtils::Color::WHITE);
        }
        else {
            ConsoleUtils::PrintColored(CenterText("  1. Export LSB to TXT", consoleWidth), ConsoleUtils::Color::WHITE);
            ConsoleUtils::PrintColored(CenterText("> 2. Import TXT to LSB", consoleWidth), ConsoleUtils::Color::CYAN);
        }

        int key = _getch();
        if (key == 224) {
            key = _getch();
            if (key == 72 && selected > 1) {
                selected--;
            }
            else if (key == 80 && selected < 2) {
                selected++;
            }
        }
        else if (key == 13) {
            done = true;
        }
    }
    return selected;
}

int main(int argc, char* argv[]) {
    CenterConsoleWindow();
    std::string directory = std::filesystem::current_path().string();
    if (argc > 1) {
        directory = argv[1];
        if (!std::filesystem::is_directory(directory)) {
            ConsoleUtils::PrintColored("Error: Invalid directory dropped!", ConsoleUtils::Color::RED);
            ConsoleUtils::PrintColored("Press any key to exit...", ConsoleUtils::Color::CYAN);
            _getch();
            return 1;
        }
    }

    while (true) {
        PrintHeader();
        int choice = DisplayMenu();

        if (choice == 1) {
            ConsoleUtils::PrintColored("Scanning for .lsb files in: " + directory, ConsoleUtils::Color::CYAN);
            std::vector<std::string> lsb_files = FindLsbFiles(directory);
            if (lsb_files.empty()) {
                ConsoleUtils::PrintColored("No .lsb files found!", ConsoleUtils::Color::RED);
            }
            else {
                ConvertLsbToTxt(lsb_files, false);
                ConsoleUtils::PrintColored("Export completed! Edit the .txt files in Notepad.", ConsoleUtils::Color::WHITE);
            }
        }
        else if (choice == 2) {
            ConvertTxtToLsb(directory);
            ConsoleUtils::PrintColored("Import completed!", ConsoleUtils::Color::CYAN);
        }

        ConsoleUtils::PrintColored("Press any key to return to menu...", ConsoleUtils::Color::CYAN);
        _getch();
    }

    return 0;
}
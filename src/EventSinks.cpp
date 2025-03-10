#include <thread>
#include <fstream>
#include <string>
#include <filesystem>
#include <sstream>
#include "EventSinks.h"
#include "GeneralFunctions.h"
#include "FileSystem.h"
#include "UIGfx.h"
#include "mini/ini.h"
#include "mINIHelper.h"

namespace logger = SKSE::log;

namespace eventSinks {
    RE::UI* ui;
    RE::BSInputDeviceManager* idm;
    RE::ConsoleLog* consoleLog;
    std::string consolePath = "";
    std::string consoleCommandEntryPath = "_global.Console.ConsoleInstance.CommandEntry.text";
    std::string consoleCommandHistoryPath = "_global.Console.ConsoleInstance.CommandHistory.text";
    std::string command_getLogFilePaths = "getlogfilepaths"; 
    std::string command_viewLogFile = "viewlogfile";
    std::string command_trackLogFile = "tracklogfile";
    std::string command_stopTrackingLogFile = "stoptrackinglogfile";
    std::string lastCommand = "";
    bool consoleMenuOpen = false;
    bool trackingLogFile = false;
    std::vector<std::filesystem::path> availableLogFilePaths;
    std::filesystem::path currentLogFilePath;
    int iNumLinesToPrint = 50;
    
    std::string GetCommandEntryText() {
        std::string s = "";

        auto menu = ui->GetMenu<RE::Console>();
        if (menu) {
            RE::GFxValue gfx;
            if (menu->uiMovie->GetVariable(&gfx, consoleCommandEntryPath.c_str())) {
                if (gfx.IsString()) {
                    s = gfx.GetString();
                }
            }
        }

        return s;
    }

    std::string GetCommandHistoryText() {
        std::string s = "";

        auto menu = ui->GetMenu<RE::Console>();
        if (menu) {
            RE::GFxValue gfx;
            if (menu->uiMovie->GetVariable(&gfx, consoleCommandHistoryPath.c_str())) {
                if (gfx.IsString()) {
                    s = gfx.GetString();
                }
            }
        }

        return s;
    }

    void SetCommandHistoryText(std::string text) {
        auto menu = ui->GetMenu<RE::Console>();
        if (menu) {
            menu->uiMovie->SetVariable(consoleCommandHistoryPath.c_str(), text.c_str());
        }
    }

    void RemoveErrorMessageFromCommandHistory(std::string errorMsg) {
        //remove the 'script command "akCommand" not found' from history
        std::string historyText = GetCommandHistoryText();
        int size = errorMsg.size();
        std::size_t index = historyText.find(errorMsg);
        //logger::info("removing errorMsg[{}]. Size[{}] index[{}]. Before history\n[{}]", errorMsg, size, index, historyText);

        if (index != std::string::npos) {
            historyText.replace(index, size, "");
            SetCommandHistoryText(historyText);
            //logger::info("removing errorMsg[{}]. Size[{}] index[{}]. After history\n[{}]", errorMsg, size, index, historyText);
        }
    }

    bool PrintLogFileToConsole(std::filesystem::path& path) {
        if (!std::filesystem::exists(path)) {
            std::string msg = std::format("file [{}] not found", path.generic_string());
            consoleLog->Print(msg.c_str());
            return false;
        }

        std::string fileName = (path.filename().string() + ": ");
        std::string contents = fs::GetFileContents(path);
        if (contents != "") {
            std::vector<std::string> lines;

            int iEnd = contents.size() - 1;
            for (int i = 0; i < iNumLinesToPrint; i++) {
                int iStart = contents.rfind("\n", iEnd);
                if (iStart != std::string::npos) {
                    iStart++;
                    std::string line = (fileName + contents.substr(iStart, (iEnd - iStart + 1)));
                    gfuncs::RemoveAllNewLinesFromString(line);
                    lines.push_back(line);
                    iEnd = (iStart - 2);
                }
                else {
                    iStart = 0;
                    std::string line = (fileName + contents.substr(iStart, (iEnd - iStart + 1)));
                    gfuncs::RemoveAllNewLinesFromString(line);
                    lines.push_back(line);
                    break;
                }
            }

            for (int i = lines.size() - 1; i >= 0; i--) {
                consoleLog->Print(lines[i].c_str());
                //logger::trace("s[{}]", lines[i]);
            }
        }
        return true;
    }

    void PrintAvailablePathsToConsole() {
        int size = availableLogFilePaths.size();
        for(int i = 0; i < size; i++){
            std::string s = std::to_string(i) + ": " + availableLogFilePaths[i].generic_string();
            consoleLog->Print(s.c_str());
        }
    }

    void SetAvailableLogPaths() {
        availableLogFilePaths.clear();

        auto logsFolder = SKSE::log::log_directory();
        if (!logsFolder) {
            return;
        }

        std::filesystem::path logsFolderPath = logsFolder.value();
        std::filesystem::path MyGamesPath = logsFolder.value();

        while (MyGamesPath.has_parent_path() && MyGamesPath.filename().string() != "My Games") {
            MyGamesPath = MyGamesPath.parent_path();
        }

        mINI::INIFile file("Data/SKSE/Plugins/ConsoleLogViewer.ini");
        mINI::INIStructure ini;
        file.read(ini);

        int i = 1;
        std::vector<std::string> ignoreStings;
        std::string sKey = "s" + std::to_string(i);
        std::string sPath = mINI::GetIniString(ini, "IgnoredFileNameStrings", sKey, "pathNotFound");
        while (sPath != "pathNotFound") {
            gfuncs::ConvertToLowerCase(sPath);
            logger::trace("ignore string[{}]", sPath);
            ignoreStings.push_back(sPath);
            i++;
            sKey = "s" + std::to_string(i);
            sPath = mINI::GetIniString(ini, "IgnoredFileNameStrings", sKey, "pathNotFound");
        }

        i = 1;
        sKey = "path" + std::to_string(i);
        sPath = mINI::GetIniString(ini, "LogFolders_MyGames", sKey, "pathNotFound");
        while (sPath != "pathNotFound") {
            std::filesystem::path fsPath = MyGamesPath; 
            fsPath.append(sPath);
            if (std::filesystem::exists(fsPath)) {
                logger::info("Adding files in folder [{}]", fsPath.generic_string());
                auto logFiles = fs::GetAllFilesInDirectory(fsPath);
                availableLogFilePaths.insert(availableLogFilePaths.end(), logFiles.begin(), logFiles.end());
            }
            else {
                logger::info("folder [{}] not found", fsPath.generic_string());
            }
            i++;
            sKey = "path" + std::to_string(i);
            sPath = mINI::GetIniString(ini, "LogFolders_MyGames", sKey, "pathNotFound");
        }
        
        std::filesystem::path skyrimRootPath = std::filesystem::current_path();

        i = 1; 
        sKey = "path" + std::to_string(i);
        sPath = mINI::GetIniString(ini, "LogFolders_Skyrim", sKey, "pathNotFound");
        while (sPath != "pathNotFound") {
            std::filesystem::path fsPath = skyrimRootPath;
            fsPath.append(sPath);
            if (std::filesystem::exists(fsPath)) {
                logger::info("Adding files in folder [{}]", fsPath.generic_string());
                auto logFiles = fs::GetAllFilesInDirectory(fsPath);
                availableLogFilePaths.insert(availableLogFilePaths.end(), logFiles.begin(), logFiles.end());
            }
            else {
                logger::info("folder [{}] not found", fsPath.generic_string());
            }
            
            i++;
            sKey = "path" + std::to_string(i);
            sPath = mINI::GetIniString(ini, "LogFolders_Skyrim", sKey, "pathNotFound");
        } 

        i = 0;
        int size = availableLogFilePaths.size();
        for (i; i < size && size > 0; i++) {
            std::string fileName = availableLogFilePaths[i].filename().string(); 
            gfuncs::ConvertToLowerCase(fileName);
            if (fileName == "" || gfuncs::StringContainsStringInVector(ignoreStings, fileName)) {
                availableLogFilePaths.erase((availableLogFilePaths.begin() + i));
                i--;
                size--;
            }
        }

        std::string sTrackedLogFile = mINI::GetIniString(ini, "Main", "sTrackedLogFile", "");
        if (sTrackedLogFile != "") {
            bool foundTrackedLogFile = false;
            for (auto& fsPath : availableLogFilePaths) {
                if (fsPath.generic_string() == sTrackedLogFile) {
                    foundTrackedLogFile = true; 
                    if (std::filesystem::exists(fsPath)) {
                        trackingLogFile = true;
                        currentLogFilePath = fsPath;
                        std::string msg = std::format("tracking [{}]", sTrackedLogFile);
                        PrintLogFileToConsole(currentLogFilePath);
                        logger::info("{}", msg);
                        consoleLog->Print(msg.c_str());
                    }
                    else {
                        logger::error("sTrackedLogFile [{}] not found", sTrackedLogFile);
                    }
                }
            }
            if (!foundTrackedLogFile) {
                logger::error("sTrackedLogFile [{}] not found in available paths", sTrackedLogFile);
            }
        }
    }

    void LoadSettingsFromIni() {
        mINI::INIFile file("Data/SKSE/Plugins/ConsoleLogViewer.ini");
        mINI::INIStructure ini;
        file.read(ini);

        iNumLinesToPrint = mINI::GetIniInt(ini, "Main", "iNumLinesToPrint", 50);
        if (iNumLinesToPrint < 1) {
            iNumLinesToPrint = 1;
        }

        command_getLogFilePaths = mINI::GetIniString(ini, "Commands", "sGetLogFiles", "GetLogFiles");
        command_viewLogFile = mINI::GetIniString(ini, "Commands", "sViewLogFile", "ViewLogFile");
        command_trackLogFile = mINI::GetIniString(ini, "Commands", "sTrackLogFile", "TrackLogFile");
        command_stopTrackingLogFile = mINI::GetIniString(ini, "Commands", "sStopTrackingLogFile", "StopTrackingLogFile");

        std::string cmd1 = std::format("{}", command_getLogFilePaths);
        std::string cmd2 = std::format("{} <logFileIndex>", command_viewLogFile);
        std::string cmd3 = std::format("{} <logFileIndex>", command_trackLogFile);
        std::string cmd4 = std::format("{}", command_stopTrackingLogFile);
        consoleLog->Print("Console Log Viewer installed. New console commands are:");
        consoleLog->Print(cmd1.c_str());
        consoleLog->Print(cmd2.c_str());
        consoleLog->Print(cmd3.c_str());
        consoleLog->Print(cmd4.c_str());

        logger::info("Console Log Viewer installed. New console commands are:");
        logger::info("{}", cmd1);
        logger::info("{}", cmd2);
        logger::info("{}", cmd3);
        logger::info("{}", cmd4);

        gfuncs::ConvertToLowerCase(command_getLogFilePaths);
        gfuncs::ConvertToLowerCase(command_viewLogFile);
        gfuncs::ConvertToLowerCase(command_trackLogFile);
        gfuncs::ConvertToLowerCase(command_stopTrackingLogFile);
    }

    void TrackFile(int index) {
        int max = availableLogFilePaths.size() - 1;
        if (index < 0 || index > max) {
            std::string msg = std::format("File index[{}] is not in range. Range is 0 to {}", index, max);
            consoleLog->Print(msg.c_str());
            return;
        }

        trackingLogFile = true;
        currentLogFilePath = availableLogFilePaths[index];
        if (PrintLogFileToConsole(currentLogFilePath)) {
            mINI::INIFile file("Data/SKSE/Plugins/ConsoleLogViewer.ini");
            mINI::INIStructure ini;
            file.read(ini);
            ini["Main"]["sTrackedLogFile"] = currentLogFilePath.generic_string();
            file.write(ini);
        }
    }

    void StopTrackingFile() {
        trackingLogFile = false;
        mINI::INIFile file("Data/SKSE/Plugins/ConsoleLogViewer.ini");
        mINI::INIStructure ini;
        file.read(ini);
        ini["Main"]["sTrackedLogFile"] = "";
        file.write(ini);
    }

    void ProcessCommand(std::string command) {
        //wait for commandHistory to be set after entering command
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        std::string lcommand = command; 
        gfuncs::ConvertToLowerCase(lcommand);

        //std::string lastMsg = consoleLog->lastMessage;
        //logger::info("lastMsg[{}]", lastMsg);

        int max = availableLogFilePaths.size() - 1;
        //logger::info("command[{}]", command);
        bool commandRecognized = false;

        if (lcommand == command_getLogFilePaths) {
            commandRecognized = true;
            PrintAvailablePathsToConsole();
        }
        else if (lcommand.find(command_viewLogFile) != std::string::npos) {
            commandRecognized = true;
            size_t i = lcommand.find(" ");
            if (i != std::string::npos) {
                i++;
                int index = std::stoi(lcommand.substr(i));
                //logger::info("index[{}]", index);
                if (index < 0 || index > max) {
                    std::string msg = std::format("File index[{}] is not in range. Range is 0 to {}", index, max);
                    consoleLog->Print(msg.c_str());
                }
                else {
                    PrintLogFileToConsole(availableLogFilePaths[index]);
                }
            }
        }
        else if (lcommand.find(command_trackLogFile) != std::string::npos) {
            commandRecognized = true;
            size_t i = lcommand.find(" ");
            if (i != std::string::npos) {
                i++;
                int index = std::stoi(lcommand.substr(i));
                //logger::info("index[{}]", index);
                if (index < 0 || index > max) {
                    std::string msg = std::format("File index[{}] is not in range. Range is 0 to {}", index, max);
                    consoleLog->Print(msg.c_str());
                }
                else {
                    TrackFile(index);
                }
            }
        }
        else if (lcommand == command_stopTrackingLogFile) {
            commandRecognized = true;
            StopTrackingFile();
        }

        if (commandRecognized) {
            std::string errorMsg = ("Script command \"" + command + "\" not found.");
            RemoveErrorMessageFromCommandHistory(errorMsg);
        }
    }

    class InputEventSink : public RE::BSTEventSink<RE::InputEvent*> {

    public:
        bool sinkAdded = false;

        RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* eventPtr, RE::BSTEventSource<RE::InputEvent*>*) {

            //logger::trace("input event");

            //don't want to send ui select events to papyrus if message box context is open, only when selecting item.
            if (ui->IsMenuOpen(RE::MessageBoxMenu::MENU_NAME)) {
                return RE::BSEventNotifyControl::kContinue;
            }

            auto* event = *eventPtr;
            if (!event) {
                return RE::BSEventNotifyControl::kContinue;
            }

            if (event->eventType == RE::INPUT_EVENT_TYPE::kButton) {
                auto* buttonEvent = event->AsButtonEvent();
                if (buttonEvent) {
                    if (buttonEvent->IsDown()) {
                        if (buttonEvent->GetIDCode() == 28) { //enter key pressed
                            std::string command = GetCommandEntryText();
                            std::thread t(ProcessCommand, command);
                            t.detach();
                        }
                    }
                }
            }

            return RE::BSEventNotifyControl::kContinue;
        }
    };
    InputEventSink* inputEventSink;

    struct MenuOpenCloseEventSink : public RE::BSTEventSink<RE::MenuOpenCloseEvent> {
        bool sinkAdded = false;

        RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*/*source*/) {
            if (!event) {
                //logger::warn("MenuOpenClose Event doesn't exist");
                return RE::BSEventNotifyControl::kContinue;
            }

            auto menuName = event->menuName;
            auto opening = event->opening;
            //logger::trace("MenuOpenCloseEvent menu[{}] opening[{}]", menuName, opening);

            if (menuName == RE::Console::MENU_NAME) {
                if (opening) {
                    consoleMenuOpen = true;

                    if (!inputEventSink->sinkAdded) {
                        inputEventSink->sinkAdded = true;
                        idm->AddEventSink(inputEventSink);
                    }

                    if (trackingLogFile) {
                        PrintLogFileToConsole(currentLogFilePath);
                    }
                }
                else {
                    consoleMenuOpen = false;
                    if (inputEventSink->sinkAdded) {
                        inputEventSink->sinkAdded = false;
                        idm->RemoveEventSink(inputEventSink);
                    }
                }
            }
            return RE::BSEventNotifyControl::kContinue;
        }
    };
    MenuOpenCloseEventSink* menuOpenCloseEventSink;
   
    void Install() {
        logger::trace("Called");
        
        ui = RE::UI::GetSingleton();
        if (!ui) {
            logger::critical("ui not found, aborting install");
            return;
        }

        idm = RE::BSInputDeviceManager::GetSingleton();
        if (!idm) {
            logger::critical("input device manager not found, aborting install");
            return;
        }

        consoleLog = RE::ConsoleLog::GetSingleton();
        if (!consoleLog) {
            logger::critical("consoleLog not found, aborting install");
            return;
        }

        LoadSettingsFromIni();
        SetAvailableLogPaths();

        if (!menuOpenCloseEventSink) { menuOpenCloseEventSink = new MenuOpenCloseEventSink(); }
        if (!inputEventSink) { inputEventSink = new InputEventSink(); }

        if (!menuOpenCloseEventSink->sinkAdded) {
            ui->AddEventSink<RE::MenuOpenCloseEvent>(menuOpenCloseEventSink);
        } 
    }
}
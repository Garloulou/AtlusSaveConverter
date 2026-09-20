#pragma once

#include "models/GameProfile.hpp"
#include "core/SteamDetector.hpp"
#include <string>
#include <vector>
#include <chrono>

namespace Ps4Atlus {

enum class LogLevel {
    Info,
    Success,
    Warning,
    Error
};

struct LogEntry {
    std::string timestamp;
    std::string message;
    LogLevel level;
};

struct DetectedSave {
    std::string filename;
    std::string fullPath;
    uintmax_t sizeBytes = 0;
    int sourceSlot = 1;
    int targetSlot = 1;
    bool isAigis = false;
    bool isSystem = false;
    std::string extraDetails;
    std::string targetFilename;
    bool isProcessed = false;
    bool isSuccess = false;
    std::string errorDetails;
};

class MainWindow {
public:
    MainWindow();
    ~MainWindow() = default;

    void Render();
    void SetSelectedGame(int index);

private:
    void ScanSourceSaves();
    void AutoDetectPcPath();
    void RefreshSteamAccounts();
    void StartChainExport();
    void UpdateChainExport();
    void AddLog(const std::string& message, LogLevel level = LogLevel::Info);

    std::vector<GameProfile> m_games;
    int m_selectedGameIndex = 0; // Persona 3 Reload
    TargetPlatform m_targetPlatform = TargetPlatform::Steam;

    // Detected Steam accounts
    std::vector<SteamAccount> m_steamAccounts;
    int m_selectedSteamAccountIndex = 0;

    // Source & Destination paths
    char m_sourcePath[512] = "";
    char m_destPath[512] = "";
    bool m_createBackup = true;
    bool m_removeDlcFlags = true;

    // Detected saves for chain export
    std::vector<DetectedSave> m_detectedSaves;

    // Chain export progress
    bool m_isConverting = false;
    size_t m_currentExportIndex = 0;
    float m_totalProgress = 0.0f;
    std::string m_currentStatusText;
    std::chrono::steady_clock::time_point m_lastStepTime;

    // Console logs
    std::vector<LogEntry> m_logs;
    bool m_autoScrollLog = true;
};

} // namespace Ps4Atlus

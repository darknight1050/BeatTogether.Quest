#include <iostream>
#include <fstream>
#include <string_view>

#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "beatsaber-hook/shared/config/config-utils.hpp"

#include "System/Action.hpp"

#include "System/Threading/CancellationToken.hpp"
using namespace System::Threading;

#include "System/Threading/Tasks/Task_1.hpp"
using namespace System::Threading::Tasks;

#include "GlobalNamespace/PlatformAuthenticationTokenProvider.hpp"
#include "GlobalNamespace/AuthenticationToken.hpp"
#include "GlobalNamespace/MasterServerEndPoint.hpp"
#include "GlobalNamespace/MenuRpcManager.hpp"
#include "GlobalNamespace/BeatmapIdentifierNetSerializable.hpp"
#include "GlobalNamespace/MultiplayerLevelLoader.hpp"
#include "GlobalNamespace/IPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/MultiplayerModeSelectionViewController.hpp"
#include "GlobalNamespace/MainMenuViewController.hpp"
#include "GlobalNamespace/MainSystemInit.hpp"
#include "GlobalNamespace/NetworkConfigSO.hpp"
#include "GlobalNamespace/HostLobbySetupViewController.hpp"
#include "GlobalNamespace/HostLobbySetupViewController_CannotStartGameReason.hpp"
#include "GlobalNamespace/AdditionalContentModel.hpp"
#include "GlobalNamespace/MultiplayerLevelSelectionFlowCoordinator.hpp"
#include "GlobalNamespace/LevelSelectionFlowCoordinator_State.hpp"
#include "GlobalNamespace/SongPackMask.hpp"
#include "GlobalNamespace/BeatmapDifficultyMask.hpp"
#include "GlobalNamespace/UserCertificateValidator.hpp"
#include "GlobalNamespace/MultiplayerLevelLoader.hpp"
#include "GlobalNamespace/GameplayModifiers.hpp"
#include "GlobalNamespace/IConnectedPlayer.hpp"
#include "GlobalNamespace/HMTask.hpp"

using namespace GlobalNamespace;

#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Transform.hpp"
#include "UnityEngine/Resources.hpp"
#include "TMPro/TextMeshProUGUI.hpp"

#include "songloader/shared/API.hpp"
#include "songdownloader/shared/BeatSaverAPI.hpp"
#include "questui/shared/CustomTypes/Components/MainThreadScheduler.hpp"

#ifndef HOST_NAME
#error "Define HOST_NAME!"
#endif

#ifndef PORT
#error "Define PORT!"
#endif

#ifndef STATUS_URL
#error "Define STATUS_URL!"
#endif

Logger& getLogger();

static ModInfo modInfo;

class ModConfig {
    public:
        ModConfig() : hostname(HOST_NAME), port(PORT), statusUrl(STATUS_URL), button("Modded\nOnline") {}
        // Should be called after modification of the fields has already taken place.
        // Creates the C# strings for the configuration.
        void load() {
            createStrings();
        }
        // Read MUST be called after load.
        void read(std::string_view filename) {
            // Each time we read, we must start by cleaning up our old strings.
            // TODO: This may not be necessary if we only ever plan to load our configuration once.
            invalidateStrings();
            std::ifstream file(filename.data());
            if (!file) {
                getLogger().debug("No readable configuration at %s.", filename.data());
            } else {
                file >> hostname >> port >> statusUrl;
                button = hostname;
            }
            file.close();
        }
        inline int get_port() const {
            return port;
        }
        inline Il2CppString* get_hostname() const {
            getLogger().info("Host name: " + to_utf8(csstrtostr(hostnameStr)));
            return valid ? hostnameStr : nullptr;
        }
        inline Il2CppString* get_button() const {
            return valid ? buttonStr : nullptr;
        }
        inline Il2CppString* get_statusUrl() const {
            getLogger().info("Status URL: " + to_utf8(csstrtostr(statusUrlStr)));
            return valid ? statusUrlStr : nullptr;
        }
    private:
        // Invalidates all Il2CppString* pointers we have
        void invalidateStrings() {
            free(hostnameStr);
            free(buttonStr);
            free(statusUrlStr);
            valid = false;
        }
        // Creates all Il2CppString* pointers we need
        void createStrings() {
            hostnameStr = RET_V_UNLESS(getLogger(), il2cpp_utils::createcsstr(hostname, il2cpp_utils::StringType::Manual));
            buttonStr = RET_V_UNLESS(getLogger(), il2cpp_utils::createcsstr(button, il2cpp_utils::StringType::Manual));
            statusUrlStr = RET_V_UNLESS(getLogger(), il2cpp_utils::createcsstr(statusUrl, il2cpp_utils::StringType::Manual));
            // If we can make the strings okay, we are valid.
            valid = true;
        }
        bool valid;
        int port;
        std::string hostname;
        std::string button;
        std::string statusUrl;
        // C# strings of the configuration strings.
        // TODO: Consider replacing the C++ string fields entirely, as they serve no purpose outside of debugging.
        Il2CppString* hostnameStr = nullptr;
        Il2CppString* buttonStr = nullptr;
        Il2CppString* statusUrlStr = nullptr;
};

static ModConfig config;

Logger& getLogger()
{
    static Logger* logger = new Logger(modInfo, LoggerOptions(false, true));
    return *logger;
}

MAKE_HOOK_OFFSETLESS(PlatformAuthenticationTokenProvider_GetAuthenticationToken, Task_1<GlobalNamespace::AuthenticationToken>*, PlatformAuthenticationTokenProvider* self)
{
    getLogger().debug("Returning custom authentication token!");
    return Task_1<AuthenticationToken>::New_ctor(AuthenticationToken(
        AuthenticationToken::Platform::OculusQuest,
        self->userId,
        self->userName,
        Array<uint8_t>::NewLength(0)
    ));
}

MAKE_HOOK_OFFSETLESS(MainSystemInit_Init, void, MainSystemInit* self) {
    MainSystemInit_Init(self);
    auto* networkConfig = self->networkConfig;

    getLogger().info("Overriding master server end point . . .");
    getLogger().info("Original status URL: " + to_utf8(csstrtostr(networkConfig->masterServerStatusUrl)));
    // If we fail to make the strings, we should fail silently
    // This could also be replaced with a CRASH_UNLESS call, if you want to fail verbosely.
    networkConfig->masterServerHostName = RET_V_UNLESS(getLogger(), config.get_hostname());
    networkConfig->masterServerPort = RET_V_UNLESS(getLogger(), config.get_port());
    networkConfig->masterServerStatusUrl = RET_V_UNLESS(getLogger(), config.get_statusUrl());
}

MAKE_HOOK_OFFSETLESS(UserCertificateValidator_ValidateCertificateChainInternal, void, UserCertificateValidator* self, GlobalNamespace::MasterServerEndPoint* endPoint, System::Security::Cryptography::X509Certificates::X509Certificate2* certificate, ::Array<::Array<uint8_t>*>* certificateChain)
{
    // TODO: Support disabling the mod if official multiplayer is ever fixed
    // It'd be best if we do certificate validation here...
    // but for now we'll just skip it.
}

// Disable the quick play button
MAKE_HOOK_OFFSETLESS(MultiplayerModeSelectionViewController_DidActivate, void, MultiplayerModeSelectionViewController* self, bool firstActivation, bool addedToHierarchy, bool systemScreenEnabling)
{
    if (firstActivation)
    {
        static auto* searchPath = il2cpp_utils::createcsstr("Buttons/QuickPlayButton", il2cpp_utils::StringType::Manual);
        UnityEngine::Transform* transform = self->get_gameObject()->get_transform();
        UnityEngine::GameObject* quickPlayButton = transform->Find(searchPath)->get_gameObject();
        quickPlayButton->SetActive(false);
    }

    MultiplayerModeSelectionViewController_DidActivate(self, firstActivation, addedToHierarchy, systemScreenEnabling);
}

// Change the "Online" menu text to "Modded Online"
MAKE_HOOK_OFFSETLESS(MainMenuViewController_DidActivate, void, MainMenuViewController* self, bool firstActivation, bool addedToHierarchy, bool systemScreenEnabling)
{   
    // Find the GameObject for the online button's text
    static auto* searchPath = il2cpp_utils::createcsstr("MainContent/OnlineButton", il2cpp_utils::StringType::Manual);
    static auto* textName = il2cpp_utils::createcsstr("Text", il2cpp_utils::StringType::Manual);
    UnityEngine::Transform* transform = self->get_gameObject()->get_transform();
    UnityEngine::GameObject* onlineButton = transform->Find(searchPath)->get_gameObject();
    UnityEngine::GameObject* onlineButtonTextObj = onlineButton->get_transform()->Find(textName)->get_gameObject();

    // Set the "Modded Online" text every time so that it doesn't change back
    TMPro::TextMeshProUGUI* onlineButtonText = onlineButtonTextObj->GetComponent<TMPro::TextMeshProUGUI*>();
    // If we fail to get any valid button text, crash verbosely.
    // TODO: This could be replaced with a non-intense crash, if we can ensure that DidActivate also works as intended.
    onlineButtonText->set_text(CRASH_UNLESS(config.get_button()));
    
    // Align the Text in the Center
    onlineButtonText->set_alignment(TMPro::TextAlignmentOptions::Center);

    MainMenuViewController_DidActivate(self, firstActivation, addedToHierarchy, systemScreenEnabling);
}

static bool isMissingLevel = false;

// This hook makes sure to grey-out the play button so that players can't start a level that someone doesn't have.
// This prevents crashes.
MAKE_HOOK_OFFSETLESS(HostLobbySetupViewController_SetPlayersMissingLevelText, void, HostLobbySetupViewController* self, Il2CppString* playersMissingLevelText) {
    getLogger().info("HostLobbySetupViewController_SetPlayersMissingLevelText");
    if(playersMissingLevelText && !isMissingLevel) {
        getLogger().info("Disabling start game as missing level text exists . . .");
        isMissingLevel = true;
        self->SetStartGameEnabled(false, HostLobbySetupViewController::CannotStartGameReason::DoNotOwnSong);
    }   else if(!playersMissingLevelText && isMissingLevel)   {
        getLogger().info("Enabling start game as missing level text does not exist . . .");
        isMissingLevel = false;
        self->SetStartGameEnabled(true, HostLobbySetupViewController::CannotStartGameReason::None);
    }

    HostLobbySetupViewController_SetPlayersMissingLevelText(self, playersMissingLevelText);
}

// Prevent the button becoming shown when we're force disabling it, as pressing it would crash
MAKE_HOOK_OFFSETLESS(HostLobbySetupViewController_SetStartGameEnabled, void, HostLobbySetupViewController* self, bool startGameEnabled, HostLobbySetupViewController::CannotStartGameReason cannotStartGameReason) {
    getLogger().info(string_format("HostLobbySetupViewController_SetStartGameEnabled. Enabled: %d. Reason: %d", startGameEnabled, cannotStartGameReason));
    if(isMissingLevel && cannotStartGameReason == HostLobbySetupViewController::CannotStartGameReason::None) {
        getLogger().info("Game attempted to enable the play button when the level was missing, stopping it!");
        startGameEnabled = false;
        cannotStartGameReason = HostLobbySetupViewController::CannotStartGameReason::DoNotOwnSong;
    }
    HostLobbySetupViewController_SetStartGameEnabled(self, startGameEnabled, cannotStartGameReason);
}

MAKE_HOOK_OFFSETLESS(MultiplayerLevelSelectionFlowCoordinator_Setup, void, MultiplayerLevelSelectionFlowCoordinator* self, LevelSelectionFlowCoordinator::State* state, SongPackMask songPackMask, BeatmapDifficultyMask allowedBeatmapDifficultyMask, Il2CppString* actionText, Il2CppString* titleText) {
    getLogger().info("Enabling custom songs in multiplayer . . .");
    MultiplayerLevelSelectionFlowCoordinator_Setup(self, state, SongPackMask::get_all(), allowedBeatmapDifficultyMask, actionText, titleText);
}

// Show the custom levels tab in multiplayer
MAKE_HOOK_OFFSETLESS(LevelSelectionNavigationController_Setup, void, LevelSelectionNavigationController* self,
    SongPackMask songPackMask, BeatmapDifficultyMask allowedBeatmapDifficultyMask, Il2CppObject* notAllowedCharacteristics,
    bool hidePacksIfOneOrNone, bool hidePracticeButton, bool showPlayerStatsInDetailView, Il2CppString* actionButtonText, IBeatmapLevelPack* levelPackToBeSelectedAfterPrecent,
    SelectLevelCategoryViewController::LevelCategory startLevelCategory, IPreviewBeatmapLevel* beatmapLevelToBeSelectedAfterPresent, bool enableCustomLevels) {
    getLogger().info("LevelSelectionNavigationController_Setup enabling custom songs . . .");
    LevelSelectionNavigationController_Setup(self, songPackMask, allowedBeatmapDifficultyMask, notAllowedCharacteristics, hidePacksIfOneOrNone, hidePracticeButton, showPlayerStatsInDetailView,
                                             actionButtonText, levelPackToBeSelectedAfterPrecent, startLevelCategory, beatmapLevelToBeSelectedAfterPresent, true);
}


bool IsCustomLevel(const std::string& levelId) {
    return levelId.starts_with(RuntimeSongLoader::API::GetCustomLevelsPrefix());
}

bool HasSong(std::string levelId) {
    for(auto& song : RuntimeSongLoader::API::GetLoadedSongs()) {
        if(to_utf8(csstrtostr(song->levelID)) == levelId) {
            return true;
        }
    }
    return false;
}

std::string GetHash(const std::string& levelId) {
    return levelId.substr(RuntimeSongLoader::API::GetCustomLevelsPrefix().length(), levelId.length() - RuntimeSongLoader::API::GetCustomLevelsPrefix().length());
}

MAKE_HOOK_OFFSETLESS(MultiplayerLevelLoader_LoadLevel, void, MultiplayerLevelLoader* self, BeatmapIdentifierNetSerializable* beatmapId, GameplayModifiers* gameplayModifiers, float initialStartTime) {
    std::string levelId = to_utf8(csstrtostr(beatmapId->levelID));
    getLogger().info("MultiplayerLevelLoader_LoadLevel: %s", levelId.c_str());
    if(IsCustomLevel(levelId)) {
        if(HasSong(levelId)) {
            MultiplayerLevelLoader_LoadLevel(self, beatmapId, gameplayModifiers, initialStartTime);
        } else {
            BeatSaver::API::GetBeatmapByHashAsync(GetHash(levelId), 
                [self, beatmapId, gameplayModifiers, initialStartTime] (std::optional<BeatSaver::Beatmap> beatmapOpt) {
                    if(beatmapOpt.has_value()) {
                        auto beatmap = beatmapOpt.value();
                        auto beatmapName = beatmap.GetName();
                        getLogger().info("Downloading map: %s", beatmapName.c_str());
                        BeatSaver::API::DownloadBeatmapAsync(beatmap, 
                            [self, beatmapId, gameplayModifiers, initialStartTime, beatmapName] (bool error) {
                                if(error) {
                                    getLogger().info("Failed downloading map: %s", beatmapName.c_str());
                                } else {
                                    getLogger().info("Downloaded map: %s", beatmapName.c_str());
                                    QuestUI::MainThreadScheduler::Schedule(
                                        [self, beatmapId, gameplayModifiers, initialStartTime] {
                                            RuntimeSongLoader::API::RefreshSongs(false,
                                                [self, beatmapId, gameplayModifiers, initialStartTime] (const std::vector<GlobalNamespace::CustomPreviewBeatmapLevel*>& songs) {
                                                    self->loaderState = MultiplayerLevelLoader::MultiplayerBeatmapLoaderState::NotLoading;
                                                    MultiplayerLevelLoader_LoadLevel(self, beatmapId, gameplayModifiers, initialStartTime);
                                                }
                                            );
                                        }
                                    );
                                }
                            }
                        );
                    }
                }
            );
        }
    } else {
        MultiplayerLevelLoader_LoadLevel(self, beatmapId, gameplayModifiers, initialStartTime);
    }
}

MAKE_HOOK_OFFSETLESS(NetworkPlayerEntitlementChecker_GetEntitlementStatus, Task_1<EntitlementsStatus>*, NetworkPlayerEntitlementChecker* self, Il2CppString* levelIdCS) {
    std::string levelId = to_utf8(csstrtostr(levelIdCS));
    getLogger().info("NetworkPlayerEntitlementChecker_GetEntitlementStatus: %s", levelId.c_str());
    if(IsCustomLevel(levelId)) {
        if(HasSong(levelId)) {
            return Task_1<EntitlementsStatus>::New_ctor(EntitlementsStatus::Ok);
        } else {
            auto task = Task_1<EntitlementsStatus>::New_ctor();
            BeatSaver::API::GetBeatmapByHashAsync(GetHash(levelId), 
                [task] (std::optional<BeatSaver::Beatmap> beatmap) {
                    QuestUI::MainThreadScheduler::Schedule(
                        [task, beatmap] {
                            if(beatmap.has_value()) {
                                task->TrySetResult(EntitlementsStatus::NotDownloaded);
                            } else {
                                task->TrySetResult(EntitlementsStatus::NotOwned);
                            }
                        }
                    );
                }
            );
            return task;
        }
    } else {
        return NetworkPlayerEntitlementChecker_GetEntitlementStatus(self, levelIdCS);
    }
}

MAKE_HOOK_OFFSETLESS(NetworkPlayerEntitlementChecker_GetPlayerLevelEntitlementsAsync, Task_1<EntitlementsStatus>*, NetworkPlayerEntitlementChecker* self, IConnectedPlayer* player, Il2CppString* levelId, CancellationToken token) {
    auto task = Task_1<EntitlementsStatus>::New_ctor();
    HMTask::New_ctor(il2cpp_utils::MakeDelegate<System::Action*>(classof(System::Action*),
        (std::function<void()>)[self, player, levelId, token, task] { 
            auto returnTask = NetworkPlayerEntitlementChecker_GetPlayerLevelEntitlementsAsync(self, player, levelId, token);
            auto result = returnTask->get_Result();
            if(result == EntitlementsStatus::NotDownloaded) {
                result = EntitlementsStatus::Ok;
            }
            task->TrySetResult(result);
        }
    ), nullptr)->Run();
    return task;
}

extern "C" void setup(ModInfo& info)
{
    info.id = ID;
    info.version = VERSION;
    modInfo = info;
}

extern "C" void load()
{
    std::string path = Configuration::getConfigFilePath(modInfo);
    path.replace(path.length() - 4, 4, "cfg");

    getLogger().info("Config path: %s", path.c_str());
    config.read(path);
    // Load and create all C# strings after we attempt to read it.
    // If we failed to read it, we will have default values.
    // If we fail to create the strings, valid will be false.
    config.load();

    il2cpp_functions::Init();

    INSTALL_HOOK_OFFSETLESS(getLogger(), PlatformAuthenticationTokenProvider_GetAuthenticationToken,
        il2cpp_utils::FindMethod("", "PlatformAuthenticationTokenProvider", "GetAuthenticationToken"));
    INSTALL_HOOK_OFFSETLESS(getLogger(), MainSystemInit_Init,
        il2cpp_utils::FindMethod("", "MainSystemInit", "Init"));
    INSTALL_HOOK_OFFSETLESS(getLogger(), UserCertificateValidator_ValidateCertificateChainInternal,
        il2cpp_utils::FindMethodUnsafe("", "UserCertificateValidator", "ValidateCertificateChainInternal", 3));
    INSTALL_HOOK_OFFSETLESS(getLogger(), MultiplayerModeSelectionViewController_DidActivate,
        il2cpp_utils::FindMethodUnsafe("", "MultiplayerModeSelectionViewController", "DidActivate", 3));
    INSTALL_HOOK_OFFSETLESS(getLogger(), MainMenuViewController_DidActivate, 
        il2cpp_utils::FindMethodUnsafe("", "MainMenuViewController", "DidActivate", 3));
    INSTALL_HOOK_OFFSETLESS(getLogger(), HostLobbySetupViewController_SetPlayersMissingLevelText,
       il2cpp_utils::FindMethodUnsafe("", "HostLobbySetupViewController", "SetPlayersMissingLevelText", 1));
    //INSTALL_HOOK_OFFSETLESS(getLogger(), MultiplayerLevelSelectionFlowCoordinator_Setup,
    INSTALL_HOOK_OFFSETLESS(getLogger(), HostLobbySetupViewController_SetStartGameEnabled,
        il2cpp_utils::FindMethodUnsafe("", "HostLobbySetupViewController", "SetStartGameEnabled", 2));
    INSTALL_HOOK_OFFSETLESS(getLogger(), LevelSelectionNavigationController_Setup,
        il2cpp_utils::FindMethodUnsafe("", "LevelSelectionNavigationController", "Setup", 11));
    INSTALL_HOOK_OFFSETLESS(getLogger(), MultiplayerLevelLoader_LoadLevel,
        il2cpp_utils::FindMethodUnsafe("", "MultiplayerLevelLoader", "LoadLevel", 3));
    INSTALL_HOOK_OFFSETLESS(getLogger(), NetworkPlayerEntitlementChecker_GetEntitlementStatus,
        il2cpp_utils::FindMethodUnsafe("", "NetworkPlayerEntitlementChecker", "GetEntitlementStatus", 1));
    INSTALL_HOOK_OFFSETLESS(getLogger(), NetworkPlayerEntitlementChecker_GetPlayerLevelEntitlementsAsync,
        il2cpp_utils::FindMethodUnsafe("", "NetworkPlayerEntitlementChecker", "GetPlayerLevelEntitlementsAsync", 3));
}

#include "dibidab.h"

#include "../ecs/EntityInspector.h"
#include "../rendering/ImGuiStyle.h"

#include <graphics/textures/texture.h>
#include <audio/audio.h>
#include <audio/WavLoader.h>
#include <audio/OggLoader.h>
#include <asset_manager/AssetManager.h>

#include <gu/game_utils.h>
#include <gu/game_config.h>
#include <gu/profiler.h>

#include <files/file_utils.h>
#include <files/FileWatcher.h>
#include <code_editor/CodeEditor.h>
#include <utils/startup_args.h>

#include <mutex>

dibidab::EngineSettings dibidab::settings;

std::map<std::string, std::string> dibidab::startupArgs;

Session &dibidab::getCurrentSession()
{
    auto *s = tryGetCurrentSession();
    if (s == nullptr)
        throw gu_err("There's no Session active at the moment");
    return *s;
}

delegate<void()> dibidab::onSessionChange;
Session *currSession = nullptr;

Session *dibidab::tryGetCurrentSession()
{
    return currSession;
}

void dibidab::setCurrentSession(Session *s)
{
    delete currSession;
    currSession = s;

#ifndef DIBIDAB_NO_SAVE_GAME
    if (s)
        luau::getLuaState()["saveGame"] = s->saveGame.luaTable;
#endif

    onSessionChange();
}


void showDeveloperOptionsMenuBar()
{
    ImGui::BeginMainMenuBar();

    if (ImGui::BeginMenu("Game"))
    {
        ImGui::MenuItem(
            "Show developer options",
            KeyInput::getKeyName(dibidab::settings.keyInput.toggleDeveloperOptions),
            &dibidab::settings.bShowDeveloperOptions
        );

        if (ImGui::BeginMenu("Graphics"))
        {
            static bool bVSync = gu::getVSync();
            ImGui::MenuItem("VSync", "", &bVSync);
            gu::setVSync(bVSync);

            ImGui::MenuItem("Fullscreen", KeyInput::getKeyName(dibidab::settings.keyInput.toggleFullscreen), &gu::bFullscreen);

            if (ImGui::BeginMenu("Edit shader"))
            {
                for (auto &[name, asset] : AssetManager::getAssetsForType<std::string>())
                {
                    if (!su::endsWith(name, ".frag") && !su::endsWith(name, ".vert") && !su::endsWith(name, ".glsl"))
                        continue;
                    if (ImGui::MenuItem(name.c_str()))
                    {
                        auto &tab = CodeEditor::tabs.emplace_back();
                        tab.title = asset->fullPath;

                        tab.code = *((std::string *)asset->obj);
                        tab.languageDefinition = TextEditor::LanguageDefinition::C();
                        tab.save = [] (auto &tab) {

                            fu::writeBinary(tab.title.c_str(), tab.code.data(), tab.code.length());

                            AssetManager::loadFile(tab.title, "assets/");
                        };
                        tab.revert = [] (auto &tab) {
                            tab.code = fu::readString(tab.title.c_str());
                        };
                    }
                }
                ImGui::EndMenu();
            }
            ImGui::Separator();

            ImGui::MenuItem(reinterpret_cast<const char *>(glGetString(GL_VERSION)), nullptr, false, false);
            ImGui::MenuItem(reinterpret_cast<const char *>(glGetString(GL_RENDERER)), nullptr, false, false);
            ImGui::EndMenu();
        }

        ImGui::Separator();

        std::string luamem = "Lua memory: " + std::to_string(luau::getLuaState().memory_used() / (1024.f*1024.f)) + "MB";
        ImGui::MenuItem(luamem.c_str(), nullptr, false, false);

        ImGui::EndMenu();
    }

    EntityInspector::drawInspectingDropDown();

    ImGui::EndMainMenuBar();

    CodeEditor::drawGUI(
        ImGui::GetIO().Fonts->Fonts.back()  // default monospace font (added by setImGuiStyle())
    );
}

void dibidab::addDefaultAssetLoaders()
{
#ifdef DIBIDAB_ADD_TEXTURE_ASSET_LOADER
    AssetManager::addAssetLoader<Texture>(".png|.jpg|.jpeg|.tga|.bmp|.psd|.gif", [](auto path) {

        return new Texture(Texture::fromImageFile(path.c_str()));
    });
#endif
    AssetManager::addAssetLoader<std::string>({ ".frag", ".vert", ".glsl" }, [](auto path) {

        return new std::string(fu::readString(path.c_str()));
    });
    AssetManager::addAssetLoader<json>({ ".json" }, [](auto path) {

        return new json(json::parse(fu::readString(path.c_str())));
    });
    AssetManager::addAssetLoader<au::Sound>({ ".wav" }, [](auto path) {

        auto sound = new au::Sound;
        au::WavLoader(path.c_str(), *sound);
        return sound;
    });
    AssetManager::addAssetLoader<au::Sound>({ ".ogg" }, [](auto path) {

        auto sound = new au::Sound;
        au::OggLoader::load(path.c_str(), *sound);
        return sound;
    });
    AssetManager::addAssetLoader<luau::Script>({ ".lua" }, [](auto path) {

        return new luau::Script(path);
    });
}

std::mutex assetToReloadMutex;
std::string assetToReload;
FileWatcher assetWatcher;

void dibidab::init(int argc, char **argv, gu::Config &config)
{
    startupArgsToMap(argc, argv, dibidab::startupArgs);

    config.width = dibidab::settings.graphics.windowSize.x;
    config.height = dibidab::settings.graphics.windowSize.y;
    config.bVSync = dibidab::settings.graphics.vsync;
    config.samples = 0;
    config.bPrintOpenGLMessages = dibidab::settings.graphics.printOpenGLMessages;
    config.bPrintOpenGLErrors = dibidab::settings.graphics.printOpenGLErrors;
    config.openGLMajorVersion = dibidab::settings.graphics.openGLMajorVersion;
    config.openGLMinorVersion = dibidab::settings.graphics.openGLMinorVersion;
    gu::bFullscreen = dibidab::settings.graphics.fullscreen; // dont ask me why this is not in config
    if (!gu::init(config))
        throw gu_err("Error while initializing gu");

    std::cout << "Running game with\n - GL_VERSION: " << glGetString(GL_VERSION) << "\n";
    std::cout << " - GL_RENDERER: " << glGetString(GL_RENDERER) << "\n";

    // audio:
    au::init();

    setImGuiStyleAndConfig();

    #ifdef linux
    assetWatcher.addDirectoryToWatch("assets", true);

    assetWatcher.onChange = [&] (auto path)
    {
        assetToReloadMutex.lock();
        assetToReload = path;
        assetToReloadMutex.unlock();
    };
    assetWatcher.startWatchingAsync();
    #endif

    AssetManager::loadDirectory("assets");

    // save window size in settings:
    static auto onResize = gu::onResize += [] {
        if (!gu::bFullscreen) // dont save fullscreen-resolution
            dibidab::settings.graphics.windowSize = ivec2(
                gu::virtualWidth, gu::virtualHeight
            );
    };

    static auto beforeRender = gu::beforeRender += [&](double deltaTime) {
        if (currSession)
            currSession->update(deltaTime);

        if (KeyInput::justPressed(dibidab::settings.keyInput.reloadAssets) && dibidab::settings.bShowDeveloperOptions)
            AssetManager::loadDirectory("assets", true);

        {
            assetToReloadMutex.lock();
            if (!assetToReload.empty())
                AssetManager::loadFile(assetToReload, "assets/", true);
            assetToReload.clear();
            assetToReloadMutex.unlock();
        }

        {
            dibidab::settings.graphics.vsync = gu::getVSync();
            dibidab::settings.graphics.fullscreen = gu::bFullscreen;
        }

        if (KeyInput::justPressed(dibidab::settings.keyInput.toggleDeveloperOptions))
            dibidab::settings.bShowDeveloperOptions ^= 1;

        if (KeyInput::justPressed(dibidab::settings.keyInput.toggleFullscreen))
            gu::bFullscreen = !gu::bFullscreen;

        if (dibidab::settings.bShowDeveloperOptions)
            showDeveloperOptionsMenuBar();
        gu::profiler::showGUI = dibidab::settings.bShowDeveloperOptions;
    };

}

void dibidab::run()
{
    gu::run();
    dibidab::setCurrentSession(nullptr);
    au::terminate();
}

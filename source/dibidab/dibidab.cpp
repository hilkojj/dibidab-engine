#include "dibidab.h"

#include "../ecs/Inspector.h"
#include "../rendering/ImGuiStyle.h"
#include "../level/Level.h"

#include "../generated/registry.struct_info.h"

#include <gu/game_utils.h>
#include <gu/profiler.h>
#include <graphics/textures/texture.h>
#include <assets/AssetManager.h>
#include <files/file_utils.h>
#include <files/FileWatcher.h>
#include <code_editor/CodeEditor.h>
#include <utils/startup_args.h>

#include <mutex>

namespace dibidab
{
    delegate<void(dibidab::level::Level *)> onLevelChange;

    dibidab::EngineSettings settings;

    std::map<std::string, std::string> startupArgs;

    std::mutex assetToReloadMutex;
    std::string assetToReload;
    FileWatcher assetWatcher;

    level::Level *currentLevel = nullptr;
}

void showDeveloperOptionsMenuBar()
{
    ImGui::BeginMainMenuBar();

    if (ImGui::BeginMenu("Game"))
    {
        ImGui::MenuItem(
            "Show developer options",
            KeyInput::getKeyName(dibidab::settings.developerKeyInput.toggleDeveloperOptions),
            &dibidab::settings.bShowDeveloperOptions
        );

        if (ImGui::BeginMenu("Graphics"))
        {
            static bool bVSync = gu::getVSync();
            ImGui::MenuItem("VSync", "", &bVSync);
            gu::setVSync(bVSync);

            ImGui::MenuItem("Fullscreen", KeyInput::getKeyName(dibidab::settings.developerKeyInput.toggleFullscreen), &gu::bFullscreen);

            if (ImGui::BeginMenu("Edit shader"))
            {
                for (auto &[name, asset] : AssetManager::getAssetsForType<std::string>())
                {
                    if (!su::endsWith(name, ".frag") && !su::endsWith(name, ".vert") && !su::endsWith(name, ".glsl"))
                    {
                        continue;
                    }
                    if (ImGui::MenuItem(name.c_str()))
                    {
                        auto &tab = CodeEditor::tabs.emplace_back();
                        tab.title = asset->fullPath;

                        tab.code = *((std::string *)asset->obj);
                        tab.languageDefinition = TextEditor::LanguageDefinition::C();
                        tab.save = [] (auto &tab)
                        {
                            fu::writeBinary(tab.title.c_str(), tab.code.data(), tab.code.length());

                            AssetManager::loadFile(tab.title, "assets/");
                        };
                        tab.revert = [] (auto &tab)
                        {
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

        const std::string luaMemoryStr = "Lua memory: " + std::to_string(luau::getLuaState().memory_used() / (1024.0f * 1024.0f)) + "MB";
        ImGui::MenuItem(luaMemoryStr.c_str(), nullptr, false, false);

        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();

    CodeEditor::drawGUI(
        ImGui::GetIO().Fonts->Fonts.back()  // default monospace font (added by setImGuiStyle())
    );
}

void addDefaultAssetLoaders(const dibidab::Config &config)
{
    if (config.addAssetLoaders.bLua)
    {
        AssetManager::addAssetLoader<luau::Script>({ ".lua" }, [] (const std::string &path)
        {
            return new luau::Script(path);
        });
    }
    if (config.addAssetLoaders.bJson)
    {
        AssetManager::addAssetLoader<json>({ ".json" }, [] (const std::string &path)
        {
            return new json(json::parse(fu::readString(path.c_str())));
        });
    }
    if (config.addAssetLoaders.bShaders)
    {
        AssetManager::addAssetLoader<std::string>({ ".frag", ".vert", ".glsl" }, [] (const std::string &path)
        {
            return new std::string(fu::readString(path.c_str()));
        });
    }
    if (config.addAssetLoaders.bTextures)
    {
        AssetManager::addAssetLoader<Texture>({ ".png", ".jpg", ".jpeg", ".tga", ".bmp", ".psd", ".gif" },
            [] (const std::string &path)
        {
            return new Texture(Texture::fromImageFile(path.c_str()));
        });
    }
}

gu::Config dibidab::guConfigFromSettings()
{
    gu::Config config;
    config.width = dibidab::settings.graphics.windowSize.x;
    config.height = dibidab::settings.graphics.windowSize.y;
    config.bVSync = dibidab::settings.graphics.bVSync;
    config.samples = 0;
    config.bPrintOpenGLMessages = dibidab::settings.graphics.bPrintOpenGLMessages;
    config.bPrintOpenGLErrors = dibidab::settings.graphics.bPrintOpenGLErrors;
    config.openGLMajorVersion = dibidab::settings.graphics.openGLMajorVersion;
    config.openGLMinorVersion = dibidab::settings.graphics.openGLMinorVersion;
    return config;
}

void dibidab::init(int argc, char **argv, Config &config)
{
    startupArgsToMap(argc, argv, dibidab::startupArgs);
    registerStructs();

    gu::bFullscreen = dibidab::settings.graphics.bFullscreen;

    if (!gu::init(config.guConfig))
    {
        throw gu_err("Error while initializing gu");
    }

    std::cout << "Running game with\n - GL_VERSION: " << glGetString(GL_VERSION) << "\n";
    std::cout << " - GL_RENDERER: " << glGetString(GL_RENDERER) << "\n";

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

    addDefaultAssetLoaders(config);
    AssetManager::loadDirectory("assets");

    // save window size in settings:
    static auto onResize = gu::onResize += []
    {
        if (!gu::bFullscreen) // dont save fullscreen-resolution
        {
            dibidab::settings.graphics.windowSize = ivec2(
                gu::virtualWidth, gu::virtualHeight
            );
        }
    };

    static auto beforeRender = gu::beforeRender += [&](double deltaTime)
    {
        if (level::Level *level = getLevel())
        {
            level->update(deltaTime);
        }

        if (KeyInput::justPressed(dibidab::settings.developerKeyInput.reloadAssets) && dibidab::settings.bShowDeveloperOptions)
        {
            AssetManager::loadDirectory("assets", true);
        }

        {
            assetToReloadMutex.lock();
            if (!assetToReload.empty())
            {
                AssetManager::loadFile(assetToReload, "assets/", true);
            }
            assetToReload.clear();
            assetToReloadMutex.unlock();
        }

        {
            dibidab::settings.graphics.bVSync = gu::getVSync();
            dibidab::settings.graphics.bFullscreen = gu::bFullscreen;
        }

        if (KeyInput::justPressed(dibidab::settings.developerKeyInput.toggleDeveloperOptions))
        {
            dibidab::settings.bShowDeveloperOptions ^= 1;
        }
        gu::profiler::showGUI = dibidab::settings.bShowDeveloperOptions;

        if (KeyInput::justPressed(dibidab::settings.developerKeyInput.toggleFullscreen))
        {
            gu::bFullscreen = !gu::bFullscreen;
        }

        if (dibidab::settings.bShowDeveloperOptions)
        {
            showDeveloperOptionsMenuBar();
        }
    };
}

void dibidab::run()
{
    gu::run();
    setLevel(nullptr);
    dibidab::assetWatcher.stopWatching();
}

void dibidab::setLevel(dibidab::level::Level *level)
{
    if (currentLevel != nullptr && currentLevel->isUpdating())
    {
        throw gu_err("Cannot change level while updating level!");
    }
    delete currentLevel;
    currentLevel = level;
    if (currentLevel != nullptr)
    {
        currentLevel->initialize();
    }
    onLevelChange(level);
}

dibidab::level::Level *dibidab::getLevel()
{
    return currentLevel;
}

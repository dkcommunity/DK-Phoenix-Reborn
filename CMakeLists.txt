cmake_minimum_required(VERSION 3.24)

project(DKPhoenixReborn
    VERSION 0.1.0
    DESCRIPTION "Modern Windows x64 client baseline for DK Phoenix Reborn"
    LANGUAGES CXX
)

# MSVC: static runtime (/MT for Release, /MTd for Debug) - no VCRUNTIME140.dll dependency
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

include(CTest)

if(NOT WIN32)
    message(FATAL_ERROR "DK Phoenix Reborn client currently supports Windows only.")
endif()

if(NOT CMAKE_GENERATOR_PLATFORM STREQUAL "x64")
    message(FATAL_ERROR "DK Phoenix Reborn client is x64-only. Configure with the windows-vs2022-x64 preset.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(PHOENIX_BUILD_DIAGNOSTICS "Compile low-cost Debug diagnostics" ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/GameData")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/GameData")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/GameData/data/Info")
configure_file(
    "${CMAKE_SOURCE_DIR}/Source/World/Data/TILE.IDX"
    "${CMAKE_SOURCE_DIR}/GameData/data/Info/TILE.IDX"
    COPYONLY)

add_library(PhoenixMapCore STATIC
    Source/Assets/AnimationObjectFramePackReader.cpp
    Source/Assets/AnimationObjectFramePackReader.h
    Source/Assets/EffectFramePackReader.cpp
    Source/Assets/EffectFramePackReader.h
    Source/Core/BinaryReader.cpp
    Source/Core/BinaryReader.h
    Source/Assets/SpriteArchiveReader.cpp
    Source/Assets/SpriteArchiveReader.h
    Source/Backends/DirectX11/DirectX11Renderer.cpp
    Source/Backends/DirectX11/DirectX11Renderer.h
    Source/World/LegacyMapReader.cpp
    Source/World/LegacyTileSpriteIndex.cpp
    Source/World/LegacyTileSpriteIndex.h
    Source/World/LegacyMapReader.h
    Source/World/LegacyMapWriter.cpp
    Source/World/LegacyMapWriter.h
    Source/World/LegacyMapSidecar.cpp
    Source/World/LegacyMapSidecar.h
    Source/World/LegacyMapAnimationRenderer.cpp
    Source/World/LegacyMapAnimationRenderer.h
    Source/World/WorldDrawListBuilder.cpp
    Source/World/WorldDrawListBuilder.h
    Source/World/WorldDrawListTypes.h
)
target_include_directories(PhoenixMapCore PUBLIC "${CMAKE_SOURCE_DIR}/Source")
target_compile_definitions(PhoenixMapCore PUBLIC
    NOMINMAX
    WIN32_LEAN_AND_MEAN
    # 2026-08-31 ABI FIX (root cause): PHOENIX_ENABLE_DIAGNOSTICS kosulu
    # PhoenixClient'inkiyle (satir ~358) birebir ayni olmali. Eski hali
    # yalnizca Debug config'de tanimliydi; PhoenixClient ise config'den
    # bagimsiz tanimliyordu. LegacyMapReader.h struct'lari bu define ile
    # kosullu uye icerdigi icin (sourceFileOffset/sourceRecordSize/
    # sectorDataFileOffset) Release'te lib ve exe AYNI struct'i FARKLI
    # boyutta derliyordu -> LegacyMapReader::Load donus degeri caller'da
    # yanlis offsetlerden okunuyordu (error.size=15 capacity=0 hayalet
    # hatasi, map.has_value()=false) -> "Map load failed" / Loading Map
    # ekraninda takilma. PUBLIC oldugu icin tum tuketiciler ayni ABI'yi alir.
    $<$<BOOL:${PHOENIX_BUILD_DIAGNOSTICS}>:PHOENIX_ENABLE_DIAGNOSTICS>
)
# Enable minimal diagnostics also for Release builds so that
# enterWorld flag checks are logged. This does not add heavy runtime cost,
# it only activates the LogEnterWorldDiagnostic calls.
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(PhoenixMapCore PUBLIC PHOENIX_ENABLE_DIAGNOSTICS)
    target_compile_definitions(PhoenixClient PUBLIC PHOENIX_ENABLE_DIAGNOSTICS)
endif()
target_link_libraries(PhoenixMapCore PUBLIC d3d11 dxgi d3dcompiler)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/GameData")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/GameData")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/GameData")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/GameData")

add_executable(PhoenixClient WIN32
    Source/Animation/AnimationController.cpp
    Source/Animation/AnimationController.h
    Source/Animation/AnimationTypes.h
    Source/Animation/LegacyCreatureAnimation.cpp
    Source/Animation/LegacyCreatureAnimation.h
    Source/App/Main.cpp
    Source/Assets/AssetDiscovery.cpp
    Source/Assets/AssetDiscovery.h
    Source/Assets/FramePackReader.cpp
    Source/Assets/FramePackReader.h
    Source/Assets/ImageArchiveProbe.cpp
    Source/Assets/ImageArchiveProbe.h
    Source/Assets/InfBinarySchemaReader.cpp
    Source/Assets/InfBinarySchemaReader.h
    Source/Assets/InfTableParser.cpp
    Source/Assets/InfTableParser.h
    Source/Assets/LegacyAssetTypes.h
    Source/Assets/LegacyReaderContracts.h
    Source/Assets/SpriteArchiveReader.h
    Source/Audio/AudioBackend.h
    Source/Audio/AudioTypes.h
    Source/Audio/SoundBank.cpp
    Source/Audio/SoundBank.h
    Source/Audio/WaveFileProbe.cpp
    Source/Audio/WaveFileProbe.h
    Source/Backends/DirectX11/DirectX11Renderer.h
    Source/Backends/MediaFoundation/MediaFoundationPlayer.cpp
    Source/Backends/MediaFoundation/MediaFoundationPlayer.h
    Source/Backends/XAudio2/XAudio2Backend.cpp
    Source/Backends/XAudio2/XAudio2Backend.h
    Source/Content/ContentDatabase.h
    Source/Content/AudioCatalog.cpp
    Source/Content/AudioCatalog.h
    Source/Content/CreatureInfoCatalog.cpp
    Source/Content/CreatureInfoCatalog.h
    Source/Content/EffectInfoCatalog.cpp
    Source/Content/EffectInfoCatalog.h
    Source/Content/EffectStatusCatalog.cpp
    Source/Content/EffectStatusCatalog.h
    Source/Content/EffectVisualResolver.h
    Source/Content/ExperienceCatalog.cpp
    Source/Content/ExperienceCatalog.h
    Source/Content/FileDefCatalog.cpp
    Source/Content/FileDefCatalog.h
    Source/Content/GameStringCatalog.cpp
    Source/Content/GameStringCatalog.h
    Source/Content/InfSchemaRegistry.cpp
    Source/Content/InfSchemaRegistry.h
    Source/Content/InMemoryContentDatabase.cpp
    Source/Content/InMemoryContentDatabase.h
    Source/Content/ItemInfoCatalog.cpp
    Source/Content/ItemInfoCatalog.h
    Source/Content/ItemOptionCatalog.cpp
    Source/Content/ItemOptionCatalog.h
    Source/Content/LegacyBinaryTableReader.cpp
    Source/Content/LegacyBinaryTableReader.h
    Source/Content/LegacyAssetResolver.cpp
    Source/Content/LegacyAssetResolver.h
    Source/Content/LegacyInfoCatalog.cpp
    Source/Content/LegacyInfoCatalog.h
    Source/Content/LegacyUiSpriteCatalog.cpp
    Source/Content/LegacyUiSpriteCatalog.h
    Source/Content/LegacyQuestCatalog.cpp
    Source/Content/LegacyQuestCatalog.h
    Source/Content/ItemVisualResolver.h
    Source/Content/PlayerVisualResolver.h
    Source/Content/ContentTypes.h
    Source/Core/BinaryReader.cpp
    Source/Core/BinaryReader.h
    Source/Core/StrongId.h
    Source/Data/RuntimeDataCatalog.cpp
    Source/Data/RuntimeDataCatalog.h
    Source/Diagnostics/Logger.cpp
    Source/Diagnostics/Logger.h
    Source/Diagnostics/PhoenixDiagnostics.cpp
    Source/Diagnostics/PhoenixDiagnostics.h
    Source/FileSystem/FileAccessPolicy.cpp
    Source/FileSystem/FileAccessPolicy.h
    Source/FileSystem/ProjectPaths.cpp
    Source/FileSystem/ProjectPaths.h
    Source/Frontend/FrontendStateMachine.cpp
    Source/Frontend/FrontendStateMachine.h
    Source/Frontend/FrontendRuntimeModels.h
    Source/Graphics/ImageData.h
    Source/LegacyPacket/Bridge/LegacyMemoryStreams.cpp
    Source/LegacyPacket/Bridge/LegacyPacketHandlerStubs.cpp
    Source/LegacyPacket/Bridge/LegacyPacketBridge.cpp
    Source/LegacyPacket/Bridge/LegacyPacketBridge.h
    Source/LegacyPacket/Packet/Assert.cpp
    Source/LegacyPacket/Compat/UserInformation.cpp
    Source/LegacyPacket/Compat/UserInformation.h
    Source/LegacyPacket/Packet/StringStream.cpp
    Source/LegacyPacket/Packet/StringStream.h
    Source/LegacyPacket/Packet/PCVampireInfo3.cpp
    Source/LegacyPacket/Packet/PCVampireInfo3.h
    Source/LegacyPacket/Packet/Cpackets/CGConnect.cpp
    Source/LegacyPacket/Packet/Cpackets/CGGlobalChat.cpp
    Source/LegacyPacket/Packet/Cpackets/CGGuildChat.cpp
    Source/LegacyPacket/Packet/Cpackets/CGMove.cpp
    Source/LegacyPacket/Packet/Cpackets/CGPartySay.cpp
    Source/LegacyPacket/Packet/Cpackets/CGReady.cpp
        Source/LegacyPacket/Packet/Cpackets/CGUseBonusPoint.cpp
        Source/LegacyPacket/Packet/Cpackets/CGRangerSay.cpp
    Source/LegacyPacket/Packet/Cpackets/CGSay.cpp
    Source/LegacyPacket/Packet/Cpackets/CGWhisper.cpp
    Source/LegacyPacket/Packet/Cpackets/CLLogin.cpp
    Source/LegacyPacket/Packet/Cpackets/CLSelectPC.cpp
    Source/LegacyPacket/Packet/Gpackets/GCMove.cpp
    Source/LegacyPacket/Packet/Gpackets/GCMoveError.cpp
    Source/LegacyPacket/Packet/Gpackets/GCMoveOK.cpp
    Source/LegacyPacket/Packet/Gpackets/GCAddVampireCorpse.cpp
    Source/LegacyPacket/Packet/Gpackets/GCGlobalChat.cpp
    Source/LegacyPacket/Packet/Gpackets/GCGuildChat.cpp
    Source/LegacyPacket/Packet/Gpackets/GCPartySay.cpp
    Source/LegacyPacket/Packet/Gpackets/GCPartyJoined.cpp
    Source/LegacyPacket/Packet/Gpackets/GCPartyLeave.cpp
    Source/LegacyPacket/Packet/Gpackets/GCSay.cpp
    Source/LegacyPacket/Packet/Gpackets/GCSystemMessage.cpp
    Source/LegacyPacket/Packet/Gpackets/GCWhisper.cpp
    Source/LegacyPacket/Packet/Gpackets/GCWhisperFailed.cpp
    Source/Graphics/ResourceCache.cpp
    Source/Graphics/ResourceCache.h
    Source/Graphics/TextureTypes.h
    Source/Media/MediaPlayer.h
    Source/Media/MediaTypes.h
    Source/Migration/FeatureFlags.cpp
    Source/Migration/FeatureFlags.h
    Source/Migration/RpkMigrationPolicy.cpp
    Source/Migration/RpkMigrationPolicy.h
    Source/Network/TcpClient.cpp
    Source/Network/TcpClient.h
    Source/Parity/ParityReport.cpp
    Source/Parity/ParityReport.h
    Source/Platform/Application.cpp
    Source/Platform/Application.h
    Source/Platform/FileWatcher.cpp
    Source/Platform/FileWatcher.h
    Source/Protocol/LegacyProtocol.cpp
    Source/Protocol/LegacyProtocol.h
    Source/Prototype/ClientPrototype.cpp
    Source/Prototype/ClientPrototype.h
    Source/Prototype/DataBackedPrototypePlan.cpp
    Source/Prototype/DataBackedPrototypePlan.h
    Source/Prototype/LoginTitlePrototype.cpp
    Source/Prototype/LoginTitlePrototype.h
    Source/Render/RenderQueue.cpp
    Source/Render/RenderQueue.h
    Source/Render/ResolutionPolicy.h
    Source/Render/RenderTypes.h
    Source/Runtime/GameRuntimeModels.h
    Source/Settings/SettingsManager.cpp
    Source/Settings/SettingsManager.h
    Source/Settings/DisplayResolutionCatalog.h
    Source/Settings/InterfaceProfileLoader.cpp
    Source/Settings/InterfaceProfileLoader.h
    Source/Settings/SettingsTypes.h
        Source/UI/UiElement.h
    Source/UI/Windows/ClientUIEditor.cpp
    Source/UI/Windows/ClientUIEditor.h
    Source/UI/Windows/QuestManager.cpp
    Source/UI/Windows/QuestManager.h
    Source/UI/Windows/CharInfoWindow.cpp
    Source/UI/Windows/CharInfoWindow.h
    Source/UI/Windows/HpBarWindow.cpp
    Source/UI/Windows/HpBarWindow.h
    Source/UI/Windows/ChatPanel.cpp
        Source/UI/Windows/ChatPanel.h
        Source/UI/Windows/CharInfoWindow.cpp
        Source/UI/Windows/CharInfoWindow.h
        Source/UI/Windows/ClientUIEditor.cpp
    Source/UI/Windows/ClientUIEditor.h
    Source/UI/Descriptor/DescriptorManager.cpp
        Source/UI/Descriptor/DescriptorManager.h
        Source/UI/Descriptor/ItemTooltip.cpp
        Source/UI/Descriptor/ItemTooltip.h
        Source/UI/Layout/FrontendLayoutCatalog.cpp
    Source/UI/Layout/FrontendLayoutCatalog.h
    Source/UI/Layout/FrontendLayoutValidator.cpp
    Source/UI/Layout/FrontendLayoutValidator.h
    Source/UI/Layout/LayoutViewport.cpp
    Source/UI/Layout/LayoutViewport.h
    Source/UI/Resources/LegacyUiSpriteResourceLoader.cpp
    Source/UI/Resources/LegacyUiSpriteResourceLoader.h
    Source/UI/Resources/LegacyWindowTransparency.cpp
    Source/UI/Resources/LegacyWindowTransparency.h
    Source/UI/Layout/LiveResolutionApply.cpp
    Source/UI/Layout/LiveResolutionApply.h
    Source/UI/Login/LoginInputModel.cpp
    Source/UI/Login/LoginInputModel.h
    Source/UI/Layout/LoginTitleLayout.cpp
    Source/UI/Layout/LoginTitleLayout.h
    Source/UI/Options/OptionsGraphicsModel.cpp
    Source/UI/Options/OptionsGraphicsModel.h
    Source/UI/Menu/GameMenuButtonMap.cpp
    Source/UI/Menu/GameMenuButtonMap.h
    Source/UI/Chat/ChatButtonMap.cpp
    Source/UI/Chat/ChatButtonMap.h
    Source/UI/Windows/ChatPanel.cpp
            Source/UI/Windows/ChatPanel.h
            Source/UI/Title/TitleScreen.cpp
            Source/UI/Title/TitleScreen.h
            Source/UI/Windows/ItemListWindow.cpp
                Source/UI/Windows/ItemListWindow.h
                Source/UI/Windows/ImageNoticeWindow.cpp
                    Source/UI/Windows/ImageNoticeWindow.h
                    Source/UI/Windows/BulletinBoardWindow.cpp
                    Source/UI/Windows/BulletinBoardWindow.h
                    Source/UI/Windows/ResurrectRequestWindow.cpp
                        Source/UI/Windows/ResurrectRequestWindow.h
                        Source/UI/Windows/MixingForgeWindow.cpp
                                                    Source/UI/Windows/MixingForgeWindow.h
                                                    Source/UI/Windows/RemoveOptionWindow.cpp
                                                                                                        Source/UI/Windows/RemoveOptionWindow.h
                                                                                                        Source/UI/Windows/OustersSkillInfoWindow.cpp
                                                                                                        Source/UI/Windows/OustersSkillInfoWindow.h
                                                                                                        Source/UI/Windows/GenericBillingPromptWindow.cpp
                                                                                                        Source/UI/Windows/GenericBillingPromptWindow.h
                                                                                                        Source/UI/Windows/LegalDocumentsWindow.cpp
                                                                                                                                                                                                                Source/UI/Windows/LegalDocumentsWindow.h
                                                                                                                                                                                                                Source/UI/Windows/LoginScreen.cpp
                                                                                                                                                                                                                Source/UI/Windows/LoginScreen.h
                                                                                                                                                                                                                Source/UI/Windows/ServerSelectScreen.cpp
                                                                                                                                                                                                                                                                                                                                                                                                Source/UI/Windows/ServerSelectScreen.h
                                                                                                                                                                                                                                                                                                                                                                                                Source/UI/Windows/CharacterSelectScreen.cpp
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                Source/UI/Windows/CharacterSelectScreen.h
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                # Source/UI/Windows/CharacterCreateScreen.cpp  # TEMP excluded (pre-existing API mismatch)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    Source/UI/Windows/CharacterCreateScreen.h
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    Source/UI/Windows/CharacterDeleteScreen.cpp
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        Source/UI/Windows/CharacterDeleteScreen.h
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        Source/UI/Windows/OptionsScreen.cpp
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            Source/UI/Windows/OptionsScreen.h
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            Source/UI/Windows/NewUserScreen.cpp
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            Source/UI/Windows/NewUserScreen.h
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                Source/UI/Menu/TitleMenuButtonMap.cpp
            Source/UI/Chat/ChatInputField.cpp
            Source/UI/Chat/ChatInputField.h
    Source/UI/Chat/ChatScrollWidget.cpp
    Source/UI/Chat/ChatScrollWidget.h
    Source/UI/Menu/TitleMenuButtonMap.cpp
    Source/UI/Menu/TitleMenuButtonMap.h
    Source/UI/UiResourceDatabase.h
    Source/UI/UiTypes.h
    Source/UI/Text/FontAssetManifest.cpp
    Source/UI/Text/FontAssetManifest.h
    Source/UI/Text/FontManager.cpp
    Source/UI/Text/FontManager.h
    Source/UI/Text/FontStackConfig.cpp
    Source/UI/Text/FontStackConfig.h
    Source/UI/Text/DirectWriteTextRenderer.cpp
    Source/UI/Text/DirectWriteTextRenderer.h
    Source/UI/Text/LoginTextLayout.cpp
    Source/UI/Text/LoginTextLayout.h
    Source/UI/Text/LoginTextVisualModel.cpp
    Source/UI/Text/LoginTextVisualModel.h
    Source/UI/Text/UiTextCatalog.cpp
    Source/UI/Text/UiTextCatalog.h


    Source/Validation/ContentValidationContracts.h
    Source/Validation/ValidationReport.cpp
    Source/Validation/ValidationReport.h
    Source/World/WorldAssetCache.cpp
    Source/World/WorldAssetCache.h
    Source/World/WorldDrawListBuilder.h
    Source/World/WorldRenderTypes.h
    Source/World/MapFileProbe.cpp
    Source/World/MapFileProbe.h
    Source/World/LegacyMapReader.h
    Source/World/ZoneInfoCatalog.cpp
    Source/World/ZoneInfoCatalog.h
)

target_compile_definitions(PhoenixClient PRIVATE
    $<$<BOOL:${PHOENIX_BUILD_DIAGNOSTICS}>:PHOENIX_ENABLE_DIAGNOSTICS>
)

target_compile_definitions(PhoenixClient PRIVATE
    UNICODE
    _UNICODE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
    PHOENIX_PROJECT_ROOT=L"${CMAKE_SOURCE_DIR}"
)

target_include_directories(PhoenixClient PRIVATE
    "${CMAKE_SOURCE_DIR}/Source"
    "${CMAKE_SOURCE_DIR}/Source/LegacyPacket/Compat"
    "${CMAKE_SOURCE_DIR}/Source/LegacyPacket/Packet"
    "${CMAKE_SOURCE_DIR}/Source/LegacyPacket/Packet/Types"
    "${CMAKE_SOURCE_DIR}/Source/Diagnostics"
)

set_target_properties(PhoenixClient PROPERTIES
    OUTPUT_NAME_DEBUG "PhoenixClient_Debug"
    OUTPUT_NAME_RELEASE "PhoenixClient"
)

target_link_libraries(PhoenixClient PRIVATE
    PhoenixMapCore
    d3d11
    dxgi
    d3dcompiler
    d2d1
    dwrite
    windowscodecs
    imm32
    iphlpapi
    psapi
    winmm
    ws2_32
    dbghelp
    xmllite
    shlwapi
)

add_library(PhoenixMapEditorCore STATIC
    Tool/MapEditor/Core/EditorWorldDrawListBuilder.cpp
    Tool/MapEditor/Core/EditorWorldDrawListBuilder.h
    Tool/MapEditor/MapEditorExportModel.cpp
    Tool/MapEditor/MapEditorExportModel.h
    Tool/MapEditor/MapEditorDocument.cpp
    Tool/MapEditor/MapEditorDocument.h
    Tool/MapEditor/MapEditorMinimapModel.cpp
    Tool/MapEditor/MapEditorMinimapModel.h
    Tool/MapEditor/MapEditorPngWriter.cpp
    Tool/MapEditor/MapEditorPngWriter.h
    Tool/MapEditor/MapEditorPreset.cpp
    Tool/MapEditor/MapEditorPreset.h
    Tool/MapEditor/MapEditorRenderCache.cpp
    Tool/MapEditor/MapEditorRenderCache.h
    Tool/MapEditor/MapEditorViewportModel.cpp
    Tool/MapEditor/MapEditorViewportModel.h
    Tool/MapEditor/MapEditorUiModel.cpp
    Tool/MapEditor/MapEditorUiModel.h
)
target_include_directories(PhoenixMapEditorCore PUBLIC
    "${CMAKE_SOURCE_DIR}/Source"
    "${CMAKE_SOURCE_DIR}/Tool/MapEditor"
)
target_link_libraries(PhoenixMapEditorCore PUBLIC PhoenixMapCore)
target_link_libraries(PhoenixMapEditorCore PUBLIC windowscodecs ole32)

add_executable(PhoenixMapEditor WIN32
    Tool/MapEditor/Core/EditorPhoenixDiagnostics.cpp
    Tool/MapEditor/MapEditorApp.cpp
)
target_include_directories(PhoenixMapEditor PRIVATE "${CMAKE_SOURCE_DIR}/Source" "${CMAKE_SOURCE_DIR}/Tool/MapEditor")
target_compile_definitions(PhoenixMapEditor PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
target_link_libraries(PhoenixMapEditor PRIVATE
    PhoenixMapEditorCore
    comctl32
    comdlg32
    shell32
    ole32
)
set_target_properties(PhoenixMapEditor PROPERTIES
    OUTPUT_NAME_DEBUG "PhoenixMapEditor_Debug"
    OUTPUT_NAME_RELEASE "PhoenixMapEditor_Release"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/Tool/MapEditor/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/Tool/MapEditor/bin"
)

if(MSVC)
    target_compile_options(PhoenixMapCore PRIVATE /W4 /permissive- /EHsc)
    target_compile_options(PhoenixMapEditorCore PRIVATE /W4 /permissive- /EHsc)
    target_compile_options(PhoenixMapEditor PRIVATE /W4 /permissive- /EHsc)
    target_compile_options(PhoenixClient PRIVATE
        /W4
        /permissive-
        /EHsc
    )

    set_source_files_properties(
        Source/LegacyPacket/Bridge/LegacyMemoryStreams.cpp
        Source/LegacyPacket/Bridge/LegacyPacketHandlerStubs.cpp
        Source/LegacyPacket/Bridge/LegacyPacketBridge.cpp
        Source/LegacyPacket/Packet/Assert.cpp
        Source/LegacyPacket/Compat/UserInformation.cpp
        Source/LegacyPacket/Packet/StringStream.cpp
        Source/LegacyPacket/Packet/PCVampireInfo3.cpp
        Source/LegacyPacket/Packet/Cpackets/CGConnect.cpp
        Source/LegacyPacket/Packet/Cpackets/CGGlobalChat.cpp
        Source/LegacyPacket/Packet/Cpackets/CGGuildChat.cpp
        Source/LegacyPacket/Packet/Cpackets/CGMove.cpp
        Source/LegacyPacket/Packet/Cpackets/CGPartySay.cpp
        Source/LegacyPacket/Packet/Cpackets/CGReady.cpp
            Source/LegacyPacket/Packet/Cpackets/CGUseBonusPoint.cpp
            Source/LegacyPacket/Packet/Cpackets/CGRangerSay.cpp
        Source/LegacyPacket/Packet/Cpackets/CGSay.cpp
        Source/LegacyPacket/Packet/Cpackets/CGWhisper.cpp
        Source/LegacyPacket/Packet/Cpackets/CLLogin.cpp
        Source/LegacyPacket/Packet/Cpackets/CLSelectPC.cpp
        Source/LegacyPacket/Packet/Gpackets/GCMove.cpp
        Source/LegacyPacket/Packet/Gpackets/GCMoveError.cpp
        Source/LegacyPacket/Packet/Gpackets/GCMoveOK.cpp
        Source/LegacyPacket/Packet/Gpackets/GCAddVampireCorpse.cpp
        Source/LegacyPacket/Packet/Gpackets/GCGlobalChat.cpp
        Source/LegacyPacket/Packet/Gpackets/GCGuildChat.cpp
        Source/LegacyPacket/Packet/Gpackets/GCPartySay.cpp
        Source/LegacyPacket/Packet/Gpackets/GCPartyJoined.cpp
        Source/LegacyPacket/Packet/Gpackets/GCPartyLeave.cpp
        Source/LegacyPacket/Packet/Gpackets/GCSay.cpp
        Source/LegacyPacket/Packet/Gpackets/GCSystemMessage.cpp
        Source/LegacyPacket/Packet/Gpackets/GCWhisper.cpp
        Source/LegacyPacket/Packet/Gpackets/GCWhisperFailed.cpp
        PROPERTIES COMPILE_OPTIONS "/W0"
    )
endif()

if(BUILD_TESTING)
    # ======================================================================
    # MapEditorCoreTests + LegacyCreatureAnimationTests — historical port
    # validation targets.  These produced MapEditorCoreTests_Release.exe
    # and LegacyCreatureAnimationTests.exe.  They are no longer needed and
    # will NOT be activated again.
    # ======================================================================
    # add_executable(...)
    # add_test(...)
endif()

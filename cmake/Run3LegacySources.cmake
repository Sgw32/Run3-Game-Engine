# Translation units recorded in Run3.vcproj. Keep this list explicit: it is the
# review boundary for the legacy compatibility target and must not be replaced
# with a filesystem glob.
set(RUN3_LEGACY_VCPROJ_SOURCES
    tinystr.cpp
    tinyxml.cpp
    tinyxmlerror.cpp
    tinyxmlparser.cpp
    DotSceneLoader.cpp
    LoadMap.cpp
    Run3SoundRuntime.cpp
    SoundManager.cpp
    buttonGUI.cpp
    ConCommands.cpp
    CrosshairOp.cpp
    CustomSceneManager.cpp
    DecalProjector.cpp
    Display.cpp
    DisplayLuaCallback.cpp
    Energy.cpp
    ExplosionManager.cpp
    FacialAnimation.cpp
    FacialAnimationManager.cpp
    FadeListener.cpp
    GibManager.cpp
    global.cpp
    HUD.cpp
    InputManager2.cpp
    Inventory.cpp
    LightPerfomaceManager.cpp
    Loader.cpp
    MagicManager.cpp
    main.cpp
    MeshDecalMgr.cpp
    MirrorManager.cpp
    Modulator.cpp
    MusicPlayer.cpp
    NamedPipeServer.cpp
    ogreconsole.cpp
    POs.cpp
    PreVideoViewer.cpp
    PSSMShadowListener.cpp
    Run3Batcher.cpp
    Run3Benchmark.cpp
    Run3Input.cpp
    Run3Shadowing.cpp
    SaveGame.cpp
    SceneLoadOverlay.cpp
    Sequence.cpp
    SequenceLuaCallback.cpp
    Serial.cpp
    SharedLuaCallback.cpp
    SkyManager.cpp
    SoftwareOcclusionCulling.cpp
    StereoManager.cpp
    SuperFX.cpp
    Timeshift.cpp
    WaterManager.cpp
    ZonePortalManager.cpp
    PhysObject.cpp
    PhysObjectMatCallback.cpp
    BlastWave.cpp
    BloodEmitter.cpp
    Button.cpp
    ButtonContactMatCallback.cpp
    Computer.cpp
    Credits.cpp
    CWeapon.cpp
    DefaultAEnt.cpp
    ExplosionEmitter.cpp
    Fader.cpp
    func_door.cpp
    FuzzyTest.cpp
    FuzzyTest2.cpp
    Generator.cpp
    Ladder.cpp
    LensFlare.cpp
    Pendulum.cpp
    Pickup.cpp
    PickupMatCallback.cpp
    Ragdoll.cpp
    Rotating.cpp
    SeqScript.cpp
    Soundscape.cpp
    Train.cpp
    Trigger.cpp
    VolumeLightManager.cpp
    Bullet.cpp
    BulletHitManager.cpp
    generic_lua_weapon.cpp
    LaserMinigun.cpp
    Punch.cpp
    Shockrifle.cpp
    Event.cpp
    EventChangeLevel.cpp
    EventEntC.cpp
    CutScene.cpp
    Player.cpp
    PlayerContactCallback.cpp
    AIManager.cpp
    NPCManager.cpp
    enemyMatCallback.cpp
    neutralMatCallback.cpp
    npc_aerial.cpp
    npc_enemy.cpp
    npc_friend.cpp
    npc_neutral.cpp
    npc_template.cpp
    AmbientLight.cpp
    DeferredShading.cpp
    GeomUtils.cpp
    LightMaterialGenerator.cpp
    MaterialGenerator.cpp
    MLight.cpp
    Eliza.cpp
    graphics.cpp
    recorder.cpp
    strings.cpp
    Tokenizer.cpp
    CTParameters.cpp
    CTSection.cpp
    CTSerializer.cpp
    CTStem.cpp)

# Reusable source subset compiled during Step 3. Every file is in the vcproj
# list above and has no dependency on an unreproducible legacy SDK.
set(RUN3_LEGACY_REUSABLE_SOURCES
    tinyxml.cpp
    tinyxmlerror.cpp
    tinyxmlparser.cpp
    recorder.cpp
    strings.cpp
    Tokenizer.cpp
    CTParameters.cpp
    CTSection.cpp
    CTSerializer.cpp
    CTStem.cpp
    AmbientLight.cpp
    GeomUtils.cpp
    LightMaterialGenerator.cpp
    MaterialGenerator.cpp
    MLight.cpp
    LensFlare.cpp
    DefaultAEnt.cpp
    EventEntC.cpp
    SceneLoadOverlay.cpp
    Run3Batcher.cpp
    PSSMShadowListener.cpp)

# The application entrypoint is deliberately not part of the reusable library.
set(RUN3_LEGACY_ENTRYPOINT_SOURCE main.cpp)

# Reviewed experimental/demo utilities. These are not required by The Long Way
# and will not be ported as part of the runtime compatibility target.
set(RUN3_LEGACY_OBSOLETE_DEMO_TOOL_SOURCES
    FuzzyTest.cpp
    FuzzyTest2.cpp
    graphics.cpp)

# This vcproj-listed file contains only an incomplete, unterminated commented
# copy of TinyXML's string implementation. TIXML_USE_STL does not need it.
set(RUN3_LEGACY_MALFORMED_UNUSED_SOURCES tinystr.cpp)

set(_run3_legacy_accounted_sources
    ${RUN3_LEGACY_REUSABLE_SOURCES}
    ${RUN3_LEGACY_ENTRYPOINT_SOURCE}
    ${RUN3_LEGACY_OBSOLETE_DEMO_TOOL_SOURCES}
    ${RUN3_LEGACY_MALFORMED_UNUSED_SOURCES})
set(RUN3_LEGACY_DEFERRED_SOURCES ${RUN3_LEGACY_VCPROJ_SOURCES})
list(REMOVE_ITEM RUN3_LEGACY_DEFERRED_SOURCES
     ${_run3_legacy_accounted_sources})

list(LENGTH RUN3_LEGACY_VCPROJ_SOURCES RUN3_LEGACY_VCPROJ_SOURCE_COUNT)
list(LENGTH RUN3_LEGACY_REUSABLE_SOURCES RUN3_LEGACY_REUSABLE_SOURCE_COUNT)
foreach(_source IN LISTS RUN3_LEGACY_VCPROJ_SOURCES)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_source}")
        message(FATAL_ERROR "Run3.vcproj source is missing: ${_source}")
    endif()
endforeach()

unset(_run3_legacy_accounted_sources)
unset(_source)

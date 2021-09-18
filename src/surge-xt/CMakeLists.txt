# vi:set sw=2 et:
project(surge-xt VERSION ${CMAKE_PROJECT_VERSION})

file(GLOB SURGE_XT_JUCE_RESOURCES_GLOB
  ${CMAKE_SOURCE_DIR}/resources/classic-skin-svgs/*.svg
  ${CMAKE_SOURCE_DIR}/resources/fonts/Lato*ttf
  ${CMAKE_SOURCE_DIR}/resources/fonts/IndieFlower.ttf
  ${CMAKE_SOURCE_DIR}/resources/fonts/FiraMono-Regular.ttf
  ${CMAKE_SOURCE_DIR}/resources/surge-xt/memory-skin.xml
  )

juce_add_binary_data(surge-xt-binary
  NAMESPACE SurgeXTBinary
  HEADER_NAME SurgeXTBinary.h
  SOURCES ${SURGE_XT_JUCE_RESOURCES_GLOB}
  )

if(WIN32 AND ${CMAKE_SIZEOF_VOID_P} EQUAL 4)
  set(SURGE_XT_PRODUCT_NAME "Surge XT (32-bit)")
else()
  set(SURGE_XT_PRODUCT_NAME "Surge XT")
endif()

juce_add_plugin(${PROJECT_NAME}
  PRODUCT_NAME ${SURGE_XT_PRODUCT_NAME}
  COMPANY_NAME "Surge Synth Team"
  COMPANY_WEBSITE "https://surge-synth-team.org/"
  BUNDLE_ID "org.surge-synth-team.surge-xt"
  PLUGIN_MANUFACTURER_CODE VmbA
  PLUGIN_CODE SgXT

  ICON_BIG "${CMAKE_SOURCE_DIR}/resources/surge-xt/SurgeLogo.png"

  IS_SYNTH TRUE
  NEEDS_MIDI_INPUT TRUE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE

  COPY_PLUGIN_AFTER_BUILD ${SURGE_COPY_AFTER_BUILD}

  VST3_CATEGORIES Instrument Synth
  VST2_CATEGORY kPlugCategSynth
  AU_MAIN_TYPE kAudioUnitType_MusicDevice

  LV2_URI https://surge-synthesizer.github.io/lv2/surge-xt
  LV2_SHARED_LIBRARY_NAME SurgeXT

  FORMATS ${SURGE_JUCE_FORMATS}
  )

if(JUCE_ASIO_SUPPORT)
  target_compile_definitions(${PROJECT_NAME} PUBLIC JUCE_ASIO=1)
endif()

target_sources(${PROJECT_NAME} PRIVATE
  SurgeSynthEditor.cpp
  SurgeSynthEditor.h
  SurgeSynthProcessor.cpp
  SurgeSynthProcessor.h

  util/LockFreeStack.h

  gui/AccessibleHelpers.h
  gui/ModulationGridConfiguration.h
  gui/RefreshableOverlay.h
  gui/RuntimeFont.cpp
  gui/RuntimeFont.h
  gui/SkinFontLoader.cpp
  gui/SkinImageMaps.h
  gui/SkinSupport.cpp
  gui/SkinSupport.h
  gui/SurgeGUICallbackInterfaces.h
  gui/SurgeGUIEditor.cpp
  gui/SurgeGUIEditor.h
  gui/SurgeGUIEditorHtmlGenerators.cpp
  gui/SurgeGUIEditorInfowindow.cpp
  gui/SurgeGUIEditorOverlays.cpp
  gui/SurgeGUIEditorTags.h
  gui/SurgeGUIEditorValueCallbacks.cpp
  gui/SurgeGUIUtils.cpp
  gui/SurgeGUIUtils.h
  gui/SurgeImage.cpp
  gui/SurgeImage.h
  gui/SurgeImageStore.cpp
  gui/SurgeImageStore.h
  gui/SurgeJUCEHelpers.h
  gui/SurgeJUCELookAndFeel.cpp
  gui/SurgeJUCELookAndFeel.h
  gui/UIInstrumentation.cpp
  gui/UIInstrumentation.h
  gui/overlays/AboutScreen.cpp
  gui/overlays/AboutScreen.h
  gui/overlays/CoveringMessageOverlay.cpp
  gui/overlays/CoveringMessageOverlay.h
  gui/overlays/LuaEditors.cpp
  gui/overlays/LuaEditors.h
  gui/overlays/MSEGEditor.cpp
  gui/overlays/MSEGEditor.h
  gui/overlays/MiniEdit.cpp
  gui/overlays/MiniEdit.h
  gui/overlays/ModulationEditor.cpp
  gui/overlays/ModulationEditor.h
  gui/overlays/OverlayWrapper.cpp
  gui/overlays/OverlayWrapper.h
  gui/overlays/PatchDBViewer.cpp
  gui/overlays/PatchDBViewer.h
  gui/overlays/PatchStoreDialog.cpp
  gui/overlays/PatchStoreDialog.h
  gui/overlays/TuningOverlays.cpp
  gui/overlays/TuningOverlays.h
  gui/overlays/TypeinParamEditor.cpp
  gui/overlays/TypeinParamEditor.h
  gui/overlays/WaveShaperAnalysis.cpp
  gui/overlays/WaveShaperAnalysis.h
  gui/widgets/EffectChooser.cpp
  gui/widgets/EffectChooser.h
  gui/widgets/EffectLabel.h
  gui/widgets/LFOAndStepDisplay.cpp
  gui/widgets/LFOAndStepDisplay.h
  gui/widgets/MainFrame.cpp
  gui/widgets/MainFrame.h
  gui/widgets/MenuCustomComponents.cpp
  gui/widgets/MenuCustomComponents.h
  gui/widgets/MenuForDiscreteParams.cpp
  gui/widgets/MenuForDiscreteParams.h
  gui/widgets/ModulatableControlInterface.h
  gui/widgets/ModulatableSlider.cpp
  gui/widgets/ModulatableSlider.h
  gui/widgets/ModulationSourceButton.cpp
  gui/widgets/ModulationSourceButton.h
  gui/widgets/MultiSwitch.cpp
  gui/widgets/MultiSwitch.h
  gui/widgets/NumberField.cpp
  gui/widgets/NumberField.h
  gui/widgets/OscillatorWaveformDisplay.cpp
  gui/widgets/OscillatorWaveformDisplay.h
  gui/widgets/ParameterInfowindow.cpp
  gui/widgets/ParameterInfowindow.h
  gui/widgets/PatchSelector.cpp
  gui/widgets/PatchSelector.h
  gui/widgets/Switch.cpp
  gui/widgets/Switch.h
  gui/widgets/VerticalLabel.cpp
  gui/widgets/VerticalLabel.h
  gui/widgets/VuMeter.cpp
  gui/widgets/VuMeter.h
  gui/widgets/WaveShaperSelector.cpp
  gui/widgets/WaveShaperSelector.h
  gui/widgets/WidgetBaseMixin.h
  gui/widgets/XMLConfiguredMenus.cpp
  gui/widgets/XMLConfiguredMenus.h
  )

if(EXISTS ${SURGE_JUCE_PATH}/modules/juce_gui_basics/accessibility/juce_AccessibilityHandler.h)
  set(SURGE_JUCE_ACCESSIBLE TRUE)
  message(STATUS "Including JUCE Accessibility Support")
else()
  message(STATUS "No JUCE Accessibility Support in this version")
endif()

if(EXISTS ${SURGE_JUCE_PATH}/modules/juce_audio_processors/processors/juce_AudioProcessorEditorHostContext.h)
  set(SURGE_JUCE_HOST_CONTEXT TRUE)
  message(STATUS "Including JUCE Host Context Support")
else()
  message(STATUS "No JUCE Host Context Support in this version")
endif()

if(EXISTS ${SURGE_JUCE_PATH}/modules/juce_audio_processors/utilities/juce_VST3ClientExtensions.h)
  set(SURGE_JUCE_VST3_EXTENSIONS TRUE)
  message(STATUS "Including JUCE VST3 Client Extensions")
else()
  message(STATUS "No JUCE VST3 Client Extensions")
endif()

target_compile_definitions(${PROJECT_NAME} PUBLIC
  SURGE_JUCE_ACCESSIBLE=$<BOOL:${SURGE_JUCE_ACCESSIBLE}>
  SURGE_JUCE_HOST_CONTEXT=$<BOOL:${SURGE_JUCE_HOST_CONTEXT}>
  SURGE_JUCE_VST3_EXTENSIONS=$<BOOL:${SURGE_JUCE_VST3_EXTENSIONS}>

  $<IF:$<CONFIG:DEBUG>,BUILD_IS_DEBUG,BUILD_IS_RELEASE>=1
  )

foreach(format ${SURGE_JUCE_FORMATS})
  target_sources(${PROJECT_NAME}_${format} PRIVATE plugin_type_extensions/SurgeSynth${format}Extensions.cpp)
endforeach()

target_link_libraries(${PROJECT_NAME} PRIVATE
  surge::surge-common
  surge-juce
  juce::juce_graphics
  juce::juce_audio_utils
  juce::juce_audio_processors
  juce::juce_audio_plugin_client
  PUBLIC surge-xt-binary # FIXME
  )

# FIXME: surge-gui and surge-xt depend on each other :(
target_include_directories(${PROJECT_NAME}
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/gui
  ${CMAKE_CURRENT_SOURCE_DIR})

surge_juce_package(${PROJECT_NAME} "Surge XT")
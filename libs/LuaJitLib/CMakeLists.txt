# Added LUA_ADD_EXECUTABLE Ryan Phillips <ryan at trolocsis.com>
# This CMakeLists.txt has been first taken from LuaDist
# Copyright (C) 2007-2011 LuaDist.
# Created by Peter Drahoš
# Redistribution and use of this file is allowed according to the terms of the MIT license.
# Debugged and (now seriously) modIFied by Ronan Collobert, for Torch7

project(LuaJIT C ASM)

#
# OK so what the heck is this? Well this is a surge-specific thing to make it cross compile
# a macOS universal lib. The CMake below doesn't understand CMAKE_OSX_ARCHITECTURES yet.
#

if(APPLE AND NOT CMAKE_CROSSCOMPILING)
  message(STATUS "Custom build for macOSX of luajit in fat mode")
  SET(LUAJIT_DIR ${CMAKE_CURRENT_LIST_DIR}/LuaJIT)
  SET(LUAJIT_BLD ${CMAKE_CURRENT_BINARY_DIR}/luajit)

  add_library(luajit-5.1 INTERFACE)
  target_link_libraries(luajit-5.1 INTERFACE ${LUAJIT_BLD}/bin/libluajit.a)
  target_include_directories(luajit-5.1 INTERFACE ${LUAJIT_DIR}/src)

  if(NOT EXISTS ${LUAJIT_BLD})
    message(STATUS "Building luajit at cmake time on macos right now")
    message(STATUS "./build-fat-luajit.sh \"${LUAJIT_BLD}\" in ${CMAKE_CURRENT_LIST_DIR}")
    execute_process(WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
      COMMAND ./build-fat-luajit.sh "${LUAJIT_BLD}"
      RESULT_VARIABLE LUAJIT_MAC_RESULT
      OUTPUT_VARIABLE O
      ERROR_VARIABLE E)
    if(${LUAJIT_MAC_RESULT} GREATER 0)
      message(WARNING ${O})
      message(WARNING ${E})
      message(FATAL_ERROR "MacOS Failed to build a universal luajit. Run 'build-fat-luajit' for more")
    endif()
    file(GLOB_RECURSE LUACONTENTS "${LUAJIT_BLD}/bin/*")
    message(STATUS ${LUACONTENTS})
  endif()
else()
  include(CMakeParseArguments)
  include(CheckCSourceCompiles)
  include(CheckFunctionExists)
  include(CheckLibraryExists)

  # In cross builds, find a host compiler
  if(CMAKE_CROSSCOMPILING)
    set(LUAJIT_HOSTCCS gcc clang)
    find_program(LUAJIT_HOSTCC NAMES ${LUAJIT_HOSTCCS})
    if(NOT LUAJIT_HOSTCC)
      string(REPLACE ";" ", " LUAJIT_HOSTCCS "${LUAJIT_HOSTCCS}")
      message(FATAL_ERROR "${PROJECT_NAME}: No host compiler found, tried: ${LUAJIT_HOSTCCS}")
    endif()
    # LuaJIT requires host and target to have the same pointer size
    math(EXPR LUAJIT_BITNESS "${CMAKE_SIZEOF_VOID_P} * 8" OUTPUT_FORMAT DECIMAL)
    message(STATUS "${PROJECT_NAME}: Using host compiler ${LUAJIT_HOSTCC} -m${LUAJIT_BITNESS}")
  endif()

  # Add an executable that can run on the build host
  function(luajit_add_host_executable name)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "" "COMPILE_DEFINITIONS;INCLUDE_DIRECTORIES;LIBS;SOURCES")
    if(CMAKE_CROSSCOMPILING)
      add_custom_target(${name} DEPENDS ${ARG_SOURCES})
      list(FILTER ARG_SOURCES INCLUDE REGEX "\.c$")
      if(APPLE)
        list(APPEND ARG_COMPILE_DEFINITIONS LUAJIT_OS=LUAJIT_OS_OSX)
      elseif(WIN32)
        list(APPEND ARG_COMPILE_DEFINITIONS LUAJIT_OS=LUAJIT_OS_WINDOWS)
      elseif(UNIX)
        list(APPEND ARG_COMPILE_DEFINITIONS LUAJIT_OS=LUAJIT_OS_LINUX)
      else()
        list(APPEND ARG_COMPILE_DEFINITIONS LUAJIT_OS=LUAJIT_OS_OTHER)
      endif()
      list(TRANSFORM ARG_COMPILE_DEFINITIONS PREPEND -D)
      list(TRANSFORM ARG_INCLUDE_DIRECTORIES PREPEND -I)
      list(TRANSFORM ARG_LIBS PREPEND -l)
      add_custom_command(TARGET ${name}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>
        COMMAND ${LUAJIT_HOSTCC} -m${LUAJIT_BITNESS} -o ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/${name} ${ARG_COMPILE_DEFINITIONS} ${ARG_INCLUDE_DIRECTORIES} ${ARG_SOURCES} ${ARG_LIBS}
        )
    else()
      add_executable(${name} ${ARG_SOURCES})
      get_property(generatorIsMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
      if(NOT generatorIsMultiConfig)
        set_target_properties(${name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>)
      endif()
      target_compile_definitions(${name} PRIVATE ${ARG_COMPILE_DEFINITIONS})
      target_include_directories(${name} PRIVATE ${ARG_INCLUDE_DIRECTORIES})
      target_link_libraries(${name} PRIVATE ${ARG_LIBS})
    endif()
  endfunction()

  # Check for a library on the build host
  function(luajit_check_host_library_exists lib func variable)
    if(CMAKE_CROSSCOMPILING)
      if(DEFINED CACHE{${variable}})
        return()
      endif()
      set(fname ${CMAKE_CURRENT_BINARY_DIR}/LuaJITCheckHostLibraryExists_${variable})
      file(WRITE ${fname}.c "extern int ${func}(void); int main(void) { return ${func}(); }\n")
      message(STATUS ${LUAJIT_HOSTCC} -m${LUAJIT_BITNESS} -o ${fname} -l${lib} ${fname}.c)
      execute_process(COMMAND ${LUAJIT_HOSTCC} -m${LUAJIT_BITNESS} -o ${fname} -l${lib} ${fname}.c
        RESULT_VARIABLE status
        OUTPUT_QUIET
        ERROR_QUIET
        )
      if(status AND NOT status EQUAL 0)
        set(status 0)
      else()
        set(status 1)
      endif()
      set(${variable} ${status} CACHE INTERNAL "Have host library ${lib}")
    else()
      check_library_exists(${lib} ${func} "" ${variable})
    endif()
  endfunction()

  SET(LUAJIT_DIR ${CMAKE_CURRENT_LIST_DIR}/LuaJIT)

  SET(CMAKE_REQUIRED_INCLUDES
    ${LUAJIT_DIR}
    ${LUAJIT_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    )

  OPTION(WITH_AMALG "Build eveything in one shot (needs memory)" ON)

  # Ugly warnings
  IF(MSVC)
    ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)
  ENDIF()

  # LuaJIT specIFic
  option(LUAJIT_DISABLE_FFI "Disable FFI." OFF)
  option(LUAJIT_ENABLE_LUA52COMPAT "Enable Lua 5.2 compatibility." ON)
  option(LUAJIT_DISABLE_JIT "Disable JIT." OFF)
  option(LUAJIT_CPU_SSE2 "Use SSE2 instead of x87 instructions." ON)
  option(LUAJIT_CPU_NOCMOV "Disable NOCMOV." OFF)
  option(LUAJIT_DISABLE_GC64 "Disable GC64" OFF)
  MARK_AS_ADVANCED(LUAJIT_DISABLE_FFI LUAJIT_ENABLE_LUA52COMPAT
    LUAJIT_DISABLE_GC64
    LUAJIT_DISABLE_JIT LUAJIT_CPU_SSE2 LUAJIT_CPU_NOCMOV)

  IF(LUAJIT_DISABLE_FFI)
    ADD_DEFINITIONS(-DLUAJIT_DISABLE_FFI)
  ENDIF()

  IF(LUAJIT_ENABLE_LUA52COMPAT)
    ADD_DEFINITIONS(-DLUAJIT_ENABLE_LUA52COMPAT)
  ENDIF()

  IF(LUAJIT_DISABLE_GC64)
    ADD_DEFINITIONS(-DLUAJIT_DISABLE_GC64)
  ENDIF()

  IF(LUAJIT_DISABLE_JIT)
    ADD_DEFINITIONS(-DLUAJIT_DISABLE_JIT)
  ENDIF()

  IF(LUAJIT_CPU_SSE2)
    ADD_DEFINITIONS(-DLUAJIT_CPU_SSE2)
  ENDIF()

  IF(LUAJIT_CPU_NOCMOV)
    ADD_DEFINITIONS(-DLUAJIT_CPU_NOCMOV)
  ENDIF()
  ######

  IF(${CMAKE_SIZEOF_VOID_P} EQUAL 8)
    ADD_DEFINITIONS(-D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE)
  ENDIF()

  IF(WIN32 AND NOT CYGWIN)
    SET(LJVM_MODE peobj)
  elseIF(APPLE)
    SET(CMAKE_EXE_LINKER_FLAGS "-pagezero_size 10000 -image_base 100000000 ${CMAKE_EXE_LINKER_FLAGS}")
    SET(LJVM_MODE machasm)
  else()
    SET(LJVM_MODE elfasm)
  endIF()

  IF(NOT WIN32)
    FIND_LIBRARY(DL_LIBRARY "dl")
    IF(DL_LIBRARY)
      SET(CMAKE_REQUIRED_LIBRARIES ${DL_LIBRARY})
      LIST(APPEND LIBS ${DL_LIBRARY})
    ENDIF(DL_LIBRARY)
    CHECK_FUNCTION_EXISTS(dlopen LUA_USE_DLOPEN)
    IF(NOT LUA_USE_DLOPEN)
      MESSAGE(ERROR "Cannot compile a useful lua.
  Function dlopen() seems not to be supported on your platform.
  Apparently you are not on a Windows platform as well.
  So lua has no way to deal with shared libraries!
  
  You can safely ignore this if cross compiling on linux to arm tho
")
    ENDIF(NOT LUA_USE_DLOPEN)
  ENDIF(NOT WIN32)

  check_library_exists(m sin "" LUA_USE_LIBM)
  IF(LUA_USE_LIBM)
    list(APPEND LIBS m)
  endIF()

  IF(CMAKE_SYSTEM_NAME MATCHES "OpenBSD")
    list(APPEND LIBS pthread c++abi)
  endIF()

  ## SOURCES
  MACRO(LJ_TEST_ARCH stuff)
    MESSAGE(STATUS "Checking ${stuff} ${CMAKE_REQUIRED_INCLUDES}")
    CHECK_C_SOURCE_COMPILES("
  #undef ${stuff}
  #include \"lj_arch.h\"
  #if ${stuff}
  int main() { return 0; }
  #else
  #error \"not defined\"
  #endif
  " ${stuff})
  ENDMACRO()

  MACRO(LJ_TEST_ARCH_VALUE stuff value)
    CHECK_C_SOURCE_COMPILES("
  #undef ${stuff}
  #include \"lj_arch.h\"
  #if ${stuff} == ${value}
  int main() { return 0; }
  #else
  #error \"not defined\"
  #endif
  " ${stuff}_${value})
  ENDMACRO()


  FOREACH(arch X64 X86 ARM ARM64 AARM64)
    LJ_TEST_ARCH(LJ_TARGET_${arch})
    IF(LJ_TARGET_${arch})
      STRING(TOLOWER ${arch} TARGET_LJARCH)
      MESSAGE(STATUS "LuaJIT target: ${TARGET_LJARCH}")
      BREAK()
    ENDIF()
  ENDFOREACH()

  IF(NOT TARGET_LJARCH)
    MESSAGE(FATAL_ERROR "Architecture not supported!")
  ELSE()
    MESSAGE(STATUS "LuaJIT target: ${TARGET_LJARCH}")
  ENDIF()

  FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/jit)
  FILE(GLOB jit_files ${LUAJIT_DIR}/src/jit/*.lua)
  FILE(COPY ${jit_files} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/jit)

  SET(DASM_ARCH ${TARGET_LJARCH})
  SET(DASM_FLAGS)
  SET(TARGET_ARCH)
  LIST(APPEND TARGET_ARCH "LUAJIT_TARGET=LUAJIT_ARCH_${TARGET_LJARCH}")
  LJ_TEST_ARCH_VALUE(LJ_ARCH_BITS 64)
  IF(LJ_ARCH_BITS_64)
    SET(DASM_FLAGS ${DASM_FLAGS} -D P64)
  ENDIF()
  LJ_TEST_ARCH_VALUE(LJ_HASJIT 1)
  IF(LJ_HASJIT_1)
    SET(DASM_FLAGS ${DASM_FLAGS} -D JIT)
  ENDIF()
  LJ_TEST_ARCH_VALUE(LJ_HASFFI 1)
  IF(LJ_HASFFI_1)
    SET(DASM_FLAGS ${DASM_FLAGS} -D FFI)
  ENDIF()
  LJ_TEST_ARCH_VALUE(LJ_DUALNUM 1)
  IF(LJ_DUALNUM_1)
    SET(DASM_FLAGS ${DASM_FLAGS} -D DUALNUM)
  ENDIF()
  LJ_TEST_ARCH_VALUE(LJ_ARCH_HASFPU 1)
  IF(LJ_ARCH_HASFPU_1)
    SET(DASM_FLAGS ${DASM_FLAGS} -D FPU)
    LIST(APPEND TARGET_ARCH "LJ_ARCH_HASFPU=1")
  ELSE()
    LIST(APPEND TARGET_ARCH "LJ_ARCH_HASFPU=0")
  ENDIF()
  LJ_TEST_ARCH_VALUE(LJ_ABI_SOFTFP 1)
  IF(NOT LJ_ABI_SOFTFP_1)
    SET(DASM_FLAGS ${DASM_FLAGS} -D HFABI)
    LIST(APPEND TARGET_ARCH "LJ_ABI_SOFTFP=0")
  ELSE()
    LIST(APPEND TARGET_ARCH "LJ_ABI_SOFTFP=1")
  ENDIF()
  IF(WIN32)
    SET(DASM_FLAGS ${DASM_FLAGS} -LN -D WIN)
  ENDIF()
  IF(TARGET_LJARCH STREQUAL "x86")
    LJ_TEST_ARCH_VALUE(__SSE2__ 1)
    IF(__SSE2__1)
      SET(DASM_FLAGS ${DASM_FLAGS} -D SSE)
    ENDIF()
  ENDIF()
  IF(TARGET_LJARCH STREQUAL "ppc")
    LJ_TEST_ARCH_VALUE(LJ_ARCH_SQRT 1)
    IF(NOT LJ_ARCH_SQRT_1)
      SET(DASM_FLAGS ${DASM_FLAGS} -D SQRT)
    ENDIF()
    LJ_TEST_ARCH_VALUE(LJ_ARCH_PPC64 1)
    IF(NOT LJ_ARCH_PPC64_1)
      SET(DASM_FLAGS ${DASM_FLAGS} -D GPR64)
    ENDIF()
  ENDIF()

  luajit_check_host_library_exists(m sin MINILUA_USE_LIBM)
  IF(MINILUA_USE_LIBM)
    set(MINILUA_LIBS m)
  ENDIF()
  if(LINUX_ON_ARM)
    message(STATUS "Forcing minilua libm on LINUX_ON_ARM")
    list(APPEND MINILUA_LIBS m)
  endif()

  luajit_add_host_executable(minilua
    SOURCES ${LUAJIT_DIR}/src/host/minilua.c
    COMPILE_DEFINITIONS ${TARGET_ARCH}
    INCLUDE_DIRECTORIES ${CMAKE_REQUIRED_INCLUDES}
    LIBS ${MINILUA_LIBS}
    )

  add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/buildvm_arch.h
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/minilua ${LUAJIT_DIR}/dynasm/dynasm.lua ${DASM_FLAGS} -o ${CMAKE_CURRENT_BINARY_DIR}/buildvm_arch.h ${LUAJIT_DIR}/src/vm_${DASM_ARCH}.dasc
    DEPENDS ${LUAJIT_DIR}/dynasm/dynasm.lua minilua
    )

  ## Source Lists
  SET(SRC_LJLIB
    ${LUAJIT_DIR}/src/lib_base.c
    ${LUAJIT_DIR}/src/lib_math.c
    ${LUAJIT_DIR}/src/lib_bit.c
    ${LUAJIT_DIR}/src/lib_string.c
    ${LUAJIT_DIR}/src/lib_table.c
    ${LUAJIT_DIR}/src/lib_io.c
    ${LUAJIT_DIR}/src/lib_os.c
    ${LUAJIT_DIR}/src/lib_package.c
    ${LUAJIT_DIR}/src/lib_debug.c
    ${LUAJIT_DIR}/src/lib_jit.c
    ${LUAJIT_DIR}/src/lib_ffi.c
    )

  SET(SRC_LIBAUX
    ${LUAJIT_DIR}/src/lib_aux.c
    ${LUAJIT_DIR}/src/lib_init.c
    )
  file(GLOB_RECURSE SRC_LJCORE "${LUAJIT_DIR}/src/lj_*.c")
  list(APPEND SRC_LJCORE ${SRC_LJLIB} ${SRC_LIBAUX})
  file(GLOB_RECURSE SRC_BUILDVM "${LUAJIT_DIR}/src/host/buildvm*.c")
  list(APPEND SRC_BUILDVM ${CMAKE_CURRENT_BINARY_DIR}/buildvm_arch.h)

  ## GENERATE
  luajit_add_host_executable(buildvm
    SOURCES ${SRC_BUILDVM}
    COMPILE_DEFINITIONS ${TARGET_ARCH}
    INCLUDE_DIRECTORIES ${CMAKE_REQUIRED_INCLUDES}
    )

  macro(add_buildvm_target _target _mode)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_target}
      COMMAND ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/buildvm ARGS -m ${_mode} -o ${CMAKE_CURRENT_BINARY_DIR}/${_target} ${ARGN}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      DEPENDS buildvm ${ARGN}
      )
  endmacro(add_buildvm_target)

  IF(WIN32)
    add_buildvm_target(lj_vm.obj peobj)
    SET(LJ_VM_SRC ${CMAKE_CURRENT_BINARY_DIR}/lj_vm.obj)
  else()
    add_buildvm_target(lj_vm.S ${LJVM_MODE})
    SET(LJ_VM_SRC ${CMAKE_CURRENT_BINARY_DIR}/lj_vm.S)
  endIF()
  add_buildvm_target(lj_ffdef.h ffdef ${SRC_LJLIB})
  add_buildvm_target(lj_bcdef.h bcdef ${SRC_LJLIB})
  add_buildvm_target(lj_folddef.h folddef ${LUAJIT_DIR}/src/lj_opt_fold.c)
  add_buildvm_target(lj_recdef.h recdef ${SRC_LJLIB})
  add_buildvm_target(lj_libdef.h libdef ${SRC_LJLIB})
  add_buildvm_target(vmdef.lua vmdef ${SRC_LJLIB})

  SET(DEPS
    ${LJ_VM_SRC}
    ${CMAKE_CURRENT_BINARY_DIR}/lj_ffdef.h
    ${CMAKE_CURRENT_BINARY_DIR}/lj_bcdef.h
    ${CMAKE_CURRENT_BINARY_DIR}/lj_libdef.h
    ${CMAKE_CURRENT_BINARY_DIR}/lj_recdef.h
    ${CMAKE_CURRENT_BINARY_DIR}/lj_folddef.h
    ${CMAKE_CURRENT_BINARY_DIR}/vmdef.lua
    )

  ## COMPILE
  include_directories(
    ${LUAJIT_DIR}/dynasm
    ${LUAJIT_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
  )

  IF(WITH_SHARED_LUA)
    IF(WITH_AMALG)
      add_library(luajit-5.1 SHARED ${LUAJIT_DIR}/src/ljamalg.c ${DEPS})
    ELSE()
      add_library(luajit-5.1 SHARED ${SRC_LJCORE} ${DEPS})
    ENDIF()
    SET_TARGET_PROPERTIES(luajit-5.1 PROPERTIES OUTPUT_NAME "lua51")
  ELSE()
    IF(WITH_AMALG)
      add_library(luajit-5.1 STATIC ${LUAJIT_DIR}/src/ljamalg.c ${DEPS})
    ELSE()
      add_library(luajit-5.1 STATIC ${SRC_LJCORE} ${DEPS})
    ENDIF()
    SET_TARGET_PROPERTIES(luajit-5.1 PROPERTIES
      PREFIX "lib" IMPORT_PREFIX "lib" OUTPUT_NAME "luajit")
  ENDIF()

  target_link_libraries(luajit-5.1 ${LIBS})
  target_include_directories(luajit-5.1 PUBLIC ${LUAJIT_DIR}/src)

  IF(WIN32)
    add_executable(luajit ${LUAJIT_DIR}/src/luajit.c)
    target_link_libraries(luajit luajit-5.1)
  ELSE()
    IF(WITH_AMALG)
      add_executable(luajit ${LUAJIT_DIR}/src/luajit.c ${LUAJIT_DIR}/src/ljamalg.c ${DEPS})
      # When using WITH_AMALG during a parallel build, its possible to run into
      # false-positive "error: 'fold_hash' undeclared" compile errors due to a weird interaction
      # when building two ljamalg.c at the same time.
      #
      # This adds a fake dependency from one to the other, forcing the build process to
      # compile them sequentially rather than parallel.
      #
      # See https://github.com/torch/luajit-rocks/issues/39
      add_dependencies(luajit luajit-5.1)
    ELSE()
      add_executable(luajit ${LUAJIT_DIR}/src/luajit.c ${SRC_LJCORE} ${DEPS})
    ENDIF()
    target_link_libraries(luajit ${LIBS})
    SET_TARGET_PROPERTIES(luajit PROPERTIES ENABLE_EXPORTS ON)
  ENDIF()

  MACRO(LUAJIT_add_custom_commands luajit_target)
    SET(target_srcs "")
    FOREACH(file ${ARGN})
      IF(${file} MATCHES ".*\\.lua$")
        IF(NOT IS_ABSOLUTE ${file})
          SET(file "${CMAKE_CURRENT_SOURCE_DIR}/${file}")
        endIF()
        SET(source_file ${file})
        string(LENGTH ${CMAKE_SOURCE_DIR} _luajit_source_dir_length)
        string(LENGTH ${file} _luajit_file_length)
        math(EXPR _begin "${_luajit_source_dir_length} + 1")
        math(EXPR _stripped_file_length "${_luajit_file_length} - ${_luajit_source_dir_length} - 1")
        string(SUBSTRING ${file} ${_begin} ${_stripped_file_length} stripped_file)

        SET(generated_file "${CMAKE_CURRENT_BINARY_DIR}/jitted_tmp/${stripped_file}_${luajit_target}_generated${CMAKE_C_OUTPUT_EXTENSION}")

        add_custom_command(
          OUTPUT ${generated_file}
          MAIN_DEPENDENCY ${source_file}
          DEPENDS luajit
          COMMAND luajit
          ARGS -bg
          ${source_file}
          ${generated_file}
          COMMENT "Building Luajitted ${source_file}: ${generated_file}"
        )

        get_filename_component(basedir ${generated_file} PATH)
        file(MAKE_DIRECTORY ${basedir})

        SET(target_srcs ${target_srcs} ${generated_file})
        SET_source_files_properties(
          ${generated_file}
          properties
          external_object true # this is an object file
          generated true        # to say that "it is OK that the obj-files do not exist before build time"
        )
      ELSE()
        SET(target_srcs ${target_srcs} ${file})
      ENDIF(${file} MATCHES ".*\\.lua$")
    ENDFOREACH(file)
  ENDMACRO()

  MACRO(LUA_ADD_EXECUTABLE luajit_target)
    LUAJIT_add_custom_commands(${luajit_target} ${ARGN})
    add_executable(${luajit_target} ${target_srcs})
  ENDMACRO(LUA_ADD_EXECUTABLE luajit_target)
endif()

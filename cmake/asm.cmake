if(WIN32)
    set(_default_binfmt pe)
elseif(APPLE)
    set(_default_binfmt mach-o)
else()
    set(_default_binfmt elf)
endif()

set(AR_BINARY_FORMAT "${_default_binfmt}" CACHE STRING "ar binary format (elf, mach-o, pe, xcoff)")
set_property(CACHE AR_BINARY_FORMAT PROPERTY STRINGS elf mach-o pe xcoff)

unset(_default_binfmt)


## ABI

math(EXPR _bits "${CMAKE_SIZEOF_VOID_P}*8")

if(WIN32)
    set(_default_abi ms)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^arm" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
    set(_default_abi aapcs)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^mips")
    if(_bits EQUAL 32)
        set(_default_abi o32)
    else()
        set(_default_abi n64)
    endif()
else()
    set(_default_abi sysv)
endif()

set(AR_ABI "${_default_abi}" CACHE STRING "AR ABI (aapcs, eabi, ms, n32, n64, o32, o64, sysv, x32)")
set_property(CACHE AR_ABI PROPERTY STRINGS aapcs eabi ms n32 n64 o32 o64 sysv x32)

unset(_default_abi)

## Arch-and-model

set(_all_archs arm arm64 loongarch64 mips32 mips64 ppc32 ppc64 riscv64 s390x i386 x86_64 combined)

# Try at start to auto determine arch from CMake.
if(CMAKE_SYSTEM_PROCESSOR IN_LIST _all_archs)
    set(_default_arch ${CMAKE_SYSTEM_PROCESSOR})
elseif(_bits EQUAL 32)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^arm")
        set(_default_arch arm)
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^mips")
        set(_default_arch mips32)
    else()
        set(_default_arch i386)
    endif()
else()
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64" OR
            CMAKE_SYSTEM_PROCESSOR MATCHES "^arm") # armv8
        set(_default_arch arm64)
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^mips")
        set(_default_arch mips64)
    else()
        set(_default_arch x86_64)
    endif()
endif()

set(AR_ARCHITECTURE "${_default_arch}" CACHE STRING "Architecture (arm, arm64, loongarch64, mips32, mips64, ppc32, ppc64, riscv64, s390x, i386, x86_64, combined)")
set_property(CACHE AR_ARCHITECTURE PROPERTY STRINGS ${_all_archs})

unset(_all_archs)
unset(_bits)
unset(_default_arch)

## Assembler type

if(MSVC)
    set(_default_asm masm)
else()
    set(_default_asm gas)
endif()

set(AR_ASSEMBLER "${_default_asm}" CACHE STRING "AR assembler (masm, gas, armasm)")
set_property(CACHE AR_ASSEMBLER PROPERTY STRINGS masm gas armasm)

unset(_default_asm)

## Assembler source suffix

if(AR_BINARY_FORMAT STREQUAL pe)
    set(_default_ext .asm)
elseif(AR_ASSEMBLER STREQUAL gas)
    set(_default_ext .S)
else()
    set(_default_ext .asm)
endif()

set(AR_ASM_SUFFIX "${_default_ext}" CACHE STRING "AR assembler source suffix (.asm, .S)")
set_property(CACHE AR_ASM_SUFFIX PROPERTY STRINGS .asm .S)

unset(_default_ext)


if(AR_ASSEMBLER STREQUAL gas)
    if(CMAKE_CXX_PLATFORM_ID MATCHES "Cygwin")
        enable_language(ASM-ATT)
    else()
        enable_language(ASM)
    endif()
else()
    enable_language(ASM_MASM)
endif()


message(STATUS "Async runtime: "
        "architecture ${AR_ARCHITECTURE}, "
        "binary format ${AR_BINARY_FORMAT}, "
        "ABI ${AR_ABI}, "
        "assembler ${AR_ASSEMBLER}, "
        "suffix ${AR_ASM_SUFFIX}")


if(AR_BINARY_FORMAT STREQUAL mach-o)
    set(AR_BINARY_FORMAT macho)
endif()

set(_asm_suffix ${AR_ARCHITECTURE}_${AR_ABI}_${AR_BINARY_FORMAT}_${AR_ASSEMBLER}${AR_ASM_SUFFIX})

set(ASM_SOURCES
        src/asm/make_${_asm_suffix}
        src/asm/jump_${_asm_suffix}
        src/asm/ontop_${_asm_suffix}
        )

unset(_asm_suffix)

if(AR_ASSEMBLER STREQUAL masm AND AR_ARCHITECTURE STREQUAL i386)
    set_source_files_properties(${ASM_SOURCES} PROPERTIES COMPILE_FLAGS "/safeseh")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set_property(SOURCE ${ASM_SOURCES} APPEND PROPERTY COMPILE_OPTIONS "-x" "assembler-with-cpp")
endif()

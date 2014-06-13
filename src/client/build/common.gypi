{
  'variables': {
    'library%': 'static_library',
    'PROTOBUF' : '<!(echo %PROTOBUF%)/src',
    'GTEST' : '<!(echo %GTEST%)',
    'ZLIB' : '<!(echo %ZLIB%)',
    'WTL80' : '<!(echo %WTL80%)',
    'CERT' : '<(DEPTH)/resources/test.pfx',
    'CERT_PSWD' : '111111',
  },
  'target_defaults': {
    'cflags': ['-fPIC', '-std=c++0x'],
    'defines': ['ARCH_PIII', 'GUNIT_NO_GOOGLE3', 'ARCH_CPU_X86_FAMILY'],
    'include_dirs': [
      '<(DEPTH)',
      '<(DEPTH)/build/msvc_precompile/',
      '<(PROTOBUF)',
      '<(GTEST)/include/',
      '<(ZLIB)',
      '..',
    ],
    'default_configuration': 'Debug',
    'configurations': {
      'Debug': {
        'defines': [ 'DEBUG', '_DEBUG' ],
        'msvs_settings': {
          'VCCLCompilerTool': {
            'RuntimeLibrary': '1',                 # /MTd
            'Optimization': 0,                     # / Od
          },
          'VCLinkerTool': {
            'LinkIncremental': 1,
            'GenerateDebugInformation': 'true',
            'AdditionalLibraryDirectories': [
                '../external/thelibrary/lib/debug'
            ]
          },
        },
      },
      'Release': {
        'defines': [ 'NDEBUG' ],
        'msvs_settings': {
          'VCCLCompilerTool': {
            'RuntimeLibrary': '0',                 # /MT
            'Optimization': 2,                     # / O2
            'InlineFunctionExpansion': 2,
            'WholeProgramOptimization': 'true',
            'OmitFramePointers': 'true',
            'EnableFunctionLevelLinking': 'true',
            'EnableIntrinsicFunctions': 'true'            
          },
          'VCLinkerTool': {
            'LinkTimeCodeGeneration': 1,
            'OptimizeReferences': 2,
            'EnableCOMDATFolding': 2,
            'LinkIncremental': 1,
          }
        },
      },
    },
  },
  'conditions': [
    ['OS=="linux"', {
      'target_defaults': {
        'defines': [
          'HASH_NAMESPACE=__gnu_cxx',
          'COMPILER_GCC',
          'OS_LINUX',
          'OS_POSIX',
          '__STDC_CONSTANT_MACROS',   # for UINT64_C-like macros in stdint.h
        ],
      },
    },],
    ['OS=="mac"', {
      'target_defaults': {
        'cflags': ['-Wc++11-extensions'],
        'defines': ['HASH_NAMESPACE=__gnu_cxx', 'OS_MACOSX', 'OS_POSIX'],
        'include_dirs': [
          '../base/mac/stl',
        ],
        'xcode_settings': {
          'ALWAYS_SEARCH_USER_PATHS': 'NO',
          # 'CLANG_CXX_LANGUAGE_STANDARD': 'gnu++0x',
          'GCC_C_LANGUAGE_STANDARD':  'gnu99',
          'CLANG_ENABLE_OBJC_ARC': 'YES',
          'ARCHS': '$(ARCHS_STANDARD_64_BIT)',
        },
      },
    },],
    ['OS=="win"', {
      'target_defaults': {
        'variables': {
          'signing%': 'false',
        },
        'defines': [
          'COMPILER_MSVC',
          '_ATL_ALLOW_CHAR_UNSIGNED',
          '_CRT_NON_CONFORMING_SWPRINTFS', # atlapp.h need this macro to use
                                           # proper version of vswprintf
          '_CRT_SECURE_NO_WARNINGS',
          '_HAS_EXCEPTIONS=0',
          'STL_MSVC',
          'STRSAFE_NO_DEPRECATE',
          'UNICODE',
          '_UNICODE',
          '_WIN32_WINNT=0x0501', # Windows 2000 is not supported since Goopy V2
          '_WIN32_IE=0x0500',
          '_WINDOWS',
          'WINVER=0x0501',       # Windows 2000 is not supported since Goopy V2
          '_WTL_NO_CSTRING',
          'OS_WIN', # Support chrome threading
          'OS_WINDOWS',
          'COMPILER_MSVC',
          'ARCH_CPU_X86_FAMILY',
          'NOMINMAX',
          '_SCL_SECURE_NO_WARNINGS',
          '_VARIADIC_MAX=10',
          '__windows__',
          'NO_SHLWAPI_ISOS',
          '_USING_V110_SDK71_',
        ],
        'include_dirs': [
          '<(DEPTH)/base/posix/',
          '<(WTL80)/include',
        ],
        'msvs_disabled_warnings': [
          4018,
          4100,
          4125,
          4127,
          4244,
          4267,
          4288, # Suppresses loop control variable used outside of the loop
          4389,
          4800,
          4996,
        ],  # /wdXXXX
        'msvs_configuration_attributes': {
          'OutputDirectory': '<(DEPTH)/out/bin/$(ConfigurationName)',
          'IntermediateDirectory': '<(DEPTH)/out/obj/$(ProjectName)/$(Configuration)',
        },
        'msvs_settings': {
          'VCCLCompilerTool': {
            'ExceptionHandling': '1',              # /EHsc
            'BufferSecurityCheck': 'true',         # /GS
            'EnableFunctionLevelLinking': 'true',  # /Gy
            'DefaultCharIsUnsigned': 'true',       # /J
            'SuppressStartupBanner': 'true',       # /nologo
            'WarningLevel': '3',                   # /W3
            'WarnAsError': 'true',                 # /WX
            'UsePrecompiledHeader': '1',           # /Yc
            'UseFullPaths': 'true',                # /FC
            'PrecompiledHeaderThrough': 'precompile.h',
            'ForcedIncludeFiles': 'precompile.h',
          },
          'VCLinkerTool': {
            'AdditionalDependencies': [
              'advapi32.lib',
              'crypt32.lib',
              'dbghelp.lib',
              'kernel32.lib',
              'iphlpapi.lib',
              'imm32.lib',
              'psapi.lib',
              'shell32.lib',
              'shlwapi.lib',
              'urlmon.lib',
              'user32.lib',
              'usp10.lib',
              'version.lib',
              'wininet.lib',
              'msxml2.lib',    # needed by msxml parser
              'ole32.lib',     # needed by "CoInintialize" in msxml parser
              'oleaut32.lib',  # needed by "SysFreeString" in msxml parser
            ],
            'DataExecutionPrevention': '2',        # /NXCOMPAT
            'LinkIncremental': '1',                # /INCREMENTAL:NO
            'RandomizedBaseAddress': '2',          # /DYNAMICBASE
            'GenerateManifest': 'false',           # /MANIFEST:NO
          },
        },
        'target_conditions': [
          ['signing == "true"', {
            'msvs_postbuild': 'signtool.exe sign /f <(CERT) /p <(CERT_PSWD) $(TargetPath)'
          }],
          ['_target_name.endswith("_x64")', {
            'msvs_configuration_platform': 'x64',
            'defines': ['WIN64', 'ARCH_CPU_64_BITS',],
          }, { #else
            'msvs_configuration_platform': 'WIN32',
            'defines': ['WIN32',],
          },]
        ],
      },
    }],
  ],
}

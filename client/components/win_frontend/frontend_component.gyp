{
  'variables': {
    'srcs': [
      'application_ui_component.cc',
      'application_ui_component.h',
      'composition_window.cc',
      'composition_window.h',
      'composition_window_layouter.cc',
      'composition_window_layouter.h',
      'frontend_component.cc',
      'frontend_component.h',
      'frontend_factory.cc',
      'frontend_factory.h',
      'input_method.cc',
      'ipc_singleton.cc',
      'ipc_singleton.h',
      'ipc_ui_manager.cc',
      'ipc_ui_manager.h',
      'main.cc',
      'main.rc',
      'main.def',
      'resource.h',
      'text_styles.cc',
      'text_styles.h',
    ],
  },
  'targets': [
    {
      'target_name': 'frontend_component',
      'type': 'shared_library',
      'dependencies': [
	'<(DEPTH)/appsensorapi/appsensorapi.gyp:appsensorapi',
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/common/common.gyp:common',
        '<(DEPTH)/components/common/common.gyp:component_common',
        '<(DEPTH)/imm/imm.gyp:imm',
        '<(DEPTH)/ipc/ipc.gyp:ipc',
        '<(DEPTH)/ipc/protos/protos.gyp:protos-cpp',
        '<(DEPTH)/locale/locale.gyp:locale',
        '<(DEPTH)/tsf/tsf.gyp:tsf',
      ],
      'include_dirs': [
        '<(DEPTH)/third_party/google_gadgets_for_linux',
      ],
      'sources': [
        '<@(srcs)',
      ],
      'msvs_settings=': {
        'VCLinkerTool': {
          'ModuleDefinitionFile':'main.def',
        },
      },
      'variables': {
        'signing%': 'true',
      },
    },
    {
      'target_name': 'frontend_component_x64',
      'type': 'shared_library',
      'dependencies': [
	'<(DEPTH)/appsensorapi/appsensorapi.gyp:appsensorapi_x64',
        '<(DEPTH)/base/base.gyp:base_x64',
        '<(DEPTH)/common/common.gyp:common_x64',
        '<(DEPTH)/components/common/common.gyp:component_common_x64',
        '<(DEPTH)/imm/imm.gyp:imm_x64',
        '<(DEPTH)/ipc/ipc.gyp:ipc_x64',
        '<(DEPTH)/ipc/protos/protos.gyp:protos-cpp_x64',
        '<(DEPTH)/locale/locale.gyp:locale_x64',
        '<(DEPTH)/tsf/tsf.gyp:tsf_x64',
      ],
      'include_dirs': [
        '<(DEPTH)/third_party/google_gadgets_for_linux',
      ],
      'sources': [
        '<@(srcs)',
      ],
      'msvs_settings=': {
        'VCLinkerTool': {
          'ModuleDefinitionFile':'main.def',
        },
      },
      'variables': {
        'signing%': 'true',
      },
    },
  ],
}

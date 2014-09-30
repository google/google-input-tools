# ggadget tests are defined as one executable per test, so we need to
# define mutiple targets for them. To run all the tests together, use
#   xcodebuild -project ggadget_unittests.xcodeproj -target "Run All Tests"
# on Mac
{
  'target_defaults': {
    'type': 'executable',
    'dependencies': [
      'ggadget.gyp:extensions',
      'ggadget.gyp:ggadget',
      'unittest/unittest.gyp:ggadget_gtest',
    ],
    'defines': [
      'GGL_FOR_GOOPY',
    ],
    'include_dirs': [
      '.',
      '<(SHARED_INTERMEDIATE_DIR)/third_party/google_gadgets_for_linux',
    ],
  },
  'targets': [
    {
      'target_name': 'basic_element_unittests',
      'sources': [
        'ggadget/tests/basic_element_test.cc',
      ],
    }, # target: basic_element_test

    {
      'target_name': 'color_unittests',
      'sources': [
        'ggadget/tests/color_test.cc',
      ],
    }, # target: color_test

    {
      'target_name': 'common_unittests',
      'sources': [
        'ggadget/tests/common_test.cc',
      ],
    }, # target: common_test

    {
      'target_name': 'element_factory_unittests',
      'sources': [
        'ggadget/tests/element_factory_test.cc',
      ],
    }, # target: element_factory_test

    {
      'target_name': 'elements_unittests',
      'sources': [
        'ggadget/tests/elements_test.cc',
      ],
    }, # target: elements_test

    {
      'target_name': 'image_cache_unittests',
      'sources': [
        'ggadget/tests/image_cache_test.cc',
      ],
    }, # target: image_cache_test

    {
      'target_name': 'linear_element_unittests',
      'sources': [
        'ggadget/tests/linear_element_test.cc',
      ],
    }, # target: linear_test

    {
      'target_name': 'math_utils_unittests',
      'sources': [
        'ggadget/tests/math_utils_test.cc',
      ],
    }, # target: math_utils_test

    {
      'target_name': 'messages_unittests',
      'sources': [
        'ggadget/tests/messages_test.cc',
      ],
    }, # target: messages_test

    {
      'target_name': 'permissions_unittests',
      'sources': [
        'ggadget/tests/permissions_test.cc',
      ],
    }, # target: permissions_test

    {
      'target_name': 'scriptable_enumerator_unittests',
      'sources': [
        'ggadget/tests/scriptable_enumerator_test.cc',
        'ggadget/tests/scriptables.cc',
      ],
    }, # target: scriptable_enumerator_test

    {
      'target_name': 'scriptable_helper_unittests',
      'sources': [
        'ggadget/tests/scriptable_helper_test.cc',
        'ggadget/tests/scriptables.cc',
      ],
    }, # target: scriptable_helper_test

    {
      'target_name': 'signal_unittests',
      'sources': [
        'ggadget/tests/signal_test.cc',
        'ggadget/tests/slots.cc',
      ],
    }, # target: signal_test

    {
      'target_name': 'slot_unittests',
      'sources': [
        'ggadget/tests/slot_test.cc',
        'ggadget/tests/slots.cc',
      ],
    }, # target: slot_test

    {
      'target_name': 'string_utils_unittests',
      'sources': [
        'ggadget/tests/string_utils_test.cc',
      ],
    }, # target: string_utils_test

    {
      'target_name': 'system_utils_unittests',
      'sources': [
        'ggadget/tests/system_utils_test.cc',
      ],
    }, # target: system_utils_test

    {
      'target_name': 'text_formats_unittests',
      'sources': [
        'ggadget/tests/text_formats_test.cc',
      ],
    }, # target: text_formats_test

    {
      'target_name': 'unicode_utils_unittests',
      'sources': [
        'ggadget/tests/unicode_utils_test.cc',
      ],
    }, # target: unicode_utils_test

    {
      'target_name': 'variant_unittests',
      'sources': [
        'ggadget/tests/variant_test.cc',
      ],
    }, # target: variant_test

    {
      'target_name': 'view_unittests',
      'sources': [
        'ggadget/tests/view_test.cc',
      ],
    }, # target: view_test

    {
      'target_name': 'xml_dom_unittests',
      'sources': [
        'ggadget/tests/xml_dom_test.cc',
      ],
    }, # target: xml_dom_test
  ],
}

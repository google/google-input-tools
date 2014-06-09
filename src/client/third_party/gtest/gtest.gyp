# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    {
      'target_name': 'gtest',
      'type': 'static_library',
      'sources': [
        '<(GTEST)/include/gtest/gtest-death-test.h',
        '<(GTEST)/include/gtest/gtest-message.h',
        '<(GTEST)/include/gtest/gtest-param-test.h',
        '<(GTEST)/include/gtest/gtest-printers.h',
        '<(GTEST)/include/gtest/gtest-spi.h',
        '<(GTEST)/include/gtest/gtest-test-part.h',
        '<(GTEST)/include/gtest/gtest-typed-test.h',
        '<(GTEST)/include/gtest/gtest.h',
        '<(GTEST)/include/gtest/gtest_pred_impl.h',
        '<(GTEST)/include/gtest/internal/gtest-death-test-internal.h',
        '<(GTEST)/include/gtest/internal/gtest-filepath.h',
        '<(GTEST)/include/gtest/internal/gtest-internal.h',
        '<(GTEST)/include/gtest/internal/gtest-linked_ptr.h',
        '<(GTEST)/include/gtest/internal/gtest-param-util-generated.h',
        '<(GTEST)/include/gtest/internal/gtest-param-util.h',
        '<(GTEST)/include/gtest/internal/gtest-port.h',
        '<(GTEST)/include/gtest/internal/gtest-string.h',
        '<(GTEST)/include/gtest/internal/gtest-tuple.h',
        '<(GTEST)/include/gtest/internal/gtest-type-util.h',
        '<(GTEST)/src/gtest-death-test.cc',
        '<(GTEST)/src/gtest-filepath.cc',
        '<(GTEST)/src/gtest-internal-inl.h',
        '<(GTEST)/src/gtest-port.cc',
        '<(GTEST)/src/gtest-printers.cc',
        '<(GTEST)/src/gtest-test-part.cc',
        '<(GTEST)/src/gtest-typed-test.cc',
        '<(GTEST)/src/gtest.cc',
      ],
      'include_dirs': [
        '<(GTEST)',
        '<(GTEST)/include',
      ],
      'dependencies': [
        'gtest_prod',
      ],
      'conditions': [
        ['OS == "mac" or OS == "ios"', {
          'sources': [
            'gtest_mac.h',
            'gtest_mac.mm',
          ],
          'link_settings': {
            'libraries': [
              '$(SDKROOT)/System/Library/Frameworks/Foundation.framework',
            ],
          },
        }],
        ['OS=="linux" or OS=="mac"', {
          'defines': [
            # gtest isn't able to figure out when RTTI is disabled for gcc
            # versions older than 4.3.2, and assumes it's enabled.  Our Mac
            # and Linux builds disable RTTI, and cannot guarantee that the
            # compiler will be 4.3.2. or newer.  The Mac, for example, uses
            # 4.2.1 as that is the latest available on that platform.  gtest
            # must be instructed that RTTI is disabled here, and for any
            # direct dependents that might include gtest headers.
            'GTEST_HAS_RTTI=0',
          ],
          'direct_dependent_settings': {
            'defines': [
              'GTEST_HAS_RTTI=0',
            ],
          },
        }],
        ['OS=="android" and android_app_abi=="x86"', {
          'defines': [
            'GTEST_HAS_CLONE=0',
          ],
          'direct_dependent_settings': {
            'defines': [
              'GTEST_HAS_CLONE=0',
            ],
          },
        }],
        ['OS=="android"', {
          # We want gtest features that use tr1::tuple, but we currently
          # don't support the variadic templates used by libstdc++'s
          # implementation. gtest supports this scenario by providing its
          # own implementation but we must opt in to it.
          'defines': [
            'GTEST_USE_OWN_TR1_TUPLE=1',
            # GTEST_USE_OWN_TR1_TUPLE only works if GTEST_HAS_TR1_TUPLE is set.
            # gtest r625 made it so that GTEST_HAS_TR1_TUPLE is set to 0
            # automatically on android, so it has to be set explicitly here.
            'GTEST_HAS_TR1_TUPLE=1',
          ],
          'direct_dependent_settings': {
            'defines': [
              'GTEST_USE_OWN_TR1_TUPLE=1',
              'GTEST_HAS_TR1_TUPLE=1',
            ],
          },
        }],
        ['OS=="win" and (MSVS_VERSION=="2012" or MSVS_VERSION=="2012e")', {
          'defines': [
            '_VARIADIC_MAX=10',
          ],
          'direct_dependent_settings': {
            'defines': [
              '_VARIADIC_MAX=10',
            ],
          },
        }],
      ],
      'direct_dependent_settings': {
        'defines': [
          'UNIT_TEST',
        ],
        'include_dirs': [
          '<(GTEST)',
          '<(GTEST)/include',
        ],
        'target_conditions': [
          ['_type=="executable"', {
            'test': 1,
            'conditions': [
              ['OS=="mac"', {
                'run_as': {
                  'action????': ['${BUILT_PRODUCTS_DIR}/${PRODUCT_NAME}'],
                },
              }],
              ['OS=="win"', {
                'run_as': {
                  'action????': ['$(TargetPath)', '--gtest_print_time'],
                },
              }],
            ],
          }],
		],
        'msvs_disabled_warnings': [4800],
      },
    },
    {
      'target_name': 'gtest_main',
      'type': 'static_library',
      'dependencies': [
        'gtest',
      ],
      'include_dirs': [
        '<(GTEST)',
        '<(GTEST)/include',
      ],
      'sources': [
        '<(GTEST)/src/gtest_main.cc',
      ],
    },
    {
      'target_name': 'gtest_prod',
      'toolsets': ['host', 'target'],
      'type': 'none',
      'include_dirs': [
        '<(GTEST)',
        '<(GTEST)/include',
      ],
      'sources': [
        '<(GTEST)/include/gtest/gtest_prod.h',
      ],
    },
  ],
}

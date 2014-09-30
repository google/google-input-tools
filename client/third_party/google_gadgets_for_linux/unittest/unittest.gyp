# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    {
      'target_name': 'ggadget_gtest',
      'type': 'static_library',
      'sources': [
        'gtest.cc',
        'gtest.h',
        'gtest_internal.h',
        'gtest_internal_inl.h',
        'gtest_pred_impl.h',
        'gtest_prod.h',
        'gtest_spi.h',
      ],
      'include_dirs': [
        '.'
      ],
      'dependencies': [
        'gtest_prod',
      ],
      'conditions': [
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
          '.',  # So that gtest headers can find themselves.
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
      'target_name': 'gtest_prod',
      'toolsets': ['host', 'target'],
      'type': 'none',
      'include_dirs': [
        '.',
      ],
      'sources': [
        'gtest_prod.h',
      ],
    },
  ],
}

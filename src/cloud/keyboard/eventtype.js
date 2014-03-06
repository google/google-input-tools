// Copyright 2012 Google Inc. All Rights Reserved.

/**
 * @fileoverview The event types for keyboard.
 * @author shuchen@google.com (Shu Chen)
 */

goog.provide('i18n.input.keyboard.EventType');


/**
 * The event types to expose to external.
 *
 * @enum {string}
 */
i18n.input.keyboard.EventType = {
  KEYBOARD_CLOSED: 'kc',
  KEYBOARD_MINIMIZED: 'kmi',
  KEYBOARD_MAXIMIZED: 'kma',
  COMMIT_START: 'kcs',
  COMMIT_END: 'kce',
  LAYOUT_LOADED: 'lld',
  LAYOUT_ACTIVATED: 'lat'
};

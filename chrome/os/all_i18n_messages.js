// Copyright 2016 The ChromeOS IME Authors. All Rights Reserved.
// limitations under the License.
// See the License for the specific language governing permissions and
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// distributed under the License is distributed on an "AS-IS" BASIS,
// Unless required by applicable law or agreed to in writing, software
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// You may obtain a copy of the License at
// you may not use this file except in compliance with the License.
// Licensed under the Apache License, Version 2.0 (the "License");
//
goog.require('goog.chrome.extensions.i18n');


/**
 * Parse "MSG_D83D_DE02" to "\uD83D\uDE02"
 *
 * @param {string} The original key.
 * @return {string} The parsed key.
 */
var emojiData = emojiData || {};
var parseKey = function(k) {
  var values = k.split('_');
  var ints = [];
  for (var i = 1; i < values.length; i++) {
    ints[i - 1] = parseInt(values[i], 16);
  }
  return String.fromCharCode.apply(String, ints);
};

// Adds the emoji messags.
for (var key in emojiMsgs) {
  msgs[key] = emojiData[parseKey(key)] || emojiMsgs[key];
}

for (var k in jaMsgs) {
  msgs[k] = jaMsgs[k];
}

print(goog.chrome.extensions.i18n.messagesToJsonString(msgs));

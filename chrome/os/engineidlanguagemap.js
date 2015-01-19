// Copyright 2015 The ChromeOS IME Authors. All Rights Reserved.
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
goog.provide('i18n.input.chrome.EngineIdLanguageMap');


/**
 * The engine id and language map.
 *    key: engine id.
 *    value: array of languages.
 *
 * @type {!Object.<string, !Array.<string>>}
 */
i18n.input.chrome.EngineIdLanguageMap = {
  'nacl_mozc_jp': ['ja'],
  'nacl_mozc_us': ['ja'],
  'xkb:us::eng': ['en', 'en-US', 'en-AU', 'en-NZ'],
  'xkb:us::ind': ['id'],
  'xkb:us::fil': ['fil'],
  'xkb:us::msa': ['ms'],
  'xkb:us:intl:eng': ['en', 'en-US'],
  'xkb:us:intl:nld': ['nl'],
  'xkb:us:intl:por': ['pt-BR'],
  'xkb:us:altgr-intl:eng': ['en', 'en-US'],
  'xkb:us:dvorak:eng': ['en', 'en-US'],
  'xkb:us:colemak:eng': ['en', 'en-US'],
  'xkb:be::nld': ['nl'],
  'xkb:fr::fra': ['fr', 'fr-FR'],
  'xkb:be::fra': ['fr'],
  'xkb:ca::fra': ['fr', 'fr-CA'],
  'xkb:ch:fr:fra': ['fr', 'fr-CH'],
  'xkb:ca:multix:fra': ['fr', 'fr-CA'],
  'xkb:de::ger': ['de', 'de-DE'],
  'xkb:de:neo:ger': ['de', 'de-DE'],
  'xkb:be::ger': ['de'],
  'xkb:ch::ger': ['de', 'de-CH'],
  'xkb:jp::jpn': ['ja'],
  'xkb:ru::rus': ['ru'],
  'xkb:ru:phonetic:rus': ['ru'],
  'xkb:br::por': ['pt-BR', 'pt'],
  'xkb:bg::bul': ['bg'],
  'xkb:bg:phonetic:bul': ['bg'],
  'xkb:ca:eng:eng': ['en', 'en-CA'],
  'xkb:cz::cze': ['cs'],
  'xkb:cz:qwerty:cze': ['cs'],
  'xkb:ee::est': ['et'],
  'xkb:es::spa': ['es'],
  'xkb:es:cat:cat': ['ca'],
  'xkb:dk::dan': ['da'],
  'xkb:gr::gre': ['el'],
  'xkb:il::heb': ['he'],
  'xkb:latam::spa': ['es', 'es-419'],
  'xkb:lt::lit': ['lt'],
  'xkb:lv:apostrophe:lav': ['lv'],
  'xkb:hr::scr': ['hr'],
  'xkb:gb:extd:eng': ['en', 'en-GB'],
  'xkb:gb:dvorak:eng': ['en', 'en-GB'],
  'xkb:fi::fin': ['fi'],
  'xkb:hu::hun': ['hu'],
  'xkb:it::ita': ['it', 'it-IT'],
  'xkb:is::ice': ['is'],
  'xkb:no::nob': ['nb', 'nn', 'no'],
  'xkb:pl::pol': ['pl'],
  'xkb:pt::por': ['pt-PT', 'pt'],
  'xkb:ro::rum': ['ro'],
  'xkb:ro:std:rum': ['ro'],
  'xkb:se::swe': ['sv'],
  'xkb:sk::slo': ['sk'],
  'xkb:si::slv': ['sl'],
  'xkb:rs::srp': ['sr'],
  'xkb:tr::tur': ['tr'],
  'xkb:ua::ukr': ['uk'],
  'xkb:by::bel': ['be'],
  'xkb:am:phonetic:arm': ['hy'],
  'xkb:ge::geo': ['ka'],
  'xkb:mn::mon': ['mn'],
  'xkb:ie::ga': ['ga'],
  'xkb:mt::mlt': ['mt']
};

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



var jaMsgs = {};

// The following string is used by Japanese IME.
/**
 * @desc Original string in Japanese is "Google 日本語入力（日本語キーボード用）"
*/
jaMsgs.MSG_appNameJpKeyboard = goog.getMsg(
    'Google Japanese Input (for Japanese keyboard)');


/**
 * @desc Original string in Japanese is "Google 日本語入力（USキーボード用）"
*/
jaMsgs.MSG_appNameUsKeyboard = goog.getMsg(
    'Google Japanese Input (for US keyboard)');


/**
 * @desc Original string in Japanese is "ひらがな"
*/
jaMsgs.MSG_compositionModeHiragana = goog.getMsg('Hiragana');


/**
 * @desc Original string in Japanese is "全角カタカナ"
*/
jaMsgs.MSG_compositionModeFullKatakana = goog.getMsg('Katakana');


/**
 * @desc Original string in Japanese is "全角英数"
*/
jaMsgs.MSG_compositionModeFullAscii = goog.getMsg('Wide Latin');


/**
 * @desc Original string in Japanese is "半角カタカナ"
*/
jaMsgs.MSG_compositionModeHalfKatakana = goog.getMsg('Half width katakana');


/**
 * @desc Original string in Japanese is "半角英数"
*/
jaMsgs.MSG_compositionModeHalfAscii = goog.getMsg('Latin');


/**
 * @desc Original string in Japanese is "直接入力"
*/
jaMsgs.MSG_compositionModeDirect = goog.getMsg('Direct input');


/**
 * @desc Original string in Japanese is "句読点:"
*/
jaMsgs.MSG_configPunctuationMethod = goog.getMsg('Punctuation style:');


/**
 * @desc Original string in Japanese is "ローマ字入力・かな入力:"
*/
jaMsgs.MSG_configPreeditMethod = goog.getMsg('Input mode:');


/**
 * @desc Original string in Japanese is "ローマ字入力"
*/
jaMsgs.MSG_configPreeditMethodRomaji = goog.getMsg('Romaji');


/**
 * @desc Original string in Japanese is "かな入力"
*/
jaMsgs.MSG_configPreeditMethodKana = goog.getMsg('Kana');


/**
 * @desc Original string in Japanese is "記号:"
*/
jaMsgs.MSG_configSymbolMethod = goog.getMsg('Symbol style:');


/**
 * @desc Original string in Japanese is "スペースの入力:"
*/
jaMsgs.MSG_configSpaceCharacterForm = goog.getMsg('Space input style:');


/**
 * @desc Original string in Japanese is "入力モードに従う"
*/
jaMsgs.MSG_configSpaceCharacterFormFollow = goog.getMsg('Follow input mode');


/**
 * @desc Original string in Japanese is "全角"
*/
jaMsgs.MSG_configSpaceCharacterFormFull = goog.getMsg('Fullwidth');


/**
 * @desc Original string in Japanese is "半角"
*/
jaMsgs.MSG_configSpaceCharacterFormHalf = goog.getMsg('Halfwidth');


/**
 * @desc Original string in Japanese is "候補選択ショートカット:"
*/
jaMsgs.MSG_configSelectionShortcut = goog.getMsg('Selection shortcut:');


/**
 * @desc Original string in Japanese is "なし"
*/
jaMsgs.MSG_configSelectionShortcutNo = goog.getMsg('No shortcut');


/**
 * @desc Original string in Japanese is "シフトキーでの入力切替:"
*/
jaMsgs.MSG_configShiftKeyModeSwitch = goog.getMsg('Shift key mode switch:');


/**
 * @desc Original string in Japanese is "オフ"
*/
jaMsgs.MSG_configShiftKeyModeSwitchOff = goog.getMsg('Off');


/**
 * @desc Original string in Japanese is "英数字"
*/
jaMsgs.MSG_configShiftKeyModeSwitchAlphanumeric = goog.getMsg(
    'Alphanumeric');


/**
 * @desc Original string in Japanese is "カタカナ"
*/
jaMsgs.MSG_configShiftKeyModeSwitchKatakana = goog.getMsg('Katakana');


/**
 * @desc Original string in Japanese is "キー設定の選択:"
*/
jaMsgs.MSG_configSessionKeymap = goog.getMsg('Keymap style:');


/**
 * @desc Original string in Japanese is "ATOK"
*/
jaMsgs.MSG_configSessionKeymapAtok = goog.getMsg('ATOK');


/**
 * @desc Original string in Japanese is "Chrome OS"
*/
jaMsgs.MSG_configSessionKeymapChromeOs = goog.getMsg('Chrome OS');


/**
 * @desc Original string in Japanese is "MS-IME"
*/
jaMsgs.MSG_configSessionKeymapMsIme = goog.getMsg('MS-IME');


/**
 * @desc Original string in Japanese is "ことえり"
*/
jaMsgs.MSG_configSessionKeymapKotoeri = goog.getMsg('Kotoeri');


/**
 * @desc Original string in Japanese is "カスタム"
*/
jaMsgs.MSG_configSessionKeymapCustom = goog.getMsg('Custom keymap');


/**
 * @desc Original string in Japanese is
 * "学習機能、入力履歴からのサジェスト機能、ユーザ辞書機能を無効にする"
*/
jaMsgs.MSG_configIncognitoMode = goog.getMsg(
    'Disable personalized conversions and suggestions as well as user ' +
    'dictionary');


/**
 * @desc Original string in Japanese is "自動英数変換を有効にする"
*/
jaMsgs.MSG_configUseAutoImeTurnOff = goog.getMsg(
    'Automatically switch to halfwidth');


/**
 * @desc Original string in Japanese is "入力履歴からのサジェスト自動表示を有効にする"
*/
jaMsgs.MSG_configUseHistorySuggest = goog.getMsg('Use input history');


/**
 * @desc Original string in Japanese is
 * "システム辞書からのサジェスト自動表示を有効にする"
*/
jaMsgs.MSG_configUseDictionarySuggest = goog.getMsg('Use system dictionary');


/**
 * @desc Original string in Japanese is "サジェストの候補数:"
*/
jaMsgs.MSG_configSuggestionsSize = goog.getMsg('Number of suggestions:');


/**
 * @desc Original string in Japanese is "日本語入力の設定"
*/
jaMsgs.MSG_configSettingsTitle = goog.getMsg('Japanese input settings');


/**
 * @desc Original string in Japanese is "基本設定"
*/
jaMsgs.MSG_configBasicsTitle = goog.getMsg('Basics');


/**
 * @desc Original string in Japanese is "入力補助"
*/
jaMsgs.MSG_configInputAssistanceTitle = goog.getMsg('Input assistance');


/**
 * @desc Original string in Japanese is "サジェスト"
*/
jaMsgs.MSG_configSuggestTitle = goog.getMsg('Suggest');


/**
 * @desc Original string in Japanese is "プライバシー"
*/
jaMsgs.MSG_configPrivacyTitle = goog.getMsg('Privacy');


/**
 * @desc Original string in Japanese is "入力履歴の消去..."
*/
jaMsgs.MSG_configClearHistory = goog.getMsg('Clear personalization data...');


/**
 * @desc Original string in Japanese is "入力履歴を消去する"
*/
jaMsgs.MSG_configClearHistoryTitle = goog.getMsg('Clear personalization data');


/**
 * @desc Original string in Japanese is "次のアイテムを消去:"
*/
jaMsgs.MSG_configClearHistoryMessage = goog.getMsg(
    'Obliterate the following items:');


/**
 * @desc Original string in Japanese is "変換履歴"
*/
jaMsgs.MSG_configClearHistoryConversionHistory = goog.getMsg(
    'Conversion history');


/**
 * @desc Original string in Japanese is "サジェスト用履歴"
*/
jaMsgs.MSG_configClearHistorySuggestionHistory = goog.getMsg(
    'Suggestion history');


/**
 * @desc Original string in Japanese is "入力履歴を消去する"
*/
jaMsgs.MSG_configClearHistoryOkButton = goog.getMsg(
    'Clear personalization data');


/**
 * @desc Original string in Japanese is "キャンセル"
*/
jaMsgs.MSG_configClearHistoryCancelButton = goog.getMsg('Cancel');


/**
 * @desc Original string in Japanese is
 * "Copyright © 2014 Google Inc. All Rights Reserved."
*/
jaMsgs.MSG_configCreditsDescription = goog.getMsg(
    'Copyright © 2013 Google Inc. All Rights Reserved.');


/**
 * @desc Only translate it in Japanese. Don't translate in any other languages.
 * Original string in Japanese is "本ソフトウェアは<a href="./credits_ja.html">
 * オープンソースソフトウェア</a>を利用しています。"
*/
jaMsgs.MSG_configOssCreditsDescription = goog.getMsg(
    'This software is made possible by ' +
    '<a href="./credits_en.html">open source software</a>.');


/**
 * @desc Original string in Japanese is "キャンセル"
*/
jaMsgs.MSG_configDialogCancel = goog.getMsg('Cancel');


/**
 * @desc Original string in Japanese is "OK"
*/
jaMsgs.MSG_configDialogOk = goog.getMsg('OK');


/**
 * @desc Original string in Japanese is "ユーザー辞書"
*/
jaMsgs.MSG_configDictionaryToolTitle = goog.getMsg('User dictionaries');


/**
 * @desc Original string in Japanese is
 * "よく使う単語をユーザー辞書に登録することができます。"
*/
jaMsgs.MSG_configDictionaryToolDescription = goog.getMsg(
    'Add your own words to the user dictionary in order to customize the ' +
    'conversion candidates.');


/**
 * @desc Original string in Japanese is "ユーザー辞書の管理..."
*/
jaMsgs.MSG_configDictionaryToolButton = goog.getMsg(
    'Manage user dictionary...');


/**
 * @desc Original string in Japanese is "ユーザー辞書"
*/
jaMsgs.MSG_dictionaryToolPageTitle = goog.getMsg('User dictionaries');


/**
 * @desc Original string in Japanese is "辞書"
*/
jaMsgs.MSG_dictionaryToolDictionaryListTitle = goog.getMsg('Dictionaries');


/**
 * @desc Original string in Japanese is "よみ"
*/
jaMsgs.MSG_dictionaryToolReadingTitle = goog.getMsg('Reading');


/**
 * @desc Original string in Japanese is "単語"
*/
jaMsgs.MSG_dictionaryToolWordTitle = goog.getMsg('Word');


/**
 * @desc Original string in Japanese is "品詞"
*/
jaMsgs.MSG_dictionaryToolCategoryTitle = goog.getMsg('Category');


/**
 * @desc Original string in Japanese is "コメント"
*/
jaMsgs.MSG_dictionaryToolCommentTitle = goog.getMsg('Comment');


/**
 * @desc Original string in Japanese is "辞書管理"
*/
jaMsgs.MSG_dictionaryToolMenuTitle = goog.getMsg('Dictionary management');


/**
 * @desc Original string in Japanese is "エクスポート"
*/
jaMsgs.MSG_dictionaryToolExportButton = goog.getMsg('Export');


/**
 * @desc Original string in Japanese is "インポート..."
*/
jaMsgs.MSG_dictionaryToolImportButton = goog.getMsg('Import...');


/**
 * @desc Original string in Japanese is "完了"
*/
jaMsgs.MSG_dictionaryToolDoneButton = goog.getMsg('Done');


/**
 * @desc Original string in Japanese is "新しい単語のよみ"
*/
jaMsgs.MSG_dictionaryToolReadingNewInput = goog.getMsg('New reading');


/**
 * @desc Original string in Japanese is "新しい単語"
*/
jaMsgs.MSG_dictionaryToolWordNewInput = goog.getMsg('New word');


/**
 * @desc Original string in Japanese is "コメント"
*/
jaMsgs.MSG_dictionaryToolCommentNewInput = goog.getMsg('Comment');


/**
 * @desc Original string in Japanese is "辞書を削除"
*/
jaMsgs.MSG_dictionaryToolDeleteDictionaryTitle = goog.getMsg(
    'Delete dictionary');


/**
 * @desc Original string in Japanese is "辞書名"
*/
jaMsgs.MSG_dictionaryToolDictionaryName = goog.getMsg(
    'Dictionary Name');


/**
 * @desc Original string in Japanese is "新規辞書"
*/
jaMsgs.MSG_dictionaryToolNewDictionaryNamePlaceholder = goog.getMsg(
    'New dictionary');


/**
 * @desc Original string in Japanese is "{$dictName}を削除しますか?"
*/
jaMsgs.MSG_dictionaryToolDeleteDictionaryConfirm = goog.getMsg(
    'Do you want to delete {$dictName}?', {
      'dictName': {
        'content': '$1',
        'example': 'Dictionary Name'
      }
    });


/**
 * @desc Original string in Japanese is "操作を実行できません。"
*/
jaMsgs.MSG_dictionaryToolStatusErrorGeneral = goog.getMsg(
    'The operation has failed.');


/**
 * @desc Original string in Japanese is "ファイルを開けません。"
*/
jaMsgs.MSG_dictionaryToolStatusErrorFileNotFound = goog.getMsg(
    'Could not open the file.');


/**
 * @desc Original string in Japanese is "ファイルを読み込めません。"
*/
jaMsgs.MSG_dictionaryToolStatusErrorInvalidFileFormat = goog.getMsg(
    'Could not read the file.');


/**
 * @desc Original string in Japanese is "空のファイルです。"
*/
jaMsgs.MSG_dictionaryToolStatusErrorEmptyFile = goog.getMsg(
    'The file is empty.');


/**
 * @desc Original string in Japanese is "ファイルが大きすぎます。"
*/
jaMsgs.MSG_dictionaryToolStatusErrorFileSizeLimitExceeded = goog.getMsg(
    'The data size exceeds the file size limit.');


/**
 * @desc Original string in Japanese is "これ以上辞書を作成できません。"
*/
jaMsgs.MSG_dictionaryToolStatusErrorDictionarySizeLimitExceeded =
    goog.getMsg('Can\'t create any more dictionaries.');


/**
 * @desc Original string in Japanese is "一つの辞書に含む単語が多すぎます。"
*/
jaMsgs.MSG_dictionaryToolStatusErrorEntrySizeLimitExceeded =
    goog.getMsg('Can\'t create any more entries in this dictionary.');


/**
 * @desc Original string in Japanese is "辞書名を入力してください。"
*/
jaMsgs.MSG_dictionaryToolStatusErrorDictionaryNameEmpty = goog.getMsg(
    'Please type a dictionary name.');


/**
 * @desc Original string in Japanese is "辞書名が長すぎます。"
*/
jaMsgs.MSG_dictionaryToolStatusErrorDictionaryNameTooLong = goog.getMsg(
    'The name is too long.');


/**
 * @desc Original string in Japanese is "辞書名に使用できない文字が含まれています。"
*/
jaMsgs.MSG_dictionaryToolStatusErrorDictionaryNameContainsInvalidCharacter =
    goog.getMsg('The name contains invalid character(s).');


/**
 * @desc Original string in Japanese is "その辞書名はすでに使われています。"
*/
jaMsgs.MSG_dictionaryToolStatusErrorDictionaryNameDuplicated = goog.getMsg(
    'The name already exists.');


/**
 * @desc Original string in Japanese is "よみを入力してください。"
*/
jaMsgs.MSG_dictionaryToolStatusErrorReadingEmpty = goog.getMsg(
    'Please type the reading.');


/**
 * @desc Original string in Japanese is "よみが長すぎます。"
*/
jaMsgs.MSG_dictionaryToolStatusErrorReadingTooLong = goog.getMsg(
    'The reading is too long.');


/**
 * @desc Original string in Japanese is "よみに使用できない文字が含まれています。
 * Ascii文字、ひらがな、濁点(゛)、半濁点(゜)、中点(・)、長音符(ー)、読点(、)、句点(。)、
 * 括弧(「」『』)、波ダッシュ(〜)をよみに使用できます。"
*/
jaMsgs.MSG_dictionaryToolStatusErrorReadingContainsInvalidCharacter =
    goog.getMsg('The reading contains invalid character(s). You can use use ' +
        'ASCII characters, Hiragana, voiced sound mark(゛), semi-voiced ' +
        'sound mark(゜), middle dot(・), prolonged sound mark(ー), Japanese ' +
        'comma(、), Japanese period(。), Japanese brackets(「」『』) and ' +
        'Japanese wavedash(〜) in the reading.');


/**
 * @desc Original string in Japanese is "単語を入力してください。"
*/
jaMsgs.MSG_dictionaryToolStatusErrorWordEmpty = goog.getMsg(
    'Please type the word.');


/**
 * @desc Original string in Japanese is "単語が長すぎます。"
*/
jaMsgs.MSG_dictionaryToolStatusErrorWordTooLong = goog.getMsg(
    'The word is too long.');


/**
 * @desc Original string in Japanese is "単語に使用できない文字が含まれています。"
*/
jaMsgs.MSG_dictionaryToolStatusErrorWordContainsInvalidCharacter =
    goog.getMsg('The word contains invalid character(s).');


/**
 * @desc Original string in Japanese is "インポートする単語が多すぎます。"
*/
jaMsgs.MSG_dictionaryToolStatusErrorImportTooManyWords = goog.getMsg(
    'The import source contains too many words.');


/**
 * @desc Original string in Japanese is "インポートできない単語がありました。"
*/
jaMsgs.MSG_dictionaryToolStatusErrorImportInvalidEntries = goog.getMsg(
    'Some words could not be imported.');


/**
 * @desc Original string in Japanese is "これ以上操作を取り消せません。"
*/
jaMsgs.MSG_dictionaryToolStatusErrorNoUndoHistory = goog.getMsg(
    'No more undo-able operation.');


/**
 * @desc Original string in Japanese is "新規辞書エラー"
*/
jaMsgs.MSG_dictionaryToolStatusErrorTitleCreateDictionary = goog.getMsg(
    'Create dictionary error');


/**
 * @desc Original string in Japanese is "辞書削除エラー"
*/
jaMsgs.MSG_dictionaryToolStatusErrorTitleDeleteDictionary = goog.getMsg(
    'Delete dictionary error');


/**
 * @desc Original string in Japanese is "辞書名変更エラー"
*/
jaMsgs.MSG_dictionaryToolStatusErrorTitleRenameDictionary = goog.getMsg(
    'Rename dictionary error');


/**
 * @desc Original string in Japanese is "単語編集エラー"
*/
jaMsgs.MSG_dictionaryToolStatusErrorTitleEditEntry = goog.getMsg(
    'Edit entry error');


/**
 * @desc Original string in Japanese is "単語削除エラー"
*/
jaMsgs.MSG_dictionaryToolStatusErrorTitleDeleteEntry = goog.getMsg(
    'Delete entry error');


/**
 * @desc Original string in Japanese is "単語追加エラー"
*/
jaMsgs.MSG_dictionaryToolStatusErrorTitleAddEntry = goog.getMsg(
    'Add entry error');


/**
 * @desc Original string in Japanese is "インポートエラー"
*/
jaMsgs.MSG_dictionaryToolStatusErrorTitleImportData = goog.getMsg(
    'Import error');


/**
 * @desc Original string in Japanese is
 * "使用統計データや障害レポートを Google に自動送信する"
*/
jaMsgs.MSG_configUploadUsageStats = goog.getMsg(
    'Automatically send usage statistics and crash reports to Google');

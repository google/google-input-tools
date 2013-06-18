/*
 * Copyright 2013 Google Inc. All Rights Reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <iostream>
#include <string>

#include <ppapi/cpp/instance.h>
#include <ppapi/cpp/module.h>
#include <ppapi/cpp/var.h>
#include <hangul-1.0/hangul.h>
#include <json/json.h>

#include "hanja.h"
#include "unicode_util.h"

using std::string;

namespace {

const char kResponseSuccess[] = "SUCCESS";
const char kResponseError[] = "ERROR";
const char kHanjaTableURL[] = "hanja.txt";
const char kSymbolTableURL[] = "symbol.txt";

class HangulInstance : public pp::Instance {
 public:
  explicit HangulInstance(PP_Instance instance) : pp::Instance(instance) {
    // Initialize Hangul. By default keyboard is 2-set
    static const char *keyboard = "2";
    hangul_input_ = hangul_ic_new(keyboard);
    // Initialize Hanja
    hanja_lookup_ = new HanjaLookup(this);
    hanja_lookup_->LoadFromURL(kHanjaTableURL);
    hanja_lookup_->LoadFromURL(kSymbolTableURL);
  }

  virtual ~HangulInstance() {
    hangul_ic_delete(hangul_input_);
  }

  // There are two kinds of messages. One is to set keyboard layout, where the
  // format is {"keyboard": layout}.
  // The other is to convert raw input to hangul or hangul to hanja,
  // where the format is like:
  // {"text":"ganji", "num":10}
  virtual void HandleMessage(const pp::Var &message) {
    if (!message.is_string()) {
      ReportError("Request is not a string");
      return;
    }
    string json_string = message.AsString();

    // Parse JSON format request
    Json::Value request;
    Json::Reader reader;
    if (!reader.parse(json_string, request)) {
      ReportError("Failed to parse request json");
      return;
    }

    const Json::Value text_field = request["text"];
    if (text_field.isString()) {
      string text = text_field.asString();
      HandleConversionRequest(text, request);
      return;
    }

    const Json::Value keyboard_field = request["keyboard"];
    if (keyboard_field.isString()) {
      // Set the keyboard layout
      string keyboard = keyboard_field.asString();
      hangul_ic_select_keyboard(hangul_input_, keyboard.c_str());
      return;
    }

    ReportError("Invalid request: " + json_string);
  }

 private:
  void HandleConversionRequest(const string& text, const Json::Value& request) {
    const Json::Value num_field = request["num"];
    if (!num_field.isInt()) {
      ReportError("Invalid format: property 'num' isn't integer");
      return;
    }
    size_t num_candidates = num_field.asInt();
    // If raw input characters are all ASCII, they would be transliterated into
    // hangul characters. Otherwise text must be valid hangul characters and
    // hanja candidates will be returned.
    bool is_all_ascii = IsAllAscii(text);
    if (is_all_ascii) {
      MatchHangul(text, num_candidates);
    } else {
      MatchHanja(text, num_candidates);
    }
  }

  // Determines if all characters in text are ACSII characters
  bool IsAllAscii(const string& text) {
    size_t text_len = text.length();
    for (size_t i = 0; i < text_len; i++) {
      unsigned char ch = text[i];
      if (ch > 0x7F) {
        return false;
      }
    }
    return true;
  }

  void MatchHangul(const string& text, const size_t num_candidates) {
    string input = text;
    size_t input_len = input.length();
    Json::Value hangul_candidates;
    Json::Value matched_length;

    UCSString hangul = Transliterate(input);
    size_t hangul_len = hangul.length();
    size_t offset = 0;
    for (size_t i = 0; i < hangul_len; i++) {
      string character = unicode_util::Ucs4ToUtf8(hangul.c_str() + i, 1);
      hangul_candidates.append(character);
      // Try to find out matched length of each character
      size_t len;
      for (len = 1; offset + len < input_len; len++) {
        string input_sec = input.substr(offset, len);
        UCSString hangul_sec = Transliterate(input_sec);
        // If the segment of input text can be converted to the
        if (hangul_sec[0] == hangul[i]) {
          break;
        }
      }
      matched_length.append(len);
      offset += len;
    }
    Json::Value additional_fields;
    additional_fields["matched_length"] = matched_length;
    GenerateResponse(text,
                     hangul_candidates,
                     matched_length,
                     additional_fields);
  }

  // Converts raw input into hangul characters
  UCSString Transliterate(const string& text) {
    // Clear inner state of libhangul and process every character
    hangul_ic_reset(hangul_input_);
    size_t text_len = text.length();
    UCSString hangul_text;
    for (size_t i = 0; i < text_len; i++) {
      hangul_ic_process(hangul_input_, text[i]);
      hangul_text += hangul_ic_get_commit_string(hangul_input_);
    }
    // Get committed hangul characters and hanja candidates
    hangul_text += hangul_ic_flush(hangul_input_);
    return hangul_text;
  }

  void MatchHanja(const string& text, const size_t num_candidates) {
    UCSString hangul_text = unicode_util::Utf8ToUcs4(text.c_str(), 0);
    size_t hangul_len = hangul_text.length();
    size_t num_matched_candidates = 0;
    Json::Value hanja_candidates;
    Json::Value matched_length;
    Json::Value annotation;
    // Match every prefix of hangul_text
    for (size_t len = hangul_len; len >= 1; len--) {
      hangul_text[len] = 0;
      string hangul_utf8 = unicode_util::Ucs4ToUtf8(hangul_text.c_str(), 0);
      std::vector<HanjaLookup::Item>::const_iterator iter, iter_end;
      hanja_lookup_->Match(hangul_utf8, &iter, &iter_end);
      for (; iter != iter_end; ++iter) {
        hanja_candidates.append(iter->hanja);
        matched_length.append(len);
        annotation.append(iter->comment);
        num_matched_candidates++;
        if (num_candidates > 0 && num_matched_candidates == num_candidates) {
          break;
        }
      }
      if (num_candidates > 0 && num_matched_candidates == num_candidates) {
        break;
      }
    }
    Json::Value additional_fields;
    additional_fields["matched_length"] = matched_length;
    additional_fields["annotation"] = annotation;
    GenerateResponse(text,
                     hanja_candidates,
                     matched_length,
                     additional_fields);
  }

  void GenerateResponse(const string& original,
                        const Json::Value& candidates,
                        const Json::Value& matched_length,
                        const Json::Value& additional_fields) {
    // Generate result
    Json::Value result;
    result.append(original);
    result.append(candidates);
    result.append(matched_length);
    result.append(additional_fields);
    Json::Value payload;
    payload.append(result);
    Json::Value response;
    response.append(kResponseSuccess);
    response.append(payload);
    PostMessage(response);
  }

  void PostMessage(const Json::Value &res) {
    Json::FastWriter writer;
    string json_string = writer.write(res);
    pp::Instance::PostMessage(pp::Var(json_string));
  }

  void ReportError(const string error_message) {
    Json::Value response;
    response.append(kResponseError);
    response.append(error_message);
    PostMessage(response);
  }

  HangulInputContext *hangul_input_;
  HanjaLookup *hanja_lookup_;
};

class HangulModule : public pp::Module {
 public:
  HangulModule() : pp::Module() {}
  virtual ~HangulModule() {}

  virtual pp::Instance *CreateInstance(PP_Instance instance) {
    return new HangulInstance(instance);
  }
};

}  // namespace

namespace pp {
  Module *CreateModule() {
    return new HangulModule();
  }
}

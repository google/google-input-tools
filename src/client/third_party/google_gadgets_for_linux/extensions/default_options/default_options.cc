/*
  Copyright 2008 Google Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include <cstdlib>
#include <ggadget/encryptor_interface.h>
#include <ggadget/file_manager_factory.h>
#include <ggadget/main_loop_interface.h>
#include <ggadget/memory_options.h>
#include <ggadget/string_utils.h>
#include <ggadget/system_utils.h>
#include <ggadget/xml_parser_interface.h>

namespace ggadget {
namespace {

static const char kOptionsFilePrefix[] = "profile://options/";
// Options will be automatically flushed to disk every 2 ~ 3 minutes.
static const int kAutoFlushInterval = 120000;
// This variant is to prevent multiple options flush in the same step.
static const int kAutoFlushIntervalVariant = 60000;

static const size_t kDefaultOptionsSizeLimit = 0x100000; // 1MB.
static const size_t kGlobalOptionsSizeLimit = 0x1000000; // 16MB.

// An options file is an XML file in the following format:
// <code>
// <options>
//  <item name="item name" type="item type" [encrypted="0|1"] [internal="0|1"]>
//    item value</item>
//   ...
// </options>
// </code>
//
// External option items are visible to gadget scripts, while internal items
// are not.
// Values are encoded in a format like quoted-printable.
//
// There are following types of items:
//   - b: boolean
//   - i: integer
//   - d: double
//   - s: string
//   - j: JSONString
//   - D: Date, stores the milliseconds since EPOCH.
//
// Except for type="D", the convertion rule between typed value and string
// is the same as Variant::ConvertTo...() and Variant::ConvertToString().

class DefaultOptions : public MemoryOptions {
 public:
  DefaultOptions(const char *name, size_t size_limit)
      : MemoryOptions(size_limit),
        main_loop_(GetGlobalMainLoop()),
        file_manager_(GetGlobalFileManager()),
        parser_(GetXMLParser()),
        encryptor_(GetEncryptor()),
        name_(name),
        location_(std::string(kOptionsFilePrefix) + name + ".xml"),
        changed_(false),
        ref_count_(0),
        timer_(0) {
    ASSERT(name && *name);
    ASSERT(main_loop_);
    ASSERT(file_manager_);
    ASSERT(parser_);
    ASSERT(encryptor_);

    if (!name || !*name)
      return;

    // Schedule the auto flush timer.
    timer_ = main_loop_->AddTimeoutWatch(
        kAutoFlushInterval + rand() % kAutoFlushIntervalVariant,
        new WatchCallbackSlot(NewSlot(this, &DefaultOptions::OnFlushTimer)));

    // Monitor options change.
    ConnectOnOptionChanged(NewSlot(this, &DefaultOptions::OnOptionChange));

    std::string data;
    if (!file_manager_->ReadFile(location_.c_str(), &data)) {
      // Not a fatal error, just leave this Options empty.
      return;
    }

    StringMap table;
    if (parser_->ParseXMLIntoXPathMap(data, NULL, location_.c_str(),
                                      "options", NULL, NULL, &table)) {
      for (StringMap::const_iterator it = table.begin();
           it != table.end(); ++it) {
        const std::string &key = it->first;
        if (key.find('@') != key.npos)
          continue;

        const char *name = GetXPathValue(table, key + "@name");
        const char *type = GetXPathValue(table, key + "@type");
        if (!name || !type) {
          LOG("Missing required name and/or type attribute in config file '%s'",
              location_.c_str());
          continue;
        }

        const char *encrypted_attr = GetXPathValue(table, key + "@encrypted");
        bool encrypted = encrypted_attr && encrypted_attr[0] == '1';
        std::string value_str = UnescapeValue(it->second);
        if (encrypted) {
          std::string temp(value_str);
          if (!encryptor_->Decrypt(temp, &value_str)) {
            LOG("Failed to decript value for item '%s' in config file '%s'",
                name, location_.c_str());
            continue;
          }
        }

        Variant value = ParseValueStr(type, value_str);
        if (value.type() != Variant::TYPE_VOID) {
          const char *internal_attr = GetXPathValue(table, key + "@internal");
          std::string unescaped_name = UnescapeValue(name);
          bool internal = internal_attr && internal_attr[0] == '1';
          if (internal) {
            PutInternalValue(unescaped_name.c_str(), value);
          } else {
            PutValue(unescaped_name.c_str(), value);
            // Still preserve the encrypted state.
            if (encrypted)
              EncryptValue(unescaped_name.c_str());
          }
        } else {
          LOG("Failed to decode value for item '%s' in config file '%s'",
              name, location_.c_str());
        }
      }
    }
  }

  virtual ~DefaultOptions() {
    Flush();
    main_loop_->RemoveWatch(timer_);
  }

  bool OnFlushTimer(int timer) {
    GGL_UNUSED(timer);
    // DeleteStorage() sets file_manager_ to NULL.
    if (!file_manager_)
      return false;

    Flush();
    return true;
  }

  void OnOptionChange(const char *option) {
    GGL_UNUSED(option);
    changed_ = true;
  }

  static const char *GetXPathValue(const StringMap &table,
                                   const std::string &key) {
    StringMap::const_iterator it = table.find(key);
    return it == table.end() ? NULL : it->second.c_str();
  }

  Variant ParseValueStr(const char *type, const std::string &value_str) {
    switch (type[0]) {
      case 'b': {
        bool value = false;
        return Variant(value_str).ConvertToBool(&value) ?
               Variant(value) : Variant();
      }
      case 'i': {
        int64_t value = 0;
        return Variant(value_str).ConvertToInt64(&value) ?
               Variant(value) : Variant();
      }
      case 'd': {
        double value = 0;
        return Variant(value_str).ConvertToDouble(&value) ?
               Variant(value) : Variant();
      }
      case 's':
        return Variant(value_str);
      case 'j':
        return Variant(JSONString(value_str));
      case 'D': {
        int64_t value = 0;
        return Variant(value_str).ConvertToInt64(&value) ?
               Variant(Date(value)) : Variant();
      }
      default:
        LOG("Unknown option item type: '%s'", type);
        return Variant();
    }
  }

  char GetValueType(const Variant &value) {
    switch (value.type()) {
      case Variant::TYPE_BOOL:
        return 'b';
      case Variant::TYPE_INT64:
        return 'i';
      case Variant::TYPE_DOUBLE:
        return 'd';
      case Variant::TYPE_JSON:
        return 'j';
      case Variant::TYPE_DATE:
        return 'D';
      default: // All other types are stored as string type.
        return 's';
    }
  }

  // Because XML has some restrictions on set of characters, we must escape
  // the out of range data into proper format.
  static std::string EscapeValue(const std::string &input) {
    std::string result;
    result.reserve(input.size());
    for (size_t i = 0; i < input.size(); i++) {
      char c = input[i];
      // This range is very conservative, but harmless, because only this
      // program will read the data.
      if (c < 0x20 || c >= 0x7f || c == '=') {
        char buf[4];
        snprintf(buf, sizeof(buf), "=%02X", static_cast<unsigned char>(c));
        result.append(buf);
      } else {
        result.append(1, c);
      }
    }
    return result;
  }

  static std::string UnescapeValue(const std::string &input) {
    std::string result;
    result.reserve(input.size());
    for (size_t i = 0; i < input.size(); i++) {
      char c = input[i];
      if (c == '=' && i < input.size() - 2) {
        unsigned int t;
        sscanf(input.c_str() + i + 1, "%02X", &t);
        c = static_cast<char>(t);
        i += 2;
      }
      result.append(1, c);
    }
    return result;
  }

  void WriteItemCommon(const char *name, const Variant &value,
                       bool internal, bool encrypted) {
    out_data_ += " <item name=\"";
    out_data_ += parser_->EncodeXMLString(EscapeValue(name).c_str());
    out_data_ += "\" type=\"";
    out_data_ += GetValueType(value);
    out_data_ += "\"";
    if (internal)
      out_data_ += " internal=\"1\"";

    std::string str_value;
    // JSON and DATE types can't be converted to string by default logic.
    if (value.type() == Variant::TYPE_JSON)
      str_value = VariantValue<JSONString>()(value).value;
    else if (value.type() == Variant::TYPE_DATE)
      str_value = StringPrintf("%jd", VariantValue<Date>()(value).value);
    else
      value.ConvertToString(&str_value); // Errors are ignored.

    if (encrypted) {
      out_data_ += " encrypted=\"1\"";
      std::string temp(str_value);
      encryptor_->Encrypt(temp, &str_value);
    }
    out_data_ += ">";
    out_data_ += parser_->EncodeXMLString(EscapeValue(str_value).c_str());
    out_data_ += "</item>\n";
  }

  bool WriteItem(const char *name, const Variant &value, bool encrypted) {
    WriteItemCommon(name, value, false, encrypted);
    return true;
  }

  bool WriteInternalItem(const char *name, const Variant &value) {
    WriteItemCommon(name, value, true, false);
    return true;
  }

  virtual void PutInternalValue(const char *name, const Variant &value) {
    MemoryOptions::PutInternalValue(name, value);
    changed_ = true;
  }

  virtual bool Flush() {
    if (!file_manager_)
      return false;
    if (!changed_)
      return true;

    DLOG("Flush options file: %s", location_.c_str());
    out_data_.clear();
    out_data_ = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<options>\n";
    size_t out_data_header_size = out_data_.size();
    EnumerateItems(NewSlot(this, &DefaultOptions::WriteItem));
    EnumerateInternalItems(NewSlot(this, &DefaultOptions::WriteInternalItem));

    if (out_data_.size() == out_data_header_size) {
      // There is no item, remove the options file.
      file_manager_->RemoveFile(location_.c_str());
      return true;
    } else {
      out_data_ += "</options>\n";
      bool result = file_manager_->WriteFile(location_.c_str(), out_data_,
                                             true);
      out_data_.clear();
      if (result)
        changed_ = false;
      return result;
    }
  }

  virtual void DeleteStorage() {
    MemoryOptions::DeleteStorage();
    file_manager_->RemoveFile(location_.c_str());
    file_manager_ = NULL;
    // Delete it from the map to prevent it from being further used.
    options_map_->erase(name_);
  }

  // Singleton management.
  typedef LightMap<std::string, DefaultOptions *> OptionsMap;
  static DefaultOptions *GetOptions(const char *name, size_t size_limit) {
    ASSERT(name && *name);
    DefaultOptions *options;
    OptionsMap::const_iterator it = options_map_->find(name);
    if (it == options_map_->end()) {
      options = new DefaultOptions(name, size_limit);
      (*options_map_)[name] = options;
    } else {
      options = it->second;
    }
    return options;
  }

  static void FinalizeAllOptions() {
    for (OptionsMap::iterator it = options_map_->begin();
         it != options_map_->end(); ++it) {
      DLOG("Finalize option: %s", it->first.c_str());
      delete it->second;
    }
    options_map_->clear();
  }

  void Ref() {
    ref_count_++;
  }

  void Unref() {
    ASSERT(ref_count_ > 0);
    ref_count_--;
    if (ref_count_ == 0) {
      options_map_->erase(name_);
      delete this;
    }
  }

  MainLoopInterface *main_loop_;
  FileManagerInterface *file_manager_;
  XMLParserInterface *parser_;
  EncryptorInterface *encryptor_;
  std::string name_;
  std::string location_;
  std::string out_data_;  // Only available during Flush().
  bool changed_; // Whether changed since last Flush().
  int ref_count_;
  int timer_;

  static OptionsMap *options_map_;
};

DefaultOptions::OptionsMap *DefaultOptions::options_map_ = NULL;

class OptionsDelegator : public OptionsInterface {
 public:
  OptionsDelegator(DefaultOptions *back_options)
      : back_options_(back_options) {
    back_options_->Ref();
  }
  ~OptionsDelegator() {
    back_options_->Unref();
  }

  virtual Connection *ConnectOnOptionChanged(
      Slot1<void, const char *> *handler) {
    return back_options_->ConnectOnOptionChanged(handler);
  }
  virtual size_t GetCount() { return back_options_->GetCount(); }
  virtual void Add(const char *name, const Variant &value) {
    return back_options_->Add(name, value);
  }
  virtual bool Exists(const char *name) { return back_options_->Exists(name); }
  virtual Variant GetDefaultValue(const char *name) {
    return back_options_->GetDefaultValue(name);
  }
  virtual void PutDefaultValue(const char *name, const Variant &value) {
    back_options_->PutDefaultValue(name, value);
  }
  virtual Variant GetValue(const char *name) {
    return back_options_->GetValue(name);
  }
  virtual void PutValue(const char *name, const Variant &value) {
    back_options_->PutValue(name, value);
  }
  virtual void Remove(const char *name) { back_options_->Remove(name); }
  virtual void RemoveAll() { back_options_->RemoveAll(); }
  virtual void EncryptValue(const char *name) {
    back_options_->EncryptValue(name);
  }
  virtual bool IsEncrypted(const char *name) {
    return back_options_->IsEncrypted(name);
  }
  virtual Variant GetInternalValue(const char *name) {
    return back_options_->GetInternalValue(name);
  }
  virtual void PutInternalValue(const char *name, const Variant &value) {
    back_options_->PutInternalValue(name, value);
  }
  virtual bool Flush() { return back_options_->Flush(); }
  virtual void DeleteStorage() { back_options_->DeleteStorage(); }
  virtual bool EnumerateItems(
      Slot3<bool, const char *, const Variant &, bool> *callback) {
    return back_options_->EnumerateItems(callback);
  }
  virtual bool EnumerateInternalItems(
      Slot2<bool, const char *, const Variant &> *callback) {
    return back_options_->EnumerateInternalItems(callback);
  }

  DefaultOptions *back_options_;
};

OptionsInterface *DefaultOptionsFactory(const char *name) {
  return new OptionsDelegator(
      DefaultOptions::GetOptions(name, kDefaultOptionsSizeLimit));
}

static OptionsDelegator *g_global_options = NULL;

} // anonymous namespace
} // namespace ggadget

#define Initialize default_options_LTX_Initialize
#define Finalize default_options_LTX_Finalize

extern "C" {
  bool Initialize() {
    LOGI("Initialize default_options extension.");

    // options_map_ must be created by new, otherwise it might be destroyed
    // before calling module's Finalize().
    if (!ggadget::DefaultOptions::options_map_) {
      ggadget::DefaultOptions::options_map_ =
          new ggadget::DefaultOptions::OptionsMap;
    }

    // The default options file has much bigger size limit than normal options.
    if (!ggadget::g_global_options) {
      ggadget::g_global_options = new ggadget::OptionsDelegator(
          ggadget::DefaultOptions::GetOptions(
              "global-options", ggadget::kGlobalOptionsSizeLimit));
    }

    return ggadget::SetOptionsFactory(&ggadget::DefaultOptionsFactory) &&
           ggadget::SetGlobalOptions(ggadget::g_global_options);
  }

  void Finalize() {
    LOGI("Finalize default_options extension.");
    delete ggadget::g_global_options;
    ggadget::DefaultOptions::FinalizeAllOptions();
    delete ggadget::DefaultOptions::options_map_;
  }
}

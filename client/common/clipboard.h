/*
  Copyright 2014 Google Inc.

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
//

#ifndef GOOPY_COMMON_CLIPBOARD_H_
#define GOOPY_COMMON_CLIPBOARD_H_

#include <map>
#include <string>
#include <vector>

#include "base/basictypes.h"

class Clipboard {
 public:
  typedef unsigned int FormatType;
  typedef SIZE BitmapSize;

  // ObjectType designates the type of data to be stored in the clipboard. This
  // designation is shared across all OSes. The system-specific designation
  // is defined by FormatType. A single ObjectType might be represented by
  // several system-specific FormatTypes. For example, on Linux the CBF_TEXT
  // ObjectType maps to "text/plain", "STRING", and several other formats. On
  // windows it maps to CF_UNICODETEXT.
  enum ObjectType {
    CBF_TEXT,
    CBF_HTML,
    CBF_BOOKMARK,
    CBF_LINK,
    CBF_FILES,
    CBF_WEBKIT,
    CBF_BITMAP,
    CBF_SMBITMAP  // bitmap from shared memory
  };

  // ObjectMap is a map from ObjectType to associated data.
  // The data is organized differently for each ObjectType. The following
  // table summarizes what kind of data is stored for each key.
  // * indicates an optional argument.
  //
  // Key           Arguments    Type
  // -------------------------------------
  // CBF_TEXT      text         char array
  // CBF_HTML      html         char array
  //               url*         char array
  // CBF_BOOKMARK  html         char array
  //               url          char array
  // CBF_LINK      html         char array
  //               url          char array
  // CBF_FILES     files        char array representing multiple files.
  //                            Filenames are separated by null characters and
  //                            the final filename is double null terminated.
  //                            The filenames are encoded in platform-specific
  //                            encoding.
  // CBF_WEBKIT    none         empty vector
  // CBF_BITMAP    pixels       byte array
  //               size         gfx::Size struct
  // CBF_SMBITMAP  shared_mem   shared memory handle
  //               size         gfx::Size struct
  typedef std::vector<char> ObjectMapParam;
  typedef std::vector<ObjectMapParam> ObjectMapParams;
  typedef std::map<int /* ObjectType */, ObjectMapParams> ObjectMap;

  Clipboard();
  ~Clipboard();

  // Convienent method to write an utf16 string to clipboard. The method is
  // provided to optimize performance and reduce unnecessary conversion between
  // strings.
  void WriteText(const wstring& text);

  // Write a bunch of objects to the system clipboard. Copies are made of the
  // contents of |objects|. On Windows they are copied to the system clipboard.
  // On linux they are copied into a structure owned by the Clipboard object and
  // kept until the system clipboard is set again.
  void WriteObjects(const ObjectMap& objects);

  // Tests whether the clipboard contains a certain format
  bool IsFormatAvailable(const FormatType& format) const;

  // Reads UNICODE text from the clipboard, if available.
  void ReadText(wstring* result) const;

  // Reads ASCII text from the clipboard, if available.
  void ReadAsciiText(std::string* result) const;

  // Destroy the internal objects.
  void Destroy();

 private:
  void WriteText(const char* text_data, size_t text_len);

  void DispatchObject(ObjectType type, const ObjectMapParams& params);

  void WriteBitmap(const char* pixel_data, const char* size_data);

  void WriteBitmapFromHandle(HBITMAP source_hbitmap,
                             const BitmapSize& size);

  // Safely write to system clipboard. Free |handle| on failure.
  void WriteToClipboard(unsigned int format, HANDLE handle);

  // Free a handle depending on its type (as intuited from format)
  static void FreeData(unsigned int format, HANDLE data);

  // Return the window that should be the clipboard owner, creating it
  // if neccessary.  Marked const for lazily initialization by const methods.
  HWND GetClipboardWindow() const;

  // Mark this as mutable so const methods can still do lazy initialization.
  mutable HWND clipboard_owner_;

  // True if we can create a window.
  bool create_window_;

  DISALLOW_EVIL_CONSTRUCTORS(Clipboard);
};

#endif  // GOOPY_COMMON_CLIPBOARD_H_

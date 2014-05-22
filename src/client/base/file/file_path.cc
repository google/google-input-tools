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

#include "base/file/file_path.h"

#include <string.h>
#include <algorithm>

#include "base/basictypes.h"
#include "base/logging.h"

#if defined(OS_WIN)
#include <windows.h>
#endif

#if defined(FILE_PATH_USES_WIN_SEPARATORS)
const FilePath::CharType FilePath::kSeparators[] = FILE_PATH_LITERAL("\\/");
#else  // FILE_PATH_USES_WIN_SEPARATORS
const FilePath::CharType FilePath::kSeparators[] = FILE_PATH_LITERAL("/");
#endif  // FILE_PATH_USES_WIN_SEPARATORS

const FilePath::CharType FilePath::kCurrentDirectory[] = FILE_PATH_LITERAL(".");
const FilePath::CharType FilePath::kParentDirectory[] = FILE_PATH_LITERAL("..");

const FilePath::CharType FilePath::kExtensionSeparator = FILE_PATH_LITERAL('.');

typedef FilePath::StringType StringType;

namespace {

int InternalCompareIgnoreCase(const StringType& string1,
                              const StringType& string2) {
  // Perform character-wise upper case comparison rather than using the
  // fully Unicode-aware CompareString(). For details see:
  // http://blogs.msdn.com/michkap/archive/2005/10/17/481600.aspx
  StringType::const_iterator i1 = string1.begin();
  StringType::const_iterator i2 = string2.begin();
  StringType::const_iterator string1end = string1.end();
  StringType::const_iterator string2end = string2.end();
  for ( ; i1 != string1end && i2 != string2end; ++i1, ++i2) {
#if defined(OS_WIN)
// Windows specific implementation of file string comparisons
    wchar_t c1 = (wchar_t)LOWORD(::CharUpperW((LPWSTR)MAKELONG(*i1, 0)));
    wchar_t c2 = (wchar_t)LOWORD(::CharUpperW((LPWSTR)MAKELONG(*i2, 0)));
#else
    char c1 = toupper(*i1);
    char c2 = toupper(*i2);
#endif
    if (c1 < c2)
      return -1;
    if (c1 > c2)
      return 1;
  }
  if (i1 != string1end)
    return 1;
  if (i2 != string2end)
    return -1;
  return 0;
}

// If this FilePath contains a drive letter specification, returns the
// position of the last character of the drive letter specification,
// otherwise returns npos.  This can only be true on Windows, when a pathname
// begins with a letter followed by a colon.  On other platforms, this always
// returns npos.
StringType::size_type FindDriveLetter(const StringType& path) {
#if defined(FILE_PATH_USES_DRIVE_LETTERS)
  // This is dependent on an ASCII-based character set, but that's a
  // reasonable assumption.  iswalpha can be too inclusive here.
  if (path.length() >= 2 && path[1] == L':' &&
      ((path[0] >= L'A' && path[0] <= L'Z') ||
       (path[0] >= L'a' && path[0] <= L'z'))) {
    return 1;
  }
#endif  // FILE_PATH_USES_DRIVE_LETTERS
  return StringType::npos;
}

#if defined(FILE_PATH_USES_DRIVE_LETTERS)
bool EqualDriveLetterCaseInsensitive(const StringType& a,
                                     const StringType& b) {
  size_t a_letter_pos = FindDriveLetter(a);
  size_t b_letter_pos = FindDriveLetter(b);

  if (a_letter_pos == StringType::npos || b_letter_pos == StringType::npos)
    return a == b;

  StringType a_letter(a.substr(0, a_letter_pos + 1));
  StringType b_letter(b.substr(0, b_letter_pos + 1));
  if (InternalCompareIgnoreCase(a_letter, b_letter) != 0)
    return false;

  StringType a_rest(a.substr(a_letter_pos + 1));
  StringType b_rest(b.substr(b_letter_pos + 1));
  return a_rest == b_rest;
}
#endif  // defined(FILE_PATH_USES_DRIVE_LETTERS)

bool IsPathAbsolute(const StringType& path) {
#if defined(FILE_PATH_USES_DRIVE_LETTERS)
  StringType::size_type letter = FindDriveLetter(path);
  if (letter != StringType::npos) {
    // Look for a separator right after the drive specification.
    return path.length() > letter + 1 &&
        FilePath::IsSeparator(path[letter + 1]);
  }
  // Look for a pair of leading separators.
  return path.length() > 1 &&
      FilePath::IsSeparator(path[0]) && FilePath::IsSeparator(path[1]);
#else  // FILE_PATH_USES_DRIVE_LETTERS
  // Look for a separator in the first position.
  return path.length() > 0 && FilePath::IsSeparator(path[0]);
#endif  // FILE_PATH_USES_DRIVE_LETTERS
}

bool AreAllSeparators(const StringType& input) {
  for (StringType::const_iterator it = input.begin();
      it != input.end(); ++it) {
    if (!FilePath::IsSeparator(*it))
      return false;
  }

  return true;
}

// Find the position of the '.' that separates the extension from the rest
// of the file name. The position is relative to BaseName(), not value().
StringType::size_type ExtensionSeparatorPosition(const StringType& path) {
  // Special case "." and ".."
  if (path == FilePath::kCurrentDirectory || path == FilePath::kParentDirectory)
    return StringType::npos;

  const StringType::size_type last_dot =
      path.rfind(FilePath::kExtensionSeparator);

  return last_dot;
}

}  // namespace

FilePath::FilePath() {
}

FilePath::FilePath(const FilePath& that) : path_(that.path_) {
}

FilePath::FilePath(const StringType& path) : path_(path) {
}

FilePath::~FilePath() {
}

FilePath& FilePath::operator=(const FilePath& that) {
  path_ = that.path_;
  return *this;
}

bool FilePath::operator==(const FilePath& that) const {
#if defined(FILE_PATH_USES_DRIVE_LETTERS)
  return EqualDriveLetterCaseInsensitive(this->path_, that.path_);
#else  // defined(FILE_PATH_USES_DRIVE_LETTERS)
  return path_ == that.path_;
#endif  // defined(FILE_PATH_USES_DRIVE_LETTERS)
}

bool FilePath::operator!=(const FilePath& that) const {
#if defined(FILE_PATH_USES_DRIVE_LETTERS)
  return !EqualDriveLetterCaseInsensitive(this->path_, that.path_);
#else  // defined(FILE_PATH_USES_DRIVE_LETTERS)
  return path_ != that.path_;
#endif  // defined(FILE_PATH_USES_DRIVE_LETTERS)
}

// static
bool FilePath::IsSeparator(CharType character) {
  for (size_t i = 0; i < arraysize(kSeparators) - 1; ++i) {
    if (character == kSeparators[i]) {
      return true;
    }
  }

  return false;
}

void FilePath::GetComponents(std::vector<StringType>* components) const {
  DCHECK(components);
  if (!components)
    return;
  components->clear();
  if (value().empty())
    return;

  std::vector<StringType> ret_val;
  FilePath current = *this;
  FilePath base;

  // Capture path components.
  while (current != current.DirName()) {
    base = current.BaseName();
    if (!AreAllSeparators(base.value()))
      ret_val.push_back(base.value());
    current = current.DirName();
  }

  // Capture root, if any.
  base = current.BaseName();
  if (!base.value().empty() && base.value() != kCurrentDirectory)
    ret_val.push_back(current.BaseName().value());

  // Capture drive letter, if any.
  FilePath dir = current.DirName();
  StringType::size_type letter = FindDriveLetter(dir.value());
  if (letter != StringType::npos) {
    ret_val.push_back(StringType(dir.value(), 0, letter + 1));
  }

  *components = std::vector<StringType>(ret_val.rbegin(), ret_val.rend());
}

bool FilePath::IsParent(const FilePath& child) const {
  return AppendRelativePath(child, NULL);
}

bool FilePath::AppendRelativePath(const FilePath& child,
                                  FilePath* path) const {
  std::vector<StringType> parent_components;
  std::vector<StringType> child_components;
  GetComponents(&parent_components);
  child.GetComponents(&child_components);

  if (parent_components.empty() ||
      parent_components.size() >= child_components.size())
    return false;

  std::vector<StringType>::const_iterator parent_comp =
      parent_components.begin();
  std::vector<StringType>::const_iterator child_comp =
      child_components.begin();

#if defined(FILE_PATH_USES_DRIVE_LETTERS)
  // Windows can access case sensitive filesystems, so component
  // comparisions must be case sensitive, but drive letters are
  // never case sensitive.
  if ((FindDriveLetter(*parent_comp) != StringType::npos) &&
      (FindDriveLetter(*child_comp) != StringType::npos)) {
    if (!CompareEqualIgnoreCase(*parent_comp, *child_comp))
      return false;
    ++parent_comp;
    ++child_comp;
  }
#endif  // defined(FILE_PATH_USES_DRIVE_LETTERS)

  while (parent_comp != parent_components.end()) {
    if (*parent_comp != *child_comp)
      return false;
    ++parent_comp;
    ++child_comp;
  }

  if (path != NULL) {
    for (; child_comp != child_components.end(); ++child_comp) {
      *path = path->Append(*child_comp);
    }
  }
  return true;
}

// libgen's dirname and basename aren't guaranteed to be thread-safe and aren't
// guaranteed to not modify their input strings, and in fact are implemented
// differently in this regard on different platforms.  Don't use them, but
// adhere to their behavior.
FilePath FilePath::DirName() const {
  FilePath new_path(path_);
  new_path.StripTrailingSeparatorsInternal();

  // The drive letter, if any, always needs to remain in the output.  If there
  // is no drive letter, as will always be the case on platforms which do not
  // support drive letters, letter will be npos, or -1, so the comparisons and
  // resizes below using letter will still be valid.
  StringType::size_type letter = FindDriveLetter(new_path.path_);

  StringType::size_type last_separator =
      new_path.path_.find_last_of(kSeparators, StringType::npos,
                                  arraysize(kSeparators) - 1);
  if (last_separator == StringType::npos) {
    // path_ is in the current directory.
    new_path.path_.resize(letter + 1);
  } else if (last_separator == letter + 1) {
    // path_ is in the root directory.
    new_path.path_.resize(letter + 2);
  } else if (last_separator == letter + 2 &&
             IsSeparator(new_path.path_[letter + 1])) {
    // path_ is in "//" (possibly with a drive letter); leave the double
    // separator intact indicating alternate root.
    new_path.path_.resize(letter + 3);
  } else if (last_separator != 0) {
    // path_ is somewhere else, trim the basename.
    new_path.path_.resize(last_separator);
  }

  new_path.StripTrailingSeparatorsInternal();
  if (!new_path.path_.length())
    new_path.path_ = kCurrentDirectory;

  return new_path;
}

FilePath FilePath::BaseName() const {
  FilePath new_path(path_);
  new_path.StripTrailingSeparatorsInternal();

  // The drive letter, if any, is always stripped.
  StringType::size_type letter = FindDriveLetter(new_path.path_);
  if (letter != StringType::npos) {
    new_path.path_.erase(0, letter + 1);
  }

  // Keep everything after the final separator, but if the pathname is only
  // one character and it's a separator, leave it alone.
  StringType::size_type last_separator =
      new_path.path_.find_last_of(kSeparators, StringType::npos,
                                  arraysize(kSeparators) - 1);
  if (last_separator != StringType::npos &&
      last_separator < new_path.path_.length() - 1) {
    new_path.path_.erase(0, last_separator + 1);
  }

  return new_path;
}

StringType FilePath::Extension() const {
  FilePath base(BaseName());
  const StringType::size_type dot = ExtensionSeparatorPosition(base.path_);
  if (dot == StringType::npos)
    return StringType();

  return base.path_.substr(dot, StringType::npos);
}

FilePath FilePath::RemoveExtension() const {
  if (Extension().empty())
    return *this;

  const StringType::size_type dot = ExtensionSeparatorPosition(path_);
  if (dot == StringType::npos)
    return *this;

  return FilePath(path_.substr(0, dot));
}

FilePath FilePath::InsertBeforeExtension(const StringType& suffix) const {
  if (suffix.empty())
    return FilePath(path_);

  if (path_.empty())
    return FilePath();

  StringType base = BaseName().value();
  if (base.empty())
    return FilePath();
  if (*(base.end() - 1) == kExtensionSeparator) {
    // Special case "." and ".."
    if (base == kCurrentDirectory || base == kParentDirectory) {
      return FilePath();
    }
  }

  StringType ext = Extension();
  StringType ret = RemoveExtension().value();
  ret.append(suffix);
  ret.append(ext);
  return FilePath(ret);
}

FilePath FilePath::ReplaceExtension(const StringType& extension) const {
  if (path_.empty())
    return FilePath();

  StringType base = BaseName().value();
  if (base.empty())
    return FilePath();
  if (*(base.end() - 1) == kExtensionSeparator) {
    // Special case "." and ".."
    if (base == kCurrentDirectory || base == kParentDirectory) {
      return FilePath();
    }
  }

  FilePath no_ext = RemoveExtension();
  // If the new extension is "" or ".", then just remove the current extension.
  if (extension.empty() || extension == StringType(1, kExtensionSeparator))
    return no_ext;

  StringType str = no_ext.value();
  if (extension[0] != kExtensionSeparator)
    str.append(1, kExtensionSeparator);
  str.append(extension);
  return FilePath(str);
}

bool FilePath::MatchesExtension(const StringType& extension) const {
  DCHECK(extension.empty() || extension[0] == kExtensionSeparator);

  StringType current_extension = Extension();

  if (current_extension.length() != extension.length())
    return false;

  return FilePath::CompareEqualIgnoreCase(extension, current_extension);
}

FilePath FilePath::Append(const StringType& component) const {
  DCHECK(!IsPathAbsolute(component));
  if (path_.compare(kCurrentDirectory) == 0) {
    // Append normally doesn't do any normalization, but as a special case,
    // when appending to kCurrentDirectory, just return a new path for the
    // component argument.  Appending component to kCurrentDirectory would
    // serve no purpose other than needlessly lengthening the path, and
    // it's likely in practice to wind up with FilePath objects containing
    // only kCurrentDirectory when calling DirName on a single relative path
    // component.
    return FilePath(component);
  }

  FilePath new_path(path_);
  new_path.StripTrailingSeparatorsInternal();

  // Don't append a separator if the path is empty (indicating the current
  // directory) or if the path component is empty (indicating nothing to
  // append).
  if (component.length() > 0 && new_path.path_.length() > 0) {
    // Don't append a separator if the path still ends with a trailing
    // separator after stripping (indicating the root directory).
    if (!IsSeparator(new_path.path_[new_path.path_.length() - 1])) {
      // Don't append a separator if the path is just a drive letter.
      if (FindDriveLetter(new_path.path_) + 1 != new_path.path_.length()) {
        new_path.path_.append(1, kSeparators[0]);
      }
    }
  }

  new_path.path_.append(component);
  return new_path;
}

FilePath FilePath::Append(const FilePath& component) const {
  return Append(component.value());
}

bool FilePath::IsAbsolute() const {
  return IsPathAbsolute(path_);
}

FilePath FilePath::StripTrailingSeparators() const {
  FilePath new_path(path_);
  new_path.StripTrailingSeparatorsInternal();

  return new_path;
}

bool FilePath::ReferencesParent() const {
  std::vector<StringType> components;
  GetComponents(&components);

  std::vector<StringType>::const_iterator it = components.begin();
  for (; it != components.end(); ++it) {
    const StringType& component = *it;
    if (component == kParentDirectory)
      return true;
  }
  return false;
}

void FilePath::StripTrailingSeparatorsInternal() {
  // If there is no drive letter, start will be 1, which will prevent stripping
  // the leading separator if there is only one separator.  If there is a drive
  // letter, start will be set appropriately to prevent stripping the first
  // separator following the drive letter, if a separator immediately follows
  // the drive letter.
  StringType::size_type start = FindDriveLetter(path_) + 2;

  StringType::size_type last_stripped = StringType::npos;
  for (StringType::size_type pos = path_.length();
       pos > start && IsSeparator(path_[pos - 1]);
       --pos) {
    // If the string only has two separators and they're at the beginning,
    // don't strip them, unless the string began with more than two separators.
    if (pos != start + 1 || last_stripped == start + 2 ||
        !IsSeparator(path_[start - 1])) {
      path_.resize(pos - 1);
      last_stripped = pos;
    }
  }
}

FilePath FilePath::NormalizePathSeparators() const {
#if defined(FILE_PATH_USES_WIN_SEPARATORS)
  StringType copy = path_;
  for (size_t i = 1; i < arraysize(kSeparators); ++i) {
    std::replace(copy.begin(), copy.end(), kSeparators[i], kSeparators[0]);
  }
  return FilePath(copy);
#else
  return *this;
#endif
}

int FilePath::CompareIgnoreCase(const StringType& string1,
                                const StringType& string2) {
  return InternalCompareIgnoreCase(string1, string2);
}

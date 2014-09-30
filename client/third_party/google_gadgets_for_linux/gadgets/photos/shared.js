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

// for debug convenience, and options.putValue
function plainObject(obj) {
  if (obj == undefined) {
    return "undefined";
  }
  if (typeof obj == "string" || obj instanceof String) {
    return "\"" + obj.replace(/\\/g, "\\\\").replace(/\n/g, "\\n")
      .replace(/\r/g, "\\r").replace(/\"/g, "\\\"")
      + "\"";
  }
  if (obj instanceof Function) {
    return "" + obj;
  }
  if (obj instanceof Array) {
    return "[" + arrayMap(obj, plainObject).join(",") + "]";
  }
  if (obj instanceof Object) {
    var os = [];
    for (var name in obj) {
      os.push(plainObject(name) + ":" + plainObject(obj[name]));
    }
    return "{" + os.join(",") + "}";
  }
  return "" + obj;
}

function fromPlainString(str) {
  eval("var $obj=" + str);
  return $obj;
}

function enumToArray(coll) {
  var aa = [];
  if (!coll) return aa;
  if (coll.atEnd == undefined || ! coll.atEnd instanceof Function) {
    coll = new Enumerator(coll);
  }
  while (!coll.atEnd()) {
    var item = coll.item();
    aa.push(item);
    coll.moveNext();
  }
  return aa;
}

function optionsPut(key, value, isDefault) {
  if (isDefault)
    options.putDefaultValue(key, plainObject(value));
  else
    options.putValue(key, plainObject(value));
}

function optionsGet(key, value) {
  return fromPlainString(options.getValue(key));
}

function arrayMap(a, func) {
  var ret = [];
  for (var i = 0; i < a.length; ++i) {
    var tmp = func(a[i]);
    if (tmp != undefined)
      ret = ret.concat(tmp);
  }
  return ret;
}

function extendObject(dst, src) {
  for (var k in src) {
    dst[k] = src[k];
  }
  return dst;
}

function extendGiven(dst, src) {
  for (var i = 2; i < arguments.length; ++i) {
    dst[arguments[i]] = src[arguments[i]];
  }
  return dst;
}


// return a func has the ability of $_func with $_object as this.
function bindFunction($_func, $_object/*, ... */) {
  var $_bindArgs = arguments;
  return function() {
    var $_args = [], i;
    for (i = 2; i < $_bindArgs.length; ++i)
      $_args.push($_bindArgs[i]);
    for (i = 0; i < arguments.length; ++i)
      $_args.push(arguments[i]);
    return $_func.apply($_object, $_args);
  };
}

function checkFile(fn) {
  if (system && system.filesystem) {
    if (system.filesystem.FileExists(fn)) {
      return 1;
    }
    if (system.filesystem.FolderExists(fn)) {
      return 2;
    }
  }
  return 0;
}

function scaleToMax(obj, dst) {
  if (obj.width * dst.height > obj.height * dst.width) {
    if (obj.width) {
      obj.height = Math.round(obj.height * dst.width / obj.width);
      obj.width = dst.width;
    }
  } else {
    if (obj.height) {
      obj.width = Math.round(obj.width * dst.height / obj.height);
      obj.height = dst.height;
    }
  }
}

function resizedImage(img, cont, model) {
  var to = {width: img.srcWidth, height: img.srcHeight};
  scaleToMax(to, {width: cont.width, height: cont.height});
  if (model == "photo") {
    img.width = cont.width;
    img.height = cont.height;
    img.x = img.y = 0;
  } else {
    img.width = to.width;
    img.height = to.height;
    img.x = (cont.width - to.width) / 2;
    img.y = (cont.height - to.height) / 2;
  }
  if (to.width < img.srcWidth) {
    // crop src size is good for memory, but bad for zoom x2 limit!
    // img.setSrcSize(to.width, to.height);
  }
}


function parseHttpDate(str) {
  // RFC850 contains -, without it, Date can parse it!
  return new Date(str.replace(/-/g, " "));
}

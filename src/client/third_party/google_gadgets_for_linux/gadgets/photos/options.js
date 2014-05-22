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

function escapeEntity(str) {
  return str.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/, "&gt;");
}

function listItemWrap(str) {
  return "<item>" + itemWrap(str) + "</item>";
}

function itemWrap(str) {
  var estr = escapeEntity(str).replace(/\'/g, "&apos;");
  return "<label size='8' tooltip='" + estr + "'>" + estr + "</label>";
}

function optionOpen() {
  var aus = optionsGet("feedurls");
  putAllURLs(aus);
  slideInterval.value = optionsGet("slideInterval");
  setTimeout(function() {
               slideIntervalWrap.x = msgSlideIntervalBefore.offsetWidth + 5;
               msgSlideIntervalAfter.x = slideIntervalWrap.x + slideIntervalWrap.width + 5;
             },
             100
            );
}

function childrenToArray(children) {
  var aa = [];
  var chs = new Enumerator(children);
  while (!chs.atEnd()) {
    var item = chs.item();
    if (item && item.tagName) {
      aa.push(chs.item());
    }
    chs.moveNext();
  }
  return aa;
}

function getAllURLs() {
  var aus = [], i;
  if (feedurls_wrap.visible) {
    aus = feedurls.value.split(/ *[\n\r]+ */g);
  } else {
    var achs = childrenToArray(urlitems.children);
    for (i = 0; i < achs.length; ++i) {
      var url = getText(achs[i]);
      if (url) aus.push(url);
    }
  }
  return aus;
}

function putAllURLs(aus) {
  for (var i = 0; i < aus.length; ++i) {
    if (aus[i])
      urlitems.appendElement(listItemWrap(aus[i]));
  }
}

function checkUrl(s) {
  s = s || "";
  if (s.match("^#"))
    return 10000;
  if (s.match("^http://")) {
    return 1;
  }
  return checkFile(s);
}

function getText(node) {
  var ret = "";
  if (node) {
    if (node.tagName == "label") {
      return node.innerText;
    }
    var achs = childrenToArray(node.children);
    for (var i = 0; i < achs.length; ++i)
      ret += getText(achs[i]);
  }
  return ret;
}

function urlitemsKeypress() {
  //debug.trace("ONKEYPRESS WITH KEY=" + event.keyCode);
  var sidx = urlitems.selectedIndex;
  if (sidx >= 0) {
    if (event.keyCode == 106) {//j to down
      var achs = childrenToArray(urlitems.children);
      if (sidx < achs.length - 1) {
        var ct = getText(achs[sidx]);
        var nt = getText(achs[sidx + 1]);
        achs[sidx].removeAllElements();
        achs[sidx + 1].removeAllElements();
        achs[sidx].appendElement(itemWrap(nt));
        achs[sidx + 1].appendElement(itemWrap(ct));
        ++urlitems.selectedIndex;
      }
    } else if (event.keyCode == 107) {//k to up
      var achs = childrenToArray(urlitems.children);
      if (sidx > 0) {
        var ct = getText(achs[sidx]);
        var nt = getText(achs[sidx - 1]);
        achs[sidx].removeAllElements();
        achs[sidx - 1].removeAllElements();
        achs[sidx].appendElement(itemWrap(nt));
        achs[sidx - 1].appendElement(itemWrap(ct));
        --urlitems.selectedIndex;
      }
    } else if (event.keyCode == 68) {//D to delete
      onRemoveUrl();
    }
  }
  if (event.keyCode == 125){ //}
    if (feedurls.value) {
      feedurls.value = getAllURLs().join("\n");
      feedurls_wrap.visible = "true";
    } else {
      feedurls.value = 1;
    }
  } else {
    feedurls.value = 0;
  }
  event.returnValue = true;
}

function onAddUrl() {
  var s = urleditor.value;
  if (!s) return;//ignore empty
  if (checkUrl(s)) {
    urlitems.removeString(s);
    urlitems.appendElement(listItemWrap(s));
    urlitems.clearSelection();
    urleditor.value = "";
  } else {
    alert(OPTIONS_INVALIDURL.replace("%s", s));
  }
}

function onRemoveUrl() {
  var achs = childrenToArray(urlitems.children);
  for (var i = 0; i < achs.length; ++i) {
    if (achs[i].selected) {
      urleditor.value = getText(achs[i]);
      urlitems.removeElement(achs[i]);
    }
  }
}

function onBrowseLocal() {
  var fn = BrowseForFile(
    OPTIONS_BROWSE_IMAGE_FILES +
      "|*.jpg;*.jpeg;*.png;*.gif;*.tif;*.tiff;" +
      "*.JPG;*.JPEG;*.PNG;*.GIF;*.TIF;*.TIFF|" +
      OPTIONS_BROWSE_ALL_FILES +
      "|*");
  if (fn) {
    var dn = system.filesystem.getParentFolderName(fn);
    urlitems.removeString(dn);
    urlitems.appendElement(listItemWrap(dn));
    urlitems.clearSelection();
  }
}

function onChangeInterval(a, type)
{
  var sli;
  try {
    sli = slideInterval.value.replace(/[^0-9]/g, "") - 0;
    if (sli == NaN) {
      sli = 15;
    }
  } catch(e) {
  }
  if (type) {
    sli += a;
  }
  if (sli <= 5) {
    sli = 5;
    downinterval.enabled = "false";
  } else {
    downinterval.enabled = "true";
  }
  slideInterval.value = sli;
}

function optionOk() {
  optionsPut("feedurls", getAllURLs());
  var sint = slideInterval.value.replace(/\D/g, "") - 0 || 4;
  if (sint < 3) sint = 3;
  optionsPut("slideInterval", sint);
}

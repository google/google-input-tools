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

function SourceItem(url, feed, title, link, desc, type) {
  this.url = url || "";
  this.feed = feed || "";//for Referer header
  this.title = title || this.url;
  this.link = link || this.url;
  this.desc = desc || this.url;
  this.type = type || "";
  if (!this.type) {
    if (this.url.indexOf("http://") == 0) {
      this.type = "http";
    } else {
      this.type = "file";
    }
  }
  //debug.trace("gen source url=" + url);
}

SourceItem.prototype = {
  toString: function() {
    return "SourceItem(url:" + this.url + ")";
  },

  _fileImageRegExp:
  new RegExp("\\.(?:(?:jpe?g)|(?:gif)|(?:tiff?)|(?:png))$", "i"),

  checkValid: function() {
    return this.url &&
      (this.type != "file" ||
       (this.url.match(this._fileImageRegExp) && checkFile(this.url))) &&
      !kBlackList[this.url];
  },

  setImage: function(stream) {
    this.fetched = 1;
    this.image = stream;
    this.onFetched(this, 1);
  },

  fetchHttpReadyHandler: function(req) {
    if (req.readyState == 4 && req.status == 200) {
      var contentType = req.getResponseHeader("content-type") || "";
      if (contentType.match("image")) {
        this.setImage(req.responseStream);
      } else {
        debug.trace("expect a image, but get type " + contentType +
                    " url=" + this.url);
        this.error = 1;
        this.onFetched(this, -1);
      }
    } else if (req.readyState == 4) {
      this.error = 1;
      this.onFetched(this, -1);
      debug.trace("http: fetch url=" + this.url + ", get status=" + req.status);
    }
  },

  fetch: function() {
    if (this.fetched || this.error || this.image) return;
    this.fetched = 1;
    if (this.type == "http") {
      var req = new XMLHttpRequest();
      req.open("GET", this.url, true);
      // this is forbidden in GGL now(?)
      //if (this.feed && this.feed.url != this.url)
      //req.setRequestHeader("Referer", this.feed.url);
      req.onreadystatechange = bindFunction(
        this.fetchHttpReadyHandler,
        this, req);
      try {
        req.send();
      } catch (e) {
        debug.trace("http send got exception:" + e.message +
                    ", when fetch feed url=" + this.url);
        this.error = 1;
        this.onFetched(this, -1);
      }
    }
    if (this.type == "file") {
      this.setImage(this.url);
    }
  },

  unfetch: function() {
    this.fetched = 0;
    this.image = undefined;
  }
};

//most time object is null
function FeedItem(url, type, object) {
  //return plugin type if url match
  for (var i = 0; i < FeedItem.prototype.plugins.length; ++i) {
    if (url.match(FeedItem.prototype.plugins[i].urlPattern)) {
      return new FeedItem.prototype.plugins[i].object(url, type, object);
    }
  }
  if (!type) {
    if (url.indexOf("#") == 0)
      type = "just-comment";
    else if (url.indexOf("http://") == 0) {
      type = "http";
    } else if (url.indexOf("file://") == 0) {
      type = "file";
      url = url.slice(7);
    } else
      type = "file";
  }
  //register a type
  if (type.indexOf(":plugins:") == 0) {
    FeedItem.prototype.plugins.push({
                                      name: type.substr(9),
                                      urlPattern: url, object: arguments[2]});
  }
  this.type = type;
  this.url = url;
}

FeedItem.prototype = {
  plugins: [],
  sourceList: [],
  expiredUTC: 0,
  lastFetchedUTC: 0,

  addSourceItem: function(item) {
    var items = [].concat(item);
    var validItems = [];
    for (var i = 0; i < items.length; ++i) {
      if (items[i].checkValid()) {
        items[i].onFetched = this.onSourceItemFetched;
        validItems.push(items[i]);
      }
    }
    this.sourceList = this.sourceList.concat(validItems);
    this.onAddSourceItem(this, validItems);
  },

  removeSourceItem: function(item) {
    for (var i = 0; i < this.sourceList.length; ++i) {
      if (item.url == this.sourceList[i].url) {
        this.sourceList.splice(i, 1);
        break;
      }
    }
    //debug.trace("after remove, len="  + this.sourceList.length);
  },

  clearSourceList: function() {
    this.onClearSourceList(this, this.sourceList);
    this.sourceList = [];
  },

  checkExpired: function(utc) {
    if (this.isImage || this.sourceProcessing) return 0;
    if (!this.lastFetchedUTC) return 1;//first time
    utc = utc || new Date().getTime();
    if (this.expiredUTC) {
      return (this.expiredUTC <= utc);
    } else {
      return (utc - this.lastFetchedUTC) > 1800 * 1000;//no cache
    }
  },

  checkValid: function() {
    if (!this.url || this.type == "just-comment") return undefined;
    if (this.type == "file") {
      var ft = checkFile(this.url);
      if (ft == 1) {
        var sitem = new SourceItem(this.url, this);
        if (!sitem.checkValid()) return undefined;
      }
      if (ft == 0) return undefined;
    }
    return this;
  },

  fetch: function() {
    var utc = new Date().getTime();
    if (!this.checkExpired(utc)) {
      debug.trace("unexpired, not fetch....");
      return;
    }
    if (this.type == "http") {
      this.fetchHttp();
    } else if (this.type == "file") {
      this.fetchFile();
    } else {
      debug.trace("unsupported feed type=" + this.type + " url=" + this.url);
    }
  },

  _directoryProcessing: function(co, count) {
    var sitems = [];
    var i = 0;
    while (co.fileset == undefined || co.fileset.atEnd()) {
      if (i > count) {
        debug.trace("directory too deep without image once");
        return 1;
      }
      while (co.folderset == undefined || co.folderset.atEnd()) {
        if (co.folderss.length == 0) return 0;// end
        co.folderset = co.folderss.shift();
      }
      var fd = co.folderset.item();
      try {
        debug.trace("Fetch folder: " + fd);
        var folderset = new Enumerator(fd.SubFolders);
        if (!folderset.atEnd()) {
          co.folderss.push(folderset);
        }
        co.fileset = new Enumerator(fd.Files);
      } catch(e) {
        debug.trace("Failed to process folder: " + fd + " error: " + e);
      }
      co.folderset.moveNext();
      ++i;
    }
    for (i = 0; i < count && !co.fileset.atEnd(); co.fileset.moveNext(), ++i) {
      var fn = co.fileset.item().Path;
      sitems.push(new SourceItem(fn, this));
    }
    this.addSourceItem(sitems);
    return co.fileset.atEnd() && co.folderset.atEnd() &&
      co.folderss.length == 0 ? 0 : 1;
  },

  fetchFile: function() {
    var ft = checkFile(this.url);
    if (ft == 1) {
      this.isImage = 1;
      this.clearSourceList();
      this.addSourceItem(new SourceItem(this.url, this));
    }
    if (ft == 2) {
      try {
        var sitems = [];
        var fd = system.filesystem.GetFolder(this.url);
        debug.trace("Fetch folder: " + fd);
        var fileset = new Enumerator(fd.Files);
        var folderset = new Enumerator(fd.SubFolders);
        this.clearSourceList();
        this.sourceProcessing = bindFunction(this._directoryProcessing,
                                             this,
                                             {fileset: fileset,
                                              folderset: folderset,
                                              folderss: []});
      } catch(e) {
        debug.trace("caught exception when read dir " + this.url + ": "
                    + e.message);
      }
    }
    if (ft == 0) {
      debug.trace("unexists local file/dir: " + this.url);
    }
    this.onFetched(this, ft);
    this.lastFetchedUTC = new Date().getTime();
    this.expiredUTC = this.lastFetchedUTC + 3600 * 1000;
  },

  httpContentTypeMaps: [
    {pattern: "atom", type: ":atom"},
    {pattern: "rss", type: ":rss"},
    {pattern: "image", type: ":image"},
    {pattern: "xml", type: ":xml"},
    {pattern: "html", type: ":html"}
  ],

  httpUrlTypeMaps : [
    {pattern: "\\brss$", type: ":rss"},
    {pattern: "\\batom$", type: ":atom"},
    {pattern: "rss$", type: ":rss"},
    {pattern: "atom$", type: ":atom"},
    {pattern: "\\brss\\b", type: ":rss"},
    {pattern: "\\batom\\b", type: ":atom"},
    {pattern: "rss", type: ":rss"},
    {pattern: "atom", type: ":atom"}
  ],

  // parse the cache header, return a expire utc or 0 if non cached
  parseHttpRequestExpires: function(req) {
    var cacheControl = req.getResponseHeader("cache-control");
    if (cacheControl) {
      if (cacheControl.match("no-cache")) return 0;
      var mm = cacheControl.match("(?:(?:s-maxage)|(?:max-age))=([^0-9]+)");
      var maxage = 0;
      if (mm) {
        maxage = mm[1];
        return this.lastFetchedUTC + maxage;
      }
    }
    var expires = req.getResponseHeader("expires") || "";
    if (expires)
      return new Date(expires.replace(/-/g, " ")).getTime() || 0;
    return 0;//no cache relative stuff find...
  },

  //Caution: function called with this=FeedItem
  fetchHttpReadyHandler: function(req) {
    if (req.readyState == 4 && req.status == 304) {
      debug.trace("not modified feed " + this.url);
      this.onFetched(this, 2);
    }
    if (req.readyState == 4 && (req.status == 200 || req.status == 304)) {
      this.httpETag = req.getResponseHeader("etag");
      this.lastFetchedUTC = new Date().getTime();
      this.expiredUTC = this.parseHttpRequestExpires(req);
      if (this.expiredUTC - this.lastFetchedUTC <= 60 * 1000) {
        this.expiredUTC = 0;//use default if it too samll
      }
    } else if (req.readyState == 4) {
      debug.trace("got status " + req.status + " in fetch feed " + this.url);
      this.error = 1;
      this.expiredUTC = 0;//use default cache time to retry
      this.onFetched(this, -1);
    }
    if (req.readyState == 4 && req.status == 200) {
      var contentType = req.getResponseHeader("content-type") || "";
      var type = "";
      for (var i = 0; i < this.httpContentTypeMaps.length; ++i) {
        var pat = this.httpContentTypeMaps[i];
        if (contentType.match(pat.pattern)) {
          type = pat.type;
          break;
        }
      }
      if (!type) {
        debug.trace("unsupported http contentType feed, contentType=" +
                    contentType + " now guess from the url=" + this.url);
        for (var i = 0; i < this.httpUrlTypeMaps.length; ++i) {
          var pat = this.httpUrlTypeMaps[i];
          if (this.url.match(pat.pattern)) {
            type = pat.type;
            break;
          }
        }
      }
      this.httpProcessorType = type;
      this.clearSourceList();
      this.error = 0;
      var bd = new Date();
      var ret = this.httpProcessContent(req);
      var ed = new Date();
      //debug.trace("PROCESS CONTENT TIME = " + (ed.getTime() - bd.getTime()));
      this.onFetched(this, ret);
    }
  },

  fetchHttp: function() {
    debug.trace("fetch http feed url=" + this.url);
    var req = new XMLHttpRequest();
    req.open("GET", this.url, true);
    req.onreadystatechange = bindFunction(
      this.fetchHttpReadyHandler,
      this, req);
    if (this.httpETag) {
      req.setRequestHeader("If-None-Match", this.httpETag);
    } else if (this.lastFetchedUTC) {
      req.setRequestHeader("If-Modified-Since",
        new Date(this.lastFetchedUTC).toUTCString());
    }
    try {
      req.send();
    } catch (e) {
      // unused error now. maybe delete the feed??
      this.error = 1;
      this.onFetched(this, -1);
      debug.trace("http send got exception:" + e.message +
                  ", when fetch feed url=" + this.url);
    }
  },

  httpGetXmlDoc : function(req) {
    var xmldoc = req.responseXML;
    if (!xmldoc) {
      debug.trace("parse xml error from feed url=" + this.url);
      return null;
    }
    if (!xmldoc.documentElement) {//fix IE!
      xmldoc = new DOMDocument();
      xmldoc.resolveExternals = false;
      xmldoc.validateOnParse = false;
      xmldoc.setProperty('ProhibitDTD', false);
      xmldoc.loadXML(req.responseText);
    }
    return xmldoc;
  },

  _htmlFilImgRegExp:
    new RegExp("<img [^>]*\\bsrc=((?:http://[^ ]+)|"
               + "(?:'http://[^']+')|(?:\"http://[^\"]+\"))"),

  _htmlFilImg: function(text) {
    text = text || "";
    var src = "";
    var sm = text.match(this._htmlFilImgRegExp);
    if (sm) {
      src = sm[1];
      if (src.charAt(0) == '"' || src.charAt(0) == "'")
        src = src.slice(1, -1);
    }
    return src;
  },

  _rssItemsProcessing: function(co, count) {
    var sitems = [];
    var bd = new Date().getTime();
    for (var i = 0, item; i < count && (item = co.items[co.idx + i]); ++i) {
      if (item.nodeType != 1 || item.tagName != "item") continue;
      sitems.push(
        new SourceItem(
          new XmlSelector(">enclosure[url]", item)
            .add("media:content[type^=image][url]", item)
            .add("media:tumbnail[url]", item).attr("url", 1) ||
            new XmlSelector(">image>url", item).text(1) ||
            this._htmlFilImg(new XmlSelector(">description", item)
                             .add(item)// not a proper photo rss, ....
                             .text(2)),
          this,
          new XmlSelector(">title", item).text(1),
          new XmlSelector(">link", item).text(1),
          new XmlSelector(">description", item).text(1),
          "http"
        ));
    }
    this.addSourceItem(sitems);
    debug.trace("processing rss (" + co.idx + "," + (co.idx + i) +
                ") got " + sitems.length + " from " + " url=" + this.url);
    co.idx += i;
    if (i < count || item == undefined) return 0;
    //debug.trace("!!PROCESSING TIME: " + (new Date().getTime() - bd));
    return 1;//need continue;
  },

  _atomItemsProcessing: function(co, count) {
    var sitems = [];
    var bd = new Date().getTime();
    for (var i = 0, item; i < count && (item = co.items[co.idx + i]); ++i) {
      if (item.nodeType != 1 || item.tagName != "entry") continue;
      sitems.push(
        new SourceItem(
          new XmlSelector(">link[rel=enclosure][type^=image][href]", item)
            .attr("href", 1) ||
            new XmlSelector(">content[type=^image][src=^http://]", item)
            .attr("src", 1) ||
            new XmlSelector("media:content[type^=image][url]", item)
            .attr("url", 1) ||
            this._htmlFilImg(new XmlSelector(">content[type=html]", item)
                             .text(1)) ||
            this._htmlFilImg(new XmlSelector(item).text(2)),
          this,
          new XmlSelector(">title", item).text(1),
          new XmlSelector(">link[rel=alternate]", item).attr("href", 1),
          new XmlSelector(">summary", item).text(1) ||
            new XmlSelector(">media:group>media:description", item).text(1)
        ));
    }
    this.addSourceItem(sitems);
    debug.trace("processing rss (" + co.idx + "," + (co.idx + i) +
                ") got " + sitems.length + " from " + " url=" + this.url);
    co.idx += i;
    if (i < count || item == undefined) return 0;
    //debug.trace("!!PROCESSING TIME: " + (new Date().getTime() - bd));
    return 1;//need continue;
  },

  _htmlNewSourceItemFromRegExpMatch: function(mr){
    var src = mr[1];
    if (src.charAt(0) == '"' || src.charAt(0) == "'")
      src = src.slice(1, -1);
    return new SourceItem(src, this);
  },

  _htmlItemsProcessing: function(co, count) {
    var sitems = [], i, mr;
    for (i = 0, mr; i < count && ((mr = co.imgRegExp.exec(co.text)) != null);
         ++i) {
      sitems.push(this._htmlNewSourceItemFromRegExpMatch(mr));
    }
    this.addSourceItem(sitems);
    if (i < count || mr == null) return 0;
    return 1;
  },

  httpProcessContent: function(req) {
    debug.trace("try to process http request content with type " +
                this.httpProcessorType);
    if (!this.httpProcessorType) {
      debug.trace("unset type, content not processed...");
      return -1;
    }
    var xmldoc;
    if (this.httpProcessorType == ":xml" ||
        this.httpProcessorType == ":rss" ||
        this.httpProcessorType == ":atom") {
      xmldoc = this.httpGetXmlDoc(req);
      if (xmldoc && xmldoc.documentElement) {
        if (this.httpProcessorType == ":xml") {
          debug.trace("plain xml, try to analysis");
          var rttag = xmldoc.documentElement.tagName;
          if (rttag == "rss") {
            this.httpProcessorType = ":rss";
          } else if (rttag == "feed") {
            this.httpProcessorType = ":atom";
          } else if (rttag == "html") {
            this.httpProcessorType = ":html";
          } else {
            debug.trace("unsupported xml format with root tag=" + rttag);
            return -1;
          }
        }
      } else {
        this.httpProcessorType = ":error";
      }
    }
    if (this.httpProcessorType == ":rss") {
      var channel = new XmlSelector(">rss>channel", xmldoc);
      if (channel.length() != 1) {
        debug.trace("not like a rss document!");
        return - 10000;
      }
      var co = {items: channel.elms[0].childNodes, idx: 0};
      this.sourceProcessing = bindFunction(this._rssItemsProcessing,
                                           this,
                                           co);
      return 1;
    }//end of type :rss
    if (this.httpProcessorType == ":atom") {
      var xselfeed = new XmlSelector(">feed", xmldoc);
      if (xselfeed.length() == 0) {
        debug.trace("not like a atom doc...");
        return -100001;
      }
      var co = {items: xselfeed.elms[0].childNodes, idx: 0};
      this.sourceProcessing = bindFunction(this._atomItemsProcessing,
                                           this,
                                           co);
      return 1;
    }// end of type :atom
    if (this.httpProcessorType == ":image") {
      //TODO: the resoponseData is ignored!
      var item = new SourceItem(this.url, this);
      this.isImage = 1;
      this.addSourceItem(item);
      return 1;
    }
    if (this.httpProcessorType == ":html") {
      debug.trace("processed as a html ...");
      var text = req.responseText;
      var co = {text: req.responseText,
                imgRegExp: new RegExp(this._htmlFilImgRegExp.source, "g")};
      this.sourceProcessing = bindFunction(this._htmlItemsProcessing,
                                           this,
                                           co);
      return 1;
    }
    if (this.httpProcessorType == ":error") {
      debug.trace("got error when fetch url=" + this.url);
    }
    return -1;
  }
};

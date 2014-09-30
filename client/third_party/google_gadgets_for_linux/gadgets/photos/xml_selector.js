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

// this class is more powerfull than needed but less than for general usage

function XmlSelector(sel, context) {
  if (context) {
    this.init(context);
    if (typeof sel == "string")
      return this.select(sel);
    else {
      debug.trace ("sel=" + sel + " not a string, ignored");
    }
  } else {
    this.init(sel);
  }
}

XmlSelector.prototype = {
  elms: [],

  init: function(nodes) {
    var elms = nodes || [];
    if (elms.nodeType) {
      elms = [elms];
    } else if (elms instanceof XmlSelector) {
      elms = elms.elms;
    } else if (elms instanceof Array) {
      //do nothing...
    } else if (elms.length) {
      var tmp = [];
      var n = elms.length;
      for (var i = 0; i < n; ++i) {
        tmp.push(elms[i]);
      }
      elms = tmp;
    }
    this.elms = elms;
  },

  length: function() {
    return this.elms.length;
  },


  funcsel: function(func, co) {
    var elms = [];
    for (var i = 0; i < this.elms.length; ++i) {
      co.idx = i;
      if (func(this.elms[i], co)) {
        elms.push(this.elms[i]);
      }
    }
    return new XmlSelector(elms);
  },

  add: function(sel, cont) {
    var tmp = new XmlSelector(sel, cont);
    tmp.elms = this.elms.concat(tmp.elms);
    return tmp;
  },

  map: function(func, co) {
    var res = [];
    for (var i = 0; i < this.elms.length; ++i) {
      co.idx = i;
      var ret = func(this.elms[i], co);
      if (ret != undefined)
        res = res.concat(ret);
    }
    return res;
  },

  merge: function(func, co) {
    return new XmlSelector(this.map(func, co));
  },

  node_text: function(node) {
    var ret = "";
    if (node.nodeType == 8) return ret;
    if (node.nodeType != 1) return node.nodeValue || ret;
    var childs = node.childNodes;
    if (childs) {
      for (var i = 0, cn; cn = childs[i]; ++i) {
        if (cn.nodeType != 8) {//not comment
          ret += cn.nodeType != 1 ?
            cn.nodeValue : this.node_text(cn);
        }
      }
    }
    return ret;
  },

  text: function(loopCheck) {
    var ret = "";
    if (this.elms.length) {
      if (loopCheck) {
        for (var i = 0; i < this.elms.length; ++i) {
          ret += this.node_text(this.elms[i]);
          if (loopCheck == 1 && ret) break;
        }
      } else
        ret = this.node_text(this.elms[0]);
    }
    return ret;
  },


  attr: function(attr, loopCheck) {//set or get attribute
    var ret = "";
    if (this.elms.length) {
      if (loopCheck) {
        for (var i = 0; i < this.elms.length; ++i) {
          if (this.elms[i].nodeType == 1)
            ret = this.elms[i].getAttribute(attr);
          if (ret) break;
        }
      } else {
        var node = this.elms[0];
        if (node.nodeType == 1)
          ret = node.getAttribute(attr);
      }
    }
    return ret;
  },

  findBlance: function(str, s, l, r) {
    var e = str.length;
    var b = 0;
    while (s < e) {
      switch (str.charAt(s)) {
      case l:
        ++b;
        break;
      case r:
        --b;
        break;
      case "\\":
        ++s;
        break;
      default:
        break;
      }
      if (b == 0) return s;
      ++s;
    }
    return -1;
  },

  tagReg: "[a-zA-Z_][a-zA-Z_:]*",
  tagRegExp: new RegExp("^(?:(?:" + "[a-zA-Z_][a-zA-Z_:]*" + ")|\\*)"),
  filRegExp: new RegExp("^:[a-zA-Z_]+"),
  attrRegExp: new RegExp("^(?:\\[(?:[^\\]\\\\]|\\\\.)*\\])"),
  mattrRegExp:
    new RegExp("^\\s*\\[([-a-zA-Z_0-9]+)(?:([$*\\^]?)(=)(.*))?\\]\\s*"),
  chsRegExp: new RegExp("^>\\s*((?:" + "[a-zA-Z_][a-zA-Z_:]*" + ")|\\*)"),
  trimRegExp: new RegExp("(?:^\\s+)|(?:\\s+$)"),

  select: function(sel) {
    sel = sel || "";
    //debug.trace("[select>] sel=" + sel + " # elms=" +this.length() );
    sel = sel.replace(this.trimRegExp, "");
    if (!sel) return this;
    var mtag = sel.match(this.tagRegExp);
    if (mtag) {
      return this.merge(
        function(node, co) {
          var elms = [];
          var nodes = (node.nodeType == 1) ?
            node.getElementsByTagName(co.sel) : [];
          // some implementation using enumerator with O(n) length function
          for (var i = 0, n; n = nodes[i]; ++i) {
            elms.push(n);
          }
          return elms;
        },
        {sel: mtag[0]}
      ).select(sel.substr(mtag[0].length));
    }
    var mfil = sel.match(this.filRegExp);
    if (mfil) {
      var ed = this.findBlance(sel, mfil[0].length, "(", ")");
      if (ed >= 0) {
        return this.matchFil(sel.slice(0, ed + 1)).select(sel.slice(ed + 2));
      }
    }
    var mattr = sel.match(this.attrRegExp);
    if (mattr) {
      return this.matchAttribute(mattr[0]).select(sel.substr(mattr[0].length));
    }
    var mch = sel.match(this.chsRegExp);
    if (mch) {
      var tag = mch[1];
      var chsel = sel.slice(mch[0].length);
      //debug.trace("match ch sel, mch=(" + mch.join(",") + "), #="
      // + this.length());
      return this.merge(
        function(node, co) {
          return new XmlSelector(node.childNodes).funcsel(
            function(node, co) {
              if (node.nodeType != 1) return 0;
              return tag == "*" ? 1 : node.tagName == tag;
            }, {})
          .elms;
        },
        {}
      ).select(chsel);
    }
    if (sel.charAt(0) == ",") {
      debug.trace("unsupported ',' now, ignored... " + sel.slice(1));
      return this; //.add(sel.slice(1));
    }
    debug.trace("not correct sel=" + sel);
    return this;//error
  },

  not: function(sel) {
    var post = this.select(sel);
    var elms = [];
    var found = 0;
    for (var i = 0; i < this.elms.length; ++i) {
      found = 0;
      for (var j = 0; j < post.length; ++j) {
        if (this.elms[i] == post[j]) {
          found = 1;break;
        }
      }
      if (!found)
        elms.push(this.elms[i]);
    }
    return new XmlSelector(elms);
  },

  _slash2RegExp: new RegExp("\\\\(\\\\|\\])", "g"),

  matchAttribute: function(sel) {
    //debug.trace("in matchAttribute, sel=" + sel + " #=" + this.length());
    var mres = sel.match(this.mattrRegExp);
    if (mres == null) {
      debug.trace("error sel in matchAttribute, sel=" + sel);
      return this;
    }
    var attr = mres[1] || "",
      type = mres[2] || "",
      haseq = mres[3] || "",
      text = mres[4] || "",
      mas;
    text.replace(this._slash2RegExp, "$1");
    if (type == "") {
      mas = "^" + text + "$";
    } else if (type == "^") {
      mas = "^" + text;
    } else if (type == "$") {
      mas = text + "$";
    }
    var rmas = new RegExp(mas);
    return this.funcsel(
      function(node, co) {
        //debug.trace("in match attr,attr=" + attr+" node type=" +
        // node.nodeType + " tag=" + node.tagName);
        if (node.nodeType != 1) return 0;
        var nattr = node.getAttribute(attr);
        if (haseq) return nattr.match(rmas);
        return nattr ? 1 : 0;
      }, {});
  },

  _filNameRegExp: new RegExp("^:[a-zA-Z_]+"),

  matchFil: function(sel) {
    var fil = sel.match(this._filNameRegExp)[0];
    var chsel = sel.slice(fil.length + 1, -1);
    //debug.trace("match fil, fil=" + fil + " chsel=" + chsel);
    var fil_map = {
      has: function(node, co) {
        return new XmlSelector(chsel, node).length();
      },

      idx: function(node, co) {
        var i = co.idx;
        var n = co.len;
        eval("var res=" + chsel);
        return res;
      },

      empty: function(node, co) {
        return node.hasChildNodes();
      },

      nonempty: function(node, co) {
        return !fil_map["empty"](node, co);
      },

      contains: function(node, co) {
        if (!node.firstChild) return 0;
        var nv = node.firstChild.nodeValue;
        return nv ? nv.indexOf(chsel) >= 0 : 0;
      },

      nthchilds: function(node, co) {
        return new XmlSelector(":idx(" + chsel + ")",
                               node.parentNode.childNodes).length();
      }
    };
    return this.funcsel(
      fil_map[fil.slice(1)] || function() {return 1;},
      {len: this.elms.length});
  }
};

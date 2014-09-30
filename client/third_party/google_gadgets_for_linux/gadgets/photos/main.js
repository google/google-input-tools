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

view.onopen = function() {
  initMain();
  kSlideShower.bindSlideShow();
  kSlideShower.slideStart();
  kSlideShower.resizedView();
};

var kSlideShower;

var kMessager;

var kOptions = {
  feedurls: [],
  slideInterval: 15,
  showers: [shower0, shower1],
  matrixShowers: [mwimg0, mwimg1, mwimg2, mwimg3, mwimg4,
      mwimg5, mwimg6, mwimg7, mwimg8],
  slideWrap: popin_wrap,
  matrixWrap: popout_wrap,
  showersWrap: showers_wrap,
  slideUI: slide_ui
};

var kBlackList = {};

function Messager(wrap, messager, time) {
  this.wrap = wrap;
  this.messager = messager;
  this.time = time || 3000;
  this.stop();
}

Messager.prototype = {
  animationToken: undefined,

  set: function(msg) {
    this.messager.innerText = msg;
    this.start();
  },

  start: function() {
    this.stop();
    this.wrap.visible = true;
    this.animationToken = beginAnimation(
      bindFunction(this.animation, this),
      100, 255, this.time);
  },

  stop: function() {
    if (this.animationToken) {
      cancelAnimation(this.animationToken);
      this.animationToken = undefined;
    }
    this.wrap.visible =  false;
  },

  animation: function() {
    if (event.value >= 253) {
      this.stop();
      return;
    }
    var opacity = 2 * event.value;
    if (opacity > 255) opacity = 511 - opacity;
    this.wrap.opacity = opacity;
  }
};

function initMain() {
  optionsPut("feedurls",
      [
      "http://images.google.com/images?q=nature&imgsz=xxlarge"
        ],
        1);
  optionsPut("slideInterval", 15, 1);
  optionsPut("readyMax", 10, 1);
  optionsPut("blackList", {}, 1);
  kOptions.feedurls = optionsGet("feedurls");
  kOptions.slideInterval = optionsGet("slideInterval") - 0;
  kBlackList = optionsGet("blackList");

  kMessager = new Messager(messagerWrap, messager);

  kSlideShower = new SlideShower(kOptions);
  kSlideShower.onSlideStop = function() {
    playpause.image = "images/play_1.png";
    playpause.overImage = "images/play_2.png";
    playpause.downimage = "images/play_3.png";
  };
  kSlideShower.onSlideStart = function() {
    playpause.image = "images/pause_1.png";
    playpause.overImage = "images/pause_2.png";
    playpause.downImage = "images/pause_3.png";
  };
  plugin.onAddCustomMenuItems = function(menu) {
    menu.AddItem(MENU_LANDSCAPE, 0, function() {
      viewRationResize(4, 3);
    });
    menu.AddItem(MENU_PORTRAIT, 0, function() {
      viewRationResize(3, 4);
    });
    menu.AddItem(
      MENU_NO_CROP,
      kSlideShower.states.scaleModel == "photo" ? 0 : gddMenuItemFlagChecked,
      function() {
        if (kSlideShower.states.scaleModel == "photo") {
          kSlideShower.states.scaleModel = "nocrop";
        } else {
          kSlideShower.states.scaleModel = "photo";
        }
        kSlideShower.resizedView();//force resize check
      }
    );
  };
}

// w, h must be integer >= 1
function viewRationResize(w, h) {
  var vw = view.width, vh = view.height;
  var dw, dh;
  dw = Math.round(Math.sqrt(vw * vh / w / h) * w);
  dh = Math.round(dw * h / w);
  if (system && system.screen) {
    var maxw = system.screen.size.width;
    var maxh = system.screen.size.height;
    if (dw > maxw || dh > maxh) {
      var to = {width: w, height: h};
      scaleToMax(to, {width: maxw, height: maxh});
      dw = to.width;
      dh = to.height;
    }
  }
  //debug.trace("resize to w=" + dw + " h=" + dh);
  view.resizeTo(dw, dh);
}

view.onoptionchanged = function() {
  kOptions.feedurls = optionsGet("feedurls");
  kOptions.slideInterval = optionsGet("slideInterval");
  kSlideShower.onOptionsChanged(kOptions);
};

view.onsize = function() {
  if (view.width < 95) view.width = 95;
  if (view.height < 50) view.height = 50;
  kSlideShower.resizedView();
  messagerWrap.width = view.width - 11;
};

view.ondock = function() {
  //debug.trace("DOCK HAPPEND...................................");
};

view.onundock = function() {
  //debug.trace("UNDOCK HAPPEND...................................");
};

view.onpopout = function() {
  kSlideShower.openMatrixView();
};

view.onpopin = function() {
  kSlideShower.closeMatrixView();
};

function onDragOver() {
  var accepts = getDragDropFeeds(event);
  if (accepts.length == 0) {
    event.returnValue = false;
    kMessager.set(MSG_DRAG_FEED_INVALID);
    return;
  }
}

function onDragDrop() {
  var accepts = getDragDropFeeds(event);
  if (accepts.length) {
    var feeds = optionsGet("feedurls");
    feeds = feeds.concat(accepts);
    optionsPut("feedurls", feeds);
  }
  kMessager.set(MSG_DRAG_FEED_ACCEPTS.replace("%d", accepts.length));
}

function getDragDropFeeds(e) {
  var uris = enumToArray(e.dragFiles).concat(enumToArray(e.dragUrls));
  var accepts = [];
  for (var i = 0; i < uris.length; ++i) {
    var feed = new FeedItem(uris[i]);
    if (feed.checkValid()) {
      accepts.push(uris[i]);
    }
  }
  return accepts;
}

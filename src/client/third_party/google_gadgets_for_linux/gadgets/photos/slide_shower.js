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

function SlideShower(kOptions) {
  for (var key in kOptions)
    this.options[key] = kOptions[key];
  this.bindSlideShow = bindFunction(this.showNext, this, 1, 0);
  this.bindOnSourceItemFetched = bindFunction(this.onSourceItemFetched, this);
  this.bindOnFeedItemFetched = bindFunction(this.onFeedItemFetched, this);
  this.bindOnAddSourceItem = bindFunction(this.onAddSourceItem, this);
  this.bindOnClearSourceList = bindFunction(this.onClearSourceList, this);
  this.replaceFeeds(kOptions.feedurls);
  this.states.checkFeedsToken = setInterval(bindFunction(this.checkFeeds, this),
                                            this.options.checkFeedsInterval);
  this.checkFeeds();//trigger immediately
}

SlideShower.prototype = {
  options: {
    checkFeedsMax: 50,//max num of check(not fetch)  feeds every interval
    checkFeedsInterval: 15 * 1000,// should be greater then the every processing time
    processingFeedsMax: 5,

    showers: [],
    matrixShowers: [],
    slideWrap: undefined,
    matrixWrap: undefined,
    showersWrap: undefined,
    slideInterval: 30,
    readyMax: 30,        //sum of below, not used, set for check
    showedBufferMax: 10, //if touch, half cut off
    fetchingMax: 10,     // max num fetching toghter
    preFetchNum: 10      // trigger fetchMore when ready less than
  },

  states: {
    shower: 0,
    animationToken: undefined,
    slideShowToken: undefined,
    checkFeedsToken: undefined,
    showingIndex: -1, // showing image position in readyImages. It's volatile
    matrixItems: [],
    scaleModel: "photo"
  },

  // array of SourceItem ready to show
  readyImages: [],
  // array of SourceItem in fetching, It's checkFetch's duty to
  // move it to readyImages
  fetchingImages: [],
  processingFeeds: [],
  feedList: [],
  sourceItemNum: 0,

  slideToggle: function() {
    if (this.states.slideShowToken) {
      this.slideStop();
      this.onSlideStop();
      return 0;
    } else {
      this.slideStart();
      this.onSlideStart();
      return 1;
    }
  },

  slideStart: function() {
    if (!this.states.slideShowToken) {
      this.states.slideShowToken =
        setInterval(this.bindSlideShow, this.options.slideInterval * 1000);
    }
  },

  slideStop: function() {
    if (this.states.slideShowToken) {
      clearInterval(this.states.slideShowToken);
      this.states.slideShowToken = undefined;
    }
  },

  // internally only
  showIndex: function(index, noEffect) {
    if (index>=0 && this.states.showingIndex >= 0 &&
        index < this.readyImages.length &&
        this.states.showingIndex < this.readyImages.length &&
        this.readyImages[index].url ==
        this.readyImages[this.states.showingIndex].url) {
      ++noEffect;
    }
    return this.showItem(this.readyImages[index], noEffect);
  },

  // internally only
  showItem: function(sourceItem, noEffect) {
    noEffect = noEffect || 0;
    var shower_up, shower_down;
    if (this.states.shower) {
      shower_up = this.options.showers[0];
      shower_down = this.options.showers[1];
    } else {
      shower_up = this.options.showers[1];
      shower_down = this.options.showers[0];
    }
    if (sourceItem.error || sourceItem.image == undefined) {
      return -1; //failed in preset errror
    }
    shower_up.src = sourceItem.image;
    if (shower_up.srcWidth == 0 || shower_up.srcHeight == 0) {
      return -2; //failed in image content
    }
    this.states.shower = !this.states.shower;
    this.resizedImage(shower_up, this.options.showersWrap);
    shower_up.onclick = bindFunction(this.detailsToggle, this, sourceItem);
    shower_up.enabled = true;
    shower_up.tooltip = sourceItem.title;
    if (!noEffect && this.options.slideInterval >= 6) {
      shower_up.opacity = 0;
      shower_down.opacity = 255;
      if (this.states.animationToken) {
        cancelAnimation(this.states.animationToken);
      }
      this.states.animationToken = beginAnimation(
        bindFunction(
          function() {
            if (event.value == 32) {
              shower_up.opacity = 255;
              shower_down.opacity = 0;
              this.states.animationToken = 0;
            } else {
              shower_up.opacity = event.value * 8;
              shower_down.opacity = 255 - event.value * 8;
            }
          }, this),
        0, 32,
        2000);
      // for the chance of the animation not touch the end value
      setTimeout(bindFunction(
                   function() {
                     if (this.states.animationToken)
                       cancelAnimation(this.states.animationToken);
                     this.states.animationToken = 0;
                   }, this), 2000 + 100);
    } else {
      shower_up.opacity = 255;
      shower_down.opacity = 0;
    }
    return 1;
  },

  showNext: function(direction, noEffect) {
    //debug.trace("SlideShower.showNext(" + direction + "," + noEffect + ")");
    this.checkFetch();
    if (this.readyImages.length == 0) {
      this.states.showingIndex = -1;
      return 0;
    }
    if (noEffect && this.states.slideShowToken) {
      this.slideStop();
      this.slideStart();
    }
    var next_index;
    if (direction > 0) {
      next_index = this.states.showingIndex + 1;
      if (next_index >= this.readyImages.length) {
        next_index %= this.readyImages.length;
      }
      if (this.showIndex(next_index, noEffect) > 0) {
        this.states.showingIndex = next_index;
      } else {
        // remove bad image from feed's sourceList and readyImages
        this.removeSourceItemInReady(next_index);
        if (noEffect < 4) //try more 2 times
          return this.showNext(direction, noEffect + 1);
      }
    } else if (direction < 0) {
      next_index = this.states.showingIndex - 1;
      if (next_index < 0) {
        //at the begin, no pre
        return 0;
      }
      this.showIndex(next_index, noEffect);
      this.states.showingIndex = next_index;
    }
  },

  checkFetch: function() {
    for (var i = 0; i < this.fetchingImages.length; ++i) {
      if (this.fetchingImages[i].error) {
        this.removeSourceItemInFetching(i);
        --i;//retry this index
      } else if (this.fetchingImages[i].image) {
        this.readyImages.push(this.fetchingImages[i]);
        this.fetchingImages.splice(i, 1);
        if (this.readyImages.length == 1) {
          this.showIndex(0, 1);
        }
        --i;
      }
    }
    if (this.readyImages.length > this.options.readyMax) {
      debug.error("SHOULD NEVER HAPPEN:fetch more then readyMax images, #="
                  + this.readyImages.length);
    }
    if (this.states.showingIndex >= this.options.showedBufferMax) {
      var cut_len = Math.floor(this.options.showedBufferMax / 2);
      var cutImages = this.readyImages.splice(0, cut_len);
      //debug.trace("cut from readyImage, #=" + cut_len);
      this.states.showingIndex -= cut_len;
      for (var i = 0; i < cut_len; ++i) {
        cutImages[i].unfetch();
      }
    }
    if (this.readyImages.length - this.states.showingIndex <
        this.options.preFetchNum) {
      this.fetchMore();
    }
    debug.trace("ready=" + this.readyImages.length +
                " fetching=" + this.fetchingImages.length +
                " source=" + this.sourceItemNum +
                " showing=" + this.states.showingIndex
               );
  },

  traceFeeds: function() {
    for (var i = 0; i < this.feedList.length; ++i) {
      var fs = 0, es = 0, is = 0;
      for (var j = 0; j < this.feedList[i].sourceList.length; ++j) {
        var sitem = this.feedList[i].sourceList[j];
        if (sitem.fetched) ++ fs;
        if (sitem.image) ++ is;
        if (sitem.error) ++ es;
      }
      debug.trace("feed source#=" + this.feedList[i].sourceList.length +
                  " fetch#=" + fs +
                  " image#=" + is +
                  " error#=" + es +
                  " url=" + this.feedList[i].url);
    }
  },

  traceSourceItemNum: function() {
    if (this.sourceItemNum < 0) {
      debug.error("SHOULD NEVER HAPPEN: sourcesItemNum=" + this.sourceItemNum);
    }
  },

  fetchMore: function() {
    if (this.sourceItemNum == 0) return 0;
    var num = this.options.fetchingMax - this.fetchingImages.length;
    for (var i = 0; i < num; ++i) {
      var rand_idx = Math.floor(Math.random() * this.sourceItemNum);
      if (rand_idx == this.sourceItemNum) -- rand_idx;
      var feed_idx;
      for (feed_idx = 0; feed_idx < this.feedList.length; ++feed_idx) {
        if (rand_idx < this.feedList[feed_idx].sourceList.length) {
          break;
        }
        rand_idx -= this.feedList[feed_idx].sourceList.length;
      }
      if (feed_idx == this.feedList.length) {
        debug.error("SHOULD NEVER HAPPEN:non source to fetch" +
                    " source#=" + this.sourceItemNum +
                    " feedList#=" + this.feedList.length
                   );
        this.traceFeeds();
      } else {
        var sourceItem = this.feedList[feed_idx].sourceList[rand_idx];
        if (sourceItem.fetched) {
          //debug.trace("ignore already fetched source.url=" + sourceItem.url);
        } else {
          // Caution: order is very important for local file
          this.fetchingImages.push(sourceItem);
          sourceItem.fetch();
        }
      }
    }
  },

  checkFeeds: function() {
    if (this.states.feedFetching) return 0;
    if (this.processingFeeds.length) {
      var p = Math.floor(Math.random() * this.processingFeeds.length);
      if (p == this.processingFeeds.length) -- p;
      debug.trace("processing p=" + p + " of #=" + this.processingFeeds.length);
      var ret = this.processingFeeds[p].sourceProcessing(50);
      if (!ret) {
        var item = this.processingFeeds.splice(p, 1)[0];
        delete item.sourceProcessing;
      }
    }
    if (this.processingFeeds.length >= this.options.processingFeedsMax) {
      return 1;
    }
    if (this.feedList.length) {
      var ncheck = Math.min(this.options.checkFeedsMax, this.feedList.length);
      var p = Math.floor(Math.random() * this.feedList.length);
      if (p == this.feedList.length) -- p;
      var utc = new Date().getTime();
      for (var i = 0; i < ncheck; ++i, ++p) {
        if (p >= this.feedList.length) p = 0;
        if (this.feedList[p].checkExpired(utc)) {
          this.states.feedFetching = 1;
          this.feedList[p].fetch();
          return 1;
        } else {
          debug.trace("unexpired feed " + this.feedList[p].url);
        }
      }
    }
    return 0;
  },

  replaceFeeds: function(newFeedUrls) {
    newFeedUrls.sort();
    var oldFeedList = this.feedList;
    this.feedList = [];
    this.processingFeeds = [];
    this.sourceItemNum = 0;
    var cp = 0;
    for (var i = 0; i < newFeedUrls.length; ++i) {
      var url = newFeedUrls[i];
      while (oldFeedList.length > cp &&
             oldFeedList[cp].url < url) {
        ++ cp;
      }
      if (cp < oldFeedList.length &&
          oldFeedList[cp].url == url) {
        this.feedList.push(oldFeedList[cp]);
        this.sourceItemNum += oldFeedList[cp].sourceList.length;
        if (oldFeedList[cp].sourceProcessing)
          this.processingFeeds.push(oldFeedList[cp]);
        ++ cp;
        debug.trace("add old feed {" + url + "}");
      } else {
        var feed = new FeedItem(url);
        if (feed.checkValid()) {
          feed.onAddSourceItem = this.bindOnAddSourceItem;
          feed.onSourceItemFetched = this.bindOnSourceItemFetched;
          feed.onFetched = this.bindOnFeedItemFetched;
          feed.onClearSourceList = this.bindOnClearSourceList;
          this.feedList.push(feed);
          debug.trace("add new feed " + url);
        } else {
          debug.trace("feed is not valid" + feed.url);
        }
      }
    }
  },

  addToBlack: function(sourceItem) {
    kBlackList[sourceItem.url] = 1;
    optionsPut("blackList", kBlackList);
    this.removeSourceItems(sourceItem);
  },

  removeSourceItemInReady: function(index) {
    --this.sourceItemNum;
    this.traceSourceItemNum();
    var item = this.readyImages[index];
    item.unfetch();
    item.feed.removeSourceItem(item);
    this.readyImages.splice(index, 1);
  },

  removeSourceItemInFetching: function(index) {
    --this.sourceItemNum;
    this.traceSourceItemNum();
    var item = this.fetchingImages[index];
    item.unfetch();
    item.feed.removeSourceItem(item);
    this.fetchingImages.splice(index, 1);
  },

  removeSourceItems: function(sourceItems) {
    var items = [].concat(sourceItems);
    this.sourceItemNum -= items.length;
    this.traceSourceItemNum();
    for (var i = 0; i < items.length; ++i) {
      var item = items[i];
      if (item.fetched) {
        for (var j = 0; j < this.readyImages.length; ++j) {
          if (this.readyImages[j].url == item.url) {
            this.readyImages.splice(j, 1);
            if (j < this.states.showingIndex) -- this.states.showingIndex;
            break;
          }
        }
        for (var j = 0; j < this.fetchingImages.length; ++j) {
          if (this.fetchingImages[j].url == item.url) {
            this.fetchingImages.splice(j, 1);
            break;
          }
        }
        for (var j = 0; j < this.states.matrixItems.length; ++j) {
          if (this.states.matrixItems[j].url == item.url) {
            this.options.matrixShowers[j].setSrcSize(0, 0);
            this.options.matrixShowers[j].visible = false;
          }
        }
      }
      item.unfetch();
      item.feed.removeSourceItem(item);
    }
    debug.trace("remove source item #=" + items.length);
  },

  onClearSourceList: function(feed, sourceList) {
    this.sourceItemNum -= sourceList.length;
    this.traceSourceItemNum();
  },

  onAddSourceItem: function(feed, item) {
    var items = [].concat(item);
    this.sourceItemNum += items.length;
    if (this.readyImages.length == 0 && this.fetchingImages.length == 0) {
      this.checkFetch();
    }
    //debug.trace("add source item #=" + items.length +
    // " sum=" + this.sourceItemNum);
  },


  onSourceItemFetched: function(item, result) {
    // only need to check when there is non image ready
    if (this.readyImages.length == 0) {
      this.checkFetch();
    }
  },

  onFeedItemFetched: function(feed, status) {
    this.states.feedFetching = 0;
    if (status > 0) {
      if (feed.sourceProcessing) {
        this.processingFeeds.push(feed);
        if (this.readyImages.length == 0 && this.fetchingImages.length == 0) {
          this.checkFeeds();
        }
      }
    } else {
      //error ignored here
    }
  },

  onOptionsChanged: function(nOptions) {
    if (event.propertyName == "feedurls" && nOptions.feedurls) {
      this.replaceFeeds(nOptions.feedurls);
    }
    if (event.propertyName == "slideInterval" &&
        nOptions.slideInterval != this.options.slideInterval &&
        this.states.slideShowToken) {
      this.options.slideInterval = nOptions.slideInterval;
      this.slideStop();
      this.slideStart();
    }
    for (var key in nOptions)
      this.options[key] = nOptions[key];
  },

  // stuff about view....
  openMatrixView: function(reOpen) {
    if (this.readyImages.length == 0) return;
    if (reOpen) {

    } else {
      this.states.matrixViewSavedPause = this.states.slideShowToken;
      this.slideStop();
      this.options.slideWrap.visible = false;
      this.options.matrixWrap.visible = true;
    }
    var start, end;
    if (this.showingIndex == -1 || this.readyImages.length <= 9) {
      start = 0;
      end = Math.min(this.readyImages.length, 9);
    } else {
      if (this.states.showingIndex <= 4) {
        start = 0;
        end = 9;
      } else {
        end = Math.min(this.states.showingIndex + 4, this.readyImages.length);
        start = end - 9;
      }
    }
    this.states.matrixItems = this.readyImages.slice(start, end);
    var mtcont = {width: view.width * .32, height: view.height * .32};
    for (var i = 0; i < 9; ++i) {
      var shower = this.options.matrixShowers[i];
      if (i < this.states.matrixItems.length) {
        shower.src = this.states.matrixItems[i].image;
        shower.enabled = true;
        shower.visible = true;
        shower.onclick = bindFunction(this.detailsToggle,
                                      this, this.states.matrixItems[i]);
        shower.tooltip = this.states.matrixItems[i].title;
        this.resizedImage(this.options.matrixShowers[i], mtcont);
      } else {
        shower.setSrcSize(0, 0);
        shower.onclick = undefined;
        shower.visible = false;
      }
    }
  },

  closeMatrixView: function() {
    if (this.states.matrixViewSavedPause)
      this.slideStart();
    this.options.slideWrap.visible = true;
    this.options.matrixWrap.visible = false;
    this.states.matrixItems = [];
  },

  detailsOnClose: function(dv, flag) {
    var detailsWidth = dv.detailsViewData.getValue("detailsWidth");
    var detailsHeight = dv.detailsViewData.getValue("detailsHeight");
    if (detailsWidth && detailsHeight) {
      optionsPut("detailsSize", {width: detailsWidth,
                                 height: detailsHeight});
    }
    if (flag == gddDetailsViewFlagRemoveButton) {
      this.addToBlack(this.states.detailsShowing);
    }
    if (flag == gddDetailsViewFlagToolbarOpen) {
      var link = (this.states.detailsShowing.type == "file" ?  "file://" : "" ) +
                   this.states.detailsShowing.link;
      framework.openUrl(link);
    }
    this.states.detailsShowing = undefined;
  },

  detailsToggle: function(sourceItem) {
    if (this.states.detailsShowing &&
        this.states.detailsShowing.url == sourceItem.url) {
      return this.detailsClose();
    }
    this.detailsView(sourceItem);
  },

  detailsClose: function() {
    this.states.detailsShowing = undefined;
    plugin.CloseDetailsView();
  },

  detailsView: function(sourceItem) {
    if (this.states.detailsShowing) {
      if (this.states.detailsShowing.url != sourceItem.url)
        this.detailsClose();
      else
        return;
    }
    var dv = new DetailsView();
    dv.SetContent(
      "",
      undefined,
      "details.xml",
      false,
      0);
    dv.detailsViewData.putValue("sourceItem", sourceItem);
    this.states.detailsShowing = sourceItem;
    plugin.ShowDetailsView(dv,
                           sourceItem.title,
                           gddDetailsViewFlagToolbarOpen |
                           gddDetailsViewFlagRemoveButton,
                           bindFunction(this.detailsOnClose, this, dv));
  },

  resizedImage: function(shower, container) {
    resizedImage(shower, container, arguments[3] || this.states.scaleModel);
  },

  resizedView: function() {
    var w = view.width, h = view.height, i;
    bkgnd.width = w - 8;
    bkgnd.height = h - 8;
    bkgnd_l.height = h - 6;
    bkgnd_r.height = h - 6;
    bkgnd_b.width = w - 6;
    bkgnd_t.width = w - 6;
    this.options.showersWrap.width = w - 10;
    this.options.showersWrap.height = h - 10;
    this.options.slideUI.y = h - 25;
    this.options.slideUI.x = (w - 95) / 2;
    for (i = 0; i < this.options.showers.length; ++i)
      this.resizedImage(this.options.showers[i], this.options.showersWrap);
    if (this.options.matrixWrap.visible) {
      var whcont = {width: view.width * .32, height: view.height * .32};
      for (i = 0; i < 9; ++i) {
        if (i < this.states.matrixItems.length) {
          this.resizedImage(this.options.matrixShowers[i], whcont);
        }
      }
    }
  }
};

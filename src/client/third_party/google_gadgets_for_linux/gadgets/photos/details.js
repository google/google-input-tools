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

function detailsOpen() {
  optionsPut("detailsSize", {}, 1);
  var osize = optionsGet("detailsSize");
  var sitem = detailsViewData.getValue("sourceItem");
  detimg.src = sitem.image;
  var imageSize = {width: detimg.srcWidth, height: detimg.srcHeight};
  var maxsize = {
    width: osize.width || 50,
    height: osize.height || 50
  };
  if (system && system.screen) {
    maxsize.width = Math.max(system.screen.size.width / 2, maxsize.width);
    maxsize.height = Math.max(system.screen.size.height / 2, maxsize.height);
  } else {
    maxsize.width = Math.max(800, maxsize.width);
    maxsize.height = Math.max(600, maxsize.height);
  }
  if (osize.width && osize.height) {
    var area = imageSize.width * imageSize.height;
    var oarea = osize.width * osize.height;
    var scale = Math.sqrt(oarea / area);
    imageSize.width *= scale;
    imageSize.height *= scale;
  }
  if (imageSize.width > maxsize.width || imageSize.height > maxsize.height) {
    scaleToMax(imageSize, maxsize);
  }
  view.onsizing = resizingDetails;
  view.resizeTo(imageSize.width, imageSize.height);
  resizedImage(detimg, view, "whole");
  view.onsize = resizedDetails;
}

function resizingDetails() {
  if (event.width < 50 || event.height < 50) {
    event.returnValue = false;
  }
}

function resizedDetails() {
  detailsViewData.putValue("detailsWidth", view.width);
  detailsViewData.putValue("detailsHeight", view.height);
  resizedImage(detimg, view, "whole");
}

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

#ifndef GGADGET_POPOUT_MAIN_VIEW_DECORATOR_H__
#define GGADGET_POPOUT_MAIN_VIEW_DECORATOR_H__

#include <ggadget/framed_view_decorator_base.h>

namespace ggadget {

/**
 * @ingroup ViewDecorator
 *
 * Decorator for popout main view.
 */
class PopOutMainViewDecorator : public FramedViewDecoratorBase {
 public:
  PopOutMainViewDecorator(ViewHostInterface *host);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(PopOutMainViewDecorator);
};

} // namespace ggadget

#endif // GGADGET_POPOUT_MAIN_VIEW_DECORATOR_H__

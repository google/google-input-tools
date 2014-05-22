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

#ifndef EXTENSIONS_MEDIAPLAYER_ELEMENT_GADGET_VIDEOSINK_H__
#define EXTENSIONS_MEDIAPLAYER_ELEMENT_GADGET_VIDEOSINK_H__

#include <gst/gst.h>
#include <gst/video/gstvideosink.h>
#include <gst/gstinfo.h>

#include <ggadget/signals.h>
#include <ggadget/slot.h>

namespace ggadget {
namespace gst {

static const char kGadgetVideoSinkElementName[] = "gadget_videosink";
static const char kGadgetVideoSinkMessageName[] =
    "gadget_videosink_element_message";

/**
 * This class implements a Gstreamer video sink plugin.
 * It should be used in following steps:
 * 1. Register the GadgetVideoSink type so that Gstreamer can find it.
 * 2. Create the corresponding video sink element using APIs provided
 *    by Gstreamer.
 * 3. Set/get properties such as PAR if necessary. Glib provides these
 *    APIs.
 * 4. Link the video sink element to upstream elements.
 * 5. After the video sink starts working, it will send a message to
 *    the message bus when any new image frame is coming. So hosts can
 *    set a watch on the message bus, and call "receive-image-handler"
 *    (a property provided by the sink) to receive the image when the
 *    watch is wakened. "receive-image-handler" is the only way to receive
 *    images. The format of image is RGB(depth=24, bpp=32, native endian).
 *    Note: The Image structure(and Image::data buffer included) returned
 *    by "receive-image-handler" is owned by the videosink, and will be
 *    recycled or destroyed after the next "receive-image-handler" call. So,
 *    only use the data between two calls, or make a copy.
 */
class GadgetVideoSink {
 public:

  /** Element message types. */
  enum MessageType {
    NEW_IMAGE,
  };

  /** Structure that contains all the information to show an image. */
  struct Image {
    const char *data;  /* Image data in RGB24 format. */
    int x, y;          /* Draw this image from position (x, y). */
    int w, h;          /* width and height of the image. */
    int stride;        /* Bytes per line (including pads). */
  };

  /**
   * This function must be called to register GadgetVideoSink before
   * gstreamer can create any element of this type.
   */
  static bool Register();

 private:
  /**
   * GadgetVideoSink elements provide the following properties.
   * The annotation at the end of each line below indicates
   * "property name : access limit : default value".
   */
  enum {
    PROP_0,                     /* Not used */
    PROP_PIXEL_ASPECT_RATIO,    /* "pixel-aspect-ratio" : Read/Write : 1/1 */
    PROP_FORCE_ASPECT_RATIO,    /* "force-aspect-ratio" : Read/Write : FALSE */
    PROP_GEOMETRY_WIDTH,        /* "geometry-width" : Write : 0 */
    PROP_GEOMETRY_HEIGHT,       /* "geometry-height" : Write : 0 */
    PROP_RECEIVE_IMAGE_HANDLER  /* "receive-image-handler" : READ : internal function */
  };

  /** Buffer type for receiving video frames from upstreams. */
  class ImageBuffer;

  /** Cycle queue used internally for images. */
  class ImageQueue;

  /** Class definition needed by gstreamer. */
  struct GadgetVideoSinkClass {
    GstVideoSinkClass parent_class;
  };

  /** It's not allowed to create instance directly, but through gstreamer. */
  GadgetVideoSink();
  ~GadgetVideoSink();

  /** Registers/returns the type of our video sink element to/from gstreamer. */
  static GType GadgetVideoSinkGetType(void);
  static gboolean InitPlugin(GstPlugin *plugin);

  /** Initializes/Finalizes objects/class. */
  static void Init(GadgetVideoSink *videosink);
  static void BaseInit(gpointer g_class);
  static void ClassInit(GadgetVideoSinkClass *klass);
  static void Finalize(GObject * object);

  /** Used to negotiate with upstream. */
  static GstCaps *GetCaps(GstBaseSink *bsink);
  static gboolean SetCaps(GstBaseSink *bsink, GstCaps *caps);

  /**
   * Called by upstream when upstream-element's state changes.
   * If upstream is ready, this function is called so that our sink
   * can be ready to start to work.
   */
  static GstStateChangeReturn ChangeState(GstElement *element,
                                          GstStateChange transition);

  /**
   * Called by upstream to set message bus on the element so that
   * our element can broadcast element message on the bus.
   */
  static void SetBus(GstElement *element, GstBus *bus);

  /** Used for synchronization between elements in a pipeline. */
  static void GetTimes(GstBaseSink *bsink,
                       GstBuffer *buf,
                       GstClockTime *start,
                       GstClockTime *end);

  /** Called by upstream to allocate a buffer before it sends image to us. */
  static GstFlowReturn BufferAlloc(GstBaseSink *bsink,
                                   guint64 offset,
                                   guint size,
                                   GstCaps *caps,
                                   GstBuffer **buf);

  /** Receive events from upstreame. */
  static gboolean Event(GstBaseSink *sink, GstEvent *event);

  /** Called by upstream to show the image frame. */
  static GstFlowReturn ShowFrame(GstBaseSink *bsink, GstBuffer *buf);

  /** Called by the host to receive an image when it gets NEW_IMAGE message. */
  static Image *ReceiveImageHandler(GstElement *element);

  /** Sets/gets property value. */
  static void SetProperty(GObject * object,
                          guint prop_id,
                          const GValue * value,
                          GParamSpec * pspec);
  static void GetProperty(GObject * object,
                          guint prop_id,
                          GValue * value,
                          GParamSpec * pspec);

  /** Initializes caps. */
  void InitCaps();

  /** Helper of @ShowFrame. */
  gboolean PutImage(ImageBuffer *image);

  /** Clearups. */
  void BufferPoolClear();
  void Reset();

 private:
  /** This videosink is based on the GstVideoSink. */
  static GstVideoSinkClass *parent_class_;

  /** Indicate whether the sink type has already been registered. */
  static bool registered_;

  /** ElementFactory information. */
  static GstStaticPadTemplate gadget_videosink_template_factory_;
  static const GstElementDetails gst_videosink_details_;

  /** Must be the first property according to glib's object system. */
  GstVideoSink videosink_;
  GstCaps *caps_;
  GstBus *bus_;

  /** Structures to manage video frames. */
  Image *image_;
  ImageQueue *image_queue_;
  GSList *buffer_pool_;

  /** Framerate numerator and denominator. */
  gint fps_n_;
  gint fps_d_;

  /** Object-set pixel aspect ratio. */
  GValue *par_;
  gboolean keep_aspect_;

  /** Width and height of the region within which video will be shown. */
  int geometry_width_;
  int geometry_height_;
};

} // namespace gst
} // namespace ggadget

#endif // EXTENSIONS_MEDIAPLAYER_ELEMENT_GADGET_VIDEOSINK_H__

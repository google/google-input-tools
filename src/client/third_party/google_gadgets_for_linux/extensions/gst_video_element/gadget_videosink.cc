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

#include "gadget_videosink.h"
#include <pthread.h>

namespace ggadget {
namespace gst {

class GadgetVideoSink::ImageBuffer {
 public:
  enum BufferRecycleFlag {
    BUFFER_NOT_RECYCLED,
    BUFFER_TO_BE_RECYCLED,
    BUFFER_RECYCLED
  };

  static GType ImageBufferGetType()
  {
    static GType image_buffer_type;

    if (G_UNLIKELY(image_buffer_type == 0)) {
      static const GTypeInfo image_buffer_info = {
        sizeof(GstBufferClass),
        NULL,
        NULL,
        ImageBufferClassInit,
        NULL,
        NULL,
        sizeof(ImageBuffer),
        0,
        0,
        NULL
      };
      image_buffer_type = g_type_register_static(GST_TYPE_BUFFER,
                                                 "ImageBuffer",
                                                 &image_buffer_info,
                                                 static_cast<GTypeFlags>(0));
    }

    return image_buffer_type;
  }

#define IS_IMAGE_BUFFER(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), ImageBuffer::ImageBufferGetType()))
#define IMAGE_BUFFER(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), ImageBuffer::ImageBufferGetType(), \
                                ImageBuffer))

  static void ImageBufferClassInit(gpointer g_class, gpointer class_data)
  {
    GGL_UNUSED(class_data);
    GstMiniObjectClass *mini_object_class = GST_MINI_OBJECT_CLASS(g_class);
    mini_object_class->finalize =
        (GstMiniObjectFinalizeFunction)Finalize;
  }

  static void Finalize(ImageBuffer *image) {
    g_return_if_fail(image != NULL);

    if (!image->videosink_) {
      GST_WARNING_OBJECT(image->videosink_, "no sink found");
      return;
    }

    // For those that are already recycled or to be recycled, just return.
    if (image->recycle_flag_ != BUFFER_NOT_RECYCLED)
      return;

    if (image->width_ != GST_VIDEO_SINK_WIDTH(image->videosink_) ||
        image->height_ != GST_VIDEO_SINK_HEIGHT(image->videosink_)) {
      // The data buffer is allocated by us, we free it ourselves.
      g_free(GST_BUFFER_DATA(image));
    } else {
      // Append to buffer pool.
      gst_buffer_ref(GST_BUFFER_CAST(image));
      image->recycle_flag_ = BUFFER_RECYCLED;
      image->videosink_->buffer_pool_ =
          g_slist_prepend(image->videosink_->buffer_pool_, image);
    }
  }

  static ImageBuffer *CreateInstance(GadgetVideoSink *videosink,
                                     GstCaps *caps) {
    ImageBuffer *image =
        IMAGE_BUFFER(gst_mini_object_new(ImageBufferGetType()));
    if (!image)
      return NULL;

    GstStructure *structure = gst_caps_get_structure(caps, 0);
    if (!gst_structure_get_int (structure, "width", &image->width_) ||
        !gst_structure_get_int (structure, "height", &image->height_)) {
      GST_WARNING("failed getting geometry from caps %" GST_PTR_FORMAT, caps);
      return NULL;
    }

    // We use 32-bpp.
    image->bytes_per_line_ = 4 * image->width_;
    image->size_ = image->bytes_per_line_ * image->height_;

    GST_BUFFER_DATA(image) = (guchar *)g_malloc(image->size_);
    if (!GST_BUFFER_DATA(image)) {
      gst_buffer_unref(GST_BUFFER_CAST(image));
      return NULL;
    }
    GST_BUFFER_SIZE(image) = static_cast<guint>(image->size_);
    image->recycle_flag_ = BUFFER_NOT_RECYCLED;

    // Keep a ref to our sink.
    image->videosink_ = videosink;
    gst_object_ref(videosink);

    return image;
  }

  static void FreeInstance(ImageBuffer *image) {
    if (image == NULL)
      return;

    // Make sure it is not recycled.
    image->width_ = -1;
    image->height_ = -1;

    if (image->videosink_) {
      gst_object_unref(image->videosink_);
      image->videosink_ = NULL;
    }

    g_free(GST_BUFFER_DATA(image));
    gst_buffer_unref(GST_BUFFER_CAST(image));
  }

  void SetRecycleFlag(BufferRecycleFlag flag) {
    recycle_flag_ = flag;
  }

  BufferRecycleFlag GetRecycleFlag() {
    return recycle_flag_;
  }

  // Must be the first non-static property.
  GstBuffer buffer_;
  GadgetVideoSink *videosink_;

  // Image's real size, width, and height.
  size_t size_;
  int width_, height_;

  // We need the following information to show an image.
  int x_, y_;
  int w_, h_;
  int bytes_per_line_; // Stride

  // The state of the buffer.
  BufferRecycleFlag recycle_flag_;

 private:
   // Cannot new/delete image buffer object explicitly.
   // Use @CreateInstance and @FreeInstance instead.
  ImageBuffer();
  ~ImageBuffer();
};

// ImageQueue is a cycle buffer which manages ImageBuffers provided
// by the host.
class GadgetVideoSink::ImageQueue {
 public:
  static const int kMaxLength = 4;

  ImageQueue() : p_(0), c_(0) {
    pthread_mutex_init(&mutex_, NULL);
    for (int i = 0; i < kMaxLength; i++)
      images_[i] = NULL;
  }

  ~ImageQueue() {
    // Maybe consumer is holding the lock.
    pthread_mutex_lock(&mutex_);
    pthread_mutex_destroy(&mutex_);
    for (int i = 0; i < kMaxLength; i++) {
      if (images_[i])
        ImageBuffer::FreeInstance(images_[i]);
    }
  }

  // Only provided to producer. It can help avoid passing in duplicated image
  // buffer pointer. Since consumer never changes the image queue, it's ok with
  // no using lock here for the sole producer.
  bool DupImage(ImageBuffer *image) {
    for (int i = 0; i < kMaxLength; i++) {
      if (image && images_[i] == image)
        return true;
    }
    return false;
  }

  // Store @a image to the queue, return one that won't be used and can
  // be recycled or destroyed by the host.
  ImageBuffer *ProduceOneImage(ImageBuffer *image) {
    ASSERT(image);

    // If the mutex is being destroyed, lock may fail.
    if (pthread_mutex_lock(&mutex_) != 0)
      return NULL;

    // If it's full, don't produce new images, just discard it.
    if ((p_ + 1) % kMaxLength == c_) {
      pthread_mutex_unlock(&mutex_);
      return image;
    }

    ImageBuffer *to_be_recycled = images_[p_];
    images_[p_] = image;
    p_ = (p_ + 1) % kMaxLength;

    pthread_mutex_unlock(&mutex_);
    return to_be_recycled;
  }

  ImageBuffer *ConsumeOneImage() {
    // If the mutex is being destroyed, lock may fail.
    if (pthread_mutex_lock(&mutex_) != 0)
      return NULL;

    // Check if it's null.
    if (p_ == c_) {
      pthread_mutex_unlock(&mutex_);
      return NULL;
    }

    ImageBuffer *cur = images_[c_];
    c_ = (c_ + 1) % kMaxLength;

    pthread_mutex_unlock(&mutex_);
    return cur;
  }

 private:
  int p_;
  int c_;
  ImageBuffer *images_[kMaxLength];
  pthread_mutex_t mutex_;
};

bool GadgetVideoSink::registered_ = false;
GstVideoSinkClass *GadgetVideoSink::parent_class_ = NULL;
GstStaticPadTemplate GadgetVideoSink::gadget_videosink_template_factory_ =
    GST_STATIC_PAD_TEMPLATE(const_cast<gchar*>("sink"),
                            GST_PAD_SINK,
                            GST_PAD_ALWAYS,
                            GST_STATIC_CAPS("video/x-raw-rgb, "
                                            "framerate = (fraction) [ 0, MAX ],"
                                            "width = (int) [ 1, MAX ], "
                                            "height = (int) [ 1, MAX ]"));
const GstElementDetails GadgetVideoSink::gst_videosink_details_ =
    GST_ELEMENT_DETAILS(const_cast<gchar*>("Video sink"),
                        const_cast<gchar*>("Sink/Video"),
                        const_cast<gchar*>("A standard X based videosink"),
                        const_cast<gchar*>("Yuxiang Luo<luoyx@google.com>"));

#define IS_GADGET_VIDEOSINK(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), GadgetVideoSinkGetType()))
#define GADGET_VIDEOSINK(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GadgetVideoSinkGetType(), \
                                GadgetVideoSink))

bool GadgetVideoSink::Register() {
  if (registered_)
    return true;
// gst_plugin_register_static() is available after gstreamter 0.10.16.
#if GST_VERSION_MAJOR > 0 || GST_VERSION_MINOR > 10 || \
    (GST_VERSION_MINOR == 10 && GST_VERSION_MICRO >= 16)
  if (!gst_plugin_register_static(GST_VERSION_MAJOR, GST_VERSION_MINOR,
                                  "gadget_videosink_plugin",
                                  const_cast<gchar *>(""),
                                  GadgetVideoSink::InitPlugin,
                                  "1.0", "unknown", "", "", ""))
    return false;
#else
  // Hacked GST_PLUGIN_DEFINE_STATIC. GST_PLUGIN_DEFINE_STATIC uses gcc
  // specific "__attribute__((constructor))" which is not portable and reliable.
  static GstPluginDesc plugin_desc = {
    GST_VERSION_MAJOR, GST_VERSION_MINOR, "gadget_videosink_plugin", "",
    GadgetVideoSink::InitPlugin, "1.0", "unknown", "", "", "",
    GST_PADDING_INIT
  };
  _gst_plugin_register_static(&plugin_desc);
#endif

  // registered_ is set in InitPlugin().
  return registered_;
}

gboolean GadgetVideoSink::InitPlugin(GstPlugin *plugin) {
  registered_ = gst_element_register(plugin, kGadgetVideoSinkElementName,
                                     GST_RANK_SECONDARY,
                                     GadgetVideoSinkGetType());
  return registered_;
}

GType GadgetVideoSink::GadgetVideoSinkGetType(void) {
  static GType videosink_type = 0;

  if (!videosink_type) {
    static const GTypeInfo videosink_info = {
      sizeof(GadgetVideoSinkClass),
      BaseInit,
      NULL,
      (GClassInitFunc)ClassInit,
      NULL,
      NULL,
      sizeof(GadgetVideoSink),
      0,
      (GInstanceInitFunc)Init,
      (GTypeValueTable*)NULL
    };

    videosink_type = g_type_register_static(GST_TYPE_VIDEO_SINK,
                                            "GadgetVideoSink",
                                            &videosink_info,
                                            static_cast<GTypeFlags>(0));

    g_type_class_ref(ImageBuffer::ImageBufferGetType());
  }

  return videosink_type;
}

void GadgetVideoSink::Init(GadgetVideoSink *videosink) {
  videosink->caps_ = NULL;
  videosink->bus_ = NULL;
  videosink->image_ = NULL;
  videosink->image_queue_ = NULL;
  videosink->buffer_pool_ = NULL;
  videosink->fps_n_ = 0;
  videosink->fps_d_ = 1;
  videosink->par_ = NULL;
  videosink->keep_aspect_ = FALSE;
}

void GadgetVideoSink::BaseInit(gpointer g_class) {
  GstElementClass *element_class = GST_ELEMENT_CLASS(g_class);
  gst_element_class_set_details(element_class, &gst_videosink_details_);
  gst_element_class_add_pad_template(
      element_class,
      gst_static_pad_template_get(&gadget_videosink_template_factory_));
}

void GadgetVideoSink::ClassInit(GadgetVideoSinkClass *klass) {
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstBaseSinkClass *gstbasesink_class;

  gobject_class = reinterpret_cast<GObjectClass*>(klass);
  gstelement_class = reinterpret_cast<GstElementClass*>(klass);
  gstbasesink_class = reinterpret_cast<GstBaseSinkClass*>(klass);

  parent_class_ =
      static_cast<GstVideoSinkClass*>(g_type_class_peek_parent(klass));

  gobject_class->finalize = Finalize;
  gobject_class->set_property = SetProperty;
  gobject_class->get_property = GetProperty;

  g_object_class_install_property(
      gobject_class, PROP_FORCE_ASPECT_RATIO,
      g_param_spec_boolean("force-aspect-ratio",
                           "Force aspect ratio",
                           "When enabled, reverse caps negotiation (scaling)"
                           "will respect original aspect ratio",
                           FALSE,
                           static_cast<GParamFlags>(G_PARAM_READWRITE)));
  g_object_class_install_property(
      gobject_class, PROP_PIXEL_ASPECT_RATIO,
      g_param_spec_string("pixel-aspect-ratio",
                          "Pixel Aspect Ratio",
                          "The pixel aspect ratio of the device",
                          "1/1",
                          static_cast<GParamFlags>(G_PARAM_READWRITE)));
  g_object_class_install_property(
      gobject_class, PROP_GEOMETRY_WIDTH,
      g_param_spec_int("geometry-width",
                       "Geometry Width",
                       "Geometry Width",
                       0,
                       G_MAXINT,
                       0,
                       static_cast<GParamFlags>(G_PARAM_WRITABLE)));
  g_object_class_install_property(
      gobject_class, PROP_GEOMETRY_HEIGHT,
      g_param_spec_int("geometry-height",
                       "Geometry Height",
                       "Geometry height",
                       0,
                       G_MAXINT,
                       0,
                       static_cast<GParamFlags>(G_PARAM_WRITABLE)));
  g_object_class_install_property(
      gobject_class, PROP_RECEIVE_IMAGE_HANDLER,
      g_param_spec_pointer("receive-image-handler",
                           "Receive Image Handler",
                           "The handler is the only way to receive images"
                           "from the sink",
                           static_cast<GParamFlags>(G_PARAM_READABLE)));

  gstelement_class->change_state = ChangeState;
  gstelement_class->set_bus = SetBus;
  gstbasesink_class->get_caps = GST_DEBUG_FUNCPTR(GetCaps);
  gstbasesink_class->set_caps = GST_DEBUG_FUNCPTR(SetCaps);
  gstbasesink_class->buffer_alloc = GST_DEBUG_FUNCPTR(BufferAlloc);
  gstbasesink_class->get_times = GST_DEBUG_FUNCPTR(GetTimes);
  gstbasesink_class->event = GST_DEBUG_FUNCPTR(Event);
  gstbasesink_class->preroll = GST_DEBUG_FUNCPTR(ShowFrame);
  gstbasesink_class->render = GST_DEBUG_FUNCPTR(ShowFrame);
}

void GadgetVideoSink::Finalize(GObject * object) {
  g_return_if_fail(object != NULL);

  // We cannot delete the object directly, as gstreamer doesn't expect us
  // to free the object pointer.

  GadgetVideoSink *videosink = GADGET_VIDEOSINK(object);
  videosink->Reset();

  G_OBJECT_CLASS(parent_class_)->finalize(object);
}

GstCaps *GadgetVideoSink::GetCaps(GstBaseSink *bsink) {
  GadgetVideoSink *videosink= GADGET_VIDEOSINK(bsink);

  if (videosink->caps_) {
    return gst_caps_ref(videosink->caps_);
  }

  // get a template copy and add the pixel aspect ratio.
  size_t i;
  GstCaps *caps;
  caps = gst_caps_copy(
      gst_pad_get_pad_template_caps(GST_BASE_SINK(videosink)->sinkpad));

  for (i = 0; i < gst_caps_get_size(caps); ++i) {
    GstStructure *structure =
        gst_caps_get_structure(caps, static_cast<guint>(i));
    if (videosink->par_) {
      int nom, den;
      nom = gst_value_get_fraction_numerator(videosink->par_);
      den = gst_value_get_fraction_denominator(videosink->par_);
      gst_structure_set(structure, "pixel-aspect-ratio",
                        GST_TYPE_FRACTION, nom, den, NULL);
    } else {
      gst_structure_set(structure, "pixel-aspect-ratio",
                        GST_TYPE_FRACTION, 1, 1, NULL);
    }
  }

  return caps;
}

gboolean GadgetVideoSink::SetCaps(GstBaseSink *bsink, GstCaps *caps) {
  // We intersect caps with our template to make sure they are correct.
  GadgetVideoSink *videosink = GADGET_VIDEOSINK(bsink);
  GstCaps *intersection = gst_caps_intersect(videosink->caps_, caps);
  GST_DEBUG_OBJECT(videosink, "intersection returned %" GST_PTR_FORMAT,
                   intersection);

  if (gst_caps_is_empty(intersection)) {
    gst_caps_unref(intersection);
    return FALSE;
  }

  gst_caps_unref(intersection);

  gboolean ret = TRUE;
  GstStructure *structure;
  gint new_width, new_height;
  const GValue *fps;
  structure = gst_caps_get_structure(caps, 0);
  ret &= gst_structure_get_int(structure, "width", &new_width);
  ret &= gst_structure_get_int(structure, "height", &new_height);
  fps = gst_structure_get_value(structure, "framerate");
  ret &= (fps != NULL);
  if (!ret) {
    return FALSE;
  }

  // If the caps contain pixel-aspect-ratio, they have to match ours, otherwise
  // linking should fail.
  const GValue *par = gst_structure_get_value(structure,
                                              "pixel-aspect-ratio");
  if (par) {
    if (videosink->par_) {
      if (gst_value_compare(par, videosink->par_) != GST_VALUE_EQUAL) {
        goto wrong_aspect;
      }
    } else {
      // Try the default.
      int nom, den;
      nom = gst_value_get_fraction_numerator(par);
      den = gst_value_get_fraction_denominator(par);
      if (nom != 1 || den != 1) {
        goto wrong_aspect;
      }
    }
  }

  GST_VIDEO_SINK_WIDTH(videosink) = new_width;
  GST_VIDEO_SINK_HEIGHT(videosink) = new_height;
  videosink->fps_n_ = gst_value_get_fraction_numerator(fps);
  videosink->fps_d_ = gst_value_get_fraction_denominator(fps);

  if (GST_VIDEO_SINK_WIDTH(videosink) <= 0 ||
      GST_VIDEO_SINK_HEIGHT(videosink) <= 0) {
    return FALSE;
  }

  return TRUE;

  // ERRORS
wrong_aspect:
  {
    GST_INFO_OBJECT(videosink, "pixel aspect ratio does not match");
    return FALSE;
  }
}

GstStateChangeReturn GadgetVideoSink::ChangeState(GstElement *element,
                                                  GstStateChange transition) {
  GadgetVideoSink *videosink = GADGET_VIDEOSINK(element);
  GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      videosink->InitCaps();
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      videosink->image_ = new Image;
      videosink->image_queue_ = new ImageQueue;
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS(parent_class_)->change_state(element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      videosink->fps_n_ = 0;
      videosink->fps_d_ = 1;
      GST_VIDEO_SINK_WIDTH(videosink) = 0;
      GST_VIDEO_SINK_HEIGHT(videosink) = 0;
      delete videosink->image_;
      delete videosink->image_queue_;
      videosink->image_ = NULL;
      videosink->image_queue_ = NULL;
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      videosink->Reset();
      break;
    default:
      break;
  }

  return ret;
}

void GadgetVideoSink::SetBus(GstElement *element, GstBus *bus) {
  GadgetVideoSink *videosink = GADGET_VIDEOSINK(element);
  videosink->bus_ = bus;
}

void GadgetVideoSink::GetTimes(GstBaseSink * bsink, GstBuffer * buf,
                               GstClockTime * start, GstClockTime * end) {
  GadgetVideoSink *videosink = GADGET_VIDEOSINK(bsink);

  if (GST_BUFFER_TIMESTAMP_IS_VALID(buf)) {
    *start = GST_BUFFER_TIMESTAMP(buf);
    if (GST_BUFFER_DURATION_IS_VALID(buf)) {
      *end = *start + GST_BUFFER_DURATION(buf);
    } else {
      if (videosink->fps_n_ > 0) {
        *end = *start + gst_util_uint64_scale_int(GST_SECOND,
                                                  videosink->fps_d_,
                                                  videosink->fps_n_);
      }
    }
  }
}

// Buffer management
//
// The buffer_alloc function must either return a buffer with given size and
// caps or create a buffer with different caps attached to the buffer. This
// last option is called reverse negotiation, ie, where the sink suggests a
// different format from the upstream peer.
//
// We try to do reverse negotiation when our geometry changes and we like a
// resized buffer.
GstFlowReturn GadgetVideoSink::BufferAlloc(GstBaseSink * bsink,
                                           guint64 offset,
                                           guint size,
                                           GstCaps * caps,
                                           GstBuffer ** buf) {
  ImageBuffer *image = NULL;
  GstStructure *structure = NULL;
  GstFlowReturn ret = GST_FLOW_OK;
  gint width = 0, height = 0;
  GadgetVideoSink *videosink = GADGET_VIDEOSINK(bsink);

  GST_LOG_OBJECT(videosink,
                 "a buffer of %d bytes was requested with caps %"
                 GST_PTR_FORMAT " and offset %" G_GUINT64_FORMAT,
                 size, caps, offset);

  // assume we're going to alloc what was requested, keep track of wheter
  // we need to unref or not. When we suggest a new format upstream we will
  // create a new caps that we need to unref.
  GstCaps *alloc_caps = caps;
  bool alloc_unref = false;

  // get struct to see what is requested.
  structure = gst_caps_get_structure(caps, 0);

  if (gst_structure_get_int(structure, "width", &width) &&
      gst_structure_get_int(structure, "height", &height)) {
    GstVideoRectangle dst, src, result;

    src.w = width;
    src.h = height;

    // What is our geometry.
    dst.w = videosink->geometry_width_;
    dst.h = videosink->geometry_height_;

    if (videosink->keep_aspect_) {
      GST_LOG_OBJECT(videosink,
                     "enforcing aspect ratio in reverse caps negotiation");
      src.x = src.y = dst.x = dst.y = 0;
      gst_video_sink_center_rect(src, dst, &result, TRUE);
    } else {
      GST_LOG_OBJECT(videosink, "trying to resize to window geometry "
                     "ignoring aspect ratio");
      result.x = result.y = 0;
      result.w = dst.w;
      result.h = dst.h;
    }

    // We would like another geometry.
    if (width != result.w || height != result.h) {
      int nom, den;
      GstCaps *desired_caps;
      GstStructure *desired_struct;

      // Make a copy of the incomming caps to create the new suggestion.
      // We can't use make_writable because we might then destroy the original
      // caps which we still need when the peer does not accept the suggestion.
      desired_caps = gst_caps_copy(caps);
      desired_struct = gst_caps_get_structure(desired_caps, 0);

      gst_structure_set (desired_struct, "width", G_TYPE_INT, result.w, NULL);
      gst_structure_set (desired_struct, "height", G_TYPE_INT, result.h, NULL);

      // PAR property overrides the default one.
      if (videosink->par_) {
        nom = gst_value_get_fraction_numerator(videosink->par_);
        den = gst_value_get_fraction_denominator(videosink->par_);
        gst_structure_set(desired_struct, "pixel-aspect-ratio",
                           GST_TYPE_FRACTION, nom, den, NULL);
      } else {
        gst_structure_set(desired_struct, "pixel-aspect-ratio",
                          GST_TYPE_FRACTION, 1, 1, NULL);
      }

      // see if peer accepts our new suggestion, if there is no peer, this
      // function returns true.
      if (gst_pad_peer_accept_caps(GST_VIDEO_SINK_PAD (videosink),
                                   desired_caps)) {
        gint bpp;
        bpp = size / height / width;

        // we will not alloc a buffer of the new suggested caps. Make sure
        // we also unref this new caps after we set it on the buffer.
        alloc_caps = desired_caps;
        alloc_unref = true;
        width = result.w;
        height = result.h;
        size = bpp * width * height;
        GST_DEBUG ("peed pad accepts our desired caps %" GST_PTR_FORMAT
            " buffer size is now %d bytes", desired_caps, size);
      } else {
        GST_DEBUG("peer pad does not accept our desired caps %" GST_PTR_FORMAT,
                  desired_caps);
        // we alloc a buffer with the original incomming caps.
        width = GST_VIDEO_SINK_WIDTH(videosink);
        height = GST_VIDEO_SINK_HEIGHT(videosink);
      }
    }
  }

  // Check whether we can reuse any buffer from our buffer pool.
  while (videosink->buffer_pool_) {
    image = static_cast<ImageBuffer*>(videosink->buffer_pool_->data);
    if (image) {
      // Removing from the pool.
      videosink->buffer_pool_ = g_slist_delete_link(videosink->buffer_pool_,
                                                    videosink->buffer_pool_);
      // If the image is invalid for our need, destroy.
      if ((image->width_ != width) || (image->height_ != height)) {
        ImageBuffer::FreeInstance(image);
        image = NULL;
      } else {
        // We found a suitable image. Reset the recycle flag.
        ASSERT(image->GetRecycleFlag() == ImageBuffer::BUFFER_RECYCLED);
        image->SetRecycleFlag(ImageBuffer::BUFFER_NOT_RECYCLED);
        break;
      }
    } else {
      break;
    }
  }

  // We haven't found anything, creating a new one.
  if (!image) {
    image = ImageBuffer::CreateInstance(videosink, alloc_caps);
  }

  // Now we should have an image, set appropriate caps on it.
  g_return_val_if_fail(image != NULL, GST_FLOW_ERROR);
  gst_buffer_set_caps(GST_BUFFER_CAST(image), alloc_caps);

  // Could be our new reffed suggestion or the original unreffed caps.
  if (alloc_unref)
    gst_caps_unref(alloc_caps);

  *buf = GST_BUFFER_CAST(image);

  return ret;
}

gboolean GadgetVideoSink::Event(GstBaseSink *sink, GstEvent *event) {
  // FIXME:
  // The default event handler would post an EOS message after it receives
  // the EOS event. But it seems not for our gadget video sink. So we post
  // the EOS message manually here.
  if (GST_EVENT_TYPE(event) == GST_EVENT_EOS) {
    GadgetVideoSink *videosink = GADGET_VIDEOSINK(sink);
    GstMessage *eos = gst_message_new_eos(reinterpret_cast<GstObject*>(sink));
    if (eos)
      gst_bus_post(videosink->bus_, eos);
  }
  return TRUE;
}

GstFlowReturn GadgetVideoSink::ShowFrame(GstBaseSink *bsink, GstBuffer *buf) {
  g_return_val_if_fail(buf != NULL, GST_FLOW_ERROR);
  GadgetVideoSink *videosink = GADGET_VIDEOSINK(bsink);

  if (IS_IMAGE_BUFFER(buf)) {
    GST_LOG_OBJECT(videosink, "buffer from our pool, writing directly");
    videosink->PutImage(IMAGE_BUFFER(buf));
  } else {
    // Else we have to copy the data into our image buffer.
    GST_LOG_OBJECT(videosink, "normal buffer, copying from it");
    GST_DEBUG_OBJECT(videosink, "creating our image");
    ImageBuffer *image_buf =
      ImageBuffer::CreateInstance(videosink, GST_BUFFER_CAPS(buf));
    if (!image_buf)
      goto no_image;

    if (image_buf->size_ < GST_BUFFER_SIZE(buf)) {
      ImageBuffer::FreeInstance(image_buf);
      goto no_image;
    }
    memcpy(GST_BUFFER_DATA(image_buf), GST_BUFFER_DATA(buf),
           MIN(GST_BUFFER_SIZE(buf), image_buf->size_));

    videosink->PutImage(image_buf);

    ImageBuffer::Finalize(image_buf);
    gst_buffer_unref(GST_BUFFER_CAST(image_buf));
  }

  return GST_FLOW_OK;

no_image:
  // No image available.
  GST_DEBUG("could not create image");
  return GST_FLOW_ERROR;
}

void GadgetVideoSink::SetProperty(GObject *object,
                                  guint prop_id,
                                  const GValue *value,
                                  GParamSpec *pspec) {
  g_return_if_fail(IS_GADGET_VIDEOSINK(object));
  GadgetVideoSink *videosink = GADGET_VIDEOSINK(object);

  switch (prop_id) {
    case PROP_FORCE_ASPECT_RATIO:
      videosink->keep_aspect_ = g_value_get_boolean(value);
      break;
    case PROP_PIXEL_ASPECT_RATIO:
    {
      GValue *tmp;

      tmp = g_new0(GValue, 1);
      g_value_init(tmp, GST_TYPE_FRACTION);

      if (!g_value_transform(value, tmp)) {
        GST_WARNING_OBJECT(videosink,
                           "Could not transform string to aspect ratio");
        g_free(tmp);
      } else {
        GST_DEBUG_OBJECT(videosink, "set PAR to %d/%d",
                         gst_value_get_fraction_numerator(tmp),
                         gst_value_get_fraction_denominator(tmp));
        g_free(videosink->par_);
        videosink->par_ = tmp;
      }
    }
      break;
    case PROP_GEOMETRY_WIDTH:
      videosink->geometry_width_ = g_value_get_int(value);
      break;
    case PROP_GEOMETRY_HEIGHT:
      videosink->geometry_height_ = g_value_get_int(value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
  }
}

void GadgetVideoSink::GetProperty(GObject * object,
                                  guint prop_id,
                                  GValue * value,
                                  GParamSpec * pspec) {
  g_return_if_fail(IS_GADGET_VIDEOSINK (object));
  GadgetVideoSink *videosink = GADGET_VIDEOSINK(object);

  switch (prop_id) {
    case PROP_FORCE_ASPECT_RATIO:
      g_value_set_boolean(value, videosink->keep_aspect_);
      break;
    case PROP_PIXEL_ASPECT_RATIO:
      if (videosink->par_)
        g_value_transform(videosink->par_, value);
      break;
    case PROP_RECEIVE_IMAGE_HANDLER:
      g_value_set_pointer(value, (void*)ReceiveImageHandler);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
  }
}

// This function initializes caps for the only supported format.
void GadgetVideoSink::InitCaps() {
  ASSERT(!caps_);
  caps_ = gst_caps_new_simple("video/x-raw-rgb",
                              "bpp", G_TYPE_INT, 32,
                              "depth", G_TYPE_INT, 24,
                              "endianness", G_TYPE_INT, G_BIG_ENDIAN,
                              "red_mask", G_TYPE_INT, 0xff00,
                              "green_mask", G_TYPE_INT, 0xff0000,
                              "blue_mask", G_TYPE_INT,0xff000000,
                              "width", GST_TYPE_INT_RANGE, 1, G_MAXINT,
                              "height", GST_TYPE_INT_RANGE, 1, G_MAXINT,
                              "framerate", GST_TYPE_FRACTION_RANGE, 0, 1, G_MAXINT, 1, NULL);

  // Update par with the default one if not set yet.
  if (!par_) {
    par_ = g_new0(GValue, 1);
    g_value_init(par_, GST_TYPE_FRACTION);
    gst_value_set_fraction(par_, 1, 1);  // 1:1
  }

  int nom, den;
  nom = gst_value_get_fraction_numerator(par_);
  den = gst_value_get_fraction_denominator(par_);
  gst_caps_set_simple(caps_, const_cast<gchar*>("pixel-aspect-ratio"),
                      GST_TYPE_FRACTION, 1, 1, NULL);
}

// This function converts the image format if necessary, puts the image into
// the image queue, sends message to notify that a new image frame is coming,
// and recycles any reusable image buffer.
gboolean GadgetVideoSink::PutImage(ImageBuffer *image) {
  if (!image)
    return TRUE;

  // The upstream may pass in the same image buffer twice. For example, before
  // upstream finalizes an image buffer, a seeking operation occurs, and under
  // some condition, the upstream may reuse the image buffer instead of
  // finalizing and freeing it. Since we may store image buffers in
  // image queue or buffer pool during putting images, the image passed in
  // this time may already be stored in image queue or buffer pool during
  // the last call of this function. So, first check whether the buffer is
  // duplicated, and simply discuss it and return if it's.
  if (g_slist_find(buffer_pool_, image) || image_queue_->DupImage(image)) {
    return TRUE;
  }

  GstVideoRectangle dst, src, result;
  src.w = image->width_;
  src.h = image->height_;
  dst.w = geometry_width_;
  dst.h = geometry_height_;
  src.x = src.y = dst.x = dst.y = 0;
  gst_video_sink_center_rect(src, dst, &result, FALSE);
  image->x_ = result.x;
  image->y_ = result.y;
  image->w_ = result.w;
  image->h_ = result.h;

  // Increase refcount and set TO_BE_RECYCLED flag so that image buffer won't
  // be finalized/freed by upstream.
  gst_buffer_ref(GST_BUFFER_CAST(image));
  image->SetRecycleFlag(ImageBuffer::BUFFER_TO_BE_RECYCLED);

  // Put it to the buffer queue.
  ImageBuffer *to_be_recycled = image_queue_->ProduceOneImage(image);

  // Send a message to notify that a new frame is coming.
  if (bus_) {
    GstStructure *structure =
        gst_structure_new("New Image", kGadgetVideoSinkMessageName,
                          G_TYPE_INT, NEW_IMAGE, NULL);
    GstMessage *message =
        gst_message_new_element(reinterpret_cast<GstObject*>(this), structure);
    if (message)
      gst_bus_post(bus_, message);
  }

  if (to_be_recycled) {
    if (to_be_recycled->width_ != GST_VIDEO_SINK_WIDTH(this) ||
        to_be_recycled->height_ != GST_VIDEO_SINK_HEIGHT(this)) {
      ImageBuffer::FreeInstance(to_be_recycled);
    } else {
      to_be_recycled->SetRecycleFlag(ImageBuffer::BUFFER_RECYCLED);
      buffer_pool_ = g_slist_prepend(buffer_pool_, to_be_recycled);
    }
  }

  return TRUE;
}

void GadgetVideoSink::BufferPoolClear() {
  while (buffer_pool_) {
    ImageBuffer *image = static_cast<ImageBuffer*>(buffer_pool_->data);
    buffer_pool_ = g_slist_delete_link(buffer_pool_, buffer_pool_);
    ImageBuffer::FreeInstance(image);
  }
}

void GadgetVideoSink::Reset() {
  if (caps_) {
    gst_caps_unref(caps_);
    caps_ = NULL;
  }
  if (image_) {
    delete image_;
    image_ = NULL;
  }
  if (image_queue_) {
    delete image_queue_;
    image_queue_ = NULL;
  }

  BufferPoolClear();

  if(par_) {
    g_free(par_);
    par_ = NULL;
  }
}

GadgetVideoSink::Image *
GadgetVideoSink::ReceiveImageHandler(GstElement *element) {
  ASSERT(element);
  GadgetVideoSink *videosink = GADGET_VIDEOSINK(element);
  if (videosink->image_queue_) {
    ImageBuffer *image_internal = videosink->image_queue_->ConsumeOneImage();
    if (image_internal != NULL) {
      ASSERT(videosink->image_);
      videosink->image_->data =
          reinterpret_cast<char*>(GST_BUFFER_DATA(image_internal));
      videosink->image_->x = image_internal->x_;
      videosink->image_->y = image_internal->y_;
      videosink->image_->w = image_internal->w_;
      videosink->image_->h = image_internal->h_;
      videosink->image_->stride = image_internal->bytes_per_line_;
      return videosink->image_;
    }
  }
  return NULL;
}

} // namespace gst
} // namespace ggadget

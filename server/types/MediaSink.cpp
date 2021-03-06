/*
 * (C) Copyright 2013 Kurento (http://kurento.org/)
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the GNU Lesser General Public License
 * (LGPL) version 2.1 which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/lgpl-2.1.html
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 */

#include "MediaSink.hpp"
#include "MediaElement.hpp"

#define GST_CAT_DEFAULT kurento_media_sink
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);
#define GST_DEFAULT_NAME "KurentoMediaSink"

namespace kurento
{

MediaSink::MediaSink (std::shared_ptr<MediaElement> parent, KmsMediaType::type mediaType)
  : MediaPad (parent, KmsMediaPadDirection::SINK, mediaType)
{

}

MediaSink::~MediaSink() throw ()
{
  std::shared_ptr<MediaSrc> connectedSrcLocked;

  try {
    connectedSrcLocked = connectedSrc.lock();
  } catch (const std::bad_weak_ptr &e) {
  }

  if (connectedSrcLocked != NULL) {
    connectedSrcLocked->disconnect (this);
  }
}

std::string
MediaSink::getPadName ()
{
  if (mediaType == KmsMediaType::type::AUDIO)
    return "audio_sink";
  else
    return "video_sink";
}

static void
remove_from_parent (GstElement *element)
{
  GstBin *parent = GST_BIN (GST_OBJECT_PARENT (element) );

  if (parent == NULL)
    return;

  gst_object_ref (element);
  gst_bin_remove (parent, element);
  gst_element_set_state (element, GST_STATE_NULL);

  gst_object_unref (element);
}

static void
sink_unlinked (GstPad *pad, GstPad *peer, GstElement *filter)
{
  GstPad *src;
  GstPad *src_peer;

  src = gst_element_get_static_pad (filter, "src");
  src_peer = gst_pad_get_peer (src);

  if (src_peer != NULL) {
    gst_pad_unlink (src, src_peer);
    gst_object_unref (src_peer);
  } else {
    remove_from_parent (filter);
  }

  gst_object_unref (src);
}

static void
src_unlinked (GstPad *pad, GstPad *peer, GstElement *filter)
{
  GstPad *sink;
  GstPad *sink_peer;

  sink = gst_element_get_static_pad (filter, "sink");
  sink_peer = gst_pad_get_peer (sink);

  if (sink_peer != NULL) {
    gst_pad_unlink (sink_peer, sink);
    gst_object_unref (sink_peer);
  } else {
    remove_from_parent (filter);
  }

  gst_object_unref (sink);
}

bool
MediaSink::linkPad (std::shared_ptr<MediaSrc> mediaSrc, GstPad *src)
{
  std::shared_ptr<MediaSrc> connectedSrcLocked;
  GstPad *sink;
  bool ret = false;

  mutex.lock();

  try {
    connectedSrcLocked = connectedSrc.lock();
  } catch (const std::bad_weak_ptr &e) {
  }

  if ( (sink = gst_element_get_static_pad (getElement(), getPadName().c_str() ) ) == NULL)
    sink = gst_element_get_request_pad (getElement(), getPadName().c_str() );

  if (gst_pad_is_linked (sink) ) {
    unlink (connectedSrcLocked, sink);
  }

  if (mediaSrc->parent == parent) {
    GstBin *container;
    GstElement *filter, *parent;
    GstPad *aux_sink, *aux_src;

    GST_DEBUG ("Connecting loopback, adding a capsfilter to allow connection");
    parent = GST_ELEMENT (GST_OBJECT_PARENT (sink) );

    if (parent == NULL)
      goto end;

    container = GST_BIN (GST_OBJECT_PARENT (parent) );

    if (container == NULL)
      goto end;

    filter = gst_element_factory_make ("capsfilter", NULL);

    aux_sink = gst_element_get_static_pad (filter, "sink");
    aux_src = gst_element_get_static_pad (filter, "src");

    g_signal_connect (G_OBJECT (aux_sink), "unlinked", G_CALLBACK (sink_unlinked), filter );
    g_signal_connect (G_OBJECT (aux_src), "unlinked", G_CALLBACK (src_unlinked), filter );

    gst_bin_add (container, filter);
    gst_element_sync_state_with_parent (filter);

    if (gst_pad_link (aux_src, sink) == GST_PAD_LINK_OK) {
      if (gst_pad_link (src, aux_sink) == GST_PAD_LINK_OK)
        ret = true;
      else
        gst_pad_unlink (aux_src, sink);

    }

    g_object_unref (aux_sink);
    g_object_unref (aux_src);

    gst_debug_bin_to_dot_file_with_ts (GST_BIN (container), GST_DEBUG_GRAPH_SHOW_ALL, "loopback");

  } else {
    if (gst_pad_link (src, sink) == GST_PAD_LINK_OK)
      ret = true;
  }

  if (ret == true) {
    connectedSrc = std::weak_ptr<MediaSrc> (mediaSrc);
  } else {
    gst_element_release_request_pad (getElement(), sink);
  }

end:

  g_object_unref (sink);

  mutex.unlock();

  return ret;
}

void
MediaSink::unlink (std::shared_ptr<MediaSrc> mediaSrc, GstPad *sink)
{
  std::shared_ptr<MediaSrc> connectedSrcLocked;

  mutex.lock();

  try {
    connectedSrcLocked = connectedSrc.lock();
  } catch (const std::bad_weak_ptr &e) {
  }

  if (connectedSrcLocked != NULL && mediaSrc == connectedSrcLocked) {
    unlinkUnchecked (sink);
    connectedSrcLocked->removeSink (this);
  }

  mutex.unlock();
}

void
MediaSink::unlinkUnchecked (GstPad *sink)
{
  GstPad *peer;
  GstPad *sinkPad;

  if (sink == NULL)
    sinkPad = gst_element_get_static_pad (getElement(), getPadName().c_str() );
  else
    sinkPad = sink;

  if (sinkPad == NULL)
    return;

  peer = gst_pad_get_peer (sinkPad);

  if (peer != NULL) {
    gst_pad_unlink (peer, sinkPad);

    g_object_unref (peer);
  }

  if (sink == NULL) {
    GstElement *elem;

    elem = gst_pad_get_parent_element (sinkPad);
    gst_element_release_request_pad (elem, sinkPad);
    g_object_unref (elem);
    g_object_unref (sinkPad);
  }
}

std::shared_ptr<MediaSrc>
MediaSink::getConnectedSrc ()
{
  std::shared_ptr<MediaSrc> connectedSrcLocked;

  try {
    connectedSrcLocked = connectedSrc.lock();
  } catch (const std::bad_weak_ptr &e) {
  }

  return connectedSrcLocked;
}

MediaSink::StaticConstructor MediaSink::staticConstructor;

MediaSink::StaticConstructor::StaticConstructor()
{
  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, GST_DEFAULT_NAME, 0,
                           GST_DEFAULT_NAME);
}

} // kurento

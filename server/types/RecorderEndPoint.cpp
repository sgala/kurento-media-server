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

#include "RecorderEndPoint.hpp"

#include "KmsMediaUriEndPointType_constants.h"
#include "KmsMediaRecorderEndPointType_constants.h"
#include "KmsMediaDataType_constants.h"
#include "KmsMediaErrorCodes_constants.h"

#include "utils/utils.hpp"
#include "utils/marshalling.hpp"

#define GST_CAT_DEFAULT kurento_recorder_end_point
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);
#define GST_DEFAULT_NAME "KurentoRecorderEndPoint"

namespace kurento
{

void
RecorderEndPoint::init (std::shared_ptr<MediaPipeline> parent,
                        const std::string &uri,
                        KmsMediaProfile profile)
{
  element = gst_element_factory_make ("recorderendpoint", NULL);

  g_object_set (G_OBJECT (element), "uri", uri.c_str(), NULL);

  switch (profile.mediaMuxer) {
  case KmsMediaMuxer::WEBM:
    //value 0 means KMS_RECORDING_PROFILE_WEBM
    g_object_set ( G_OBJECT (element), "profile", 0, NULL);
    GST_INFO ("Set WEBM profile");
    break;

  case KmsMediaMuxer::MP4:
    //value 1 means KMS_RECORDING_PROFILE_MP4
    g_object_set ( G_OBJECT (element), "profile", 1, NULL);
    GST_INFO ("Set MP4 profile");
    break;
  }

  g_object_ref (element);
  gst_bin_add (GST_BIN (parent->pipeline), element);
  gst_element_sync_state_with_parent (element);
}

RecorderEndPoint::RecorderEndPoint (MediaSet &mediaSet,
                                    std::shared_ptr<MediaPipeline> parent,
                                    const std::map<std::string, KmsMediaParam> &params)
throw (KmsMediaServerException)
  : UriEndPoint (mediaSet, parent,
                 g_KmsMediaRecorderEndPointType_constants.TYPE_NAME, params)
{
  const KmsMediaParam *p;
  KmsMediaUriEndPointConstructorParams uriEpParams;
  KmsMediaRecoderEndPointConstructorParams recorderParams;
  KmsMediaProfile profile;

  p = getParam (params, g_KmsMediaUriEndPointType_constants.CONSTRUCTOR_PARAMS_DATA_TYPE);

  if (p == NULL) {
    KmsMediaServerException except;

    createKmsMediaServerException (except, g_KmsMediaErrorCodes_constants.MEDIA_OBJECT_ILLEGAL_PARAM_ERROR,
                                   "Param '" + g_KmsMediaUriEndPointType_constants.CONSTRUCTOR_PARAMS_DATA_TYPE + "' not found");
    throw except;
  }

  unmarshalKmsMediaUriEndPointConstructorParams (uriEpParams, p->data);
  //unmarshal KmsMediaRecoderEndPointConstructorParams
  p = getParam (params, g_KmsMediaRecorderEndPointType_constants.CONSTRUCTOR_PARAMS_DATA_TYPE);
  profile.mediaMuxer = KmsMediaMuxer::WEBM;

  if (p != NULL) {
    unmarshalStruct (recorderParams, p->data);

    if (recorderParams.__isset.profileType) {
      profile = recorderParams.profileType;
    }
  }

  init (parent, uriEpParams.uri, profile);
}

RecorderEndPoint::~RecorderEndPoint() throw ()
{
  gst_bin_remove (GST_BIN ( ( (std::shared_ptr<MediaPipeline> &) parent)->pipeline), element);
  gst_element_set_state (element, GST_STATE_NULL);
  g_object_unref (element);
}

RecorderEndPoint::StaticConstructor RecorderEndPoint::staticConstructor;

RecorderEndPoint::StaticConstructor::StaticConstructor()
{
  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, GST_DEFAULT_NAME, 0,
                           GST_DEFAULT_NAME);
}

} // kurento

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

#include "server_test_base.hpp"
#include <boost/test/unit_test.hpp>

#include "memory.hpp"
#include "mediaServer_constants.h"

#include <gst/gst.h>

using namespace kurento;

using namespace kurento;

#define GST_CAT_DEFAULT _server_memory_test_
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);
#define GST_DEFAULT_NAME "server_memory_test"

#define ITERATIONS 10000
#define MEMORY_TOLERANCE 1024

BOOST_FIXTURE_TEST_SUITE ( server_memory_test_suite, F )

BOOST_AUTO_TEST_CASE ( create_media_pipeline_memory_test )
{
  MediaObjectId mediaPipeline = MediaObjectId();
  MediaObjectId mo = MediaObjectId();
  int i, maxMemorySize, currentMemorySize;

  BOOST_REQUIRE_MESSAGE (initialized, "Cannot connect to the server");
  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, GST_DEFAULT_NAME, 0, GST_DEFAULT_NAME);

  client->addHandlerAddress (0, "localhost", 2323);

  for (i = 0; i < ITERATIONS; i++) {
    client->createMediaPipeline (mediaPipeline, 0);
    client->release (mediaPipeline);

    if (i == 0) {
      maxMemorySize = get_data_memory (pid) + MEMORY_TOLERANCE;
      GST_INFO ("MAX memory size: %d", maxMemorySize);
    }

    if (i % 100 == 0) {
      currentMemorySize = get_data_memory (pid);
      GST_INFO ("Memory size: %d", currentMemorySize);
      BOOST_REQUIRE (currentMemorySize <= maxMemorySize);
    }
  }
}

BOOST_AUTO_TEST_CASE ( create_player_memory_test )
{
  MediaObjectId mediaPipeline = MediaObjectId();
  MediaObjectId mo = MediaObjectId();
  int i, maxMemorySize, currentMemorySize;

  BOOST_REQUIRE_MESSAGE (initialized, "Cannot connect to the server");
  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, GST_DEFAULT_NAME, 0, GST_DEFAULT_NAME);

  client->addHandlerAddress (0, "localhost", 2323);

  for (i = 0; i < ITERATIONS; i++) {
    client->createMediaPipeline (mediaPipeline, 0);
    client->createUriEndPoint (mo, mediaPipeline, UriEndPointType::type::PLAYER_END_POINT, "file:///tmp/a");
    client->release (mediaPipeline);

    if (i == 0) {
      maxMemorySize = get_data_memory (pid) + MEMORY_TOLERANCE;
      GST_INFO ("MAX memory size: %d", maxMemorySize);
    }

    if (i % 100 == 0) {
      currentMemorySize = get_data_memory (pid);
      GST_INFO ("Memory size: %d", currentMemorySize);
      BOOST_REQUIRE (currentMemorySize <= maxMemorySize);
    }
  }
}

BOOST_AUTO_TEST_CASE ( create_recorder_memory_test )
{
  MediaObjectId mediaPipeline = MediaObjectId();
  MediaObjectId mo = MediaObjectId();
  int i, maxMemorySize, currentMemorySize;

  BOOST_REQUIRE_MESSAGE (initialized, "Cannot connect to the server");
  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, GST_DEFAULT_NAME, 0, GST_DEFAULT_NAME);

  client->addHandlerAddress (0, "localhost", 2323);

  for (i = 0; i < ITERATIONS; i++) {
    client->createMediaPipeline (mediaPipeline, 0);
    client->createUriEndPoint (mo, mediaPipeline, UriEndPointType::type::RECORDER_END_POINT, "file:///tmp/a");
    client->release (mediaPipeline);

    if (i == 0) {
      maxMemorySize = get_data_memory (pid) + MEMORY_TOLERANCE;
      GST_INFO ("MAX memory size: %d", maxMemorySize);
    }

    if (i % 100 == 0) {
      currentMemorySize = get_data_memory (pid);
      GST_INFO ("Memory size: %d", currentMemorySize);
      BOOST_REQUIRE (currentMemorySize <= maxMemorySize);
    }
  }
}

BOOST_AUTO_TEST_CASE ( create_rtp_end_point_memory_test )
{
  MediaObjectId mediaPipeline = MediaObjectId();
  MediaObjectId mo = MediaObjectId();
  int i, maxMemorySize, currentMemorySize;

  BOOST_REQUIRE_MESSAGE (initialized, "Cannot connect to the server");
  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, GST_DEFAULT_NAME, 0, GST_DEFAULT_NAME);

  client->addHandlerAddress (0, "localhost", 2323);

  for (i = 0; i < ITERATIONS; i++) {
    client->createMediaPipeline (mediaPipeline, 0);
    client->createSdpEndPoint (mo, mediaPipeline, SdpEndPointType::type::RTP_END_POINT);
    client->release (mediaPipeline);

    if (i == 0) {
      maxMemorySize = get_data_memory (pid) + MEMORY_TOLERANCE;
      GST_INFO ("MAX memory size: %d", maxMemorySize);
    }

    if (i % 100 == 0) {
      currentMemorySize = get_data_memory (pid);
      GST_INFO ("Memory size: %d", currentMemorySize);
      BOOST_REQUIRE (currentMemorySize <= maxMemorySize);
    }
  }
}

BOOST_AUTO_TEST_CASE ( create_zbar_filter_test )
{
  MediaObjectId mediaPipeline = MediaObjectId();
  MediaObjectId mo = MediaObjectId();
  int i, maxMemorySize, currentMemorySize;

  BOOST_REQUIRE_MESSAGE (initialized, "Cannot connect to the server");
  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, GST_DEFAULT_NAME, 0, GST_DEFAULT_NAME);

  client->addHandlerAddress (0, "localhost", 2323);

  for (i = 0; i < ITERATIONS; i++) {
    client->createMediaPipeline (mediaPipeline, 0);
    client->createFilter (mo, mediaPipeline, FilterType::type::ZBAR_FILTER);
    client->release (mediaPipeline);

    if (i == 0) {
      maxMemorySize = get_data_memory (pid) + MEMORY_TOLERANCE;
      GST_INFO ("MAX memory size: %d", maxMemorySize);
    }

    if (i % 100 == 0) {
      currentMemorySize = get_data_memory (pid);
      GST_INFO ("Memory size: %d", currentMemorySize);
      BOOST_REQUIRE (currentMemorySize <= maxMemorySize);
    }
  }
}

BOOST_AUTO_TEST_CASE ( connect_test )
{
  MediaObjectId mediaPipeline = MediaObjectId();
  MediaObjectId player = MediaObjectId();
  MediaObjectId recorder = MediaObjectId();
  std::vector<MediaObjectId> srcs, sinks;
  int i, maxMemorySize, currentMemorySize;

  BOOST_REQUIRE_MESSAGE (initialized, "Cannot connect to the server");
  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, GST_DEFAULT_NAME, 0, GST_DEFAULT_NAME);

  client->addHandlerAddress (0, "localhost", 2323);

  for (i = 0; i < ITERATIONS; i++) {
    client->createMediaPipeline (mediaPipeline, 0);
    client->createUriEndPoint (player, mediaPipeline, UriEndPointType::type::PLAYER_END_POINT, "file:///tmp/a");
    client->createUriEndPoint (recorder, mediaPipeline, UriEndPointType::type::RECORDER_END_POINT, "file:///tmp/b");

    client->getMediaSrcsByMediaType (srcs, player, MediaType::type::VIDEO);
    client->getMediaSinksByMediaType (sinks, recorder, MediaType::type::VIDEO);
    client->connect (srcs.front (), sinks.front () );
    client->release (mediaPipeline);

    if (i == 0) {
      maxMemorySize = get_data_memory (pid) + MEMORY_TOLERANCE;
      GST_INFO ("MAX memory size: %d", maxMemorySize);
    }

    if (i % 100 == 0) {
      currentMemorySize = get_data_memory (pid);
      GST_INFO ("Memory size: %d", currentMemorySize);
      BOOST_REQUIRE (currentMemorySize <= maxMemorySize);
    }
  }
}

BOOST_AUTO_TEST_CASE ( connect_releasing_sink_element_test )
{
  MediaObjectId mediaPipeline = MediaObjectId();
  MediaObjectId player = MediaObjectId();
  MediaObjectId recorder = MediaObjectId();
  std::vector<MediaObjectId> srcs, sinks;
  int i, maxMemorySize, currentMemorySize;

  BOOST_REQUIRE_MESSAGE (initialized, "Cannot connect to the server");
  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, GST_DEFAULT_NAME, 0, GST_DEFAULT_NAME);

  client->addHandlerAddress (0, "localhost", 2323);
  client->createMediaPipeline (mediaPipeline, 0);
  client->createUriEndPoint (player, mediaPipeline, UriEndPointType::type::PLAYER_END_POINT, "file:///tmp/a");
  client->getMediaSrcsByMediaType (srcs, player, MediaType::type::VIDEO);

  for (i = 0; i < ITERATIONS; i++) {
    client->createUriEndPoint (recorder, mediaPipeline, UriEndPointType::type::RECORDER_END_POINT, "file:///tmp/b");
    client->getMediaSinksByMediaType (sinks, recorder, MediaType::type::VIDEO);
    client->connect (srcs.front (), sinks.front () );
    client->release (recorder);

    if (i == 0) {
      maxMemorySize = get_data_memory (pid) + MEMORY_TOLERANCE;
      GST_INFO ("MAX memory size: %d", maxMemorySize);
    }

    if (i % 100 == 0) {
      currentMemorySize = get_data_memory (pid);
      GST_INFO ("Memory size: %d", currentMemorySize);
      BOOST_REQUIRE (currentMemorySize <= maxMemorySize);
    }
  }
}

BOOST_AUTO_TEST_CASE ( connect_releasing_source_element_test )
{
  MediaObjectId mediaPipeline = MediaObjectId();
  MediaObjectId player = MediaObjectId();
  MediaObjectId recorder = MediaObjectId();
  std::vector<MediaObjectId> srcs, sinks;
  int i, maxMemorySize, currentMemorySize;

  BOOST_REQUIRE_MESSAGE (initialized, "Cannot connect to the server");
  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, GST_DEFAULT_NAME, 0, GST_DEFAULT_NAME);

  client->addHandlerAddress (0, "localhost", 2323);
  client->createMediaPipeline (mediaPipeline, 0);
  client->createUriEndPoint (recorder, mediaPipeline, UriEndPointType::type::RECORDER_END_POINT, "file:///tmp/b");
  client->getMediaSinksByMediaType (sinks, recorder, MediaType::type::VIDEO);

  for (i = 0; i < ITERATIONS; i++) {
    client->createUriEndPoint (player, mediaPipeline, UriEndPointType::type::PLAYER_END_POINT, "file:///tmp/a");
    client->getMediaSrcsByMediaType (srcs, player, MediaType::type::VIDEO);
    client->connect (srcs.front (), sinks.front () );
    client->release (player);

    if (i == 0) {
      maxMemorySize = get_data_memory (pid) + MEMORY_TOLERANCE;
      GST_INFO ("MAX memory size: %d", maxMemorySize);
    }

    if (i % 100 == 0) {
      currentMemorySize = get_data_memory (pid);
      GST_INFO ("Memory size: %d", currentMemorySize);
      BOOST_REQUIRE (currentMemorySize <= maxMemorySize);
    }
  }
}

BOOST_AUTO_TEST_SUITE_END()

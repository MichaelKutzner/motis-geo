#include "doctest/doctest.h"

#include <ranges>

#include "geo/constants.h"
#include "geo/latlng.h"

#include "geo/polyline.h"

using namespace geo;

double abs(double const x) {
    return x < 0 ? -x : x;
}

TEST_CASE("polylineDistanceToPolyline_pointBeforeLine_getStartPoint") {
  auto const line = polyline{{0.0f, 0.0f}, {1.0f, 0.0f}};
  auto const test_point = latlng{-1.0f, 0.0f};

  auto const projection = distance_to_polyline(test_point, line);

  auto const start_coordinate = *line.begin();
  CHECK(projection.distance_to_polyline_ == distance(test_point, start_coordinate));
  CHECK(projection.best_ == start_coordinate);
}

TEST_CASE("polylineDistanceToPolyline_pointAfterLine_getEndPoint") {
  auto const line = polyline{{0.0f, 0.0f}, {1.0f, 0.0f}};
  auto const test_point = latlng{2.0f, 0.0f};

  auto const projection = distance_to_polyline(test_point, line);

  auto const end_coordinate = *line.rbegin();
  CHECK(projection.segment_idx_ == 0);
  CHECK(projection.distance_to_polyline_ == distance(test_point, end_coordinate));
  CHECK(projection.best_ == end_coordinate);
}

TEST_CASE("polylineDistanceToPolyline_pointOnLine_getPointOnLine") {
  auto const line = polyline{{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};
  auto const test_point = latlng{0.5f, 0.0f};

  auto const projection = distance_to_polyline(test_point, line);

  CHECK(projection.distance_to_polyline_ < kEpsilon);
  CHECK(projection.segment_idx_ == 0);
  CHECK(distance(projection.best_, test_point) < kEpsilon);
}

TEST_CASE("polylineDistanceToPolyline_pointNotOnLine_getClosestPoint") {
  auto const line = polyline{{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};
  auto const test_point = latlng{0.75f, 0.5f};

  auto const projection = distance_to_polyline(test_point, line);

  auto const best = latlng{1.0f, 0.5f};
  CHECK(projection.segment_idx_ == 1);
  CHECK(abs(distance(test_point, best) - distance(test_point, projection.best_)) < kEpsilon);
  CHECK(abs(projection.distance_to_polyline_ - distance(test_point, best)) < kEpsilon);
  CHECK(distance(projection.best_, best) < kEpsilon);
}

TEST_CASE("polylineSplitPolyline_multipleCoordinates_getSegments") {
  auto const line = polyline{{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}, {0.4f, 1.4f}, {1.4f, 1.4f}, {1.4f, 0.4f}, {0.4f, 0.4f}};
  auto const test_points = polyline{{0.99f, 0.95f}, {0.9f, 1.01f}, {0.6f, 0.9f}, {0.41f, 1.39f}, {1.0f, 1.5f}, {1.39f, 1.4f}, {1.41f, 0.6f}};

  auto const segments = split_polyline(line, test_points);

  auto const expected_split_points = polyline{{1.0f, 0.95f}, {0.9f, 1.0f}, {0.6f, 1.0f}, {0.41f, 1.40f}, {1.0f, 1.4f}, {1.39f, 1.4f}, {1.4f, 0.6f}};
  auto const expected_splits = std::vector<std::size_t>{1u, 2u, 2u, 4u, 4u, 4u, 5u};
  auto splits = std::vector<size_t>{};
  auto index = std::size_t{0u};
  for (auto const& pair : segments) {
    CHECK(distance(expected_split_points.at(index), pair.first) < 2 * kEpsilon);
    splits.push_back(pair.second);
    ++index;
  }
  CHECK(segments.size() == test_points.size());
  CHECK(expected_splits == splits);
}
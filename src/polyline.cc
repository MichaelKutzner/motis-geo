#include "geo/polyline.h"

#include "boost/geometry.hpp"

#include "geo/latlng.h"
#include "utl/pairwise.h"

#include "geo/constants.h"

#include "geo/detail/register_latlng.h"
#include "geo/detail/register_polyline.h"

namespace geo {

double length(polyline const& p) {
  return boost::geometry::length(p) * kEarthRadiusMeters;
}

polyline simplify(polyline const& p, double const max_distance) {
  polyline result;
  boost::geometry::simplify(p, result, max_distance);
  return result;
}

polyline extract(polyline const& p, size_t const from, size_t const to) {
  geo::polyline result;
  result.reserve(std::abs(static_cast<int>(from) - static_cast<int>(to)) + 1);
  int const inc = (from < to) ? 1 : -1;
  for (int i = from; i != (static_cast<int>(to) + inc); i += inc) {
    result.push_back(p[i]);
  }
  return result;
}

polyline_candidate distance_to_polyline(latlng const& x, polyline const& c) {
  auto min = std::numeric_limits<double>::max();
  auto best = latlng{};
  auto best_segment_idx = 0U;
  auto segment_idx = 0U;
  for (auto const [a, b] : utl::pairwise(c)) {
    auto const candidate = closest_on_segment(x, a, b);
    auto const dist = distance(x, candidate);
    if (dist < min) {
      min = dist;
      best = candidate;
      best_segment_idx = segment_idx;
    }
    ++segment_idx;
  }
  return {.distance_to_polyline_ = min, .best_ = best, .segment_idx_ = best_segment_idx};
}

std::vector<std::pair<latlng, std::size_t>> split_polyline(polyline const& line, polyline const& split_points) {
  auto segments = std::vector<std::pair<latlng, std::size_t>>{};
  segments.reserve(split_points.size());

  auto const generator = utl::pairwise(line);
  auto start = generator.begin();
  auto offset = std::size_t{0u};

  for (auto const& point : split_points) {
    auto min = std::numeric_limits<double>::max();
    auto best = latlng{};
    auto segment_idx = offset;
    for (auto it = start; it != generator.end(); ++it) {
      auto const& [a, b] = *it;
      auto const candidate = closest_on_segment(point, a, b);
      auto const dist = distance(point, candidate);
      if (dist < min) {
        min = dist;
        best = candidate;
        start = it;
        offset = segment_idx;
      }
      ++segment_idx;
    }
    segments.emplace_back(std::make_pair(best, offset));
  }
  return segments;
}

}  // namespace geo

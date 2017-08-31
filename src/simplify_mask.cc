#include "geo/simplify_mask.h"

#include <iostream>
#include <stack>

#include "geo/constants.h"
#include "geo/webmercator.h"

namespace geo {

constexpr auto kMaxSimplifyZoomLevel = 20;
using proj = webmercator<4096, kMaxSimplifyZoomLevel>;

using proj_xy = xy<double>;

proj_xy proj_pt_on_segment(pixel_xy const& source, pixel_xy const& target,
                           pixel_xy const& coord) {
  pixel_xy const slope_vec{target.y_ - source.y_, target.x_ - source.x_};
  pixel_xy const rel_coord{coord.y_ - source.y_, coord.x_ - source.x_};

  // dot product of two un-normed vectors
  double const unnormed_ratio =
      slope_vec.x_ * rel_coord.x_ + slope_vec.y_ * rel_coord.y_;
  double const sq_length =
      slope_vec.x_ * slope_vec.x_ + slope_vec.y_ * slope_vec.y_;

  if (sq_length < std::numeric_limits<double>::epsilon()) {
    return {static_cast<double>(source.x_), static_cast<double>(source.y_)};
  }

  double const normed_ratio = unnormed_ratio / sq_length;
  double const clamped_ratio = std::max(std::min(normed_ratio, 1.), 0.);

  return proj_xy{(1. - clamped_ratio) * source.x_ + target.x_ * clamped_ratio,
                 (1. - clamped_ratio) * source.y_ + target.y_ * clamped_ratio};
}

uint64_t sq_perpendicular_dist(pixel_xy const& start, pixel_xy const& target,
                               pixel_xy const& test) {
  auto const proj = proj_pt_on_segment(start, target, test);
  auto const dx = proj.x_ - test.x_;
  auto const dy = proj.y_ - test.y_;
  return dx * dx + dy * dy;
}

using range_t = std::pair<size_t, size_t>;
using stack_t = std::stack<range_t, std::vector<range_t>>;

bool process_level(std::vector<pixel_xy> const& line, uint64_t const threshold,
                   stack_t& stack, std::vector<bool>& mask) {
  assert(stack.empty());

  auto last = 0;
  for (auto i = 1u; i < mask.size(); ++i) {
    if (mask[i]) {
      if (i - last > 1) {
        stack.emplace(last, i);
      }

      last = i;
    }
  }

  if (stack.empty()) {
    return true;
  }

  while (!stack.empty()) {
    auto const pair = stack.top();
    stack.pop();

    uint64_t max_dist = 0;
    auto farthest_entry_index = pair.second;

    for (auto idx = pair.first + 1; idx != pair.second; ++idx) {
      auto const dist =
          sq_perpendicular_dist(line[pair.first], line[pair.second], line[idx]);

      if (dist > max_dist && dist >= threshold) {
        farthest_entry_index = idx;
        max_dist = dist;
      }
    }

    if (max_dist >= threshold) {
      mask[farthest_entry_index] = true;
      if (pair.first < farthest_entry_index) {
        stack.emplace(pair.first, farthest_entry_index);
      }
      if (farthest_entry_index < pair.second) {
        stack.emplace(farthest_entry_index, pair.second);
      }
    }
  }

  return false;
}

simplify_mask_t make_mask(polyline const& input,
                          uint32_t const pixel_precision) {
  if (input.size() < 2) {
    return simplify_mask_t{kMaxSimplifyZoomLevel + 1,
                           std::vector<bool>(input.size(), true)};
  }

  std::vector<pixel_xy> line;
  line.reserve(input.size());
  std::transform(
      begin(input), end(input), std::back_inserter(line), [](auto const& in) {
        return proj::merc_to_pixel(latlng_to_merc(in), kMaxSimplifyZoomLevel);
      });

  simplify_mask_t result;

  std::vector<bool> mask(line.size(), false);
  mask.front() = true;
  mask.back() = true;

  std::vector<range_t> stack_mem;
  stack_mem.reserve(line.size());
  stack_t stack{stack_mem};

  for (auto z = 0; z <= kMaxSimplifyZoomLevel; ++z) {
    uint64_t const delta = static_cast<uint64_t>(pixel_precision)
                           << (kMaxSimplifyZoomLevel - z);
    uint64_t const threshold = delta * delta;

    auto const done = process_level(line, threshold, stack, mask);

    if (done) {
      for (auto i = z; i <= kMaxSimplifyZoomLevel; ++i) {
        result.push_back(mask);
      }
      break;
    }

    result.push_back(mask);
  }

  assert(result.size() == kMaxSimplifyZoomLevel + 1);

  return result;
}

}  // namespace geo
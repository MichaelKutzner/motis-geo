#include "catch.hpp"

#include <vector>

#include "geo/tile.h"

std::vector<geo::tile> list_tiles(geo::tile_range const& range) {
  std::vector<geo::tile> vec;
  for (auto const& t : range) {
    vec.push_back(t);
  }
  return vec;
}

TEST_CASE("tile::direct_children()") {
  SECTION("root") {
    geo::tile root{0, 0, 0};
    auto const actual = list_tiles(root.direct_children());

    std::vector<geo::tile> expected{{0, 0, 1}, {1, 0, 1}, {0, 1, 1}, {1, 1, 1}};

    CHECK(expected == actual);
  }

  SECTION("darmstadt") {
    geo::tile parent{8585, 5565, 14};
    auto const actual = list_tiles(parent.direct_children());

    std::vector<geo::tile> expected{{17170, 11130, 15},
                                    {17171, 11130, 15},
                                    {17170, 11131, 15},
                                    {17171, 11131, 15}};

    CHECK(expected == actual);
  }
}

TEST_CASE("tile::FOO_on_z()") {
  SECTION("same level") {
    geo::tile uut(23, 42, 3);

    auto const actual = list_tiles(uut.range_on_z(3));

    REQUIRE(actual.size() == 1);
    CHECK(actual[0] == uut);

    auto const a_bounds = uut.bounds_on_z(3);
    auto const e_bounds = geo::tile_iterator_bounds{23, 42, 23 + 1, 43};
    CHECK(a_bounds == e_bounds);
  }

  SECTION("1 level down") {
    geo::tile uut{0, 0, 0};
    auto const actual = list_tiles(uut.range_on_z(1));

    std::vector<geo::tile> expected{{0, 0, 1}, {1, 0, 1}, {0, 1, 1}, {1, 1, 1}};

    CHECK(expected == actual);

    auto const a_bounds = uut.bounds_on_z(1);
    auto const e_bounds = geo::tile_iterator_bounds{0, 0, 2, 2};
    CHECK(a_bounds == e_bounds);
  }

  SECTION("2 levels down") {
    geo::tile uut{56, 84, 7};
    auto const actual = list_tiles(uut.range_on_z(9));

    std::vector<geo::tile> expected{
        {224, 336, 9}, {225, 336, 9}, {226, 336, 9}, {227, 336, 9},  //
        {224, 337, 9}, {225, 337, 9}, {226, 337, 9}, {227, 337, 9},  //
        {224, 338, 9}, {225, 338, 9}, {226, 338, 9}, {227, 338, 9},  //
        {224, 339, 9}, {225, 339, 9}, {226, 339, 9}, {227, 339, 9}};

    CHECK(expected == actual);

    auto const a_bounds = uut.bounds_on_z(9);
    auto const e_bounds = geo::tile_iterator_bounds{224, 336, 228, 340};
    CHECK(a_bounds == e_bounds);
  }

  SECTION("1 level up") {
    geo::tile uut{17170, 11131, 15};
    auto const actual = list_tiles(uut.range_on_z(14));

    std::vector<geo::tile> expected{{8585, 5565, 14}};
    CHECK(expected == actual);

    auto const a_bounds = uut.bounds_on_z(14);
    auto const e_bounds = geo::tile_iterator_bounds{8585, 5565, 8586, 5566};
    CHECK(a_bounds == e_bounds);
  }

  SECTION("3 levels up") {
    geo::tile uut{15670, 131, 15};
    auto const actual = list_tiles(uut.range_on_z(12));

    std::vector<geo::tile> expected{{1958, 16, 12}};
    CHECK(expected == actual);

    auto const a_bounds = uut.bounds_on_z(12);
    auto const e_bounds = geo::tile_iterator_bounds{1958, 16, 1959, 17};
    CHECK(a_bounds == e_bounds);
  }

  SECTION("all levels up") {
    geo::tile uut{12314, 23455, 15};
    auto const actual = list_tiles(uut.range_on_z(0));

    std::vector<geo::tile> expected{{0, 0, 0}};
    CHECK(expected == actual);

    auto const a_bounds = uut.bounds_on_z(0);
    auto const e_bounds = geo::tile_iterator_bounds{0, 0, 1, 1};
    CHECK(a_bounds == e_bounds);
  }
}

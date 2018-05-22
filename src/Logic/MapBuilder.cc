//    Copyright (C) 2018  Handsome0hell
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
//    Email: handsome0hell@gmail.com
#include "MapBuilder.h"
#include "../Map/Map.h"
#include "Pathfinder.h"
#include <cstdint>
#include <list>
#include <queue>
void MapBuilder::BuildRoomsAndPath() {
  InitForEmptyTest();
  Point previous;
  bool is_first = true;
  while (true) {
    Point tmp;
    if (!BuildRoom(&tmp)) break;
    if (!is_first) {
      BuildPath(previous, tmp);
    } else {
      is_first = false;
    }
    previous = tmp;
  }
}
void MapBuilder::InitForEmptyTest() {
  for (uint32_t i = 0; i < target_map_ -> width(); ++i) {
    for (uint32_t j = 0; j < target_map_ -> height(); ++j) {
      checked_build_able_[i][j].w = 0;
      checked_build_able_[i][j].h = 0;
    }
  }
}
void MapBuilder::UpdateCheckedBuildAble(const Point & pos_to_update) {
  Rect& now = checked_build_able_[pos_to_update.x][pos_to_update.y];
  if (pos_to_update.x > 0) {
    now = checked_build_able_[pos_to_update.x - 1][pos_to_update.y];
    if (now.w > 0) --now.w;
  }
  if (pos_to_update.x > 0 && pos_to_update.y > 0) {
    now = checked_build_able_[pos_to_update.x - 1][pos_to_update.y - 1];
    if (now.w > 0) --now.w;
    if (now.h > 0) --now.h;
  }
  if (pos_to_update.y > 0) {
    now = checked_build_able_[pos_to_update.x][pos_to_update.y - 1];
    if (now.h > 0) --now.h;
  }
}
void MapBuilder::BuildPath(const Point & from, const Point & to) {
  PathFinder path_designer;
  path_designer.set_value(Map::kBlockWall, 10);
  path_designer.set_value(Map::kBlockGround, 1);
  path_designer.set_value(Map::kBlockPath, 3);
  path_designer.set_target_map(*target_map_);
  std::list< Point > shortest_path =
      path_designer.FindShortestPath(from, to);
  std::list< Point >::iterator path_builder = shortest_path.begin();
  while (path_builder != shortest_path.end()) {
    if (target_map_ -> block(*path_builder) == Map::kBlockWall) {
        target_map_ -> set_block(*path_builder, Map::kBlockPath);
    }
    ++path_builder;
  }
}
bool MapBuilder::SelectRoomPosition(RectWithPos* rect_for_build) {
  std::queue< Point > build_able_point;
  for (rect_for_build -> left_top.x = 0;
       rect_for_build -> left_top.x < target_map_ -> width();
       ++rect_for_build -> left_top.x) {
    for (rect_for_build -> left_top.y = 0;
         rect_for_build -> left_top.y < target_map_ -> height();
         ++rect_for_build -> left_top.y) {
      if (IsRectEmpty(*rect_for_build)) {
        build_able_point.push(rect_for_build -> left_top);
      }
    }
  }
  if (build_able_point.size() == 0) return false;
  static std::uniform_int_distribution< unsigned int > rand_dis;
  typedef std::uniform_int_distribution< unsigned int >::param_type party;
  size_t select_a_pos = rand_dis(*random_gen_, party(1, build_able_point.size()));
  for (size_t i = 1; i < select_a_pos; ++i) {
    build_able_point.pop();
  }
  rect_for_build -> left_top = build_able_point.front();
  return true;
}
bool MapBuilder::BuildRoom(Point* room_pos) {
  RectWithPos new_room;
  Rect random_rect = RandomRoomRect();
  new_room.w = random_rect.w;
  new_room.h = random_rect.h;
  if (!SelectRoomPosition(&new_room)) {
    return false;
  }
  for (uint32_t i = 0; i < new_room.w; ++i) {
    for (uint32_t j = 0; j < new_room.h; ++j) {
      // Won't cause room adhesion
      if (!(i == 0 || j == 0 || i + 1 == new_room.w || j + 1 == new_room.h)) {
        target_map_ -> set_block(
                           CreatePoint(new_room.left_top.x + i,
                                       new_room.left_top.y + j),
                                       Map::kBlockGround);
      }
    }
  }
  // Keep the checked array correct
  Point now = new_room.left_top;
  now.x -= kMaxRoomWidth;
  now.y -= kMaxRoomHeight;
  now.x = std::max(0, now.x);
  now.y = std::max(0, now.y);
  for (; now.x < new_room.left_top.x + new_room.w; ++now.x) {
    for (; now.y < new_room.left_top.y + new_room.h; ++now.y) {
      if (now.x >= new_room.left_top.x && now.y >= new_room.left_top.y) {
        checked_build_able_[now.x][now.y].w = 0;
        checked_build_able_[now.x][now.y].h = 0;
      } else if (now.x < new_room.left_top.x) {
        checked_build_able_[now.x][now.y].w =
            std::min(checked_build_able_[now.x][now.y].w,
                     new_room.left_top.x - now.x);
      } else {
        checked_build_able_[now.x][now.y].h =
            std::min(checked_build_able_[now.x][now.y].h,
                     new_room.left_top.y - now.y);
      }
    }
  }
  ++new_room.left_top.x;
  ++new_room.left_top.y;
  *room_pos = new_room.left_top;
  return true;
}
bool MapBuilder::IsRectEmpty(const RectWithPos& rect_for_check) {
  const Point & rect_l_t = rect_for_check.left_top;  // Shorter code
  if ((rect_for_check.w + rect_l_t.x > target_map_ -> width()) ||
      (rect_for_check.h + rect_l_t.y > target_map_ -> height())) return false;
  UpdateCheckedBuildAble(rect_l_t);
  // Shorter code, the result will be saved to the checked array
  Rect & now = checked_build_able_[rect_l_t.x][rect_l_t.y];
  // Can I continue to expand the width or height?
  bool is_max_w = false, is_max_h = false;
  // Won't check too much area
  now.w = std::min(now.w, rect_for_check.w);
  now.h = std::min(now.h, rect_for_check.h);
  while (!(now.w == rect_for_check.w || is_max_w) ||
         !(now.h == rect_for_check.h || is_max_h)) {
    // Try to expand width
    if (!is_max_w && now.w != rect_for_check.w) {
      for (uint32_t i = 0; i < now.h; ++i) {
        if (target_map_ -> block(CreatePoint(rect_l_t.x + now.w,
                                           rect_l_t.y + i)) !=
            Map::kBlockWall &&
            target_map_ -> block(CreatePoint(rect_l_t.x + now.w,
                                           rect_l_t.y + i)) !=
            Map::kBlockPath) {
          is_max_w = true;  // Oops, can't expand anymore
          --now.w;  // Keep width
          break;
        }
      }
      ++now.w;
    }
    // Try to expand height
    if (!is_max_h && now.h != rect_for_check.h) {
      for (uint32_t i = 0; i < now.w; ++i) {
        if (target_map_ -> block(CreatePoint(rect_l_t.x + i,
                                           rect_l_t.y + now.h)) !=
            Map::kBlockWall &&
            target_map_ -> block(CreatePoint(rect_l_t.x + i,
                                           rect_l_t.y + now.h)) !=
            Map::kBlockPath) {
          is_max_h = true;  // Oops, can't expand anymore
          --now.h;  // Keep height
          break;
        }
      }
      ++now.h;
    }
  }
  if (now.w == rect_for_check.w && now.h == rect_for_check.h) {
    return true;
  } else {
    return false;
  }
}
inline Rect MapBuilder::RandomRoomRect() {
  Rect ret;
  static std::uniform_int_distribution< unsigned int > rand_dis_w =
    std::uniform_int_distribution< unsigned int >(kMinRoomWidth + 2, kMaxRoomWidth + 2);
  static std::uniform_int_distribution< unsigned int > rand_dis_h = 
    std::uniform_int_distribution< unsigned int >(kMinRoomHeight + 2, kMaxRoomHeight + 2);
  // Need some space to prevent room adhesion
  ret.w = rand_dis_w(*random_gen_);
  ret.h = rand_dis_h(*random_gen_);
  return ret;
}
inline const Rect & MapBuilder::max(const Rect & a, const Rect & b) {
  if (a.w < 0 || a.h < 0) return b;
  if (b.w < 0 || b.h < 0) return a;
  uint64_t a_area = a.w, b_area = b.w;
  a_area *= a.h, b_area *= b.h;
  if (a_area > b_area) {
    return a;
  } else {
    return b;
  }
}
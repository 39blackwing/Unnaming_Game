#ifndef UNNAMING_GAME_SRC_MAP_WORLD_H_
#define UNNAMING_GAME_SRC_MAP_WORLD_H_
#include "Map.h"
#include "../Logic/MapBuilder.h"
#include <map>
#include <cassert>
#include <functional>
class World {
 public:
  struct MemoryOfMap {
    Point left_top;
    Point right_bottom; 
    std::vector< std::vector< bool > > is_seen;
    Map_ref detail;
  };
  inline World(const std::function< int32_t(int32_t, int32_t) >& ran,
               MapBuilder* builder, const Rect& nms) :
               builder_(builder), random_gen_(ran), next_map_size_(nms) {}
  inline void set_next_map_size(const Rect& si) {next_map_size_ = si;}
  inline Map* NewMap();
  inline const Map::Target GetTarget(Map* map, const Point& pos);
  inline void Arrive(Map* map);
  inline void Left(Map* map);
  inline MemoryOfMap& GetMemory(int32_t obj_id, Map* map);

 private:
  struct MapInformation {
    Map_ref map;
    int32_t connections_num;
    std::map< int32_t, MemoryOfMap > memories_;
  };
  std::map< int32_t, MapInformation > id_to_map_;
  MapBuilder* const builder_;
  const std::function< int32_t(int32_t, int32_t) > random_gen_;
  Rect next_map_size_;
};
inline Map* World::NewMap() {
  MapInformation tmp = {Map::Create(next_map_size_.w, next_map_size_.h), 0,
                        std::map< int32_t, MemoryOfMap>()};
  tmp.map -> ForEachBlock([](Map::BlockType* block){*block = Map::kBlockWall;});
  builder_ -> set_target_map(tmp.map.get());
  builder_ -> BuildRoomsAndPath();
  auto inserter = id_to_map_.insert(std::make_pair(tmp.map -> Id(), tmp));
  return inserter.first -> second.map.get();
}
inline const Map::Target World::GetTarget(Map* map, const Point& pos) {
  Map::Target ret = map -> PortalTarget(pos);
  if (ret.map == nullptr) {
    ret.map = NewMap();
    ret.pos = ret.map -> PickARandomPointInGroundOrPath(random_gen_);
    map -> SetPortalTarget(pos, ret);
  }
  return ret;
}
inline void World::Arrive(Map* map) {
  auto finder = id_to_map_.find(map -> Id());
  assert((finder != id_to_map_.end()));
  ++(finder -> second.connections_num);
}
inline void World::Left(Map* map) {
  auto finder = id_to_map_.find(map -> Id());
  assert(finder != id_to_map_.end());
  if(--(finder -> second.connections_num) <= 0) {
    id_to_map_.erase(finder);
  }
}
inline World::MemoryOfMap& World::GetMemory(int32_t obj_id, Map* map) {
  auto info_finder = id_to_map_.find(map -> Id());
  assert(info_finder != id_to_map_.end());
  //For shorter code
  MapInformation* const map_finder= &(info_finder -> second);
  auto obj_finder = (map_finder -> memories_).find(obj_id);
  if (obj_finder == (map_finder -> memories_).end()) {
    const MemoryOfMap tmp = {{map_finder -> map -> Width(),
                              map_finder -> map -> Height()}, {0, 0},
                             std::vector< std::vector< bool > >(
                                 map_finder -> map -> Width(),
                                 std::vector< bool >(
                                     map_finder -> map -> Height(), false)),
                                 Map::Create(map_finder -> map -> Width(),
                                             map_finder -> map -> Height())};
    obj_finder = (map_finder -> memories_).insert(
                     std::make_pair(map_finder -> map -> Id(), tmp)).first;
  }
  return obj_finder -> second;
}
#endif  // UNNAMING_GAME_SRC_MAP_WORLD_H_

#include <run3/air3/AirPathFind.hpp>

#include <algorithm>
#include <cstddef>
#include <queue>
#include <utility>

namespace run3::air3 {

AirPathFind::AirPathFind(const physics::IPhysicsQuery &physicsQuery) noexcept
    : physicsQuery_(&physicsQuery) {}

void AirPathFind::setNodes(std::vector<PathNode> nodes) {
  nodes_ = std::move(nodes);
}

bool AirPathFind::lineBlocked(physics::Vec3 from, physics::Vec3 to) const {
  physics::RaycastQuery query{from, to};
  query.group = physics::CollisionGroup::Npc;
  query.mask = physics::collisionMask(physics::CollisionGroup::World) |
               physics::collisionMask(physics::CollisionGroup::Dynamic) |
               physics::collisionMask(physics::CollisionGroup::Door);
  query.includeTriggers = false;
  return physicsQuery_->raycastClosest(query).has_value();
}

std::vector<PathNode>
AirPathFind::search(const PathNode &start, const PathNode &destination) const {
  if (!lineBlocked(start.position, destination.position)) {
    return {start, destination};
  }
  std::vector<PathNode> graph;
  graph.reserve(nodes_.size() + 2);
  graph.push_back(start);
  graph.insert(graph.end(), nodes_.begin(), nodes_.end());
  graph.push_back(destination);
  const std::size_t destinationIndex = graph.size() - 1;
  std::vector<bool> visited(graph.size(), false);
  std::vector<std::size_t> parent(graph.size(), graph.size());
  std::queue<std::size_t> pending;
  visited[0] = true;
  pending.push(0);
  while (!pending.empty() && !visited[destinationIndex]) {
    const std::size_t current = pending.front();
    pending.pop();
    for (std::size_t candidate = 1; candidate < graph.size(); ++candidate) {
      if (visited[candidate] ||
          lineBlocked(graph[current].position, graph[candidate].position)) {
        continue;
      }
      visited[candidate] = true;
      parent[candidate] = current;
      pending.push(candidate);
    }
  }
  if (!visited[destinationIndex]) {
    return {};
  }
  std::vector<PathNode> path;
  for (std::size_t current = destinationIndex; current != graph.size();
       current = parent[current]) {
    path.push_back(graph[current]);
    if (current == 0) {
      break;
    }
  }
  std::reverse(path.begin(), path.end());
  return path;
}

} // namespace run3::air3

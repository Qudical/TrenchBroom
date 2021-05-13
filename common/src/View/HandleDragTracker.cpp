/*
 Copyright (C) 2021 Kristian Duske

 This file is part of TrenchBroom.

 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 TrenchBroom is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with TrenchBroom. If not, see <http://www.gnu.org/licenses/>.
 */

#include "View/HandleDragTracker.h"

#include "View/Grid.h"

#include <vecmath/distance.h>
#include <vecmath/intersection.h>
#include <vecmath/line.h>
#include <vecmath/plane.h>

namespace TrenchBroom {
    namespace View {
        std::optional<DragConfig> HandleDragTrackerDelegate::modifierKeyChange(const InputState&, const vm::vec3& /* initialHandlePosition */, const vm::vec3& /* currentHandlePosition */) {
            return std::nullopt;
        }

        void HandleDragTrackerDelegate::mouseScroll(const InputState&, const vm::vec3& /* initialHandlePosition */, const vm::vec3& /* currentHandlePosition */) {}

        void HandleDragTrackerDelegate::setRenderOptions(const InputState&, Renderer::RenderContext&) const {}

        void HandleDragTrackerDelegate::render(const InputState&, Renderer::RenderContext&, Renderer::RenderBatch&, const vm::vec3& /* initialHandlePosition */, const vm::vec3& /* currentHandlePosition */) const {}

        FindHitPosition makeLineHitFinder(const vm::line3& line) {
            return [line](const InputState& inputState) -> std::optional<vm::vec3> {
                const auto dist = vm::distance(inputState.pickRay(), line);
                if (dist.parallel) {
                    return std::nullopt;
                }
                return line.point + line.direction * dist.position2;
            };
        }

        FindHitPosition makePlaneHitFinder(const vm::plane3& plane) {
            return [plane](const InputState& inputState) -> std::optional<vm::vec3> {
                const auto distance = vm::intersect_ray_plane(inputState.pickRay(), plane);
                if (vm::is_nan(distance)) {
                    return std::nullopt;
                }
                return vm::point_at_distance(inputState.pickRay(), distance);
            };
        }

        ConvertHitToHandlePosition makeDeltaSnapper(const Grid& grid) {
            return [&grid](const InputState&, const vm::vec3& initialHandlePosition, const vm::vec3&, const vm::vec3& currentHitPosition) {
                return initialHandlePosition + grid.snap(currentHitPosition - initialHandlePosition);
            };
        }

        GetHandlePosition makeGetHandlePosition(FindHitPosition findHitPosition, ConvertHitToHandlePosition convertHitToHandlePosition) {
            return [find=std::move(findHitPosition), convert=std::move(convertHitToHandlePosition)](const InputState& inputState, const vm::vec3& initialHandlePosition, const vm::vec3& lastHandlePosition) -> std::optional<vm::vec3> {
                if (const auto hitPosition = find(inputState)) {
                    return convert(inputState, initialHandlePosition, lastHandlePosition, *hitPosition);
                }

                return std::nullopt;
            };
        }
    }
}
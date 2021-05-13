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

#pragma once

#include "PreferenceManager.h"
#include "Preferences.h"
#include "Renderer/Camera.h"
#include "Renderer/RenderService.h"
#include "View/HandleDragTracker.h"

#include <vecmath/line.h>
#include <vecmath/plane.h>
#include <vecmath/vec.h>

#include <array>
#include <memory>
#include <type_traits>

namespace TrenchBroom {
    namespace View {
        struct MoveHandleDragTrackerDelegate {
            virtual void mouseScroll(const InputState&, const vm::vec3& /* initialHandlePosition */, const vm::vec3& /* currentHandlePosition */) {}

            virtual DragStatus move(const InputState&, const vm::vec3& lastHandlePosition, const vm::vec3& currentHandlePosition) = 0;
            virtual void end(const InputState&, const vm::vec3& initialHandlePosition, const vm::vec3& currentHandlePosition) = 0;
            virtual void cancel(const vm::vec3& initialHandlePosition) = 0;

            virtual void setRenderOptions(const InputState& , Renderer::RenderContext&) const {}
            virtual void render(const InputState&, Renderer::RenderContext&, Renderer::RenderBatch&) const {}

            virtual ConvertHitToHandlePosition makeHandleConverter(const InputState&) const = 0;
        };

        /**
         * A drag tracker that implements TrenchBroom's usual pattern for moving objects.
         */
        template <typename Delegate>
        class MoveHandleDragDelegate : public HandleDragTrackerDelegate {
        private:
            static_assert(std::is_base_of_v<MoveHandleDragTrackerDelegate, Delegate>, "Delegate must extend MoveHandleDragTrackerDelegate");

            enum class MoveType {
                Vertical,
                Constricted,
                Default
            };

            Delegate m_delegate;

            MoveType m_lastMoveType{MoveType::Default};
            size_t m_lastConstrictedMoveAxis{0};
        public:
            MoveHandleDragDelegate(Delegate delegate) : 
            m_delegate{std::move(delegate)} {}

            std::optional<DragConfig> modifierKeyChange(const InputState& inputState, const vm::vec3& initialHandlePosition, const vm::vec3& currentHandlePosition) override {
                const auto nextMoveType = moveType(inputState, initialHandlePosition, currentHandlePosition);
                if (nextMoveType != m_lastMoveType) {
                    auto getHandlePosition = makeGetHandlePosition(
                        makeHitFinder(nextMoveType, inputState, initialHandlePosition, currentHandlePosition),
                        m_delegate.makeHandleConverter(inputState));

                    auto newInitialHandlePosition = initialHandlePosition;
                    auto newCurrentHandlePosition = currentHandlePosition;

                    if (m_lastMoveType == MoveType::Vertical) {
                        if (auto newHandlePosition = getHandlePosition(inputState, initialHandlePosition, currentHandlePosition)) {
                            newInitialHandlePosition = *newHandlePosition;
                            newCurrentHandlePosition = *newHandlePosition;
                        } else {
                            return std::nullopt;
                        }
                    }

                    m_lastMoveType = nextMoveType;
                    
                    if (m_lastMoveType == MoveType::Constricted) {
                        m_lastConstrictedMoveAxis = vm::find_abs_max_component(currentHandlePosition - initialHandlePosition);
                    }

                    return DragConfig {
                        std::move(getHandlePosition),
                        newInitialHandlePosition,
                        newCurrentHandlePosition
                    };
                }

                return std::nullopt;
            }

            void mouseScroll(const InputState& inputState, const vm::vec3& initialHandlePosition, const vm::vec3& currentHandlePosition) override {
                m_delegate.mouseScroll(inputState, initialHandlePosition, currentHandlePosition);
            }

            DragConfig initialize(const InputState& inputState, const vm::vec3& initialHandlePosition) override {
                if (isVerticalMove(inputState)) {
                    m_lastMoveType = MoveType::Vertical;

                    return DragConfig {
                        makeGetHandlePosition(
                            makeVerticalHitFinder(inputState, initialHandlePosition),
                            m_delegate.makeHandleConverter(inputState)),
                        initialHandlePosition,
                        initialHandlePosition
                    };
                }

                return DragConfig {
                    makeGetHandlePosition(
                        makeDefaultHitFinder(inputState, initialHandlePosition),
                        m_delegate.makeHandleConverter(inputState)),
                    initialHandlePosition,
                    initialHandlePosition
                };
            }

            DragStatus drag(const InputState& inputState, const vm::vec3& /* initialHandlePosition */, const vm::vec3& lastHandlePosition, const vm::vec3& nextHandlePosition) override {
                return m_delegate.move(inputState, lastHandlePosition, nextHandlePosition);
            }

            void end(const InputState& inputState, const vm::vec3& initialHandlePosition, const vm::vec3& currentHandlePosition) override {
                m_delegate.end(inputState, initialHandlePosition, currentHandlePosition);
            }

            void cancel(const vm::vec3& initialHandlePosition) override {
                m_delegate.cancel(initialHandlePosition);
            }

            void setRenderOptions(const InputState& inputState, Renderer::RenderContext& renderContext) const override {
                m_delegate.setRenderOptions(inputState, renderContext);
            }

            void render(const InputState& inputState, Renderer::RenderContext& renderContext, Renderer::RenderBatch& renderBatch, const vm::vec3& initialHandlePosition, const vm::vec3d& currentHandlePosition) const override {
                if (currentHandlePosition != initialHandlePosition) {
                    const auto vec = currentHandlePosition - initialHandlePosition;

                    auto renderService = Renderer::RenderService{renderContext, renderBatch};
                    renderService.setShowOccludedObjects();

                    const auto stages = std::array<vm::vec3, 3>{
                        vec * vm::vec3::pos_x(),
                        vec * vm::vec3::pos_y(),
                        vec * vm::vec3::pos_z(),
                    };

                    const auto colors = std::array<Color, 3>{
                        pref(Preferences::XAxisColor),
                        pref(Preferences::YAxisColor),
                        pref(Preferences::ZAxisColor),
                    };

                    const auto lineWidths = std::array<float, 3>{
                        m_lastMoveType == MoveType::Constricted && m_lastConstrictedMoveAxis == 0 ? 2.0f : 1.0f,
                        m_lastMoveType == MoveType::Constricted && m_lastConstrictedMoveAxis == 1 ? 2.0f : 1.0f,
                        m_lastMoveType == MoveType::Constricted && m_lastConstrictedMoveAxis == 2 ? 2.0f : 1.0f,
                    };

                    auto lastPos = initialHandlePosition;
                    for (size_t i = 0; i < 3; ++i) {
                        const auto& stage = stages[i];
                        const auto curPos = lastPos + stage;

                        renderService.setForegroundColor(colors[i]);
                        renderService.setLineWidth(lineWidths[i]);
                        renderService.renderLine(vm::vec3f{lastPos}, vm::vec3f{curPos});
                        lastPos = curPos;
                    }
                }

                m_delegate.render(inputState, renderContext, renderBatch);
            }
        private:
            static MoveType moveType(const InputState& inputState, const vm::vec3& initialHandlePosition, const vm::vec3& currentHandlePosition) {
                if (isVerticalMove(inputState)) {
                    return MoveType::Vertical;
                } else if (isConstrictedMove(inputState, initialHandlePosition, currentHandlePosition)) {
                    return MoveType::Constricted;
                } else {
                    return MoveType::Default;
                }
            }

            static bool isVerticalMove(const InputState& inputState) {
                const Renderer::Camera& camera = inputState.camera();
                return camera.perspectiveProjection() && inputState.checkModifierKey(MK_Yes, ModifierKeys::MKAlt);
            }

            static bool isConstrictedMove(const InputState& inputState, const vm::vec3& initialHandlePosition, const vm::vec3& currentHandlePosition) {
                if (inputState.checkModifierKey(MK_Yes, ModifierKeys::MKShift)) {
                    const auto delta = currentHandlePosition - initialHandlePosition;
                    return vm::get_abs_max_component(delta, 0) != vm::get_abs_max_component(delta, 1);
                }

                return false;
            }

            static FindHitPosition makeHitFinder(const MoveType moveType, const InputState& inputState, const vm::vec3& initialHandlePosition, const vm::vec3& currentHandlePosition) {
                switch (moveType) {
                    case MoveType::Vertical:
                        return makeVerticalHitFinder(inputState, currentHandlePosition);
                    case MoveType::Constricted:
                        return makeConstrictedHitFinder(initialHandlePosition, currentHandlePosition);
                    case MoveType::Default:
                        return makeDefaultHitFinder(inputState, currentHandlePosition);
                    switchDefault()
                }
            }

            static FindHitPosition makeVerticalHitFinder([[maybe_unused]] const InputState& inputState, const vm::vec3& currentHandlePosition) {
                assert(inputState.camera().perspectiveProjection());

                const auto axis = vm::vec3::pos_z();
                return makeLineHitFinder(vm::line3{currentHandlePosition, axis});
            }

            static FindHitPosition makeConstrictedHitFinder(const vm::vec3& initialHandlePosition, const vm::vec3& currentHandlePosition) {
                const auto delta = currentHandlePosition - initialHandlePosition;
                const auto axis = vm::get_abs_max_component_axis(delta);
                return makeLineHitFinder(vm::line3{initialHandlePosition, axis});
            }

            static FindHitPosition makeDefaultHitFinder(const InputState& inputState, const vm::vec3& currentHandlePosition) {
                const auto& camera = inputState.camera();
                const auto axis = camera.perspectiveProjection() ? vm::vec3::pos_z() : vm::vec3(vm::get_abs_max_component_axis(camera.direction()));
                return makePlaneHitFinder(vm::plane3{currentHandlePosition, axis});
            }
        };

        template <typename Delegate>
        std::unique_ptr<HandleDragTracker<MoveHandleDragDelegate<Delegate>>> createMoveHandleDragTracker(Delegate delegate, const InputState& inputState, const vm::vec3& initialHandlePosition) {
            return std::make_unique<HandleDragTracker<MoveHandleDragDelegate<Delegate>>>(MoveHandleDragDelegate{std::move(delegate)}, inputState, initialHandlePosition);
        }
    }
}

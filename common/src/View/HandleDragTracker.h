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

#include "FloatType.h"
#include "Renderer/Camera.h"
#include "View/DragTracker.h"
#include "View/InputState.h"

#include <vecmath/forward.h>
#include <vecmath/vec.h>

#include <functional>
#include <memory>
#include <optional>
#include <type_traits>

namespace TrenchBroom {
    namespace View {
        /**
         * Computes a handle position from the given input state.
         *
         * Takes the input state, the initial handle position and the last handle position.
         * Returns nullopt if no handle position could be determined
         */
        using GetHandlePosition = std::function<std::optional<vm::vec3>(const InputState&, const vm::vec3&, const vm::vec3&)>;

        struct DragConfig {
            GetHandlePosition getHandlePosition;
            vm::vec3 initialHandlePosition;
            vm::vec3 currentHandlePosition;
        };

        /**
         * The status of a drag. This is returned from a handle drag tracker's delegate when it reacts to a drag event.
         */
        enum class DragStatus {
            /** The drag should continue. */
            Continue,
            /** The drag should continue, but the current event could not be applied to the object being dragged. */
            Deny,
            /** The drag should be cancelled. */
            Cancel
        };

        struct HandleDragTrackerDelegate {
            virtual DragConfig initialize(const InputState& inputState, const vm::vec3& initialHandlePosition) = 0;

            virtual std::optional<DragConfig> modifierKeyChange(const InputState& inputState, const vm::vec3& initialHandlePosition, const vm::vec3& currentHandlePosition);
            virtual void mouseScroll(const InputState& inputState, const vm::vec3& initialHandlePosition, const vm::vec3& currentHandlePosition);

            virtual DragStatus drag(const InputState& inputState, const vm::vec3& initialHandlePosition, const vm::vec3& lastHandlePosition, const vm::vec3& nextHandlePosition) = 0;
            virtual void end(const InputState& inputState, const vm::vec3& initialHandlePosition, const vm::vec3& currentHandlePosition) = 0;
            virtual void cancel(const vm::vec3& initialHandlePosition) = 0;

            virtual void setRenderOptions(const InputState& inputState, Renderer::RenderContext& renderContext) const;
            virtual void render(const InputState& inputState, Renderer::RenderContext& renderContext, Renderer::RenderBatch& renderBatch, const vm::vec3& initialHandlePosition, const vm::vec3& currentHandlePosition) const;
        };

        /**
         * A drag tracker that supports dragging handles.
         *
         * In this context, a handle is a 3D point. This drag tracker keeps track of the initial handle position and the
         * current handle position. The initial handle position is the position that was passed to the constructor. It
         * can be updated if the drag config changes in response to a modifier key change.
         *
         * The current handle position updates in response to calls to drag() or a modifier key change.
         *
         * The delegate's initialize function is called once when this drag tracker is constructed. It must return the
         * drag config to use initially. The delegate's modifierKeyChange function can optionally return a drag config
         * to replace the current drag config.
         */
        template <typename Delegate>
        class HandleDragTracker : public DragTracker {
        private:
            static_assert(std::is_base_of_v<HandleDragTrackerDelegate, Delegate>, "Delegate must extend HandleDragTrackerDelegate");
            Delegate m_delegate;
            DragConfig m_config;
        public:
            /**
             * Creates a new handle drag tracker with the given delegate.
             */
            HandleDragTracker(Delegate delegate, const InputState& inputState, const vm::vec3& initialHandlePosition) : 
            m_delegate{std::move(delegate)},
            m_config{m_delegate.initialize(inputState, initialHandlePosition)} {}

            virtual ~HandleDragTracker() = default;

            const vm::vec3& initialHandlePosition() const {
                return m_config.initialHandlePosition;
            }

            const vm::vec3& currentHandlePosition() const {
                return m_config.currentHandlePosition;
            }

            /**
             * React to modifier key changes. This is delegated to the delegate, and if it returns a new drag config,
             * the drag tracker is reconfigured accordingly by replacing the hit finder, the handle converter, and the
             * initial handle position.
             */
            void modifierKeyChange(const InputState& inputState) override {
                if (auto dragConfig = m_delegate.modifierKeyChange(inputState, m_config.initialHandlePosition, m_config.currentHandlePosition)) {
                    m_config = std::move(*dragConfig);
                    assertResult(drag(inputState));
                }
            }

            /**
             * Forward the scroll event to the delegate.
             */
            void mouseScroll(const InputState& inputState) override {
                m_delegate.mouseScroll(inputState, m_config.initialHandlePosition, m_config.currentHandlePosition);
            }

            /**
             * Called when the mouse is moved during a drag. Delegates to the delegate to apply changes to the objects
             * being dragged.
             *
             * Returns true to indicate succes. If this function returns false, the drag is cancelled.
             */
            bool drag(const InputState& inputState) override {
                const auto newHandlePosition = m_config.getHandlePosition(inputState, m_config.initialHandlePosition, m_config.currentHandlePosition);
                if (!newHandlePosition || *newHandlePosition == m_config.currentHandlePosition) {
                    return true;
                }

                const auto dragResult = m_delegate.drag(inputState, m_config.initialHandlePosition, m_config.currentHandlePosition, *newHandlePosition);
                if (dragResult == DragStatus::Cancel) {
                    return false;
                }

                if (dragResult == DragStatus::Continue) {
                    m_config.currentHandlePosition = *newHandlePosition;
                }

                return true;
            }

            /**
             * Called when the drag ends normally (e.g. by releasing a mouse button).
             */
            void end(const InputState& inputState) override {
                m_delegate.end(inputState, m_config.initialHandlePosition, m_config.currentHandlePosition);
            }

            /**
             * Called when the drag ends abnormally (e.g. by hitting escape during a drag). The delegate should undo any
             * changes made in result of the drag.
             */
            void cancel() override {
                m_delegate.cancel(m_config.initialHandlePosition);
            }

            void setRenderOptions(const InputState& inputState, Renderer::RenderContext& renderContext) const override {
                m_delegate.setRenderOptions(inputState, renderContext);
            }

            /**
             * Called during the drag to allow the drag tracker to render into the corresponding view. This is simply
             * forwarded to the delegate.
             */
            void render(const InputState& inputState, Renderer::RenderContext& renderContext, Renderer::RenderBatch& renderBatch) const override {
                m_delegate.render(inputState, renderContext, renderBatch, m_config.initialHandlePosition, m_config.currentHandlePosition);
            }
        };

        template <typename Delegate>
        std::unique_ptr<HandleDragTracker<Delegate>> createHandleDragTracker(Delegate delegate, const InputState& inputState, const vm::vec3& initialHandlePosition) {
            return std::make_unique<HandleDragTracker<Delegate>>(std::move(delegate), inputState, initialHandlePosition);
        }

        class Grid;

        /**
         * Finds a hit position for the given input state.
         */
        using FindHitPosition = std::function<std::optional<vm::vec3>(const InputState&)>;
        FindHitPosition makeLineHitFinder(const vm::line3& line);
        FindHitPosition makePlaneHitFinder(const vm::plane3& plane);
        FindHitPosition makeCircleHitFinder(const vm::vec3& center, const vm::vec3& normal, FloatType radius);

        /**
         * Converts the input state, an initial handle position, a last handle position, and a current hit position to a handle position.
         */
        using ConvertHitToHandlePosition = std::function<std::optional<vm::vec3>(const InputState&, const vm::vec3&, const vm::vec3&, const vm::vec3&)>;
        ConvertHitToHandlePosition makeIdentitySnapper();
        ConvertHitToHandlePosition makeDeltaSnapper(const Grid& grid);
        ConvertHitToHandlePosition makeCircleSnapper(const Grid& grid, FloatType snapAngle, const vm::vec3& center, const vm::vec3& normal, FloatType radius);

        /**
         * Composes a hit finder and a handle position converter to a function that can be used by a handle drag tracker.
         *
         * This is often useful because finding a hit and converting its position to a handle position are separate
         * operations: the hit finder might change, but the handle converter is still the same, e.g. a function that
         * just snaps the hit position to the grid, regardless of how the hit position was found.
         */
        GetHandlePosition makeGetHandlePosition(FindHitPosition findHitPosition, ConvertHitToHandlePosition convertHitToHandlePosition);
    }
}

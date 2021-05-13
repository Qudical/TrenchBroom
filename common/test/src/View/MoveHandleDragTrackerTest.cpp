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

#include "Renderer/PerspectiveCamera.h"
#include "Renderer/OrthographicCamera.h"
#include "View/MoveHandleDragTracker.h"
#include "View/PickRequest.h"

#include <vecmath/approx.h>
#include <vecmath/vec.h>
#include <vecmath/vec_io.h>
#include <vecmath/ray.h>
#include <vecmath/ray_io.h>

#include "Catch2.h"

namespace TrenchBroom {
    namespace View {
        template <typename Move, typename End, typename Cancel, typename Render, typename MakeHandleConverter>
        struct TestDelegate : public MoveHandleDragTrackerDelegate {
            Move m_move;
            End m_end;
            Cancel m_cancel;
            Render m_render;
            MakeHandleConverter m_makeHandleConverter;

            TestDelegate(Move i_move, End i_end, Cancel i_cancel, Render i_render, MakeHandleConverter i_makeHandleConverter) :
            m_move{std::move(i_move)},
            m_end{std::move(i_end)},
            m_cancel{std::move(i_cancel)},
            m_render{std::move(i_render)},
            m_makeHandleConverter{std::move(i_makeHandleConverter)} {}

            DragStatus move(const InputState& inputState, const vm::vec3& lastHandlePosition, const vm::vec3& currentHandlePosition) {
                return m_move(inputState, lastHandlePosition, currentHandlePosition);
            }

            void end(const InputState& inputState, const vm::vec3& initialHandlePosition, const vm::vec3& currentHandlePosition) {
                m_end(inputState, initialHandlePosition, currentHandlePosition);
            }

            void cancel(const vm::vec3& initialHandlePosition) {
                m_cancel(initialHandlePosition);
            }


            void render(const InputState& inputState, Renderer::RenderContext& renderContext, Renderer::RenderBatch& renderBatch) const {
                m_render(inputState, renderContext, renderBatch);
            }


            ConvertHitToHandlePosition makeHandleConverter(const InputState& inputState) const {
                return m_makeHandleConverter(inputState);
            }

        };

        static auto makeMoveTracker(const InputState& inputState, const vm::vec3& initialHandlePosition) {
            auto move = [&](const InputState&, const vm::vec3& /* lastHandlePosition */, const vm::vec3& /* nextHandlePosition */) -> DragStatus { 
                return DragStatus::Continue;
            };
            auto end = [](const InputState&, const vm::vec3& /* initialHandlePosition */, const vm::vec3& /* currentHandlePosition */) {};
            auto cancel = [](const vm::vec3& /* initialHandlePosition */) {};
            auto render = [](const InputState&, Renderer::RenderContext&, Renderer::RenderBatch&) {};
            auto makeHandleConverter = [](const InputState&) {
                return [](const InputState&, const vm::vec3&, const vm::vec3&, const vm::vec3& currentHitPosition) { return currentHitPosition; };
            };

            auto delegate = TestDelegate<decltype(move), decltype(end), decltype(cancel), decltype(render), decltype(makeHandleConverter)>{
                std::move(move),
                std::move(end),
                std::move(cancel),
                std::move(render),
                std::move(makeHandleConverter)
            };

            return HandleDragTracker<MoveHandleDragDelegate<decltype(delegate)>>{MoveHandleDragDelegate{std::move(delegate)}, inputState, initialHandlePosition};
        }

        static auto makeInputState(const vm::vec3& rayOrigin, const vm::vec3& rayDirection, Renderer::Camera& camera, ModifierKeyState modifierKeys = ModifierKeys::MKNone) {
            auto inputState = InputState{};
            inputState.setPickRequest(PickRequest{vm::ray3{rayOrigin, vm::normalize(rayDirection)}, camera});
            inputState.setModifierKeys(modifierKeys);
            return inputState;
        }

        TEST_CASE("MoveDragTracker.constructor") {
            constexpr auto initialHandlePosition = vm::vec3{0, 64, 0};

            GIVEN("A 3D camera") {
                auto camera3d = Renderer::PerspectiveCamera{};

                WHEN("A tracker is created without any modifier keys pressed") {
                    auto tracker = makeMoveTracker(makeInputState(vm::vec3{0, 0, 64}, vm::vec3{0, 1, -1}, camera3d), initialHandlePosition);
                    
                    THEN("The tracker has set the initial and current handle positions correctly") {
                        CHECK(tracker.initialHandlePosition() == initialHandlePosition);
                        CHECK(tracker.currentHandlePosition() == initialHandlePosition);

                        AND_THEN("The tracker is using a default hit finder") {
                            // we check this indirectly by observing how the move handle position changes when dragging
                            REQUIRE(tracker.drag(makeInputState(vm::vec3{16, 16, 64}, vm::vec3{0, 1, -1}, camera3d)));
                            CHECK(tracker.currentHandlePosition() == vm::vec3{16, 80, 0});
                        }
                    }
                }

                WHEN("A tracker is created with the alt modifier pressed") {
                    auto tracker = makeMoveTracker(makeInputState(vm::vec3{0, 0, 64}, vm::vec3{0, 1, -1}, camera3d, ModifierKeys::MKAlt), initialHandlePosition);

                    THEN("The tracker is using a vertical hit finder") {
                        // we check this indirectly by observing how the move handle position changes when dragging
                        REQUIRE(tracker.drag(makeInputState(vm::vec3{16, 16, 64}, vm::vec3{0, 1, -1}, camera3d)));
                        CHECK(tracker.currentHandlePosition() == vm::approx{vm::vec3{0, 64, 16}});
                    }
                }
            }

            GIVEN("A 2D camera") {
                auto camera2d = Renderer::OrthographicCamera{};
                camera2d.moveTo(vm::vec3f{0, 0, 64});
                camera2d.lookAt(vm::vec3f{0, 0, -1}, vm::vec3f{0, 1, 0});

                WHEN("A tracker is created without any modifier keys pressed") {
                    auto tracker = makeMoveTracker(makeInputState(vm::vec3{0, 64, 64}, vm::vec3{0, 0, -1}, camera2d), initialHandlePosition);
                    
                    THEN("The tracker has set the initial and current handle positions correctly") {
                        CHECK(tracker.initialHandlePosition() == initialHandlePosition);
                        CHECK(tracker.currentHandlePosition() == initialHandlePosition);

                        AND_THEN("The tracker is using a default hit finder") {
                            // we check this indirectly by observing how the move handle position changes when dragging
                            REQUIRE(tracker.drag(makeInputState(vm::vec3{16, 80, 64}, vm::vec3{0, 0, -1}, camera2d)));
                            CHECK(tracker.currentHandlePosition() == vm::vec3{16, 80, 0});
                        }
                    }
                }

                WHEN("A tracker is created with the alt modifier pressed") {
                    auto tracker = makeMoveTracker(makeInputState(vm::vec3{0, 0, 64}, vm::vec3{0, 1, -1}, camera2d, ModifierKeys::MKAlt), initialHandlePosition);

                    THEN("The tracker is using a default hit finder") {
                            // we check this indirectly by observing how the move handle position changes when dragging
                            REQUIRE(tracker.drag(makeInputState(vm::vec3{16, 80, 64}, vm::vec3{0, 0, -1}, camera2d)));
                            CHECK(tracker.currentHandlePosition() == vm::vec3{16, 80, 0});
                    }
                }
            }
        }

        TEST_CASE("MoveDragTracker.modifierKeyChange") {
            constexpr auto initialHandlePosition = vm::vec3{0, 64, 0};

            GIVEN("A tracker created with a 3D camera") {
                auto camera3d = Renderer::PerspectiveCamera{};
                auto tracker = makeMoveTracker(makeInputState(vm::vec3{0, 0, 64}, vm::vec3{0, 1, -1}, camera3d), initialHandlePosition);
                REQUIRE(tracker.initialHandlePosition() == initialHandlePosition);

                WHEN("The alt modifier is pressed") {
                    tracker.modifierKeyChange(makeInputState(vm::vec3{0, 0, 64}, vm::vec3{0, 1, -1}, camera3d, ModifierKeys::MKAlt));

                    THEN("The tracker switches to a vertical hit finder") {
                        // we check this indirectly by observing how the move handle position changes when dragging
                        REQUIRE(tracker.drag(makeInputState(vm::vec3{16, 16, 64}, vm::vec3{0, 1, -1}, camera3d)));
                        CHECK(tracker.initialHandlePosition() == initialHandlePosition);
                        CHECK(tracker.currentHandlePosition() == vm::approx{vm::vec3{0, 64, 16}});
                    }

                    AND_WHEN("The alt modifier is released") {
                        tracker.modifierKeyChange(makeInputState(vm::vec3{0, 0, 64}, vm::vec3{0, 1, -1}, camera3d));

                        THEN("The tracker switches to a default hit finder") {
                            // we check this indirectly by observing how the move handle position changes when dragging
                            REQUIRE(tracker.drag(makeInputState(vm::vec3{16, 16, 64}, vm::vec3{0, 1, -1}, camera3d)));
                            CHECK(tracker.initialHandlePosition() == vm::vec3{0, 64, 0});
                            CHECK(tracker.currentHandlePosition() == vm::vec3{16, 80, 0});
                        }
                    }
                }

                WHEN("The shift modifier is pressed before the handle is moved") {
                    tracker.modifierKeyChange(makeInputState(vm::vec3{0, 0, 64}, vm::vec3{0, 1, -1}, camera3d, ModifierKeys::MKShift));

                    THEN("The tracker still has a default hit finder") {
                        // we check this indirectly by observing how the move handle position changes when dragging
                        REQUIRE(tracker.drag(makeInputState(vm::vec3{16, 16, 64}, vm::vec3{0, 1, -1}, camera3d)));
                        CHECK(tracker.initialHandlePosition() == initialHandlePosition);
                        CHECK(tracker.currentHandlePosition() == vm::vec3{16, 80, 0});
                    }
                }

                WHEN("The shift modifier is pressed after the handle is moved diagonally") {
                    REQUIRE(tracker.drag(makeInputState(vm::vec3{16, 16, 64}, vm::vec3{0, 1, -1}, camera3d)));
                    REQUIRE(tracker.initialHandlePosition() == initialHandlePosition);
                    REQUIRE(tracker.currentHandlePosition() == vm::vec3{16, 80, 0});

                    tracker.modifierKeyChange(makeInputState(vm::vec3{16, 16, 64}, vm::vec3{0, 1, -1}, camera3d, ModifierKeys::MKShift));

                    THEN("The tracker still has a default hit finder") {
                        // we check this indirectly by observing how the move handle position changes when dragging
                        CHECK(tracker.initialHandlePosition() == initialHandlePosition);
                        CHECK(tracker.currentHandlePosition() == vm::vec3{16, 80, 0});
                    }
                }

                WHEN("The shift modifier is pressed after the handle is moved non-diagonally") {
                    REQUIRE(tracker.drag(makeInputState(vm::vec3{16, 32, 64}, vm::vec3{0, 1, -1}, camera3d)));
                    REQUIRE(tracker.initialHandlePosition() == initialHandlePosition);
                    REQUIRE(tracker.currentHandlePosition() == vm::vec3{16, 96, 0});

                    tracker.modifierKeyChange(makeInputState(vm::vec3{16, 32, 64}, vm::vec3{0, 1, -1}, camera3d, ModifierKeys::MKShift));

                    THEN("The tracker has a constricted hit finder") {
                        // we check this indirectly by observing how the move handle position changes when dragging
                        CHECK(tracker.initialHandlePosition() == vm::vec3{0, 64, 0});
                        CHECK(tracker.currentHandlePosition() == vm::vec3{0, 96, 0});
                    }

                    AND_WHEN("The shift modifier is released") {
                        tracker.modifierKeyChange(makeInputState(vm::vec3{16, 32, 64}, vm::vec3{0, 1, -1}, camera3d));
                        
                        THEN("The tracker switches back to a default hit finder") {
                            // we check this indirectly by observing how the move handle position changes when dragging
                            CHECK(tracker.initialHandlePosition() == vm::vec3{0, 64, 0});
                            CHECK(tracker.currentHandlePosition() == vm::vec3{16, 96, 0});
                        }
                    }
                }
            }

            GIVEN("A tracker created with a 3D camera") {
                auto camera2d = Renderer::OrthographicCamera{};
                camera2d.moveTo(vm::vec3f{0, 0, 64});
                camera2d.lookAt(vm::vec3f{0, 0, -1}, vm::vec3f{0, 1, 0});

                auto tracker = makeMoveTracker(makeInputState(vm::vec3{0, 0, 64}, vm::vec3{0, 1, -1}, camera2d), initialHandlePosition);
                REQUIRE(tracker.initialHandlePosition() == initialHandlePosition);

                WHEN("The alt modifier is pressed") {
                    tracker.modifierKeyChange(makeInputState(vm::vec3{0, 64, 64}, vm::vec3{0, 0, -1}, camera2d, ModifierKeys::MKAlt));

                    THEN("The tracker does not change the hit finder") {
                        // we check this indirectly by observing how the move handle position changes when dragging
                        REQUIRE(tracker.drag(makeInputState(vm::vec3{16, 80, 64}, vm::vec3{0, 0, -1}, camera2d)));
                        CHECK(tracker.initialHandlePosition() == initialHandlePosition);
                        CHECK(tracker.currentHandlePosition() == vm::vec3{16, 80, 0});
                    }
                }
            }
        }
    }
}

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

#include <vecmath/vec.h>
#include <vecmath/vec_io.h>

#include <tuple>
#include <vector>

#include "Catch2.h"

namespace TrenchBroom {
    namespace View {
        template <typename ModifierKeyChange, typename Initialize, typename Drag, typename End, typename Cancel, typename Render>
        struct TestDelegate : public HandleDragTrackerDelegate {
            ModifierKeyChange m_modifierKeyChange;
            Initialize m_initialize;
            Drag m_drag;
            End m_end;
            Cancel m_cancel;
            Render m_render;

            TestDelegate(ModifierKeyChange i_modifierKeyChange, Initialize i_initialize, Drag i_drag, End i_end, Cancel i_cancel, Render i_render) :
            m_modifierKeyChange{std::move(i_modifierKeyChange)},
            m_initialize{std::move(i_initialize)},
            m_drag{std::move(i_drag)},
            m_end{std::move(i_end)},
            m_cancel{std::move(i_cancel)},
            m_render{std::move(i_render)} {}

            std::optional<DragConfig> modifierKeyChange(const InputState& inputState, const vm::vec3& initialHandlePosition, const vm::vec3& currentHandlePosition) {
                return m_modifierKeyChange(inputState, initialHandlePosition, currentHandlePosition);
            }

            DragConfig initialize(const InputState& inputState, const vm::vec3& initialHandlePosition) {
                return m_initialize(inputState, initialHandlePosition);
            }

            DragStatus drag(const InputState& inputState, const vm::vec3& initialHandlePosition, const vm::vec3& lastHandlePosition, const vm::vec3& nextHandlePosition) {
                return m_drag(inputState, initialHandlePosition, lastHandlePosition, nextHandlePosition);
            }

            void end(const InputState& inputState, const vm::vec3& initialHandlePosition, const vm::vec3& currentHandlePosition) {
                m_end(inputState, initialHandlePosition, currentHandlePosition);
            }

            void cancel(const vm::vec3& initialHandlePosition) {
                m_cancel(initialHandlePosition);
            }

            void render(const InputState& inputState, Renderer::RenderContext& renderContext, Renderer::RenderBatch& renderBatch, const vm::vec3& initialHandlePosition, const vm::vec3& currentHandlePosition) const {
                m_render(inputState, renderContext, renderBatch, initialHandlePosition, currentHandlePosition);
            }
        };

        template <typename ModifierKeyChange, typename Initialize, typename Drag, typename End, typename Cancel, typename Render>
        static auto makeTestDelegate(ModifierKeyChange modifierKeyChange, Initialize initialize, Drag drag, End end, Cancel cancel, Render render) {
            return TestDelegate<ModifierKeyChange, Initialize, Drag, End, Cancel, Render>{
                std::move(modifierKeyChange),
                std::move(initialize),
                std::move(drag),
                std::move(end),
                std::move(cancel),
                std::move(render)
            };
        }

        TEST_CASE("RestrictedDragTracker.constructor") {
            GIVEN("A delegate") {
                const auto initialHandlePosition = vm::vec3{3, 2, 1};

                auto tracker = HandleDragTracker{makeTestDelegate(
                    // modifierKeyChange
                    [](const InputState&, const vm::vec3& /* initialHandlePosition */, const vm::vec3& /* currentHandlePosition */) -> std::optional<DragConfig> { return std::nullopt; },
                    // initialize
                    [](const InputState&, const vm::vec3& initialHandlePosition_) {
                        return DragConfig {
                            // always returns the same handle position
                            [](const auto&, const auto&, const auto&) { return vm::vec3{1, 2, 3}; },
                            initialHandlePosition_,
                            initialHandlePosition_
                        };
                    },
                    // drag
                    [&](const InputState&, const vm::vec3& /* initialHandlePosition */, const vm::vec3& /* lastHandlePosition */, const vm::vec3& /* nextHandlePosition */) { 
                        return DragStatus::Continue;
                    },
                    // end
                    [](const InputState&, const vm::vec3& /* initialHandlePosition */, const vm::vec3& /* currentHandlePosition */) {},
                    // cancel
                    [](const vm::vec3& /* initialHandlePosition */) {},
                    // render
                    [](const InputState&, Renderer::RenderContext&, Renderer::RenderBatch&, const vm::vec3&, const vm::vec3&) {}
                ), InputState{}, initialHandlePosition};

                THEN("The initial and current handle positions are set correctly") {
                    CHECK(tracker.initialHandlePosition() == initialHandlePosition);
                    CHECK(tracker.currentHandlePosition() == initialHandlePosition);
                }
            }
        }

        TEST_CASE("RestrictedDragTracker.drag") {
            GIVEN("A drag tracker") {
                const auto initialHandlePosition = vm::vec3{1, 1, 1};
                auto handlePositionToReturn = vm::vec3{};

                auto dragArguments = std::vector<std::tuple<vm::vec3, vm::vec3, vm::vec3>>{};
                auto dragStatusToReturn = DragStatus::Continue;

                auto tracker = HandleDragTracker{makeTestDelegate(
                    // modifierKeyChange
                    [](const InputState&, const vm::vec3& /* initialHandlePosition */, const vm::vec3& /* currentHandlePosition */) -> std::optional<DragConfig> { return std::nullopt; },
                    // initialize
                    [&](const InputState&, const vm::vec3& initialHandlePosition_) {
                        return DragConfig{
                            // returns the handle position set above
                            [&](const auto&, const auto&, const auto&) { return handlePositionToReturn; },
                            initialHandlePosition_,
                            initialHandlePosition_
                        };
                    },
                    // drag
                    [&](const InputState&, const vm::vec3& initialHandlePosition_, const vm::vec3& lastHandlePosition, const vm::vec3& nextHandlePosition) { 
                        dragArguments.emplace_back(initialHandlePosition_, lastHandlePosition, nextHandlePosition);
                        return dragStatusToReturn;
                    },
                    // end
                    [](const InputState&, const vm::vec3& /* initialHandlePosition */, const vm::vec3& /* currentHandlePosition */) {},
                    // cancel
                    [](const vm::vec3& /* initialHandlePosition */) {},
                    // render
                    [](const InputState&, Renderer::RenderContext&, Renderer::RenderBatch&, const vm::vec3&, const vm::vec3&) {}
                ), InputState{}, initialHandlePosition};

                REQUIRE(tracker.initialHandlePosition() == initialHandlePosition);
                REQUIRE(tracker.currentHandlePosition() == initialHandlePosition);

                WHEN("drag is called for the first time after the drag started") {
                    handlePositionToReturn = vm::vec3{2, 2, 2};
                    REQUIRE(tracker.drag(InputState{}));

                    THEN("drag got the initial and the next handle positions") {
                        CHECK(dragArguments.back() == std::make_tuple(vm::vec3{1, 1, 1}, vm::vec3{1, 1, 1}, vm::vec3{2, 2, 2}));

                        AND_WHEN("drag is called again") {
                            handlePositionToReturn = vm::vec3{3, 3, 3};
                            REQUIRE(tracker.drag(InputState{}));

                            THEN("drag got the last and the next handle positions") {
                                CHECK(dragArguments.back() == std::make_tuple(vm::vec3{1, 1, 1}, vm::vec3{2, 2, 2}, vm::vec3{3, 3, 3}));
                            }
                        }
                    }
                }

                WHEN("drag returns drag status deny") {
                    handlePositionToReturn = vm::vec3{2, 2, 2};
                    dragStatusToReturn = DragStatus::Deny;
                    REQUIRE(tracker.drag(InputState{}));

                    THEN("drag got the initial and the next handle positions") {
                        CHECK(dragArguments.back() == std::make_tuple(vm::vec3{1, 1, 1}, vm::vec3{1, 1, 1}, vm::vec3{2, 2, 2}));

                        AND_WHEN("drag is called again") {
                            handlePositionToReturn = vm::vec3{3, 3, 3};
                            REQUIRE(tracker.drag(InputState{}));

                            THEN("drag got the initial and the next handle positions") {
                                CHECK(dragArguments.back() == std::make_tuple(vm::vec3{1, 1, 1}, vm::vec3{1, 1, 1}, vm::vec3{3, 3, 3}));
                            }
                        }
                    }
                }

                WHEN("drag returns drag status cancel") {
                    handlePositionToReturn = vm::vec3{2, 2, 2};
                    dragStatusToReturn = DragStatus::Cancel;
                    const auto dragResult = tracker.drag(InputState{});

                    THEN("the drag tracker returns false") {
                        CHECK_FALSE(dragResult);
                    }
                }
            }
        }

        TEST_CASE("RestrictedDragTracker.handlePositionComputations") {
            const auto initialHandlePosition = vm::vec3{1, 1, 1};

            auto getHandlePositionArguments = std::vector<std::tuple<vm::vec3, vm::vec3>>{};
            auto handlePositionToReturn = vm::vec3{};

            GIVEN("A drag tracker") {
                auto tracker = HandleDragTracker{makeTestDelegate(
                    // modifierKeyChange
                    [](const InputState&, const vm::vec3& /* initialHandlePosition */, const vm::vec3& /* currentHandlePosition */) -> std::optional<DragConfig> { return std::nullopt; },
                    // initialize
                    [&](const InputState&, const vm::vec3& initialHandlePosition_) {
                        return DragConfig{
                            // returns the handle position set above
                            [&](const auto&, const auto& initialHandlePosition__, const auto& lastHandlePosition) {
                                getHandlePositionArguments.emplace_back(initialHandlePosition__, lastHandlePosition);
                                return handlePositionToReturn;
                            },
                            initialHandlePosition_,
                            initialHandlePosition_
                        };
                    },
                    // drag
                    [&](const InputState&, const vm::vec3& /* initialHandlePosition */, const vm::vec3& /* lastHandlePosition */, const vm::vec3& /* nextHandlePosition */) { 
                        return DragStatus::Continue;
                    },
                    // end
                    [](const InputState&, const vm::vec3& /* initialHandlePosition */, const vm::vec3& /* currentHandlePosition */) {},
                    // cancel
                    [](const vm::vec3& /* initialHandlePosition */) {},
                    // render
                    [](const InputState&, Renderer::RenderContext&, Renderer::RenderBatch&, const vm::vec3&, const vm::vec3&) {}
                ), InputState{}, initialHandlePosition};

                REQUIRE(getHandlePositionArguments.empty());

                WHEN("drag is called for the first time") {
                    handlePositionToReturn = vm::vec3{2, 2, 2};

                    tracker.drag(InputState{});

                    THEN("getHandlePosition is called with the expected arguments") {
                        CHECK(getHandlePositionArguments == std::vector<std::tuple<vm::vec3, vm::vec3>>{
                            {vm::vec3{1, 1, 1}, vm::vec3{1, 1, 1}}
                        });
                    }

                    AND_WHEN("drag is called again") {
                        handlePositionToReturn = vm::vec3{3, 3, 3};

                        tracker.drag(InputState{});

                        THEN("getHandlePosition is called with the expected arguments") {
                            CHECK(getHandlePositionArguments == std::vector<std::tuple<vm::vec3, vm::vec3>>{
                                {vm::vec3{1, 1, 1}, vm::vec3{1, 1, 1}},
                                {vm::vec3{1, 1, 1}, vm::vec3{2, 2, 2}},
                            });
                        }
                    }
                }
            }
        }

        TEST_CASE("RestrictedDragTracker.modifierKeyChange") {
            const auto initialHandlePosition = vm::vec3{1, 1, 1};

            size_t initialDragConfigGetHandlePositionCallCount = 0;

            auto modifierKeyChangeParameters = std::vector<std::tuple<vm::vec3, vm::vec3>>{};

            GIVEN("A delegate that returns null from modifierKeyChange") {
                auto tracker = HandleDragTracker{makeTestDelegate(
                    // modifierKeyChange
                    [&](const InputState&, const vm::vec3& initialHandlePosition_, const vm::vec3& currentHandlePosition) -> std::optional<DragConfig> { 
                        modifierKeyChangeParameters.emplace_back(initialHandlePosition_, currentHandlePosition);
                        return std::nullopt;
                    },
                    // initialize
                    [&](const InputState&, const vm::vec3& initialHandlePosition_) {
                        return DragConfig{
                            // returns the handle position set above
                            [&](const auto&, const auto& /* initialHandlePosition */, const auto& /* lastHandlePosition */) {
                                ++initialDragConfigGetHandlePositionCallCount;
                                return vm::vec3{2, 2, 2};
                            },
                            initialHandlePosition_,
                            initialHandlePosition_
                        };
                    },
                    // drag
                    [&](const InputState&, const vm::vec3& /* initialHandlePosition */, const vm::vec3& /* lastHandlePosition */, const vm::vec3& /* nextHandlePosition */) { 
                        return DragStatus::Continue;
                    },
                    // end
                    [](const InputState&, const vm::vec3& /* initialHandlePosition */, const vm::vec3& /* currentHandlePosition */) {},
                    // cancel
                    [](const vm::vec3& /* initialHandlePosition */) {},
                    // render
                    [](const InputState&, Renderer::RenderContext&, Renderer::RenderBatch&, const vm::vec3&, const vm::vec3&) {}
                ), InputState{}, initialHandlePosition};

                REQUIRE(initialDragConfigGetHandlePositionCallCount == 0);

                tracker.drag(InputState{});
                REQUIRE(initialDragConfigGetHandlePositionCallCount == 1);

                WHEN("A modifier key change is notified") {
                    tracker.modifierKeyChange(InputState{});
                    
                    THEN("The initial and current handle positions are passed to the delegate") {
                        CHECK(modifierKeyChangeParameters == std::vector<std::tuple<vm::vec3, vm::vec3>>{
                            {vm::vec3{1, 1, 1}, vm::vec3{2, 2, 2}}
                        });
                        
                        AND_THEN("The next call to drag uses the initial drag config") {
                            tracker.drag(InputState{});
                            CHECK(initialDragConfigGetHandlePositionCallCount == 2);
                        }
                    }
                }
            }

            GIVEN("A delegate that returns a new drag config from modifierKeyChange") {
                size_t otherDragConfigGetHandlePositionCallCount = 0;

                auto otherHandlePositionToReturn = vm::vec3{};
                auto dragArguments = std::vector<std::tuple<vm::vec3, vm::vec3, vm::vec3>>{};

                auto tracker = HandleDragTracker{makeTestDelegate(
                    // modifierKeyChange
                    [&](const InputState&, const vm::vec3& initialHandlePosition_, const vm::vec3& currentHandlePosition) -> std::optional<DragConfig> {
                        modifierKeyChangeParameters.emplace_back(initialHandlePosition_, currentHandlePosition);
                        return DragConfig{
                            [&](const auto&, const auto& /* initialHandlePosition */, const auto& /* lastHandlePosition */) {
                                ++otherDragConfigGetHandlePositionCallCount;
                                return otherHandlePositionToReturn;
                            },
                            currentHandlePosition,
                            currentHandlePosition
                        };
                    },
                    // initialize
                    [&](const InputState&, const vm::vec3& initialHandlePosition_) {
                        return DragConfig{
                            [&](const auto&, const auto& /* initialHandlePosition */, const auto& /* lastHandlePosition */) {
                                ++initialDragConfigGetHandlePositionCallCount;
                                return vm::vec3{2, 2, 2};
                            },
                            initialHandlePosition_,
                            initialHandlePosition_
                        };
                    },
                    // drag
                    [&](const InputState&, const vm::vec3& initialHandlePosition_, const vm::vec3& lastHandlePosition, const vm::vec3& nextHandlePosition) { 
                        dragArguments.emplace_back(initialHandlePosition_, lastHandlePosition, nextHandlePosition);
                        return DragStatus::Continue;
                    },
                    // end
                    [](const InputState&, const vm::vec3& /* initialHandlePosition */, const vm::vec3& /* currentHandlePosition */) {},
                    // cancel
                    [](const vm::vec3& /* initialHandlePosition */) {},
                    // render
                    [](const InputState&, Renderer::RenderContext&, Renderer::RenderBatch&, const vm::vec3&, const vm::vec3&) {}
                ), InputState{}, initialHandlePosition};


                REQUIRE(initialDragConfigGetHandlePositionCallCount == 0);
                REQUIRE(otherDragConfigGetHandlePositionCallCount == 0);

                tracker.drag(InputState{});
                REQUIRE(initialDragConfigGetHandlePositionCallCount == 1);
                REQUIRE(dragArguments == std::vector<std::tuple<vm::vec3, vm::vec3, vm::vec3>>{
                    { vm::vec3{1, 1, 1}, vm::vec3{1, 1, 1}, vm::vec3{2, 2, 2} },
                });

                WHEN("A modifier key change is notified") {
                    otherHandlePositionToReturn = vm::vec3{3, 3, 3};
                    tracker.modifierKeyChange(InputState{});
                    
                    THEN("The initial and current handle positions are passed to the delegate") {
                        CHECK(modifierKeyChangeParameters == std::vector<std::tuple<vm::vec3, vm::vec3>>{
                            {vm::vec3{1, 1, 1}, vm::vec3{2, 2, 2}}
                        });

                        AND_THEN("A synthetic drag to the new handle position happens using the other drag config") {
                            CHECK(initialDragConfigGetHandlePositionCallCount == 1);
                            CHECK(otherDragConfigGetHandlePositionCallCount == 1);

                            CHECK(dragArguments == std::vector<std::tuple<vm::vec3, vm::vec3, vm::vec3>>{
                                { vm::vec3{1, 1, 1}, vm::vec3{1, 1, 1}, vm::vec3{2, 2, 2} },
                                { vm::vec3{1, 1, 1}, vm::vec3{2, 2, 2}, vm::vec3{3, 3, 3} },
                            });
                        }
                        
                        AND_WHEN("drag is called again") {
                            otherHandlePositionToReturn = vm::vec3{4, 4, 4};
                            tracker.drag(InputState{});
                            
                            AND_THEN("The other handle position is passed") {
                                CHECK(dragArguments == std::vector<std::tuple<vm::vec3, vm::vec3, vm::vec3>>{
                                    { vm::vec3{1, 1, 1}, vm::vec3{1, 1, 1}, vm::vec3{2, 2, 2} },
                                    { vm::vec3{1, 1, 1}, vm::vec3{2, 2, 2}, vm::vec3{3, 3, 3} },
                                    { vm::vec3{1, 1, 1}, vm::vec3{3, 3, 3}, vm::vec3{4, 4, 4} }
                                });

                                AND_THEN("The other drag config was used") {
                                    CHECK(initialDragConfigGetHandlePositionCallCount == 1);
                                    CHECK(otherDragConfigGetHandlePositionCallCount == 2);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

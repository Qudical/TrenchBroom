/*
 Copyright (C) 2010-2016 Kristian Duske
 
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

#include "TestUtils.h"

#include <cmath>
#include <gmock/gmock.h>

namespace TrenchBroom {
    bool texCoordsEqual(const Vec2f& tc1, const Vec2f& tc2) {
        for (size_t i = 0; i < 2; ++i) {
            const float dist = fabsf(tc1[i] - tc2[i]);
            const float distRemainder = dist - floorf(dist);
            
            if (!(Math::eq(0.0f, distRemainder) || Math::eq(1.0f, distRemainder)))
                return false;
        }
        return true;
    }
    
    bool pointExactlyIntegral(const Vec3d &point) {
        for (size_t i=0; i<3; i++) {
            const double value = point[i];
            if (static_cast<double>(static_cast<int>(value)) != value) {
                return false;
            }
        }
        return true;
    }
    
    TEST(TestUtilsTest, testTexCoordsEqual) {
        ASSERT_TRUE(texCoordsEqual(Vec2f(0.0, 0.0), Vec2f(0.0, 0.0)));
        ASSERT_TRUE(texCoordsEqual(Vec2f(0.0, 0.0), Vec2f(1.0, 0.0)));
        ASSERT_TRUE(texCoordsEqual(Vec2f(0.0, 0.0), Vec2f(2.00001, 0.0)));
        ASSERT_TRUE(texCoordsEqual(Vec2f(0.0, 0.0), Vec2f(-10.0, 2.0)));
        ASSERT_TRUE(texCoordsEqual(Vec2f(2.0, -3.0), Vec2f(-10.0, 2.0)));
        ASSERT_TRUE(texCoordsEqual(Vec2f(-2.0, -3.0), Vec2f(-10.0, 2.0)));
        ASSERT_TRUE(texCoordsEqual(Vec2f(0.0, 0.0), Vec2f(-1.0, 1.0)));
        ASSERT_TRUE(texCoordsEqual(Vec2f(0.0, 0.0), Vec2f(-0.00001, 0.0)));
        ASSERT_TRUE(texCoordsEqual(Vec2f(0.25, 0.0), Vec2f(-0.75, 0.0)));
        
        ASSERT_FALSE(texCoordsEqual(Vec2f(0.0, 0.0), Vec2f(0.1, 0.1)));
        ASSERT_FALSE(texCoordsEqual(Vec2f(-0.25, 0.0), Vec2f(0.25, 0.0)));
    }
    
    TEST(TestUtilsTest, pointExactlyIntegral) {
        ASSERT_TRUE(pointExactlyIntegral(Vec3d(0.0, 0.0, 0.0)));
        ASSERT_TRUE(pointExactlyIntegral(Vec3d(1024.0, 1204.0, 1024.0)));
        ASSERT_TRUE(pointExactlyIntegral(Vec3d(-10000.0, -10000.0, -10000.0)));
        
        const double near1024 = nextafter(1024.0, 1025.0);
        ASSERT_FALSE(pointExactlyIntegral(Vec3d(1024.0, near1024, 1024.0)));
        ASSERT_FALSE(pointExactlyIntegral(Vec3d(1024.5, 1024.5, 1024.5)));
    }
}

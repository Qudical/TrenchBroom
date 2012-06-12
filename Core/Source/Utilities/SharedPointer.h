/*
 Copyright (C) 2010-2012 Kristian Duske

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
 along with TrenchBroom.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TrenchBroom_SharedPointer_h
#define TrenchBroom_SharedPointer_h

#if defined _WIN32
#include <memory>
#elif defined __APPLE__
#include <tr1/memory>
#elif defined __linux__
#include <tr1/memory>
#endif

using std::tr1::shared_ptr;
using std::tr1::weak_ptr;

class {
public:
    template<typename T>
    operator shared_ptr<T>() { return shared_ptr<T>(); }
} StrongNull;

class {
public:
    template<typename T>
    operator weak_ptr<T>() { return weak_ptr<T>(); }
} WeakNull;

#endif

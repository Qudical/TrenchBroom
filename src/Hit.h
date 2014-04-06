/*
 Copyright (C) 2010-2014 Kristian Duske
 
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

#ifndef __TrenchBroom__Hit__
#define __TrenchBroom__Hit__

#include "TrenchBroom.h"
#include "VecMath.h"
#include "Holder.h"

#include <list>

namespace TrenchBroom {
    class Hit {
    public:
        typedef unsigned long HitType;
        static const HitType NoType;
        static const HitType AnyType;
        static HitType freeHitType();
        static const Hit NoHit;
    private:
        HitType m_type;
        FloatType m_distance;
        Vec3 m_hitPoint;
        BaseHolder::Ptr m_holder;
        FloatType m_error;
    public:
        template <typename T>
        Hit(const HitType type, const FloatType distance, const Vec3& hitPoint, T target, const FloatType error = 0.0) :
        m_type(type),
        m_distance(distance),
        m_hitPoint(hitPoint),
        m_holder(Holder<T>::newHolder(target)),
        m_error(error) {}
        
        bool operator< (const Hit& other) const;
        
        template <typename T>
        static Hit hit(const HitType type, const FloatType distance, const Vec3& hitPoint, T target, const FloatType error = 0.0) {
            return Hit(type, distance, hitPoint, target);
        }
        
        bool isMatch() const;
        HitType type() const;
        bool hasType(const HitType typeMask) const;
        FloatType distance() const;
        const Vec3& hitPoint() const;
        FloatType error() const;
        
        template <typename T>
        T target() const {
            return m_holder->object<T>();
        }
    };
    
    class HitFilter;
    
    class Hits {
    public:
        typedef std::list<Hit> List;
    private:
        List m_hits;
    public:
        bool empty() const;
        size_t size() const;
        
        void addHit(const Hit& hit);
        
        const Hit& findFirst(Hit::HitType type,       bool ignoreOccluders) const;
        const Hit& findFirst(Hit::HitType type,       Hit::HitType ignoreOccluderMask) const;
        const Hit& findFirst(Hit::HitType type,       const HitFilter& ignoreFilter) const;

        const Hit& findFirst(const HitFilter& filter, bool ignoreOccluders) const;
        const Hit& findFirst(const HitFilter& filter, Hit::HitType ignoreOccluderMask) const;
        const Hit& findFirst(const HitFilter& filter, const HitFilter& ignoreFilter) const;
        
        const List& all() const;
        List filter(Hit::HitType type) const;
        List filter(const HitFilter& filter) const;
    };
}

#endif /* defined(__TrenchBroom__Hit__) */

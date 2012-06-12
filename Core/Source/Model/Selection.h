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

#ifndef TrenchBroom_Selection_h
#define TrenchBroom_Selection_h

#include <vector>
#include "Model/Assets/Texture.h"
#include "Model/Map/Brush.h"
#include "Model/Map/Face.h"
#include "Model/Map/Entity.h"
#include "Utilities/Event.h"
#include "Utilities/VecMath.h"

namespace TrenchBroom {
    namespace Model {
        typedef enum {
            TB_SM_NONE,
            TB_SM_FACES,
            TB_SM_BRUSHES,
            TB_SM_ENTITIES,
            TB_SM_BRUSHES_ENTITIES
        } ESelectionMode;
        
        class Texture;
        class Entity;
        class Brush;
        class Face;
        
        class SelectionEventData {
        public:
            std::vector<Entity*> entities;
            std::vector<Brush*> brushes;
            std::vector<Face*> faces;
            SelectionEventData() {};
            SelectionEventData(const std::vector<Entity*>& entities) : entities(entities) {}
            SelectionEventData(const std::vector<Brush*>& brushes) : brushes(brushes) {}
            SelectionEventData(const std::vector<Face*>& faces) : faces(faces) {}
            SelectionEventData(Entity& entity) { entities.push_back(&entity); }
            SelectionEventData(Brush& brush) { brushes.push_back(&brush); }
            SelectionEventData(Face& face) { faces.push_back(&face); }
        };
        
        class Selection {
        private:
            std::vector<Face*> m_faces;
            std::vector<Brush*> m_brushes;
            std::vector<Brush*> m_partialBrushes;
            std::vector<Entity*> m_entities;
            std::vector<Assets::Texture*> m_mruTextures;
            ESelectionMode m_mode;
        public:
            typedef Event<const SelectionEventData&> SelectionEvent;
            SelectionEvent selectionAdded;
            SelectionEvent selectionRemoved;
            
            Selection();
            ESelectionMode mode() const;
            bool empty() const;
            const std::vector<Assets::Texture*>& mruTextures() const;
            const std::vector<Face*>& faces() const;
            const std::vector<Face*> brushFaces() const;
            const std::vector<Face*> allFaces() const;
            const std::vector<Brush*>& brushes() const;
            const std::vector<Brush*>& partialBrushes() const;
            const std::vector<Entity*>& entities() const;
            const Entity* brushSelectionEntity() const;
            Vec3f center() const;
            BBox bounds() const;
            
            void addTexture(Assets::Texture& texture);
            void addFace(Face& face);
            void addFaces(const std::vector<Face*>& faces);
            void addBrush(Brush& brush);
            void addBrushes(const std::vector<Brush*>& brushes);
            void addEntity(Entity& entity);
            void addEntities(const std::vector<Entity*>& entities);
            
            void removeFace(Face& face);
            void removeFaces(const std::vector<Face*>& faces);
            void removeBrush(Brush& brush);
            void removeBrushes(const std::vector<Brush*>& brushes);
            void removeEntity(Entity& entity);
            void removeEntities(const std::vector<Entity*>& entities);
            void removeAll();
        };
    }
}

#endif

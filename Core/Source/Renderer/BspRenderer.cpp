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

#include "BspRenderer.h"
#include "Model/Assets/Bsp.h"
#include "Model/Assets/Texture.h"
#include "Model/Map/Entity.h"
#include "Model/Preferences.h"
#include "Renderer/MapRenderer.h"
#include "Renderer/RenderContext.h"
#include "Renderer/Vbo.h"

namespace TrenchBroom {
    namespace Renderer {
        BspRenderer::BspRenderer(Model::Assets::Bsp& bsp, Vbo& vbo, Model::Assets::Palette& palette) : m_bsp(bsp), m_vbo(vbo), m_vboBlock(NULL), m_palette(palette) {}
        
        BspRenderer::~BspRenderer() {
            if (m_vboBlock != NULL)
                m_vboBlock->freeBlock();
            for (TextureCache::iterator it = m_textures.begin(); it != m_textures.end(); ++it)
                delete it->second;
            m_textures.clear();
        }
        
        void BspRenderer::render() {
            if (m_vboBlock == NULL) {
                Model::Assets::BspModel& model = *m_bsp.models[0];
                unsigned int modelVertexCount = model.vertexCount;
                unsigned int vertexSize = 5 * sizeof(float);

                m_vboBlock = m_vbo.allocBlock(modelVertexCount * vertexSize);
                m_vbo.map();
                unsigned int offset = 0;
                
                for (unsigned int i = 0; i < model.faces.size(); i++) {
                    Model::Assets::BspFace& face = *model.faces[i];
                    Model::Assets::BspTexture& bspTexture = *face.textureInfo->texture;
                    Model::Assets::Texture* texture = NULL;
                    TextureCache::iterator textureIt = m_textures.find(bspTexture.name);
                    if (textureIt == m_textures.end()) {
                        texture = new Model::Assets::Texture(bspTexture.name, bspTexture, m_palette);
                        m_textures[bspTexture.name] = texture;
                    } else {
                        texture = textureIt->second;
                    }

                    TextureVertexInfo::iterator infoIt = m_vertexInfos.find(texture);
                    if (infoIt == m_vertexInfos.end()) {
                        m_vertexInfos[texture].first.reserve(0xF);
                        m_vertexInfos[texture].second.reserve(0xF);
                    }
                    
                    InfoBuffer& infoBuffer = m_vertexInfos[texture];
                    infoBuffer.first.push_back(offset / vertexSize);
                    infoBuffer.second.push_back((GLsizei)face.vertices.size());
                    
                    for (unsigned int j = 0; j < face.vertices.size(); j++) {
                        const Vec3f& vertex = face.vertices[j];
                        Vec2f texCoords = face.textureCoordinates(vertex);
                        offset = m_vboBlock->writeVec(texCoords, offset);
                        offset = m_vboBlock->writeVec(vertex, offset);
                    }
                }
                m_vbo.unmap();
            }
            
            glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
            glInterleavedArrays(GL_T2F_V3F, 0, (const GLvoid *)(long)m_vboBlock->address);
            
            for (TextureCache::iterator textureIt = m_textures.begin(); textureIt != m_textures.end(); ++textureIt) {
                Model::Assets::Texture* texture = textureIt->second;
                InfoBuffer& infoBuffer = m_vertexInfos[texture];
                IntBuffer& indexBuffer = infoBuffer.first;
                IntBuffer& countBuffer = infoBuffer.second;
                GLint* indexPtr = &indexBuffer[0];
                GLsizei* countPtr = &countBuffer[0];
                GLsizei primCount = static_cast<int>(indexBuffer.size());
                
                texture->activate();
                glMultiDrawArrays(GL_POLYGON, indexPtr, countPtr, primCount);
                texture->deactivate();
            }
            
            glPopClientAttrib();
        }
        
        const Vec3f& BspRenderer::center() {
            return m_bsp.models[0]->center;
        }
        
        const BBox& BspRenderer::bounds() {
            return m_bsp.models[0]->bounds;
        }
    }
}
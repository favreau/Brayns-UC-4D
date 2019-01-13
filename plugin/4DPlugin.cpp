/* Copyright (c) 2019, EPFL/Blue Brain Project
 * All rights reserved. Do not distribute without permission.
 * Responsible Author: Cyrille Favreau <cyrille.favreau@epfl.ch>
 *
 * This file is part of the circuit explorer for Brayns
 * <https://github.com/favreau/Brayns-UC-4D>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3.0 as published
 * by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "4DPlugin.h"
#include "../common/log.h"

#include <brayns/common/ActionInterface.h>
#include <brayns/common/PropertyMap.h>
#include <brayns/engine/Engine.h>
#include <brayns/engine/Material.h>
#include <brayns/engine/Model.h>
#include <brayns/engine/Scene.h>
#include <brayns/parameters/ParametersManager.h>
#include <brayns/pluginapi/PluginAPI.h>

namespace
{
const size_t VERTEX_MATERIAL_ID = 0;
const size_t EDGE_MATERIAL_ID = 1;
const size_t FACE_MATERIAL_ID = 2;

const brayns::Vector3f tesseract[16] = {
    {-0.5, -0.5, -0.5}, {0.5, -0.5, -0.5}, {0.5, 0.5, -0.5}, {-0.5, 0.5, -0.5},
    {-0.5, -0.5, 0.5},  {0.5, -0.5, 0.5},  {0.5, 0.5, 0.5},  {-0.5, 0.5, 0.5},
    {-1, -1, -1},       {1, -1, -1},       {1, 1, -1},       {-1, 1, -1},
    {-1, -1, 1},        {1, -1, 1},        {1, 1, 1},        {-1, 1, 1},

};

enum MaterialShadingMode // Must be the same as what's in the Circuit Explorer
                         // plugin
{
    none = 0,
    diffuse = 1,
    electron = 2,
    cartoon = 3,
    electron_transparency = 4,
    perlin = 5
};

} // namespace

Basic4DPlugin::Basic4DPlugin()
    : ExtensionPlugin()
{
}

void Basic4DPlugin::init()
{
    auto& scene = _api->getScene();
    auto model = scene.createModel();
    if (!model)
        PLUGIN_THROW(std::runtime_error("Failed to create model"));

    // Create geometry
    _addMaterial(*model, VERTEX_MATERIAL_ID, {1, 0, 0}, 0.f);
    _addMaterial(*model, EDGE_MATERIAL_ID, {1, 1, 0}, 0.f);
    _addMaterial(*model, FACE_MATERIAL_ID, {0, 0, 1}, 0.5f);

    _model = model.get();
    _createGeometry();

    // Add model to scene
    auto modelDescriptor =
        std::make_shared<brayns::ModelDescriptor>(std::move(model),
                                                  "Tesseract");
    scene.addModel(modelDescriptor);

    auto& ap = _api->getParametersManager().getAnimationParameters();
    ap.setEnd(360);
    ap.setDt(1);
    ap.setUnit("degrees");
}

void Basic4DPlugin::_addMaterial(brayns::Model& model, const size_t materialId,
                                 const brayns::Vector3f& diffuseColor,
                                 const float reflectionIndex)
{
    auto material = model.createMaterial(materialId, "Default");
    if (!material)
        PLUGIN_THROW(std::runtime_error("Failed to create material"));

    brayns::PropertyMap properties;
    properties.setProperty(
        {"shading_mode", static_cast<int>(MaterialShadingMode::diffuse)});
    material->setProperties("default", properties);
    material->setDiffuseColor(diffuseColor);
    material->setReflectionIndex(reflectionIndex);
}

void Basic4DPlugin::preRender()
{
    auto& ap = _api->getParametersManager().getAnimationParameters();
    auto frame = ap.getFrame();
    const auto angle = float(frame) / 360.f;
    if (angle != _angle)
        _createGeometry();
    _angle = angle;
}

brayns::Vector3f Basic4DPlugin::_computeCoordinates(const brayns::Vector3f& p1,
                                                    const brayns::Vector3f& p2,
                                                    const brayns::Vector3f& p3,
                                                    const brayns::Vector3f& p4)
{
    if (_angle >= 0.f && _angle < 0.25f)
        return {p4.x() + (p1.x() - p4.x()) * _angle * 4.f,
                p4.y() + (p1.y() - p4.y()) * _angle * 4.f,
                p4.z() + (p1.z() - p4.z()) * _angle * 4.f};
    if (_angle >= 0.25f && _angle < 0.5f)
        return {p1.x() + (p2.x() - p1.x()) * (_angle - 0.25f) * 4.f,
                p1.y() + (p2.y() - p1.y()) * (_angle - 0.25f) * 4.f,
                p1.z() + (p2.z() - p1.z()) * (_angle - 0.25f) * 4.f};
    if (_angle >= 0.5f && _angle < 0.75f)
        return {p2.x() + (p3.x() - p2.x()) * (_angle - 0.5f) * 4.f,
                p2.y() + (p3.y() - p2.y()) * (_angle - 0.5f) * 4.f,
                p2.z() + (p3.z() - p2.z()) * (_angle - 0.5f) * 4.f};
    if (_angle >= 0.75f && _angle < 1.f)
        return {p3.x() + (p4.x() - p3.x()) * (_angle - 0.75f) * 4.f,
                p3.y() + (p4.y() - p3.y()) * (_angle - 0.75f) * 4.f,
                p3.z() + (p4.z() - p3.z()) * (_angle - 0.75f) * 4.f};
}

void Basic4DPlugin::_createGeometry()
{
    if (!_model)
        PLUGIN_ERROR << "Cannot create geometry on non-existant model"
                     << std::endl;

    auto& spheres = _model->getSpheres()[VERTEX_MATERIAL_ID];
    spheres.clear();
    auto& cylinders = _model->getCylinders()[EDGE_MATERIAL_ID];
    cylinders.clear();

    brayns::Vector3f vertices[16] = {
        _computeCoordinates(tesseract[8], tesseract[9], tesseract[1],
                            tesseract[0]),
        _computeCoordinates(tesseract[0], tesseract[8], tesseract[9],
                            tesseract[1]),
        _computeCoordinates(tesseract[3], tesseract[11], tesseract[10],
                            tesseract[2]),
        _computeCoordinates(tesseract[11], tesseract[10], tesseract[2],
                            tesseract[3]),
        _computeCoordinates(tesseract[12], tesseract[13], tesseract[5],
                            tesseract[4]),
        _computeCoordinates(tesseract[4], tesseract[12], tesseract[13],
                            tesseract[5]),
        _computeCoordinates(tesseract[7], tesseract[15], tesseract[14],
                            tesseract[6]),
        _computeCoordinates(tesseract[15], tesseract[14], tesseract[6],
                            tesseract[7]),
        _computeCoordinates(tesseract[9], tesseract[1], tesseract[0],
                            tesseract[8]),
        _computeCoordinates(tesseract[1], tesseract[0], tesseract[8],
                            tesseract[9]),
        _computeCoordinates(tesseract[2], tesseract[3], tesseract[11],
                            tesseract[10]),
        _computeCoordinates(tesseract[10], tesseract[2], tesseract[3],
                            tesseract[11]),
        _computeCoordinates(tesseract[13], tesseract[5], tesseract[4],
                            tesseract[12]),
        _computeCoordinates(tesseract[5], tesseract[4], tesseract[12],
                            tesseract[13]),
        _computeCoordinates(tesseract[6], tesseract[7], tesseract[15],
                            tesseract[14]),
        _computeCoordinates(tesseract[14], tesseract[6], tesseract[7],
                            tesseract[15])};

    float radius = 0.1f;
    for (size_t i = 0; i < 16; ++i)
    {
        const auto& t = vertices[i];
        _model->addSphere(VERTEX_MATERIAL_ID, {{t.x(), t.y(), t.z()}, radius});
    }

    const size_t joints[32][2] = {
        {0, 1},   {1, 2},   {2, 3},   {3, 0},   {0, 4},  {1, 5},   {2, 6},
        {3, 7},   {4, 5},   {5, 6},   {6, 7},   {7, 4},  {0, 8},   {1, 9},
        {2, 10},  {3, 11},  {4, 12},  {5, 13},  {6, 14}, {7, 15},  {8, 9},
        {9, 10},  {10, 11}, {11, 8},  {8, 12},  {9, 13}, {10, 14}, {11, 15},
        {12, 13}, {13, 14}, {14, 15}, {15, 12},
    };

    radius = 0.05f;
    for (size_t i = 0; i < 32; ++i)
    {
        const auto& t = vertices[joints[i][0]];
        const auto& u = vertices[joints[i][1]];

        _model->addCylinder(EDGE_MATERIAL_ID, {{t.x(), t.y(), t.z()},
                                               {u.x(), u.y(), u.z()},
                                               radius});
    }

    auto& meshes = _model->getTrianglesMeshes()[FACE_MATERIAL_ID];
    meshes.indices.clear();
    meshes.vertices.clear();
    uint32_t index{0};
    for (size_t i = 0; i < 32; i += 4)
    {
        brayns::Vector3f t = vertices[joints[i][0]];
        brayns::Vector3f u = vertices[joints[i + 1][0]];
        brayns::Vector3f v = vertices[joints[i + 2][0]];
        brayns::Vector3f w = vertices[joints[i + 3][0]];
        meshes.vertices.push_back(t);
        meshes.vertices.push_back(u);
        meshes.vertices.push_back(v);
        meshes.vertices.push_back(w);
        meshes.indices.push_back({index, index + 1, index + 2});
        meshes.indices.push_back({index + 2, index + 3, index});
        index += 4;
    }
}

extern "C" brayns::ExtensionPlugin* brayns_plugin_create(int /*argc*/,
                                                         char** /*argv*/)
{
    PLUGIN_INFO << "Initializing 4D plugin" << std::endl;
    return new Basic4DPlugin();
}

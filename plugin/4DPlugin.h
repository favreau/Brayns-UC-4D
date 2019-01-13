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

#ifndef BASIC4D_PLUGIN_H
#define BASIC4D_PLUGIN_H

#include <brayns/pluginapi/ExtensionPlugin.h>

class Basic4DPlugin : public brayns::ExtensionPlugin
{
public:
    Basic4DPlugin();

    void init() final;

    void preRender() final;

private:
    void _addMaterial(brayns::Model& model, const size_t materialId,
                      const brayns::Vector3f& diffuseColor,
                      const float reflectionIndex);

    brayns::Vector3f _computeCoordinates(const brayns::Vector3f& p1,
                                         const brayns::Vector3f& p2,
                                         const brayns::Vector3f& p3,
                                         const brayns::Vector3f& p4);
    void _createGeometry();

    float _angle{0.f};

    brayns::Model* _model{nullptr};
};
#endif // BASIC4D_PLUGIN_H

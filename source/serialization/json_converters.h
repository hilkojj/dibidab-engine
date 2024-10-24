#pragma once

#include <math/math_utils.h>
#include <assets/asset.h>
#include <assets/AssetManager.h>
#include <json.hpp>

namespace nlohmann
{
    template <int len, typename type, qualifier something>
    struct adl_serializer<glm::vec<len, type, something>>
    {
        static void to_json(json &j, const glm::vec<len, type, something> &v)
        {
            j = json::array();
            for (int i = 0; i < len; i++)
                j[i] = v[i];
        }

        static void from_json(const json &j, glm::vec<len, type, something> &v)
        {
            for (int i = 0; i < len; i++)
                v[i] = j.at(i);
        }
    };

    template <>
    struct adl_serializer<quat>
    {
        static void to_json(json &j, const quat &v)
        {
            vec3 euler = eulerAngles(v) * mu::RAD_TO_DEGREES;
            j = json::array({euler.x, euler.y, euler.z});
        }

        static void from_json(const json &j, quat &v)
        {
            vec3 euler(j.at(0).get<float>(), j.at(1).get<float>(), j.at(2).get<float>());
            v = quat(euler * mu::DEGREES_TO_RAD);
        }
    };

    template <typename type>
    struct adl_serializer<asset<type>>
    {
        static void to_json(json &j, const asset<type> &v)
        {
            if (v.isSet())
                j = v.getLoadedAsset()->shortPath;
            else j = "";
        }

        static void from_json(const json &j, asset<type> &v)
        {
            if (!j.is_string())
                v = asset<type>();
            else
            {
                std::string path = j;
                if (path.empty())
                    v = asset<type>();
                else
                    v.set(path.c_str());
            }
        }
    };
}

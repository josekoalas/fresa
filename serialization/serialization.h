//project verse, 2017-2021
//by jose pazos perez
//all rights reserved uwu

#pragma once

#include <yaml-cpp/yaml.h>

#include "config.h"

namespace Verse::Serialization
{
    void loadYAML(str name, YAML::Node &file);
    void writeYAML(str name, YAML::Node &file);
    void destroyYAML(str name);

    void appendYAML(str name, str key, YAML::Node &file, bool overwrite=false);
    void appendYAML(str name, std::vector<str> key, YAML::Node &file, bool overwrite=false);

    void appendYAML(str name, str key, str value, bool overwrite=false);
    void appendYAML(str name, std::vector<str> key, str value, bool overwrite=false);
    void appendYAML(str name, str key, std::vector<str> vec, bool overwrite=false);
    void appendYAML(str name, std::vector<str> key, std::vector<str> vec, bool overwrite=false);
    void appendYAML(str name, str key, int num, bool overwrite=false);
    void appendYAML(str name, std::vector<str> key, int num, bool overwrite=false);
    void appendYAML(str name, str key, std::vector<int> vec, bool overwrite=false);
    void appendYAML(str name, std::vector<str> key, std::vector<int> vec, bool overwrite=false);
    void appendYAML(str name, str key, std::vector<ui16> vec, bool overwrite=false);
    void appendYAML(str name, std::vector<str> key, std::vector<ui16> vec, bool overwrite=false);
    void appendYAML(str name, str key, float num, bool overwrite=false);
    void appendYAML(str name, std::vector<str> key, float num, bool overwrite=false);
    void appendYAML(str name, str key, bool b, bool overwrite=false);
    void appendYAML(str name, std::vector<str> key, bool b, bool overwrite=false);
    void appendYAML(str name, str key, Vec2<int> vec, bool overwrite=false);
    void appendYAML(str name, std::vector<str> key, Vec2<int> vec, bool overwrite=false);
    void appendYAML(str name, str key, std::vector<Vec2<int>> vec, bool overwrite=false);
    void appendYAML(str name, std::vector<str> key, std::vector<Vec2<int>> vec, bool overwrite=false);
    void appendYAML(str name, str key, Vec2<float> vec, bool overwrite=false);
    void appendYAML(str name, std::vector<str> key, Vec2<float> vec, bool overwrite=false);
    void appendYAML(str name, str key, Rect2<int> rect, bool overwrite=false);
    void appendYAML(str name, std::vector<str> key, Rect2<int> rect, bool overwrite=false);
    void appendYAML(str name, str key, Rect2<float> rect, bool overwrite=false);
    void appendYAML(str name, std::vector<str> key, Rect2<float> rect, bool overwrite=false);

    void removeYAML(str name, str key);
    void removeYAML(str name, std::vector<str> key);

    void loadComponentsFromYAML(EntityID eid, YAML::Node &entity, Scene *s, Config &c);
    void loadScene(str name, Scene *s, Config &c);
    EntityID loadPlayer(Scene *s, Config &c);

    void saveComponentsToYAML(EntityID eid, Scene *s, Config &c);
    void saveScene(Scene *s, Config &c, bool to_proj = false);
}

namespace YAML
{
    template<typename T>
    struct convert<Verse::Vec2<T>> {
      static Node encode(const Verse::Vec2<T>& rhs) {
        Node node;
        node.push_back(rhs.x);
        node.push_back(rhs.y);
        return node;
      }

      static bool decode(const Node& node, Verse::Vec2<T>& rhs) {
        if(!node.IsSequence() || node.size() != 2) {
          return false;
        }

        rhs.x = node[0].as<T>();
        rhs.y = node[1].as<T>();
        return true;
      }
    };

    template<typename T>
    struct convert<Verse::Rect2<T>> {
      static Node encode(const Verse::Rect2<T>& rhs) {
        Node node;
        node.push_back(*rhs.x);
        node.push_back(*rhs.y);
        node.push_back(*rhs.w);
        node.push_back(*rhs.h);
        return node;
      }

      static bool decode(const Node& node, Verse::Rect2<T>& rhs) {
        if(!node.IsSequence() || node.size() != 4) {
          return false;
        }
          
        rhs = Verse::Rect2(node[0].as<T>(), node[1].as<T>(), node[2].as<T>(), node[3].as<T>());
        return true;
        }
    };
}

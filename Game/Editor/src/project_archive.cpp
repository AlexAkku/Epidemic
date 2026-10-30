#include "project_archive.h"
#include "Epidemic/Runtime/Serialization/serialization_services.h"
#include "Epidemic/Runtime/Support/graph_serialization.h"
#include <cmath>
#include <limits>
#include <type_traits>
#include <exception>
#include <algorithm>

namespace epidemic::game::editor::detail {
namespace {
struct Failure : std::exception {
    foundation::Error error;

    explicit Failure(foundation::Error value) : error(std::move(value)) {
    }

};

void Require(bool condition) {
    if (!condition)
        throw Failure{foundation::Error::Create("editor.invalid_archive",
                                                "project archive schema or limit rejected")};

}

template <class T> T Take(foundation::Result<T> result) {
    if (!result)
        throw Failure{result.GetError()};

    return std::move(result).Value();
}

void Take(const foundation::Result<void> &result) {
    if (!result)
        throw Failure{result.GetError()};

}

struct IO {
    runtime::IArchiveWriter *writer = nullptr;
    runtime::IArchiveReader *reader = nullptr;

    void U(std::string_view key, std::uint64_t &value) {
        if (writer)
            Take(writer->WriteUInt64(key, value));
        else
            value = Take(reader->ReadUInt64(key));
    }

    void I(std::string_view key, std::int64_t &value) {
        if (writer)
            Take(writer->WriteInt64(key, value));
        else
            value = Take(reader->ReadInt64(key));
    }

    void S(std::string_view key, std::string &value) {
        if (writer)
            Take(writer->WriteString(key, value));
        else
            value = Take(reader->ReadString(key));
        Require(value.size() <= runtime::kMaxDocumentBytes);
    }

    void B(std::string_view key, bool &value) {
        if (writer)
            Take(writer->WriteBool(key, value));
        else
            value = Take(reader->ReadBool(key));
    }

    void D(std::string_view key, double &value) {
        if (writer)
            Take(writer->WriteDouble(key, value));
        else
            value = Take(reader->ReadDouble(key));
        Require(std::isfinite(value));
    }

    void F(std::string_view key, float &value) {
        double number = value;
        D(key, number);
        Require(std::abs(number) <= std::numeric_limits<float>::max());
        value = static_cast<float>(number);
    }

    template <class T> void Id(std::string_view key, T &value) {
        auto raw = value.Raw();
        U(key, raw);
        if constexpr (std::is_same_v<T, runtime::AssetId>)
            value = runtime::AssetId{foundation::StringId{raw}};

        else
            value = T{raw};

    }

    template <class T> void Enum(std::string_view key, T &value, std::uint64_t maximum) {
        auto raw = static_cast<std::uint64_t>(value);
        U(key, raw);
        Require(raw <= maximum);
        value = static_cast<T>(raw);
    }

    template <class F> void Object(std::string_view key, F function) {
        if (writer)
            Take(writer->BeginObject(key));
        else
            Take(reader->BeginObject(key));
        function();
        if (writer)
            Take(writer->EndObject());
        else
            Take(reader->EndObject());
    }

    template <class T, class F>
    void Array(std::string_view key, std::vector<T> &values, std::size_t maximum, F function) {
        if (writer) {
            Require(values.size() <= maximum);
            Take(writer->BeginArray(key, values.size()));
        } else {
            const auto size = Take(reader->BeginArray(key));
            Require(size <= maximum);
            values.resize(size);
        }

        for (std::size_t i = 0; i < values.size(); ++i) {
            if (writer)
                Take(writer->BeginArrayElement(i));
            else
                Take(reader->BeginArrayElement(i));
            function(values[i]);
            if (writer)
                Take(writer->EndArrayElement());
            else
                Take(reader->EndArrayElement());
        }

        if (writer)
            Take(writer->EndArray());
        else
            Take(reader->EndArray());
    }

    void Bytes(std::string_view key, std::vector<std::byte> &bytes) {
        if (writer)
            Take(writer->WriteBytes(key, bytes));
        else
            bytes = Take(reader->ReadBytes(key));
    }

};

void Property(IO &io, comp::PropertyValue &value) {
    auto kind = static_cast<std::uint64_t>(value.index());
    io.U("kind", kind);
    Require(kind < 4);
    switch (kind) {
    case 0: {
        std::int64_t data = io.writer ? std::get<std::int64_t>(value) : 0;
        io.I("value", data);
        value = data;
        break;
    }

    case 1: {
        double data = io.writer ? std::get<double>(value) : 0;
        io.D("value", data);
        value = data;
        break;
    }

    case 2: {
        bool data = io.writer && std::get<bool>(value);
        io.B("value", data);
        value = data;
        break;
    }

    default: {
        std::string data = io.writer ? std::get<std::string>(value) : std::string{};

        io.S("value", data);
        value = std::move(data);
    }

    }

}

void Assignment(IO &io, comp::PropertyAssignment &value) {
    io.Id("id", value.property);
    io.Object("value", [&] { Property(io, value.value); });
}

void Input(IO &io, comp::GraphInput &value) {
    io.Id("port", value.port);
    io.I("value", value.value);
}

void Behavior(IO &io, comp::GraphBehaviorDefinition &value) {
    io.Id("id", value.id);
    io.Id("graph", value.graph);
    io.Array("events", value.events, 1024, [&](auto &event) {
        io.Id("event", event.event);
        io.Id("port", event.port);
    });
    io.Array("inputs", value.inputs, 1024, [&](auto &input) { Input(io, input); });
}

void Node(IO &io, comp::PrefabNode &node, std::size_t depth, std::size_t &count) {
    Require(depth <= 32 && ++count <= 1024);
    io.Id("id", node.id);
    io.Id("archetype", node.archetype);
    io.Array("components", node.components, 128, [&](auto &component) {
        io.Id("type", component.type);
        io.Id("part", component.part);
        io.Array("properties", component.properties, 256, [&](auto &property) {
            io.Id("id", property.property);
            io.Id("parameter", property.parameter);
            io.Object("value", [&] { Property(io, property.value); });
        });
        io.Array("behaviors", component.behaviors, 64,
                 [&](auto &behavior) { Behavior(io, behavior); });
    });
    io.Array("behaviors", node.behaviors, 64, [&](auto &behavior) { Behavior(io, behavior); });
    io.Array("children", node.children, 1024,
             [&](auto &child) { Node(io, child, depth + 1, count); });
}

void Prefab(IO &io, comp::PrefabDefinition &prefab) {
    io.Id("id", prefab.id);
    io.Array("parameters", prefab.parameters, 256, [&](auto &parameter) {
        io.Id("id", parameter.id);
        io.Object("value", [&] { Property(io, parameter.value); });
    });
    std::size_t count = 0;
    io.Object("root", [&] { Node(io, prefab.root, 0, count); });
}

void Overrides(IO &io, comp::PrefabOverrides &overrides) {
    io.Array("parameters", overrides.parameters, 256, [&](auto &value) { Assignment(io, value); });
    io.Array("components", overrides.components, 4096, [&](auto &value) {
        io.Id("node", value.node);
        io.Id("component", value.component);
        io.Id("part", value.part);
        io.Object("property", [&] { Assignment(io, value.property); });
    });
    io.Array("behaviors", overrides.behaviors, 4096, [&](auto &value) {
        io.Id("node", value.node);
        io.Id("component", value.component);
        io.Id("part", value.part);
        io.Id("behavior", value.behavior);
        io.Object("input", [&] { Input(io, value.input); });
    });
}

void Vector(IO &io, runtime::Vec3 &value) {
    io.F("x", value.x);
    io.F("y", value.y);
    io.F("z", value.z);
}

void Scene(IO &io, composition::SceneCompositionDefinition &scene) {
    io.Id("id", scene.asset_id);
    io.Array("nodes", scene.scene.nodes, 4096, [&](auto &node) {
        io.Id("id", node.id);
        io.Id("parent", node.parent);
        io.Object("position", [&] { Vector(io, node.local_transform.position); });
        io.Object("scale", [&] { Vector(io, node.local_transform.scale); });
        io.Object("rotation", [&] {
            auto &q = node.local_transform.rotation;
            io.F("x", q.x);
            io.F("y", q.y);
            io.F("z", q.z);
            io.F("w", q.w);
        });
        io.Enum("mobility", node.mobility, 1);
        io.Enum("visibility", node.visibility, 1);
        bool bounds = node.local_bounds.has_value();
        io.B("has_bounds", bounds);
        if (bounds) {
            if (!node.local_bounds)
                node.local_bounds.emplace();
            io.Object("bounds", [&] {
                io.Object("min", [&] { Vector(io, node.local_bounds->min); });
                io.Object("max", [&] { Vector(io, node.local_bounds->max); });
            });
        } else
            node.local_bounds.reset();
        // world transforms, world bounds, attachment and runtime revision are derived by
        // SceneServices.
    });
    io.Array("placements", scene.placements, 4096, [&](auto &placement) {
        io.Id("node", placement.node);
        io.Id("prefab", placement.prefab);
        io.Object("overrides", [&] { Overrides(io, placement.overrides); });
    });
}

std::vector<runtime::graph::TypeId> Types(const ProjectSchema &schema) {
    std::vector<runtime::graph::TypeId> types{runtime::graph::kI64Type};

    for (const auto &[type, ignored] : schema.operations) {
        (void)ignored;
        types.push_back(type);
    }

    return types;
}

void Project(IO &io, ProjectArchive &project, const ProjectSchema &schema,
             runtime::IArchiveFactory &archives) {
    io.Id("id", project.definition.id);
    io.Id("entry", project.definition.entry_scene);
    runtime::GraphDefinitionSerializer graph_codec{Types(schema)};

    io.Array("documents", project.definition.documents, 64, [&](auto &doc) {
        io.Id("id", doc.id);
        io.S("name", doc.name);
        auto kind = static_cast<std::uint64_t>(doc.Kind());
        io.U("kind", kind);
        Require(kind <= 2);
        if (io.reader) {
            if (kind == 0)
                doc.data = composition::SceneCompositionDefinition{};

            else if (kind == 1)
                doc.data = comp::PrefabDefinition{};

            else
                doc.data = runtime::graph::GraphDefinition{};

        }

        io.Object("data", [&] {
            if (kind == 0)
                Scene(io, std::get<composition::SceneCompositionDefinition>(doc.data));
            else if (kind == 1)
                Prefab(io, std::get<comp::PrefabDefinition>(doc.data));
            else {
                auto &graph = std::get<runtime::graph::GraphDefinition>(doc.data);
                std::vector<std::byte> bytes;
                if (io.writer)
                    bytes = Take(runtime::EncodeDocument(
                        Take(runtime::SerializeToDocument(graph_codec, graph, archives))));
                io.Bytes("archive", bytes);
                if (io.reader) {
                    auto document = Take(runtime::DecodeDocument(bytes));
                    auto reader = Take(archives.CreateReader(document));
                    Take(runtime::DeserializeObject(graph_codec, *reader, graph));
                }

            }

        });
    });
    io.Array("identities", project.used_ids, 65536, [&](auto &value) {
        io.U("document", value.first);
        io.U("element", value.second);
        Require(value.first != 0);
    });
    Require(std::is_sorted(project.used_ids.begin(), project.used_ids.end()) &&
            std::adjacent_find(project.used_ids.begin(), project.used_ids.end()) ==
                project.used_ids.end());
}

const auto kProjectType = foundation::StringId::FromString("epidemic.editor.project");
} // namespace

foundation::Result<std::vector<std::byte>> WriteProject(ProjectArchive &project,
                                                        const ProjectSchema &schema) {
    using Result = foundation::Result<std::vector<std::byte>>;
    try {
        auto services = Take(runtime::CreateSerializationServices());
        auto writer = services.archives->CreateWriter();
        Require(writer != nullptr);
        IO io{writer.get(), nullptr};

        Project(io, project, schema, *services.archives);
        return runtime::EncodeDocument(Take(writer->Finalize(kProjectType, {1, 0, 0})));
    } catch (const Failure &failure) {
        return Result::Failure(failure.error);
    }

}

foundation::Result<void>
ValidateForPersistence(ProjectDefinition &definition,
                       const std::set<std::pair<std::uint64_t, std::uint64_t>> &identities,
                       const ProjectSchema &schema) {
    // Move the unpublished candidate rather than copying the entire project
    // again. Use the exact Save schema/codec limits, including finite doubles.
    ProjectArchive archive{std::move(definition), {identities.begin(), identities.end()}};

    const auto encoded = WriteProject(archive, schema);
    definition = std::move(archive.definition);
    if (!encoded)
        return foundation::Result<void>::Failure(encoded.GetError());
    return foundation::Result<void>::Success();
}

foundation::Result<ProjectArchive> ReadProject(std::span<const std::byte> bytes,
                                               const ProjectSchema &schema) {
    using Result = foundation::Result<ProjectArchive>;
    try {
        auto document = Take(runtime::DecodeDocument(bytes));
        Require(document.GetTypeId() == kProjectType &&
                document.GetSchemaVersion() == runtime::SchemaVersion{1, 0, 0});
        auto services = Take(runtime::CreateSerializationServices());
        auto reader = Take(services.archives->CreateReader(document));
        ProjectArchive project;
        IO io{nullptr, reader.get()};

        Project(io, project, schema, *services.archives);
        return Result::Success(std::move(project));
    } catch (const Failure &failure) {
        return Result::Failure(failure.error);
    }

}

} // namespace epidemic::game::editor::detail

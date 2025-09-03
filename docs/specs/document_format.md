# QuantumCanvas Studio Document Format Specification
## .qcsx Format - Complete Document Container

### Document Structure Overview

The `.qcsx` format is a ZIP-based container optimized for professional creative work with comprehensive version control, asset management, and collaborative features.

#### Container Structure
```
document.qcsx (ZIP archive)
├── /model/
│   ├── scene.json          # Main document graph
│   ├── layers.json         # Layer hierarchy and properties  
│   ├── styles.json         # Reusable styles and presets
│   └── objects/            # Individual object definitions
│       ├── obj_001.json
│       ├── obj_002.json
│       └── ...
├── /assets/
│   ├── textures/           # Raster assets
│   ├── fonts/              # Embedded fonts with licensing
│   ├── profiles/           # ICC color profiles
│   └── references/         # External file references
├── /journal/
│   ├── log.ndjson          # Event-sourced command log
│   ├── snapshots/          # Periodic model snapshots
│   └── metadata.json       # Journal configuration
├── /preview/
│   ├── thumbnail.png       # Quick preview (256x256)
│   ├── thumbnail@2x.png    # High-DPI preview (512x512)
│   └── previews/           # Layer/object previews
└── meta.json               # Document metadata and schema
```

---

## Core Data Model Specifications

### Document Metadata (meta.json)
```json
{
  "format": "qcsx",
  "schema_version": "1.2.0",
  "created": "2025-09-03T12:00:00.000Z",
  "modified": "2025-09-03T14:30:00.000Z",
  "app_version": "1.0.0-alpha.1",
  "app_build": "20250903.1430",
  
  "document_info": {
    "title": "Untitled Design",
    "description": "",
    "author": "John Doe",
    "copyright": "© 2025 John Doe",
    "keywords": ["design", "vector", "illustration"],
    "custom_properties": {}
  },
  
  "canvas": {
    "width": 1920,
    "height": 1080,
    "units": "px",
    "dpi": 300,
    "background_color": "#FFFFFF",
    "color_space": "DisplayP3"
  },
  
  "color_management": {
    "working_profile": "Display P3",
    "output_profile": "sRGB IEC61966-2.1",
    "rendering_intent": "relative_colorimetric",
    "black_point_compensation": true,
    "embedded_profiles": ["profiles/display_p3.icc", "profiles/srgb.icc"]
  },
  
  "compression": {
    "algorithm": "zstd",
    "level": 6,
    "dictionary": "qcs_v1"
  }
}
```

### Scene Graph (model/scene.json)
```json
{
  "version": 1,
  "revision_id": 15742,
  "root_layer": "layer_root",
  
  "layers": {
    "layer_root": {
      "type": "group",
      "name": "Root",
      "visible": true,
      "locked": false,
      "opacity": 1.0,
      "blend_mode": "normal",
      "children": ["layer_001", "layer_002"],
      "transform": {
        "position": [0, 0],
        "rotation": 0,
        "scale": [1, 1],
        "anchor": [0.5, 0.5],
        "matrix": [1, 0, 0, 1, 0, 0]
      }
    },
    
    "layer_001": {
      "type": "vector",
      "name": "Logo",
      "visible": true,
      "locked": false,
      "opacity": 0.8,
      "blend_mode": "multiply",
      "objects": ["obj_001", "obj_002"],
      "transform": {...},
      "effects": [
        {
          "type": "drop_shadow",
          "enabled": true,
          "params": {
            "offset": [2, 2],
            "blur_radius": 4,
            "color": "#000000",
            "opacity": 0.5
          }
        }
      ]
    },
    
    "layer_002": {
      "type": "raster",
      "name": "Background",
      "visible": true,
      "locked": false,
      "opacity": 1.0,
      "blend_mode": "normal",
      "image_ref": "assets/textures/bg_001.png",
      "transform": {...}
    }
  },
  
  "object_index": {
    "obj_001": "objects/obj_001.json",
    "obj_002": "objects/obj_002.json"
  }
}
```

### Vector Object Definition (objects/obj_001.json)
```json
{
  "id": "obj_001",
  "type": "vector_path",
  "name": "Circle",
  "created": "2025-09-03T12:15:00.000Z",
  "modified": "2025-09-03T12:20:00.000Z",
  
  "geometry": {
    "paths": [
      {
        "closed": true,
        "fill_rule": "even_odd",
        "points": [
          {"type": "move", "x": 100, "y": 50},
          {"type": "curve", "x": 150, "y": 50, "cp1x": 127.614, "cp1y": 27.386, "cp2x": 150, "cp2y": 27.386},
          {"type": "curve", "x": 150, "y": 100, "cp1x": 172.614, "cp1y": 50, "cp2x": 172.614, "cp2y": 72.614},
          {"type": "curve", "x": 100, "y": 100, "cp1x": 150, "cp1y": 122.614, "cp2x": 127.614, "cp2y": 122.614},
          {"type": "curve", "x": 100, "y": 50, "cp1x": 72.386, "cp1y": 100, "cp2x": 72.386, "cp2y": 77.386}
        ]
      }
    ]
  },
  
  "style": {
    "fill": {
      "enabled": true,
      "type": "solid",
      "color": "#FF6B6B",
      "opacity": 1.0
    },
    "stroke": {
      "enabled": true,
      "type": "solid",
      "color": "#333333",
      "width": 2.0,
      "opacity": 1.0,
      "line_cap": "round",
      "line_join": "round",
      "miter_limit": 4.0,
      "dash_pattern": []
    }
  }
}
```

---

## Event-Sourced Journal System

### Command Log Format (journal/log.ndjson)
Each line represents a single command in NDJSON format:

```jsonl
{"cmd_id": 1001, "type": "create_layer", "timestamp": "2025-09-03T12:00:00.000Z", "user_id": "user_123", "data": {"layer_type": "vector", "name": "New Layer", "parent": "layer_root"}}
{"cmd_id": 1002, "type": "add_vector_object", "timestamp": "2025-09-03T12:01:00.000Z", "user_id": "user_123", "data": {"layer_id": "layer_001", "object": "obj_001"}}
{"cmd_id": 1003, "type": "modify_object_style", "timestamp": "2025-09-03T12:02:00.000Z", "user_id": "user_123", "data": {"object_id": "obj_001", "style_path": "fill.color", "old_value": "#FF0000", "new_value": "#FF6B6B"}}
```

### Command Types and Schemas

#### Layer Operations
```typescript
interface CreateLayerCommand {
  cmd_id: number;
  type: "create_layer";
  timestamp: string;
  user_id: string;
  data: {
    layer_id: string;
    layer_type: "vector" | "raster" | "group" | "adjustment";
    name: string;
    parent: string;
    index?: number;
  };
}

interface DeleteLayerCommand {
  cmd_id: number;
  type: "delete_layer";
  timestamp: string;
  user_id: string;
  data: {
    layer_id: string;
    backup_data: any; // Full layer data for undo
  };
}
```

#### Object Operations
```typescript
interface ModifyObjectGeometryCommand {
  cmd_id: number;
  type: "modify_object_geometry";
  timestamp: string;
  user_id: string;
  data: {
    object_id: string;
    geometry_delta: {
      path_index: number;
      point_index: number;
      old_point: Point;
      new_point: Point;
    };
  };
}

interface TransformObjectCommand {
  cmd_id: number;
  type: "transform_object";
  timestamp: string;
  user_id: string;
  data: {
    object_ids: string[];
    transform_matrix: number[6]; // 2D transformation matrix
    origin: [number, number];
  };
}
```

### Snapshot System (journal/snapshots/)

#### Snapshot Metadata (snapshots/metadata.json)
```json
{
  "policy": {
    "frequency": 500,
    "time_interval": 300000,
    "before_destructive": true
  },
  "snapshots": [
    {
      "id": "snap_001",
      "revision": 500,
      "timestamp": "2025-09-03T12:05:00.000Z",
      "file": "snapshots/snap_001.zstd",
      "size_compressed": 1024768,
      "size_uncompressed": 4194304,
      "checksum": "sha256:abc123..."
    }
  ]
}
```

---

## C++ Implementation Contracts

### Document Model Core
```cpp
// src/document/document_model.hpp
namespace QuantumCanvas::Document {

using ObjectId = uint64_t;
using LayerId = uint64_t;
using RevisionId = uint64_t;

struct DocumentMetadata {
    std::string format_version;
    std::chrono::system_clock::time_point created;
    std::chrono::system_clock::time_point modified;
    std::string app_version;
    
    struct Canvas {
        Size2D<float> size;
        Units units = Units::Pixels;
        float dpi = 300.0f;
        ColorRGBA background_color;
        ColorSpace color_space = ColorSpace::DisplayP3;
    } canvas;
    
    struct ColorManagement {
        std::string working_profile;
        std::string output_profile;
        RenderingIntent rendering_intent;
        bool black_point_compensation = true;
        std::vector<std::string> embedded_profiles;
    } color_management;
};

class DocumentModel {
public:
    // Core state
    RevisionId current_revision() const noexcept;
    const DocumentMetadata& metadata() const noexcept;
    
    // Layer management
    LayerId create_layer(LayerType type, const std::string& name, LayerId parent = 0);
    void delete_layer(LayerId layer_id);
    void move_layer(LayerId layer_id, LayerId new_parent, size_t index);
    
    // Object management
    ObjectId create_object(ObjectType type, LayerId layer_id);
    void delete_object(ObjectId object_id);
    void modify_object(ObjectId object_id, const ObjectDelta& delta);
    
    // Query interface
    std::optional<Layer> get_layer(LayerId id) const;
    std::optional<Object> get_object(ObjectId id) const;
    std::vector<LayerId> get_child_layers(LayerId parent) const;
    
    // Serialization
    nlohmann::json serialize() const;
    void deserialize(const nlohmann::json& data);
    
private:
    RevisionId revision_ = 0;
    DocumentMetadata metadata_;
    std::unordered_map<LayerId, std::unique_ptr<Layer>> layers_;
    std::unordered_map<ObjectId, std::unique_ptr<Object>> objects_;
    LayerId root_layer_id_ = 0;
};

}
```

### Command Interface
```cpp
// src/document/commands/command_interface.hpp
namespace QuantumCanvas::Commands {

using CommandId = uint64_t;

struct CommandContext {
    CommandId id;
    RevisionId base_revision;
    std::chrono::system_clock::time_point timestamp;
    std::string user_id;
    nlohmann::json metadata;
};

class ICommand {
public:
    virtual ~ICommand() = default;
    
    // Command identification
    virtual std::string type() const = 0;
    virtual CommandId id() const = 0;
    
    // Validation and execution
    virtual ValidationResult validate(const DocumentModel& model) const = 0;
    virtual void execute(DocumentModel& model) = 0;
    virtual void undo(DocumentModel& model) = 0;
    
    // Serialization for journal
    virtual nlohmann::json serialize() const = 0;
    virtual void deserialize(const nlohmann::json& data) = 0;
    
    // Operational Transform support
    virtual std::unique_ptr<ICommand> transform(const ICommand& other) const = 0;
    virtual bool conflicts_with(const ICommand& other) const = 0;
};

// Command factory for deserialization
class CommandFactory {
public:
    static std::unique_ptr<ICommand> create(const std::string& type);
    static std::unique_ptr<ICommand> deserialize(const nlohmann::json& data);
    
    template<typename T>
    static void register_command(const std::string& type);
};

}
```

### Journal System
```cpp
// src/document/journal.hpp
namespace QuantumCanvas::Journal {

struct JournalEntry {
    CommandContext context;
    std::unique_ptr<Commands::ICommand> command;
    std::chrono::microseconds execution_time;
    bool success = false;
};

class Journal {
public:
    // Command logging
    void append(std::unique_ptr<Commands::ICommand> command, 
                const CommandContext& context);
    
    // Playback and navigation
    std::vector<JournalEntry> get_range(RevisionId from, RevisionId to) const;
    void replay_to_revision(DocumentModel& model, RevisionId target);
    
    // Snapshot management
    void create_snapshot(const DocumentModel& model);
    std::optional<DocumentModel> load_snapshot(RevisionId revision) const;
    void cleanup_old_snapshots(size_t keep_count = 10);
    
    // Persistence
    void save_to_stream(std::ostream& stream) const;
    void load_from_stream(std::istream& stream);
    
    // Statistics
    size_t command_count() const noexcept;
    RevisionId latest_revision() const noexcept;
    std::chrono::milliseconds total_execution_time() const noexcept;
    
private:
    std::vector<JournalEntry> entries_;
    std::map<RevisionId, std::filesystem::path> snapshots_;
    JournalConfig config_;
};

struct JournalConfig {
    size_t snapshot_frequency = 500;           // Commands between snapshots
    std::chrono::minutes snapshot_interval{5}; // Time between snapshots
    bool snapshot_before_destructive = true;   // Before flatten, rasterize, etc.
    size_t max_memory_entries = 10000;         // Keep in memory
    CompressionLevel compression = CompressionLevel::Balanced;
};

}
```

### File Format Handler
```cpp
// src/io/qcsx_format.hpp
namespace QuantumCanvas::IO {

class QCSXFormat {
public:
    // Document I/O
    std::unique_ptr<Document> load(const std::filesystem::path& path);
    void save(const Document& document, const std::filesystem::path& path);
    
    // Incremental save (append to journal)
    void append_to_journal(const std::filesystem::path& path, 
                          const Commands::ICommand& command,
                          const CommandContext& context);
    
    // Asset management
    void embed_asset(const std::filesystem::path& document_path,
                    const std::string& asset_id,
                    const AssetData& data);
    std::optional<AssetData> extract_asset(const std::filesystem::path& document_path,
                                          const std::string& asset_id);
    
    // Validation and repair
    ValidationResult validate_document(const std::filesystem::path& path);
    RepairResult repair_document(const std::filesystem::path& path);
    
    // Migration support
    bool needs_migration(const std::filesystem::path& path) const;
    MigrationResult migrate_document(const std::filesystem::path& path,
                                   const std::string& target_version);
    
private:
    struct Implementation;
    std::unique_ptr<Implementation> impl_;
};

}
```

---

## Asset Management System

### Font Embedding and Licensing
```cpp
// src/assets/font_asset.hpp
namespace QuantumCanvas::Assets {

enum class FontEmbeddingRights {
    NoEmbedding         = 0x0000,
    PreviewAndPrint     = 0x0004,
    Editable           = 0x0008,
    Installable        = 0x0000,
    RestrictedLicense  = 0x0002,
    BitmapOnly         = 0x0200
};

struct FontAsset {
    std::string family_name;
    std::string style_name;
    FontEmbeddingRights rights;
    std::string license_info;
    std::vector<uint8_t> font_data;
    std::string checksum;
    
    // Subset data for PDF export
    std::optional<std::vector<uint8_t>> subset_data;
    std::set<uint32_t> used_glyphs;
};

class FontAssetManager {
public:
    FontAssetId embed_font(const std::filesystem::path& font_path);
    std::optional<FontAsset> get_font(FontAssetId id) const;
    
    // License validation
    bool can_embed(FontAssetId id) const;
    bool can_subset(FontAssetId id) const;
    
    // PDF export preparation
    FontAsset create_subset(FontAssetId id, const std::set<uint32_t>& glyphs);
    
    // Audit trail
    std::vector<FontLicenseInfo> generate_license_report() const;
};

}
```

### Color Profile Management
```cpp
// src/assets/color_profile.hpp
namespace QuantumCanvas::Assets {

struct ColorProfile {
    std::string name;
    ColorSpace color_space;
    std::vector<uint8_t> icc_data;
    std::string checksum;
    
    // Profile metadata
    std::string description;
    std::string copyright;
    std::chrono::system_clock::time_point created;
};

class ColorProfileManager {
public:
    ProfileId embed_profile(const std::filesystem::path& icc_path);
    ProfileId embed_profile(const ColorProfile& profile);
    
    std::optional<ColorProfile> get_profile(ProfileId id) const;
    
    // System profiles
    ProfileId get_system_srgb() const;
    ProfileId get_system_display_p3() const;
    ProfileId get_system_adobe_rgb() const;
    
    // Validation
    bool validate_profile(const ColorProfile& profile) const;
};

}
```

---

## Migration and Versioning

### Schema Migration System
```cpp
// src/document/migration.hpp
namespace QuantumCanvas::Migration {

struct SchemaVersion {
    uint16_t major;
    uint16_t minor;
    uint16_t patch;
    
    bool operator<(const SchemaVersion& other) const noexcept;
    std::string to_string() const;
    static SchemaVersion from_string(const std::string& version_str);
};

class DocumentMigrator {
public:
    // Registration of migrators
    void register_migrator(SchemaVersion from, SchemaVersion to,
                          std::unique_ptr<IMigrator> migrator);
    
    // Migration execution
    MigrationResult migrate(const std::filesystem::path& document_path,
                           SchemaVersion target_version);
    
    // Query capabilities
    bool can_migrate(SchemaVersion from, SchemaVersion to) const;
    std::vector<SchemaVersion> get_migration_path(SchemaVersion from,
                                                 SchemaVersion to) const;
    
private:
    std::map<std::pair<SchemaVersion, SchemaVersion>, 
             std::unique_ptr<IMigrator>> migrators_;
};

class IMigrator {
public:
    virtual ~IMigrator() = default;
    virtual MigrationResult migrate(nlohmann::json& document_data) = 0;
    virtual std::string description() const = 0;
    virtual bool is_reversible() const = 0;
};

}
```

### Example Migration (1.1.0 → 1.2.0)
```cpp
// src/document/migrations/migrate_1_1_to_1_2.hpp
namespace QuantumCanvas::Migration {

class Migrate_1_1_to_1_2 : public IMigrator {
public:
    MigrationResult migrate(nlohmann::json& document_data) override {
        // Add color_management section to metadata
        if (!document_data["meta"].contains("color_management")) {
            document_data["meta"]["color_management"] = {
                {"working_profile", "sRGB IEC61966-2.1"},
                {"output_profile", "sRGB IEC61966-2.1"},
                {"rendering_intent", "relative_colorimetric"},
                {"black_point_compensation", true},
                {"embedded_profiles", nlohmann::json::array()}
            };
        }
        
        // Update schema version
        document_data["meta"]["schema_version"] = "1.2.0";
        
        return MigrationResult::Success;
    }
    
    std::string description() const override {
        return "Add comprehensive color management support";
    }
    
    bool is_reversible() const override {
        return true; // Can downgrade by removing color_management
    }
};

}
```

---

## Performance and Validation

### Document Validation
```cpp
// src/document/validation.hpp
namespace QuantumCanvas::Validation {

enum class ValidationSeverity {
    Info,
    Warning,
    Error,
    Critical
};

struct ValidationIssue {
    ValidationSeverity severity;
    std::string code;
    std::string message;
    std::string location; // JSON path or object ID
    std::optional<nlohmann::json> suggested_fix;
};

class DocumentValidator {
public:
    // Full document validation
    ValidationResult validate(const nlohmann::json& document_data);
    ValidationResult validate(const std::filesystem::path& document_path);
    
    // Incremental validation (for real-time editing)
    ValidationResult validate_layer(const nlohmann::json& layer_data);
    ValidationResult validate_object(const nlohmann::json& object_data);
    
    // Schema validation
    ValidationResult validate_schema(const nlohmann::json& document_data,
                                   const SchemaVersion& expected_version);
    
    // Repair suggestions
    std::vector<RepairAction> suggest_repairs(const ValidationResult& result);
    RepairResult apply_repair(const std::filesystem::path& document_path,
                            const RepairAction& action);
    
private:
    // Individual validation rules
    void validate_metadata(const nlohmann::json& meta, ValidationResult& result);
    void validate_scene_graph(const nlohmann::json& scene, ValidationResult& result);
    void validate_journal_integrity(const std::filesystem::path& document_path, 
                                   ValidationResult& result);
    void validate_asset_references(const nlohmann::json& document_data,
                                  ValidationResult& result);
};

}
```

This comprehensive document format specification provides the foundation for implementing a professional-grade creative software with full version control, collaboration support, and asset management capabilities. The format is designed to be extensible, performant, and suitable for both local and cloud-based workflows.
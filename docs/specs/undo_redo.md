# QuantumCanvas Studio Undo/Redo System Specification
## Command Pattern with Event Sourcing for Professional Creative Software

### System Overview

The QuantumCanvas Studio undo/redo system implements a comprehensive command pattern architecture combined with event sourcing to provide reliable, efficient, and collaborative undo/redo functionality across all application modules.

#### Key Features
- **Universal**: All operations (Vector, Raster, CAD, UI) use the same command system
- **Transactional**: Support for atomic multi-step operations with rollback
- **Collaborative**: Operational Transform (OT) support for real-time collaboration
- **Persistent**: Event sourcing with journal persistence and snapshots
- **Performance**: Memory-efficient with lazy loading and compression
- **Deterministic**: Reproducible operations for testing and debugging

---

## Architecture Overview

### Command Flow Architecture
```
User Action → Command Creation → Validation → Execution → Journal → UI Update
     ↓              ↓              ↓           ↓          ↓        ↓
UI Event → ICommand → Validator → CommandBus → Journal → EventBus
     ↑                             ↑           ↑
Undo/Redo ← Command Inverse ← History Manager
```

### Core Components
1. **Command Interface**: Unified interface for all operations
2. **Command Bus**: Central dispatch and validation system  
3. **History Manager**: Undo/redo stack management
4. **Journal System**: Persistent event log with snapshots
5. **Transaction Manager**: Multi-command atomic operations
6. **Collaboration Engine**: Operational Transform for concurrent editing

---

## Command Interface Specification

### Base Command Interface
```cpp
// src/core/commands/command_interface.hpp
namespace QuantumCanvas::Commands {

using CommandId = uint64_t;
using RevisionId = uint64_t;
using UserId = std::string;

struct CommandContext {
    CommandId id;
    RevisionId base_revision;
    std::chrono::system_clock::time_point timestamp;
    UserId user_id;
    std::string session_id;
    nlohmann::json metadata;
    
    // Performance tracking
    std::chrono::microseconds validation_time{0};
    std::chrono::microseconds execution_time{0};
    size_t memory_usage = 0;
};

enum class CommandResult {
    Success,
    ValidationFailed,
    ExecutionFailed,
    Conflict,
    InsufficientPermissions,
    ResourceLocked
};

class ICommand {
public:
    virtual ~ICommand() = default;
    
    // Command identification
    virtual std::string type() const = 0;
    virtual CommandId id() const = 0;
    virtual std::string description() const = 0;
    
    // Execution lifecycle
    virtual ValidationResult validate(const DocumentModel& model) const = 0;
    virtual CommandResult execute(DocumentModel& model) = 0;
    virtual CommandResult undo(DocumentModel& model) = 0;
    virtual CommandResult redo(DocumentModel& model) = 0;
    
    // State management
    virtual bool can_merge_with(const ICommand& other) const = 0;
    virtual std::unique_ptr<ICommand> merge_with(const ICommand& other) const = 0;
    virtual bool modifies_resource(const ResourceId& resource_id) const = 0;
    
    // Operational Transform support (for collaboration)
    virtual std::unique_ptr<ICommand> transform(const ICommand& concurrent_op) const = 0;
    virtual bool conflicts_with(const ICommand& other) const = 0;
    virtual ConflictResolution resolve_conflict(const ICommand& other) const = 0;
    
    // Serialization
    virtual nlohmann::json serialize() const = 0;
    virtual void deserialize(const nlohmann::json& data) = 0;
    
    // Memory management
    virtual size_t estimated_memory_usage() const = 0;
    virtual void optimize_memory_usage() = 0;
    
    // Dependencies
    virtual std::vector<CommandId> get_dependencies() const = 0;
    virtual void set_dependencies(const std::vector<CommandId>& deps) = 0;
};

// Validation result with detailed feedback
struct ValidationResult {
    bool is_valid = false;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    std::optional<std::string> suggested_fix;
    
    operator bool() const { return is_valid; }
};

// Conflict resolution strategies
enum class ConflictResolution {
    Accept,      // Accept the concurrent operation
    Reject,      // Reject the concurrent operation
    Transform,   // Transform both operations
    Merge,       // Merge operations if possible
    UserChoice   // Require user intervention
};

}
```

### Command Categories

#### Vector Graphics Commands
```cpp
// src/modules/vector/commands/vector_commands.hpp
namespace QuantumCanvas::Vector::Commands {

class CreatePathCommand : public ICommand {
public:
    CreatePathCommand(LayerId layer_id, const VectorPath& path);
    
    std::string type() const override { return "vector.create_path"; }
    std::string description() const override;
    
    ValidationResult validate(const DocumentModel& model) const override;
    CommandResult execute(DocumentModel& model) override;
    CommandResult undo(DocumentModel& model) override;
    
    // Serialization
    nlohmann::json serialize() const override;
    void deserialize(const nlohmann::json& data) override;
    
    // Operational Transform
    std::unique_ptr<ICommand> transform(const ICommand& concurrent_op) const override;
    bool conflicts_with(const ICommand& other) const override;
    
private:
    LayerId layer_id_;
    VectorPath path_;
    ObjectId created_object_id_;  // Set after execution
};

class ModifyPathCommand : public ICommand {
public:
    ModifyPathCommand(ObjectId object_id, const PathDelta& delta);
    
    std::string type() const override { return "vector.modify_path"; }
    ValidationResult validate(const DocumentModel& model) const override;
    CommandResult execute(DocumentModel& model) override;
    CommandResult undo(DocumentModel& model) override;
    
    // Command merging for continuous operations (like drawing)
    bool can_merge_with(const ICommand& other) const override;
    std::unique_ptr<ICommand> merge_with(const ICommand& other) const override;
    
private:
    ObjectId object_id_;
    PathDelta delta_;
    PathDelta inverse_delta_;  // For undo
};

class ApplyBooleanOperationCommand : public ICommand {
public:
    ApplyBooleanOperationCommand(const std::vector<ObjectId>& operands,
                               BooleanOperation operation);
    
    std::string type() const override { return "vector.boolean_operation"; }
    ValidationResult validate(const DocumentModel& model) const override;
    CommandResult execute(DocumentModel& model) override;
    CommandResult undo(DocumentModel& model) override;
    
    // High memory usage - complex geometry operations
    size_t estimated_memory_usage() const override;
    void optimize_memory_usage() override;
    
private:
    std::vector<ObjectId> operand_ids_;
    BooleanOperation operation_;
    std::vector<VectorObject> backup_objects_;  // For undo
    ObjectId result_object_id_;
};

}
```

#### Raster Graphics Commands
```cpp
// src/modules/raster/commands/raster_commands.hpp
namespace QuantumCanvas::Raster::Commands {

class ApplyBrushStrokeCommand : public ICommand {
public:
    ApplyBrushStrokeCommand(LayerId layer_id, const BrushStroke& stroke);
    
    std::string type() const override { return "raster.brush_stroke"; }
    ValidationResult validate(const DocumentModel& model) const override;
    CommandResult execute(DocumentModel& model) override;
    CommandResult undo(DocumentModel& model) override;
    
    // Continuous brush strokes can be merged
    bool can_merge_with(const ICommand& other) const override;
    std::unique_ptr<ICommand> merge_with(const ICommand& other) const override;
    
    // Large memory usage for undo data
    size_t estimated_memory_usage() const override;
    void optimize_memory_usage() override;
    
private:
    LayerId layer_id_;
    BrushStroke stroke_;
    ImageRegion affected_region_;
    CompressedImageData undo_data_;  // Compressed original pixels
};

class ApplyFilterCommand : public ICommand {
public:
    ApplyFilterCommand(LayerId layer_id, std::unique_ptr<IFilter> filter,
                      const ImageRegion& region);
    
    std::string type() const override { return "raster.apply_filter"; }
    ValidationResult validate(const DocumentModel& model) const override;
    CommandResult execute(DocumentModel& model) override;
    CommandResult undo(DocumentModel& model) override;
    
    // Filters typically cannot be merged
    bool can_merge_with(const ICommand& other) const override { return false; }
    
private:
    LayerId layer_id_;
    std::unique_ptr<IFilter> filter_;
    ImageRegion region_;
    CompressedImageData original_data_;
};

class AdjustColorsCommand : public ICommand {
public:
    AdjustColorsCommand(LayerId layer_id, const ColorAdjustment& adjustment);
    
    std::string type() const override { return "raster.adjust_colors"; }
    ValidationResult validate(const DocumentModel& model) const override;
    CommandResult execute(DocumentModel& model) override;
    CommandResult undo(DocumentModel& model) override;
    
    // Color adjustments can often be merged
    bool can_merge_with(const ICommand& other) const override;
    std::unique_ptr<ICommand> merge_with(const ICommand& other) const override;
    
private:
    LayerId layer_id_;
    ColorAdjustment adjustment_;
    ColorAdjustment inverse_adjustment_;
};

}
```

#### CAD Commands
```cpp
// src/modules/cad/commands/cad_commands.hpp  
namespace QuantumCanvas::CAD::Commands {

class CreateConstraintCommand : public ICommand {
public:
    CreateConstraintCommand(const std::vector<ObjectId>& objects,
                          std::unique_ptr<IConstraint> constraint);
    
    std::string type() const override { return "cad.create_constraint"; }
    ValidationResult validate(const DocumentModel& model) const override;
    CommandResult execute(DocumentModel& model) override;
    CommandResult undo(DocumentModel& model) override;
    
    // Constraints have complex dependencies
    std::vector<CommandId> get_dependencies() const override;
    
private:
    std::vector<ObjectId> object_ids_;
    std::unique_ptr<IConstraint> constraint_;
    ConstraintId created_constraint_id_;
};

class SolveConstraintsCommand : public ICommand {
public:
    SolveConstraintsCommand(const std::vector<ConstraintId>& constraint_ids);
    
    std::string type() const override { return "cad.solve_constraints"; }
    ValidationResult validate(const DocumentModel& model) const override;
    CommandResult execute(DocumentModel& model) override;
    CommandResult undo(DocumentModel& model) override;
    
    // Complex computational operation
    size_t estimated_memory_usage() const override;
    
private:
    std::vector<ConstraintId> constraint_ids_;
    std::vector<ObjectTransform> original_transforms_;
    SolverResult solver_result_;
};

class CreateDimensionCommand : public ICommand {
public:
    CreateDimensionCommand(const std::vector<ObjectId>& measured_objects,
                          DimensionType type, const Point2D& position);
    
    std::string type() const override { return "cad.create_dimension"; }
    ValidationResult validate(const DocumentModel& model) const override;
    CommandResult execute(DocumentModel& model) override;
    CommandResult undo(DocumentModel& model) override;
    
private:
    std::vector<ObjectId> measured_objects_;
    DimensionType dimension_type_;
    Point2D position_;
    ObjectId created_dimension_id_;
};

}
```

---

## Command Bus and Dispatch System

### Central Command Bus
```cpp
// src/core/commands/command_bus.hpp
namespace QuantumCanvas::Commands {

class CommandBus {
public:
    explicit CommandBus(DocumentModel& document);
    ~CommandBus();
    
    // Core dispatch functionality
    CommandResult dispatch(std::unique_ptr<ICommand> command);
    CommandResult dispatch_batch(std::vector<std::unique_ptr<ICommand>> commands);
    
    // Transaction support
    void begin_transaction(const std::string& description = "");
    CommandResult commit_transaction();
    void rollback_transaction();
    bool is_in_transaction() const;
    
    // Validation
    ValidationResult validate_command(const ICommand& command) const;
    ValidationResult validate_batch(const std::vector<std::unique_ptr<ICommand>>& commands) const;
    
    // History integration
    void set_history_manager(std::shared_ptr<HistoryManager> history_manager);
    
    // Collaboration integration
    void set_collaboration_engine(std::shared_ptr<CollaborationEngine> collab_engine);
    
    // Event notifications
    using CommandExecutedCallback = std::function<void(const CommandContext&, CommandResult)>;
    void on_command_executed(CommandExecutedCallback callback);
    
    // Performance monitoring
    struct PerformanceStats {
        size_t commands_executed = 0;
        std::chrono::microseconds total_validation_time{0};
        std::chrono::microseconds total_execution_time{0};
        size_t total_memory_usage = 0;
        size_t failed_validations = 0;
        size_t failed_executions = 0;
    };
    
    PerformanceStats get_performance_stats() const;
    void reset_performance_stats();
    
private:
    DocumentModel& document_;
    std::shared_ptr<HistoryManager> history_manager_;
    std::shared_ptr<CollaborationEngine> collaboration_engine_;
    
    // Transaction state
    struct Transaction {
        std::string description;
        std::vector<std::unique_ptr<ICommand>> commands;
        RevisionId start_revision;
        std::chrono::system_clock::time_point start_time;
    };
    std::optional<Transaction> current_transaction_;
    
    // Event callbacks
    std::vector<CommandExecutedCallback> command_executed_callbacks_;
    
    // Performance tracking
    mutable std::mutex stats_mutex_;
    PerformanceStats performance_stats_;
    
    // Internal methods
    CommandResult execute_command(std::unique_ptr<ICommand> command);
    void notify_command_executed(const CommandContext& context, CommandResult result);
    CommandContext create_command_context(const ICommand& command) const;
};

}
```

### Command Factory and Registry
```cpp
// src/core/commands/command_factory.hpp
namespace QuantumCanvas::Commands {

class CommandFactory {
public:
    static CommandFactory& instance();
    
    // Command registration
    template<typename CommandType>
    void register_command(const std::string& type_name);
    
    // Command creation
    std::unique_ptr<ICommand> create(const std::string& type_name) const;
    std::unique_ptr<ICommand> deserialize(const nlohmann::json& data) const;
    
    // Introspection
    std::vector<std::string> get_registered_types() const;
    bool is_type_registered(const std::string& type_name) const;
    
    // Validation
    ValidationResult validate_command_data(const nlohmann::json& data) const;
    
private:
    CommandFactory() = default;
    
    using CreateFunction = std::function<std::unique_ptr<ICommand>()>;
    std::unordered_map<std::string, CreateFunction> creators_;
    std::mutex registry_mutex_;
};

// Registration macro for convenience
#define REGISTER_COMMAND(CommandType, TypeName) \
    CommandFactory::instance().register_command<CommandType>(TypeName)

// Auto-registration helper
template<typename CommandType>
class CommandRegistrar {
public:
    explicit CommandRegistrar(const std::string& type_name) {
        CommandFactory::instance().register_command<CommandType>(type_name);
    }
};

#define AUTO_REGISTER_COMMAND(CommandType, TypeName) \
    static CommandRegistrar<CommandType> g_##CommandType##_registrar(TypeName)

}
```

---

## History Management System

### History Manager
```cpp
// src/core/commands/history_manager.hpp
namespace QuantumCanvas::Commands {

struct HistoryConfig {
    size_t max_undo_levels = 1000;
    size_t memory_limit_mb = 512;
    bool enable_command_merging = true;
    std::chrono::minutes merge_timeout{2};
    bool compress_old_commands = true;
    size_t compression_threshold = 100; // Commands before compression
};

class HistoryManager {
public:
    explicit HistoryManager(const HistoryConfig& config = {});
    ~HistoryManager();
    
    // Command recording
    void record_command(std::unique_ptr<ICommand> command, const CommandContext& context);
    void record_batch(std::vector<std::unique_ptr<ICommand>> commands, 
                     const std::string& batch_description);
    
    // Undo/Redo operations
    bool can_undo() const;
    bool can_redo() const;
    CommandResult undo(DocumentModel& model, size_t steps = 1);
    CommandResult redo(DocumentModel& model, size_t steps = 1);
    
    // History navigation
    size_t undo_count() const;
    size_t redo_count() const;
    std::vector<std::string> get_undo_descriptions(size_t count = 10) const;
    std::vector<std::string> get_redo_descriptions(size_t count = 10) const;
    
    // History manipulation
    void clear_history();
    void clear_redo_history();
    void set_history_limit(size_t max_commands);
    
    // Branching support (for advanced workflows)
    struct HistoryBranch {
        std::string name;
        RevisionId branch_point;
        std::vector<std::unique_ptr<ICommand>> commands;
    };
    
    void create_branch(const std::string& name);
    void switch_to_branch(const std::string& name);
    std::vector<std::string> get_branch_names() const;
    
    // Memory management
    size_t get_memory_usage() const;
    void optimize_memory_usage();
    void set_memory_limit(size_t limit_mb);
    
    // Persistence (for crash recovery)
    void save_to_file(const std::filesystem::path& path) const;
    void load_from_file(const std::filesystem::path& path);
    
    // Statistics
    struct HistoryStats {
        size_t total_commands_recorded = 0;
        size_t total_undos_performed = 0;
        size_t total_redos_performed = 0;
        size_t memory_usage_bytes = 0;
        size_t compressed_commands = 0;
        std::chrono::milliseconds average_undo_time{0};
        std::chrono::milliseconds average_redo_time{0};
    };
    
    HistoryStats get_statistics() const;
    
private:
    HistoryConfig config_;
    
    struct HistoryEntry {
        std::unique_ptr<ICommand> command;
        CommandContext context;
        std::chrono::system_clock::time_point recorded_time;
        size_t memory_usage = 0;
        bool is_compressed = false;
        
        // For batch operations
        bool is_batch_start = false;
        bool is_batch_end = false;
        std::string batch_description;
    };
    
    std::vector<HistoryEntry> undo_stack_;
    std::vector<HistoryEntry> redo_stack_;
    size_t current_position_ = 0;
    
    // Branching support
    std::unordered_map<std::string, HistoryBranch> branches_;
    std::string current_branch_ = "main";
    
    // Memory management
    mutable std::mutex memory_mutex_;
    size_t current_memory_usage_ = 0;
    
    // Statistics
    mutable std::mutex stats_mutex_;
    HistoryStats statistics_;
    
    // Internal methods
    void trim_history();
    void compress_old_commands();
    bool try_merge_command(std::unique_ptr<ICommand>& new_command);
    size_t calculate_command_memory_usage(const ICommand& command) const;
    void update_memory_usage();
    void cleanup_memory_if_needed();
};

}
```

### Undo/Redo UI Integration
```cpp
// src/ui/commands/history_widget.hpp
namespace QuantumCanvas::UI {

class HistoryWidget : public Widget {
public:
    explicit HistoryWidget(std::shared_ptr<Commands::HistoryManager> history_manager);
    
    // Widget interface
    void render() override;
    void handle_event(const Event& event) override;
    
    // History visualization
    void show_undo_list(bool show);
    void show_redo_list(bool show);
    void show_memory_usage(bool show);
    
    // Interaction callbacks
    using UndoCallback = std::function<void(size_t steps)>;
    using RedoCallback = std::function<void(size_t steps)>;
    
    void set_undo_callback(UndoCallback callback);
    void set_redo_callback(RedoCallback callback);
    
private:
    std::shared_ptr<Commands::HistoryManager> history_manager_;
    
    // UI state
    bool show_undo_list_ = false;
    bool show_redo_list_ = false;
    bool show_memory_usage_ = false;
    
    // Callbacks
    UndoCallback undo_callback_;
    RedoCallback redo_callback_;
    
    // Rendering helpers
    void render_undo_button();
    void render_redo_button();
    void render_history_list();
    void render_memory_indicator();
};

// Global keyboard shortcuts
class UndoRedoShortcuts {
public:
    static void register_shortcuts(InputManager& input_manager,
                                  std::shared_ptr<Commands::HistoryManager> history_manager);
    
private:
    static void handle_undo_shortcut(const KeyEvent& event);
    static void handle_redo_shortcut(const KeyEvent& event);
};

}
```

---

## Transaction System

### Transaction Manager
```cpp
// src/core/commands/transaction_manager.hpp
namespace QuantumCanvas::Commands {

enum class TransactionIsolation {
    ReadUncommitted,  // Allow dirty reads
    ReadCommitted,    // Prevent dirty reads
    RepeatableRead,   // Prevent dirty and non-repeatable reads
    Serializable      // Prevent all anomalies
};

class TransactionManager {
public:
    explicit TransactionManager(DocumentModel& document);
    
    // Transaction lifecycle
    TransactionId begin_transaction(const std::string& description = "",
                                  TransactionIsolation isolation = TransactionIsolation::ReadCommitted);
    CommandResult commit_transaction(TransactionId transaction_id);
    void rollback_transaction(TransactionId transaction_id);
    
    // Nested transactions
    TransactionId begin_nested_transaction(TransactionId parent_id, const std::string& description = "");
    
    // Transaction state
    bool is_transaction_active(TransactionId transaction_id) const;
    std::vector<TransactionId> get_active_transactions() const;
    
    // Savepoints (for partial rollback)
    SavepointId create_savepoint(TransactionId transaction_id, const std::string& name);
    void rollback_to_savepoint(SavepointId savepoint_id);
    void release_savepoint(SavepointId savepoint_id);
    
    // Lock management
    bool acquire_read_lock(const ResourceId& resource_id, TransactionId transaction_id);
    bool acquire_write_lock(const ResourceId& resource_id, TransactionId transaction_id);
    void release_locks(TransactionId transaction_id);
    
    // Deadlock detection and resolution
    void enable_deadlock_detection(bool enable);
    std::vector<TransactionId> detect_deadlocks() const;
    void resolve_deadlock(const std::vector<TransactionId>& deadlocked_transactions);
    
private:
    DocumentModel& document_;
    
    struct Transaction {
        TransactionId id;
        std::string description;
        TransactionIsolation isolation;
        std::optional<TransactionId> parent_id;
        RevisionId start_revision;
        std::chrono::system_clock::time_point start_time;
        std::vector<std::unique_ptr<ICommand>> commands;
        std::vector<SavepointId> savepoints;
        std::set<ResourceId> read_locks;
        std::set<ResourceId> write_locks;
        bool is_committed = false;
        bool is_rolled_back = false;
    };
    
    std::unordered_map<TransactionId, std::unique_ptr<Transaction>> active_transactions_;
    
    // Lock management
    struct ResourceLock {
        ResourceId resource_id;
        TransactionId holder;
        bool is_write_lock;
        std::chrono::system_clock::time_point acquired_time;
    };
    
    std::vector<ResourceLock> active_locks_;
    std::mutex locks_mutex_;
    
    // Deadlock detection
    bool deadlock_detection_enabled_ = true;
    std::thread deadlock_detection_thread_;
    std::atomic<bool> should_stop_deadlock_detection_{false};
    
    // Internal methods
    TransactionId generate_transaction_id();
    SavepointId generate_savepoint_id();
    void cleanup_completed_transaction(TransactionId transaction_id);
    void run_deadlock_detection();
    bool would_create_cycle(TransactionId transaction_id, const ResourceId& resource_id) const;
};

}
```

### RAII Transaction Guard
```cpp
// src/core/commands/transaction_guard.hpp
namespace QuantumCanvas::Commands {

class TransactionGuard {
public:
    explicit TransactionGuard(CommandBus& command_bus, const std::string& description = "");
    ~TransactionGuard();
    
    // Disable copy, enable move
    TransactionGuard(const TransactionGuard&) = delete;
    TransactionGuard& operator=(const TransactionGuard&) = delete;
    TransactionGuard(TransactionGuard&& other) noexcept;
    TransactionGuard& operator=(TransactionGuard&& other) noexcept;
    
    // Manual control
    CommandResult commit();
    void rollback();
    void release(); // Don't auto-rollback on destruction
    
    // Status
    bool is_active() const;
    bool is_committed() const;
    
private:
    CommandBus* command_bus_;
    bool is_active_ = true;
    bool is_committed_ = false;
    std::string description_;
};

// Usage example:
// {
//     TransactionGuard transaction(command_bus, "Complex Operation");
//     
//     command_bus.dispatch(std::make_unique<CreateLayerCommand>(...));
//     command_bus.dispatch(std::make_unique<AddObjectCommand>(...));
//     command_bus.dispatch(std::make_unique<ApplyStyleCommand>(...));
//     
//     if (some_condition) {
//         transaction.commit(); // Explicit commit
//     }
//     // Otherwise auto-rollback on destruction
// }

}
```

---

## Collaboration Integration

### Operational Transform for Real-Time Collaboration
```cpp
// src/core/commands/operational_transform.hpp
namespace QuantumCanvas::Collaboration {

enum class OperationPriority {
    Low = 0,
    Normal = 1,
    High = 2,
    System = 3
};

struct OperationContext {
    Commands::CommandId command_id;
    Commands::UserId user_id;
    std::string session_id;
    Commands::RevisionId base_revision;
    OperationPriority priority = OperationPriority::Normal;
    std::chrono::system_clock::time_point timestamp;
};

class OperationalTransformEngine {
public:
    // Transform operations for concurrent execution
    struct TransformResult {
        std::unique_ptr<Commands::ICommand> transformed_op;
        std::unique_ptr<Commands::ICommand> concurrent_op;
        bool can_execute_concurrently;
        std::vector<std::string> warnings;
    };
    
    TransformResult transform_operations(std::unique_ptr<Commands::ICommand> operation,
                                       std::unique_ptr<Commands::ICommand> concurrent_operation) const;
    
    // Batch transformation for multiple concurrent operations
    std::vector<std::unique_ptr<Commands::ICommand>> transform_against_history(
        std::unique_ptr<Commands::ICommand> operation,
        const std::vector<Commands::ICommand*>& history,
        Commands::RevisionId base_revision) const;
    
    // Conflict detection and resolution
    enum class ConflictType {
        None,
        ResourceConflict,   // Same resource modified
        OrderConflict,      // Order-dependent operations
        SemanticConflict,   // Semantically incompatible
        CausalConflict      // Violates causal ordering
    };
    
    ConflictType detect_conflict(const Commands::ICommand& op1, 
                               const Commands::ICommand& op2) const;
    
    // Resolve conflicts based on policy
    enum class ConflictResolutionPolicy {
        LastWriterWins,     // Timestamp-based
        UserPriorityBased,  // User hierarchy
        OperationPriority,  // Operation priority
        Manual             // Require user intervention
    };
    
    std::unique_ptr<Commands::ICommand> resolve_conflict(
        std::unique_ptr<Commands::ICommand> op1,
        std::unique_ptr<Commands::ICommand> op2,
        ConflictResolutionPolicy policy) const;
    
    // Causal ordering validation
    bool validates_causal_ordering(const std::vector<OperationContext>& operations) const;
    std::vector<OperationContext> sort_by_causal_order(std::vector<OperationContext> operations) const;
    
private:
    // Type-specific transform implementations
    std::unordered_map<std::string, 
                      std::function<TransformResult(std::unique_ptr<Commands::ICommand>,
                                                  std::unique_ptr<Commands::ICommand>)>> 
                      transform_functions_;
    
    void register_default_transforms();
};

}
```

### Collaboration-Aware Command Bus
```cpp
// src/core/commands/collaborative_command_bus.hpp
namespace QuantumCanvas::Collaboration {

class CollaborativeCommandBus : public Commands::CommandBus {
public:
    explicit CollaborativeCommandBus(DocumentModel& document,
                                   std::shared_ptr<OperationalTransformEngine> ot_engine);
    
    // Collaborative dispatch
    Commands::CommandResult dispatch_local(std::unique_ptr<Commands::ICommand> command);
    Commands::CommandResult dispatch_remote(std::unique_ptr<Commands::ICommand> command,
                                          const OperationContext& context);
    
    // Operation synchronization
    void sync_with_server();
    void handle_remote_operations(const std::vector<OperationContext>& operations);
    
    // Conflict resolution
    using ConflictCallback = std::function<Commands::ICommand*(
        std::unique_ptr<Commands::ICommand> local_op,
        std::unique_ptr<Commands::ICommand> remote_op)>;
    
    void set_conflict_resolution_callback(ConflictCallback callback);
    
    // Connection state
    void set_connected(bool connected);
    bool is_connected() const;
    
    // Offline operation queue
    std::vector<std::unique_ptr<Commands::ICommand>> get_pending_operations() const;
    void clear_pending_operations();
    
private:
    std::shared_ptr<OperationalTransformEngine> ot_engine_;
    ConflictCallback conflict_callback_;
    
    // Network state
    bool is_connected_ = false;
    std::vector<std::unique_ptr<Commands::ICommand>> pending_operations_;
    
    // Operation queues
    std::queue<OperationContext> incoming_operations_;
    std::queue<OperationContext> outgoing_operations_;
    
    std::mutex operations_mutex_;
    
    // Internal methods
    void process_incoming_operations();
    void send_pending_operations();
    Commands::CommandResult apply_with_transform(std::unique_ptr<Commands::ICommand> command);
};

}
```

---

## Performance Optimization

### Memory-Efficient Command Storage
```cpp
// src/core/commands/command_compression.hpp
namespace QuantumCanvas::Commands {

class CommandCompressor {
public:
    // Compress command data
    struct CompressedCommand {
        std::string type;
        std::vector<uint8_t> compressed_data;
        size_t original_size;
        CompressionAlgorithm algorithm;
        uint32_t checksum;
    };
    
    static CompressedCommand compress(const ICommand& command);
    static std::unique_ptr<ICommand> decompress(const CompressedCommand& compressed);
    
    // Batch compression for better ratios
    static std::vector<CompressedCommand> compress_batch(
        const std::vector<std::unique_ptr<ICommand>>& commands);
    static std::vector<std::unique_ptr<ICommand>> decompress_batch(
        const std::vector<CompressedCommand>& compressed_commands);
    
    // Configuration
    enum class CompressionLevel {
        Fast,      // Fastest compression, larger size
        Balanced,  // Balance between speed and size  
        Best       // Best compression, slower
    };
    
    static void set_compression_level(CompressionLevel level);
    static void set_compression_algorithm(CompressionAlgorithm algorithm);
    
private:
    static CompressionLevel compression_level_;
    static CompressionAlgorithm compression_algorithm_;
};

// Lazy-loading command proxy
class LazyCommand : public ICommand {
public:
    LazyCommand(const CommandCompressor::CompressedCommand& compressed);
    
    // ICommand interface - loads on first access
    std::string type() const override;
    CommandId id() const override;
    ValidationResult validate(const DocumentModel& model) const override;
    CommandResult execute(DocumentModel& model) override;
    CommandResult undo(DocumentModel& model) override;
    
    // Memory management
    void unload(); // Release decompressed data
    bool is_loaded() const;
    size_t memory_footprint() const;
    
private:
    CommandCompressor::CompressedCommand compressed_data_;
    mutable std::unique_ptr<ICommand> loaded_command_;
    mutable std::mutex load_mutex_;
    
    void ensure_loaded() const;
};

}
```

### Command Merging Strategies
```cpp
// src/core/commands/command_merger.hpp
namespace QuantumCanvas::Commands {

class CommandMerger {
public:
    // Merge compatible commands
    static std::unique_ptr<ICommand> merge_if_possible(
        std::unique_ptr<ICommand> command1,
        std::unique_ptr<ICommand> command2);
    
    // Batch merging
    static std::vector<std::unique_ptr<ICommand>> merge_compatible_commands(
        std::vector<std::unique_ptr<ICommand>> commands);
    
    // Merge policies
    enum class MergePolicy {
        Aggressive,  // Merge whenever possible
        Conservative, // Only merge obviously safe operations  
        Off          // No merging
    };
    
    static void set_merge_policy(MergePolicy policy);
    
    // Time-based merging
    static void set_merge_timeout(std::chrono::milliseconds timeout);
    
    // Memory-based merging
    static void set_memory_threshold(size_t threshold_bytes);
    
private:
    static MergePolicy merge_policy_;
    static std::chrono::milliseconds merge_timeout_;
    static size_t memory_threshold_;
    
    // Type-specific merge handlers
    static std::unordered_map<std::string, 
                             std::function<std::unique_ptr<ICommand>(
                                 std::unique_ptr<ICommand>, 
                                 std::unique_ptr<ICommand>)>> merge_handlers_;
    
    static void register_default_merge_handlers();
};

}
```

This comprehensive undo/redo system specification provides the foundation for implementing professional-grade command management with full collaboration support, transaction safety, and optimal performance characteristics suitable for creative software applications.
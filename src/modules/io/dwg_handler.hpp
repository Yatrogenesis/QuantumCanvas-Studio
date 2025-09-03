#pragma once

#include "file_format_manager.hpp"
#include "../../core/math/vector2.hpp"
#include "../../core/math/vector3.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <array>

namespace QuantumCanvas::IO {

// Forward declarations for ODA (Open Design Alliance) SDK integration
class OdDbDatabase;
class OdDbObject;
class OdDbEntity;
class OdDbBlockTableRecord;
class OdDbLayerTableRecord;
class OdDbTextStyleTableRecord;
class OdDbLinetypeTableRecord;

// AutoCAD entity types supported
enum class DWGEntityType {
    Unknown,
    Line,
    Arc,
    Circle,
    Ellipse,
    Polyline,
    LWPolyline,  // Lightweight polyline
    Spline,
    Text,
    MText,       // Multiline text
    Hatch,
    Solid,
    Point,
    Block,
    Insert,      // Block reference
    Dimension,
    Leader,
    Viewport,
    Image,
    Region,
    Body,        // 3D Solid
    Surface,
    Mesh
};

// AutoCAD units
enum class DWGUnits {
    Unitless = 0,
    Inches = 1,
    Feet = 2,
    Miles = 3,
    Millimeters = 4,
    Centimeters = 5,
    Meters = 6,
    Kilometers = 7,
    Microinches = 8,
    Mils = 9,
    Yards = 10,
    Angstroms = 11,
    Nanometers = 12,
    Microns = 13,
    Decimeters = 14,
    Decameters = 15,
    Hectometers = 16,
    Gigameters = 17,
    Astronomical = 18,
    LightYears = 19,
    Parsecs = 20
};

// Layer properties from DWG
struct DWGLayer {
    std::string name;
    std::array<float, 3> color{1.0f, 1.0f, 1.0f}; // RGB
    uint16_t colorIndex = 7;  // AutoCAD color index
    std::string linetype = "CONTINUOUS";
    float lineweight = 0.25f;  // mm
    bool visible = true;
    bool locked = false;
    bool plottable = true;
    bool frozen = false;
    
    // Extended properties
    std::string description;
    std::unordered_map<std::string, std::string> xdata;  // Extended data
};

// Block definition from DWG
struct DWGBlock {
    std::string name;
    std::array<float, 2> basePoint{0.0f, 0.0f};
    std::vector<uint32_t> entityIds;  // References to entities in the block
    bool isLayout = false;
    std::string description;
    
    // Block attributes
    struct Attribute {
        std::string tag;
        std::string prompt;
        std::string defaultValue;
        std::array<float, 2> position{0.0f, 0.0f};
        float height = 2.5f;
        float rotation = 0.0f;
        bool visible = true;
        bool constant = false;
        bool verify = false;
        bool preset = false;
    };
    
    std::vector<Attribute> attributes;
};

// Block reference (INSERT) from DWG
struct DWGInsert {
    uint32_t entityId;
    std::string blockName;
    std::array<float, 2> position{0.0f, 0.0f};
    std::array<float, 2> scale{1.0f, 1.0f};
    float rotation = 0.0f;
    
    // Attribute values for this instance
    std::unordered_map<std::string, std::string> attributeValues;
    
    // Array properties (for rectangular arrays)
    uint32_t rowCount = 1;
    uint32_t columnCount = 1;
    float rowSpacing = 0.0f;
    float columnSpacing = 0.0f;
};

// Text style from DWG
struct DWGTextStyle {
    std::string name;
    std::string fontName;
    std::string bigFontName;  // For Asian fonts
    float height = 0.0f;      // 0 = variable height
    float widthFactor = 1.0f;
    float obliqueAngle = 0.0f; // Degrees
    bool backwards = false;
    bool upsideDown = false;
    bool vertical = false;
    
    // TrueType font properties
    bool isTrueType = false;
    uint16_t charset = 1;     // ANSI_CHARSET
    uint8_t pitchAndFamily = 0;
};

// Dimension style from DWG
struct DWGDimensionStyle {
    std::string name;
    
    // Dimension line properties
    std::array<float, 3> dimLineColor{0.0f, 0.0f, 0.0f};
    std::string dimLineLinetype = "CONTINUOUS";
    float dimLineLineweight = 0.25f;
    
    // Extension line properties
    std::array<float, 3> extLineColor{0.0f, 0.0f, 0.0f};
    std::string extLineLinetype = "CONTINUOUS";
    float extLineLineweight = 0.25f;
    float extLineExtension = 1.25f;
    float extLineOffset = 0.625f;
    
    // Arrow properties
    std::string arrowBlock = "";  // Empty = default arrow
    float arrowSize = 2.5f;
    
    // Text properties
    std::string textStyle = "STANDARD";
    std::array<float, 3> textColor{0.0f, 0.0f, 0.0f};
    float textHeight = 2.5f;
    float textGap = 0.625f;
    
    // Tolerances
    bool toleranceDisplay = false;
    float toleranceUpper = 0.0f;
    float toleranceLower = 0.0f;
    
    // Units and precision
    uint16_t linearPrecision = 4;
    uint16_t angularPrecision = 0;
    float linearScaleFactor = 1.0f;
    
    // Fit and placement
    float overallScale = 1.0f;
    bool textInside = true;
    bool textAbove = false;
};

// DWG geometric entity base
struct DWGEntity {
    uint32_t id = 0;
    DWGEntityType type = DWGEntityType::Unknown;
    std::string layer = "0";
    std::array<float, 3> color{1.0f, 1.0f, 1.0f};
    uint16_t colorIndex = 256;  // ByLayer
    std::string linetype = "BYLAYER";
    float lineweight = -1.0f;  // ByLayer
    
    // Visibility and properties
    bool visible = true;
    float transparency = 0.0f;  // 0 = opaque, 1 = transparent
    
    // 3D properties
    float elevation = 0.0f;
    float thickness = 0.0f;
    std::array<float, 3> normal{0.0f, 0.0f, 1.0f};  // Z-axis direction
    
    // Extended data
    std::unordered_map<std::string, std::string> xdata;
    
    // Transformation matrix (for blocks)
    std::array<float, 16> transform{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
};

// Specific entity types
struct DWGLine : public DWGEntity {
    std::array<float, 2> startPoint{0.0f, 0.0f};
    std::array<float, 2> endPoint{0.0f, 0.0f};
    
    DWGLine() { type = DWGEntityType::Line; }
};

struct DWGCircle : public DWGEntity {
    std::array<float, 2> center{0.0f, 0.0f};
    float radius = 1.0f;
    
    DWGCircle() { type = DWGEntityType::Circle; }
};

struct DWGArc : public DWGEntity {
    std::array<float, 2> center{0.0f, 0.0f};
    float radius = 1.0f;
    float startAngle = 0.0f;  // Radians
    float endAngle = 6.28318f; // 2π radians
    
    DWGArc() { type = DWGEntityType::Arc; }
};

struct DWGPolyline : public DWGEntity {
    struct Vertex {
        std::array<float, 2> position{0.0f, 0.0f};
        float bulge = 0.0f;       // Arc bulge factor (tan(angle/4))
        float startWidth = 0.0f;
        float endWidth = 0.0f;
        uint32_t vertexId = 0;
    };
    
    std::vector<Vertex> vertices;
    bool closed = false;
    float constantWidth = 0.0f;
    
    DWGPolyline() { type = DWGEntityType::LWPolyline; }
};

struct DWGText : public DWGEntity {
    std::string content;
    std::array<float, 2> position{0.0f, 0.0f};
    float height = 2.5f;
    float rotation = 0.0f;      // Radians
    float widthFactor = 1.0f;
    float obliqueAngle = 0.0f;  // Radians
    std::string style = "STANDARD";
    
    enum HorizontalAlignment {
        Left = 0, Center = 1, Right = 2, Aligned = 3, Middle = 4, Fit = 5
    };
    enum VerticalAlignment {
        Baseline = 0, Bottom = 1, MiddleV = 2, Top = 3
    };
    
    HorizontalAlignment horizontalAlign = Left;
    VerticalAlignment verticalAlign = Baseline;
    std::array<float, 2> alignmentPoint{0.0f, 0.0f};
    
    DWGText() { type = DWGEntityType::Text; }
};

struct DWGHatch : public DWGEntity {
    enum PatternType {
        UserDefined = 0,
        Predefined = 1,
        Custom = 2
    };
    
    struct BoundaryPath {
        enum PathType {
            External = 1,
            Outermost = 16,
            Default = 0
        };
        
        PathType pathType = Default;
        std::vector<uint32_t> entityIds;  // References to boundary entities
        std::vector<std::array<float, 2>> vertices;  // For polyline boundaries
        bool closed = true;
    };
    
    std::vector<BoundaryPath> boundaryPaths;
    PatternType patternType = Predefined;
    std::string patternName = "SOLID";
    float patternScale = 1.0f;
    float patternAngle = 0.0f;  // Radians
    
    // Solid fill
    bool isSolid = true;
    std::array<float, 3> fillColor{0.5f, 0.5f, 0.5f};
    
    // Gradient fill
    bool hasGradient = false;
    std::vector<std::pair<float, std::array<float, 3>>> gradientStops;
    
    DWGHatch() { type = DWGEntityType::Hatch; }
};

// DWG file information
struct DWGFileInfo {
    // File header information
    std::string version;
    uint32_t maintenanceVersion = 0;
    std::array<uint32_t, 2> imageSeeker{0, 0};
    uint16_t codePage = 30;  // ANSI_1252
    
    // Drawing properties
    std::array<float, 2> extMin{0.0f, 0.0f};
    std::array<float, 2> extMax{0.0f, 0.0f};
    std::array<float, 2> limMin{0.0f, 0.0f};
    std::array<float, 2> limMax{420.0f, 297.0f}; // A3 size default
    
    DWGUnits units = DWGUnits::Millimeters;
    float unitScaleFactor = 1.0f;
    
    // Creation and modification info
    std::string createdBy;
    std::string lastModifiedBy;
    std::chrono::system_clock::time_point creationTime;
    std::chrono::system_clock::time_point updateTime;
    uint32_t dwgVersionSaved = 0;
    uint32_t dwgVersionMaintenance = 0;
    
    // Drawing settings
    std::string currentLayer = "0";
    std::string currentTextStyle = "STANDARD";
    std::string currentDimStyle = "STANDARD";
    float textSize = 2.5f;
    
    // Paper space settings
    bool tileMode = true;  // Model space active
    std::array<float, 2> paperSpaceExtMin{0.0f, 0.0f};
    std::array<float, 2> paperSpaceExtMax{0.0f, 0.0f};
    
    // View information
    std::array<float, 2> viewCenter{0.0f, 0.0f};
    float viewHeight = 297.0f;
    float viewWidth = 420.0f;
    float viewTwist = 0.0f;
    
    // Grid and snap settings
    bool snapOn = false;
    bool gridOn = false;
    std::array<float, 2> snapSpacing{10.0f, 10.0f};
    std::array<float, 2> gridSpacing{10.0f, 10.0f};
    
    // Custom properties
    std::unordered_map<std::string, std::string> customProperties;
};

// DWG/DXF format handler
class DWGHandler final : public IFormatHandler {
public:
    explicit DWGHandler(Rendering::RenderingEngine& engine);
    ~DWGHandler() override;
    
    // IFormatHandler interface
    FileFormat getFormat() const override { return format_; }
    std::vector<std::string> getExtensions() const override;
    std::vector<std::string> getMimeTypes() const override;
    FormatCapabilities getCapabilities() const override;
    
    FormatDetectionResult detectFormat(const std::filesystem::path& filePath) const override;
    FormatDetectionResult detectFormat(const std::vector<uint8_t>& data) const override;
    
    FileInfo getFileInfo(const std::filesystem::path& filePath) const override;
    
    std::future<std::shared_ptr<Document>> loadDocument(
        const std::filesystem::path& filePath,
        const LoadOptions& options) override;
    
    std::future<std::shared_ptr<Image>> loadImage(
        const std::filesystem::path& filePath,
        const LoadOptions& options) override;
    
    std::future<bool> saveDocument(
        const std::shared_ptr<Document>& document,
        const std::filesystem::path& filePath,
        const SaveOptions& options) override;
    
    std::future<bool> saveImage(
        const std::shared_ptr<Image>& image,
        const std::filesystem::path& filePath,
        const SaveOptions& options) override;
    
    bool validateFile(const std::filesystem::path& filePath) const override;
    bool canLoad() const override { return true; }
    bool canSave() const override { return true; }
    
    // DWG-specific methods
    DWGFileInfo getDWGFileInfo(const std::filesystem::path& filePath) const;
    std::vector<DWGLayer> extractLayers(const std::filesystem::path& filePath) const;
    std::vector<DWGBlock> extractBlocks(const std::filesystem::path& filePath) const;
    std::vector<DWGTextStyle> extractTextStyles(const std::filesystem::path& filePath) const;
    std::vector<DWGDimensionStyle> extractDimensionStyles(const std::filesystem::path& filePath) const;
    
    // Entity extraction
    std::vector<std::unique_ptr<DWGEntity>> extractEntities(
        const std::filesystem::path& filePath,
        const std::string& space = "MODEL") const;  // MODEL or PAPER
    
    // Conversion settings
    struct ConversionSettings {
        bool preserveLayerStructure = true;
        bool convertTextToPath = false;
        bool flattenBlocks = false;
        bool includeInvisibleLayers = false;
        bool includeLockedLayers = true;
        bool includePaperSpace = false;
        float defaultLineWidth = 0.25f;  // mm
        
        // Unit conversion
        DWGUnits targetUnits = DWGUnits::Millimeters;
        float scaleFactor = 1.0f;
        
        // Quality settings
        uint32_t arcSegments = 32;       // Segments for arc approximation
        float curveTolerance = 0.01f;    // mm
        bool smoothSplines = true;
        
        // Color handling
        bool useLayerColors = true;
        bool convertIndexedColors = true;
        std::array<float, 3> defaultColor{0.0f, 0.0f, 0.0f};
    };
    
    void setConversionSettings(const ConversionSettings& settings) { conversionSettings_ = settings; }
    const ConversionSettings& getConversionSettings() const { return conversionSettings_; }

private:
    Rendering::RenderingEngine& engine_;
    FileFormat format_;  // DWG or DXF
    ConversionSettings conversionSettings_;
    
    // ODA SDK integration
    bool initializeODASDK();
    void shutdownODASDK();
    bool odaInitialized_ = false;
    
    // Database operations
    std::unique_ptr<OdDbDatabase> openDatabase(const std::filesystem::path& filePath) const;
    void closeDatabase(std::unique_ptr<OdDbDatabase> database) const;
    
    // Entity conversion
    std::unique_ptr<DWGEntity> convertEntity(const OdDbEntity* entity) const;
    std::unique_ptr<DWGLine> convertLine(const OdDbEntity* entity) const;
    std::unique_ptr<DWGCircle> convertCircle(const OdDbEntity* entity) const;
    std::unique_ptr<DWGArc> convertArc(const OdDbEntity* entity) const;
    std::unique_ptr<DWGPolyline> convertPolyline(const OdDbEntity* entity) const;
    std::unique_ptr<DWGText> convertText(const OdDbEntity* entity) const;
    std::unique_ptr<DWGHatch> convertHatch(const OdDbEntity* entity) const;
    
    // Layer and style conversion
    DWGLayer convertLayer(const OdDbLayerTableRecord* layerRecord) const;
    DWGTextStyle convertTextStyle(const OdDbTextStyleTableRecord* textStyleRecord) const;
    DWGBlock convertBlock(const OdDbBlockTableRecord* blockRecord) const;
    
    // Utility methods
    std::array<float, 3> convertColor(uint16_t colorIndex) const;
    float convertLineweight(int16_t lineweight) const;
    DWGUnits detectUnits(const OdDbDatabase* database) const;
    
    // Vector path creation from DWG entities
    std::shared_ptr<VectorPath> createVectorPath(const DWGEntity& entity) const;
    void addLineToPath(std::shared_ptr<VectorPath> path, const DWGLine& line) const;
    void addCircleToPath(std::shared_ptr<VectorPath> path, const DWGCircle& circle) const;
    void addArcToPath(std::shared_ptr<VectorPath> path, const DWGArc& arc) const;
    void addPolylineToPath(std::shared_ptr<VectorPath> path, const DWGPolyline& polyline) const;
    
    // Color index to RGB conversion (AutoCAD ACI colors)
    static std::array<std::array<float, 3>, 256> aciColorTable_;
    static void initializeACIColorTable();
    
    // Error handling
    mutable std::string lastError_;
    void setLastError(const std::string& error) const { lastError_ = error; }
    
    // Progress reporting for large files
    struct ProgressReporter {
        std::function<bool(float)> callback;
        std::atomic<size_t> currentEntity{0};
        std::atomic<size_t> totalEntities{0};
        std::chrono::steady_clock::time_point startTime;
        
        bool reportProgress() {
            if (!callback) return true;
            
            float progress = totalEntities > 0 ? 
                static_cast<float>(currentEntity) / totalEntities : 0.0f;
            return callback(progress);
        }
    };
    
    mutable ProgressReporter progressReporter_;
};

// Utility functions for DWG/DXF operations
namespace DWGUtils {
    // Unit conversion
    float convertUnits(float value, DWGUnits fromUnits, DWGUnits toUnits);
    std::string getUnitsString(DWGUnits units);
    float getUnitsToMMFactor(DWGUnits units);
    
    // Color utilities
    std::array<float, 3> aciToRGB(uint16_t colorIndex);
    uint16_t rgbToACI(const std::array<float, 3>& rgb);
    bool isSpecialColor(uint16_t colorIndex);  // ByLayer, ByBlock, etc.
    
    // Geometry utilities
    float calculateArcLength(float radius, float startAngle, float endAngle);
    std::vector<std::array<float, 2>> approximateArc(
        const std::array<float, 2>& center, float radius, 
        float startAngle, float endAngle, uint32_t segments);
    
    std::vector<std::array<float, 2>> approximateSpline(
        const std::vector<std::array<float, 2>>& controlPoints,
        const std::vector<float>& knots, uint32_t degree, float tolerance);
    
    // Bulge calculation (for lightweight polylines)
    float calculateBulgeAngle(float bulge);
    std::array<float, 2> calculateBulgeCenter(
        const std::array<float, 2>& start, const std::array<float, 2>& end, float bulge);
    float calculateBulgeRadius(
        const std::array<float, 2>& start, const std::array<float, 2>& end, float bulge);
    
    // Layer utilities
    bool isLayerVisible(const DWGLayer& layer);
    bool isLayerPlottable(const DWGLayer& layer);
    std::string sanitizeLayerName(const std::string& name);
    
    // Block utilities
    bool isValidBlockName(const std::string& name);
    std::string generateUniqueBlockName(const std::string& baseName, 
                                       const std::vector<DWGBlock>& existingBlocks);
    
    // Text utilities
    std::array<float, 4> calculateTextBounds(const DWGText& text);
    std::vector<std::string> wrapText(const std::string& text, float maxWidth, 
                                     const DWGTextStyle& style);
    
    // Validation utilities
    bool isValidDWGFile(const std::filesystem::path& filePath);
    std::string getDWGVersion(const std::filesystem::path& filePath);
    bool requiresPasswordUnlock(const std::filesystem::path& filePath);
    
    // Performance utilities
    size_t estimateEntityCount(const std::filesystem::path& filePath);
    size_t estimateMemoryUsage(const std::filesystem::path& filePath);
    bool shouldUseStreamingImport(const std::filesystem::path& filePath);
}

} // namespace QuantumCanvas::IO
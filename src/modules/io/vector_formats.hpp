#pragma once

#include "file_format_manager.hpp"
#include "../../core/math/vector2.hpp"
#include "../../core/math/vector3.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <variant>

namespace QuantumCanvas::IO {

// Forward declarations
class VectorPath;
class VectorObject;
class VectorDocument;

// SVG-specific structures and constants
namespace SVG {
    // SVG length units
    enum class LengthUnit {
        px,      // Pixels (default)
        pt,      // Points (1/72 inch)
        pc,      // Picas (12 points)
        mm,      // Millimeters
        cm,      // Centimeters
        in,      // Inches
        em,      // Relative to font size
        ex,      // Relative to x-height
        percent, // Percentage
        none     // Unitless
    };
    
    // SVG length value
    struct Length {
        float value = 0.0f;
        LengthUnit unit = LengthUnit::px;
        
        // Convert to pixels given context
        float toPx(float dpi = 96.0f, float fontSize = 12.0f, float parentSize = 0.0f) const;
        
        // Parse from string
        static Length fromString(const std::string& str);
        std::string toString() const;
    };
    
    // SVG viewBox
    struct ViewBox {
        float x = 0.0f;
        float y = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
        
        bool isValid() const { return width > 0.0f && height > 0.0f; }
        std::array<float, 4> toArray() const { return {x, y, width, height}; }
        
        static ViewBox fromString(const std::string& str);
        std::string toString() const;
    };
    
    // SVG color formats
    struct Color {
        std::array<float, 4> rgba{0.0f, 0.0f, 0.0f, 1.0f};
        
        // Named color constructor
        static Color fromName(const std::string& name);
        
        // Hex color constructor (#RGB, #RRGGBB, #RRGGBBAA)
        static Color fromHex(const std::string& hex);
        
        // RGB/RGBA function constructor
        static Color fromRGB(const std::string& rgb);
        
        // HSL function constructor
        static Color fromHSL(const std::string& hsl);
        
        // Parse any CSS color format
        static Color fromString(const std::string& str);
        
        std::string toHex() const;
        std::string toRGB() const;
        std::string toRGBA() const;
    };
    
    // SVG paint (fill/stroke)
    struct Paint {
        enum Type {
            None,
            Color,
            URL,        // Reference to gradient, pattern, etc.
            CurrentColor,
            Inherit
        };
        
        Type type = None;
        Color color;
        std::string url;        // For gradients, patterns
        float opacity = 1.0f;
        
        static Paint fromString(const std::string& str);
        std::string toString() const;
    };
    
    // SVG transform matrix
    struct Transform {
        std::array<float, 6> matrix{1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f}; // a, b, c, d, e, f
        
        Transform() = default;
        Transform(const std::array<float, 6>& m) : matrix(m) {}
        
        // Transform operations
        Transform& translate(float tx, float ty);
        Transform& scale(float sx, float sy);
        Transform& rotate(float angle, float cx = 0.0f, float cy = 0.0f);
        Transform& skewX(float angle);
        Transform& skewY(float angle);
        Transform& multiply(const Transform& other);
        
        // Apply to point
        std::array<float, 2> apply(const std::array<float, 2>& point) const;
        
        // Matrix operations
        Transform inverse() const;
        float determinant() const;
        bool isIdentity() const;
        
        // Parse from SVG transform string
        static Transform fromString(const std::string& str);
        std::string toString() const;
    };
    
    // SVG style properties
    struct Style {
        Paint fill;
        Paint stroke;
        float strokeWidth = 1.0f;
        
        enum LineCap { ButtCap, RoundCap, SquareCap };
        enum LineJoin { MiterJoin, RoundJoin, BevelJoin };
        enum FillRule { NonZero, EvenOdd };
        
        LineCap strokeLineCap = ButtCap;
        LineJoin strokeLineJoin = MiterJoin;
        float strokeMiterLimit = 4.0f;
        std::vector<float> strokeDashArray;
        float strokeDashOffset = 0.0f;
        FillRule fillRule = NonZero;
        float opacity = 1.0f;
        float fillOpacity = 1.0f;
        float strokeOpacity = 1.0f;
        
        // Font properties (for text elements)
        std::string fontFamily = "serif";
        float fontSize = 12.0f;
        enum FontStyle { Normal, Italic, Oblique };
        enum FontWeight { W100 = 100, W200 = 200, W300 = 300, W400 = 400, W500 = 500, 
                         W600 = 600, W700 = 700, W800 = 800, W900 = 900 };
        FontStyle fontStyle = Normal;
        FontWeight fontWeight = W400;
        
        // Text properties
        enum TextAnchor { Start, Middle, End };
        TextAnchor textAnchor = Start;
        
        // Parse from CSS style string or individual attributes
        void parseStyle(const std::string& styleStr);
        void setAttribute(const std::string& name, const std::string& value);
        
        std::string toCSSString() const;
    };
    
    // SVG gradient stop
    struct GradientStop {
        float offset = 0.0f;        // 0.0 to 1.0
        Color color;
        float opacity = 1.0f;
        
        static GradientStop fromString(const std::string& str);
    };
    
    // SVG linear gradient
    struct LinearGradient {
        std::string id;
        std::array<float, 2> start{0.0f, 0.0f};    // x1, y1
        std::array<float, 2> end{1.0f, 0.0f};      // x2, y2
        std::vector<GradientStop> stops;
        Transform gradientTransform;
        
        enum Units { ObjectBoundingBox, UserSpaceOnUse };
        Units gradientUnits = ObjectBoundingBox;
        
        enum SpreadMethod { Pad, Reflect, Repeat };
        SpreadMethod spreadMethod = Pad;
    };
    
    // SVG radial gradient
    struct RadialGradient {
        std::string id;
        std::array<float, 2> center{0.5f, 0.5f};   // cx, cy
        float radius = 0.5f;                        // r
        std::array<float, 2> focal{0.5f, 0.5f};    // fx, fy (optional)
        std::vector<GradientStop> stops;
        Transform gradientTransform;
        
        enum Units { ObjectBoundingBox, UserSpaceOnUse };
        Units gradientUnits = ObjectBoundingBox;
        
        enum SpreadMethod { Pad, Reflect, Repeat };
        SpreadMethod spreadMethod = Pad;
    };
    
    // SVG pattern
    struct Pattern {
        std::string id;
        std::array<float, 4> patternBounds{0.0f, 0.0f, 0.0f, 0.0f}; // x, y, width, height
        ViewBox viewBox;
        Transform patternTransform;
        
        enum Units { ObjectBoundingBox, UserSpaceOnUse };
        Units patternUnits = ObjectBoundingBox;
        Units patternContentUnits = UserSpaceOnUse;
        
        // Pattern content (child elements)
        std::vector<std::shared_ptr<SVGElement>> elements;
    };
    
    // Base SVG element
    struct SVGElement {
        std::string id;
        std::string className;
        Style style;
        Transform transform;
        
        virtual ~SVGElement() = default;
        virtual std::string getElementType() const = 0;
        virtual std::shared_ptr<VectorPath> toVectorPath() const = 0;
    };
    
    // SVG path element
    struct PathElement : public SVGElement {
        std::string pathData;  // SVG path d="" attribute
        
        std::string getElementType() const override { return "path"; }
        std::shared_ptr<VectorPath> toVectorPath() const override;
        
        // Parse path data string into commands
        struct PathCommand {
            char command;                          // M, L, C, Q, A, Z, etc.
            std::vector<float> parameters;
            bool absolute = true;                  // Uppercase = absolute
        };
        
        std::vector<PathCommand> parsePathData() const;
        std::string pathDataToString(const std::vector<PathCommand>& commands) const;
    };
    
    // SVG rectangle element
    struct RectElement : public SVGElement {
        float x = 0.0f, y = 0.0f;
        float width = 0.0f, height = 0.0f;
        float rx = 0.0f, ry = 0.0f;  // Rounded corners
        
        std::string getElementType() const override { return "rect"; }
        std::shared_ptr<VectorPath> toVectorPath() const override;
    };
    
    // SVG circle element
    struct CircleElement : public SVGElement {
        float cx = 0.0f, cy = 0.0f, r = 0.0f;
        
        std::string getElementType() const override { return "circle"; }
        std::shared_ptr<VectorPath> toVectorPath() const override;
    };
    
    // SVG ellipse element
    struct EllipseElement : public SVGElement {
        float cx = 0.0f, cy = 0.0f, rx = 0.0f, ry = 0.0f;
        
        std::string getElementType() const override { return "ellipse"; }
        std::shared_ptr<VectorPath> toVectorPath() const override;
    };
    
    // SVG line element
    struct LineElement : public SVGElement {
        float x1 = 0.0f, y1 = 0.0f, x2 = 0.0f, y2 = 0.0f;
        
        std::string getElementType() const override { return "line"; }
        std::shared_ptr<VectorPath> toVectorPath() const override;
    };
    
    // SVG polyline/polygon element
    struct PolyElement : public SVGElement {
        std::vector<std::array<float, 2>> points;
        bool closed = false;  // polygon vs polyline
        
        std::string getElementType() const override { return closed ? "polygon" : "polyline"; }
        std::shared_ptr<VectorPath> toVectorPath() const override;
        
        // Parse points attribute
        static std::vector<std::array<float, 2>> parsePoints(const std::string& pointsStr);
        std::string pointsToString() const;
    };
    
    // SVG text element
    struct TextElement : public SVGElement {
        float x = 0.0f, y = 0.0f;
        std::string text;
        
        // Text spans for complex formatting
        struct TextSpan {
            std::string text;
            float dx = 0.0f, dy = 0.0f;  // Relative positioning
            Style spanStyle;
        };
        
        std::vector<TextSpan> spans;
        
        std::string getElementType() const override { return "text"; }
        std::shared_ptr<VectorPath> toVectorPath() const override;  // Convert to paths
    };
    
    // SVG group element
    struct GroupElement : public SVGElement {
        std::vector<std::shared_ptr<SVGElement>> children;
        
        std::string getElementType() const override { return "g"; }
        std::shared_ptr<VectorPath> toVectorPath() const override;
    };
    
    // SVG use element (symbol instance)
    struct UseElement : public SVGElement {
        std::string href;  // Reference to symbol/element
        float x = 0.0f, y = 0.0f;
        float width = 0.0f, height = 0.0f;
        
        std::string getElementType() const override { return "use"; }
        std::shared_ptr<VectorPath> toVectorPath() const override;
    };
    
    // SVG symbol element (reusable graphic)
    struct SymbolElement : public SVGElement {
        ViewBox viewBox;
        std::vector<std::shared_ptr<SVGElement>> children;
        
        std::string getElementType() const override { return "symbol"; }
        std::shared_ptr<VectorPath> toVectorPath() const override;
    };
    
    // SVG document root
    struct SVGDocument {
        Length width{100.0f, LengthUnit::percent};
        Length height{100.0f, LengthUnit::percent};
        ViewBox viewBox;
        
        // Definitions (gradients, patterns, symbols, etc.)
        std::vector<std::shared_ptr<LinearGradient>> linearGradients;
        std::vector<std::shared_ptr<RadialGradient>> radialGradients;
        std::vector<std::shared_ptr<Pattern>> patterns;
        std::vector<std::shared_ptr<SymbolElement>> symbols;
        
        // Root elements
        std::vector<std::shared_ptr<SVGElement>> elements;
        
        // Document properties
        std::string title;
        std::string description;
        std::unordered_map<std::string, std::string> metadata;
        
        // Calculate actual pixel dimensions
        std::array<uint32_t, 2> getPixelDimensions(float dpi = 96.0f) const;
        
        // Find element by ID
        std::shared_ptr<SVGElement> getElementById(const std::string& id) const;
        
        // Find definition by ID
        std::shared_ptr<LinearGradient> getLinearGradient(const std::string& id) const;
        std::shared_ptr<RadialGradient> getRadialGradient(const std::string& id) const;
        std::shared_ptr<Pattern> getPattern(const std::string& id) const;
        std::shared_ptr<SymbolElement> getSymbol(const std::string& id) const;
    };
} // namespace SVG

// Adobe Illustrator (.ai) specific structures
namespace AI {
    // AI file version
    enum class AIVersion {
        Unknown,
        AI8,    // Illustrator 8
        AI9,    // Illustrator 9
        AI10,   // Illustrator 10/CS
        AI11,   // Illustrator CS2
        AI12,   // Illustrator CS3
        AI13,   // Illustrator CS4
        AI14,   // Illustrator CS5
        AI15,   // Illustrator CS6
        AI16,   // Illustrator CC 2017
        AI17,   // Illustrator CC 2018
        AI18,   // Illustrator CC 2019
        AI19,   // Illustrator CC 2020
        AI20,   // Illustrator CC 2021
        AI21    // Illustrator CC 2022+
    };
    
    // AI color model
    enum class ColorModel {
        CMYK,
        RGB,
        Grayscale,
        Lab,
        Spot
    };
    
    // AI artboard
    struct Artboard {
        std::string name;
        std::array<float, 4> bounds;  // left, top, right, bottom
        bool isActive = false;
        
        // Print settings
        float bleedTop = 0.0f;
        float bleedLeft = 0.0f;
        float bleedRight = 0.0f;
        float bleedBottom = 0.0f;
        
        // Ruler settings
        std::array<float, 2> rulerOrigin{0.0f, 0.0f};
    };
    
    // AI document
    struct AIDocument {
        AIVersion version = AIVersion::Unknown;
        ColorModel colorModel = ColorModel::RGB;
        
        // Document bounds (in points)
        std::array<float, 4> documentBounds{0.0f, 0.0f, 612.0f, 792.0f}; // Letter size
        
        // Artboards
        std::vector<Artboard> artboards;
        
        // Color profiles
        std::string colorProfile;
        bool embedProfile = true;
        
        // Document properties
        std::string title;
        std::string creator = "Adobe Illustrator";
        std::chrono::system_clock::time_point creationDate;
        std::chrono::system_clock::time_point modificationDate;
        
        // Layers and objects (simplified - would contain actual vector data)
        std::vector<std::shared_ptr<VectorObject>> objects;
    };
}

// PDF vector content structures
namespace PDF {
    // PDF page
    struct PDFPage {
        uint32_t pageNumber = 1;
        std::array<float, 4> mediaBox{0.0f, 0.0f, 612.0f, 792.0f}; // Letter size
        std::array<float, 4> cropBox;  // If different from media box
        std::array<float, 4> bleedBox; // For printing
        std::array<float, 4> artBox;   // Content area
        
        float rotation = 0.0f; // 0, 90, 180, 270 degrees
        
        // Vector content
        std::vector<std::shared_ptr<VectorObject>> objects;
        
        // Text content (for extraction)
        struct TextRun {
            std::string text;
            std::string fontName;
            float fontSize = 12.0f;
            std::array<float, 2> position{0.0f, 0.0f};
            std::array<float, 3> color{0.0f, 0.0f, 0.0f};
            std::array<float, 6> textMatrix{1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f};
        };
        
        std::vector<TextRun> textRuns;
    };
    
    // PDF document info
    struct PDFDocument {
        std::string title;
        std::string author;
        std::string subject;
        std::string keywords;
        std::string creator;
        std::string producer;
        std::chrono::system_clock::time_point creationDate;
        std::chrono::system_clock::time_point modificationDate;
        
        // Document security
        bool encrypted = false;
        bool allowPrint = true;
        bool allowCopy = true;
        bool allowModify = true;
        bool allowAnnotations = true;
        
        // Pages
        std::vector<PDFPage> pages;
        
        // Color profiles and resources
        std::unordered_map<std::string, std::vector<uint8_t>> embeddedProfiles;
        std::unordered_map<std::string, std::vector<uint8_t>> embeddedFonts;
        
        // PDF version
        float version = 1.4f;
    };
}

// SVG format handler
class SVGHandler final : public IFormatHandler {
public:
    explicit SVGHandler(Rendering::RenderingEngine& engine);
    ~SVGHandler() override;
    
    // IFormatHandler interface
    FileFormat getFormat() const override { return FileFormat::SVG; }
    std::vector<std::string> getExtensions() const override { return {"svg", "svgz"}; }
    std::vector<std::string> getMimeTypes() const override { 
        return {"image/svg+xml", "application/svg+xml"}; 
    }
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
    
    // SVG-specific methods
    SVG::SVGDocument parseSVG(const std::filesystem::path& filePath) const;
    SVG::SVGDocument parseSVG(const std::string& svgContent) const;
    
    std::string generateSVG(const std::shared_ptr<Document>& document, 
                           const SaveOptions& options = {}) const;
    
    // Rasterization
    std::shared_ptr<Image> rasterizeSVG(const SVG::SVGDocument& svg,
                                       const std::array<uint32_t, 2>& size,
                                       float dpi = 96.0f) const;

private:
    Rendering::RenderingEngine& engine_;
    
    // XML parsing
    class XMLParser;
    std::unique_ptr<XMLParser> xmlParser_;
    
    // SVG element creation from XML nodes
    std::shared_ptr<SVG::SVGElement> createElementFromNode(const XMLParser::Node& node) const;
    std::shared_ptr<SVG::PathElement> createPathElement(const XMLParser::Node& node) const;
    std::shared_ptr<SVG::RectElement> createRectElement(const XMLParser::Node& node) const;
    std::shared_ptr<SVG::CircleElement> createCircleElement(const XMLParser::Node& node) const;
    std::shared_ptr<SVG::EllipseElement> createEllipseElement(const XMLParser::Node& node) const;
    std::shared_ptr<SVG::LineElement> createLineElement(const XMLParser::Node& node) const;
    std::shared_ptr<SVG::PolyElement> createPolyElement(const XMLParser::Node& node, bool closed) const;
    std::shared_ptr<SVG::TextElement> createTextElement(const XMLParser::Node& node) const;
    std::shared_ptr<SVG::GroupElement> createGroupElement(const XMLParser::Node& node) const;
    std::shared_ptr<SVG::UseElement> createUseElement(const XMLParser::Node& node) const;
    std::shared_ptr<SVG::SymbolElement> createSymbolElement(const XMLParser::Node& node) const;
    
    // Gradient and pattern parsing
    std::shared_ptr<SVG::LinearGradient> parseLinearGradient(const XMLParser::Node& node) const;
    std::shared_ptr<SVG::RadialGradient> parseRadialGradient(const XMLParser::Node& node) const;
    std::shared_ptr<SVG::Pattern> parsePattern(const XMLParser::Node& node) const;
    
    // Style parsing
    SVG::Style parseStyle(const XMLParser::Node& node) const;
    void parseStyleAttribute(SVG::Style& style, const std::string& styleStr) const;
    void parseInlineStyles(SVG::Style& style, const XMLParser::Node& node) const;
    
    // Path data parsing
    std::vector<SVG::PathElement::PathCommand> parsePathData(const std::string& pathData) const;
    
    // Coordinate parsing
    float parseCoordinate(const std::string& str, float referenceSize = 0.0f, 
                         float fontSize = 12.0f, float dpi = 96.0f) const;
    std::vector<float> parseNumberList(const std::string& str) const;
    
    // SVG to vector conversion
    std::shared_ptr<VectorDocument> convertSVGToVector(const SVG::SVGDocument& svg) const;
    
    // Vector to SVG conversion
    SVG::SVGDocument convertVectorToSVG(const std::shared_ptr<VectorDocument>& doc) const;
    
    // XML generation
    std::string generateXML(const SVG::SVGDocument& svg) const;
    std::string serializeElement(const std::shared_ptr<SVG::SVGElement>& element) const;
    std::string serializeStyle(const SVG::Style& style) const;
    std::string serializeTransform(const SVG::Transform& transform) const;
    
    // GZIP compression for SVGZ
    std::vector<uint8_t> compressGZIP(const std::string& data) const;
    std::string decompressGZIP(const std::vector<uint8_t>& compressedData) const;
    
    // Error handling
    mutable std::string lastError_;
    void setLastError(const std::string& error) const { lastError_ = error; }
};

// Adobe Illustrator format handler
class AIHandler final : public IFormatHandler {
public:
    explicit AIHandler(Rendering::RenderingEngine& engine);
    ~AIHandler() override;
    
    // IFormatHandler interface
    FileFormat getFormat() const override { return FileFormat::AI; }
    std::vector<std::string> getExtensions() const override { return {"ai"}; }
    std::vector<std::string> getMimeTypes() const override { 
        return {"application/postscript", "application/illustrator"}; 
    }
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
    bool canSave() const override { return false; }  // Read-only for now
    
    // AI-specific methods
    AI::AIDocument parseAI(const std::filesystem::path& filePath) const;
    AI::AIVersion detectAIVersion(const std::filesystem::path& filePath) const;

private:
    Rendering::RenderingEngine& engine_;
    
    // PostScript parser for AI files
    class PostScriptParser;
    std::unique_ptr<PostScriptParser> psParser_;
    
    // AI version detection
    AI::AIVersion parseVersionComment(const std::string& comment) const;
    
    // PostScript to vector conversion
    std::shared_ptr<VectorDocument> convertPSToVector(const AI::AIDocument& ai) const;
    
    // Error handling
    mutable std::string lastError_;
};

// PDF vector format handler
class PDFHandler final : public IFormatHandler {
public:
    explicit PDFHandler(Rendering::RenderingEngine& engine);
    ~PDFHandler() override;
    
    // IFormatHandler interface  
    FileFormat getFormat() const override { return FileFormat::PDF; }
    std::vector<std::string> getExtensions() const override { return {"pdf"}; }
    std::vector<std::string> getMimeTypes() const override { 
        return {"application/pdf"}; 
    }
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
    
    // PDF-specific methods
    PDF::PDFDocument parsePDF(const std::filesystem::path& filePath) const;
    uint32_t getPageCount(const std::filesystem::path& filePath) const;
    
    // Page-specific loading
    std::shared_ptr<Document> loadPage(const std::filesystem::path& filePath, 
                                      uint32_t pageIndex,
                                      const LoadOptions& options = {});
    
    // Text extraction
    std::string extractText(const std::filesystem::path& filePath,
                           uint32_t pageIndex = UINT32_MAX) const; // UINT32_MAX = all pages

private:
    Rendering::RenderingEngine& engine_;
    
    // PDF parsing (would integrate with poppler or similar)
    class PDFParser;
    std::unique_ptr<PDFParser> pdfParser_;
    
    // PDF to vector conversion
    std::shared_ptr<VectorDocument> convertPDFToVector(const PDF::PDFDocument& pdf) const;
    
    // Vector to PDF conversion
    PDF::PDFDocument convertVectorToPDF(const std::shared_ptr<VectorDocument>& doc) const;
    
    // PDF generation
    std::vector<uint8_t> generatePDF(const PDF::PDFDocument& pdf) const;
    
    // Error handling
    mutable std::string lastError_;
};

// Utility functions for vector format operations
namespace VectorFormatUtils {
    // SVG utilities
    std::string escapeXML(const std::string& text);
    std::string unescapeXML(const std::string& text);
    
    // Path data utilities
    std::string optimizePathData(const std::string& pathData);
    std::vector<std::array<float, 2>> pathToPoints(const std::string& pathData, float tolerance = 1.0f);
    std::string pointsToPath(const std::vector<std::array<float, 2>>& points, bool closed = false);
    
    // Bounding box calculation
    std::array<float, 4> calculateSVGBounds(const SVG::SVGDocument& svg);
    std::array<float, 4> calculateElementBounds(const std::shared_ptr<SVG::SVGElement>& element);
    
    // Coordinate system conversion
    std::array<float, 2> convertUnits(const std::array<float, 2>& point, 
                                     SVG::LengthUnit fromUnit, SVG::LengthUnit toUnit,
                                     float dpi = 96.0f, float fontSize = 12.0f);
    
    // Color conversion
    std::array<float, 3> hexToRGB(const std::string& hex);
    std::string rgbToHex(const std::array<float, 3>& rgb);
    std::array<float, 3> hslToRGB(float h, float s, float l);
    std::array<float, 3> rgbToHSL(const std::array<float, 3>& rgb);
    
    // Transform utilities
    std::array<float, 6> multiplyTransforms(const std::array<float, 6>& a, const std::array<float, 6>& b);
    std::array<float, 6> invertTransform(const std::array<float, 6>& transform);
    
    // Validation utilities
    bool isValidSVG(const std::string& svgContent);
    bool isValidPathData(const std::string& pathData);
    bool isValidColor(const std::string& color);
    
    // Optimization utilities
    std::string optimizeSVG(const std::string& svgContent, bool removeUnusedDefs = true,
                           bool optimizePaths = true, bool mergeStyles = true);
    
    float calculatePathLength(const std::string& pathData);
    std::array<float, 2> getPointAtLength(const std::string& pathData, float length);
    
    // Format conversion helpers
    SVG::SVGDocument convertToSVG(const std::shared_ptr<VectorDocument>& doc);
    std::shared_ptr<VectorDocument> convertFromSVG(const SVG::SVGDocument& svg);
    
    // Performance utilities
    bool shouldSimplifyPath(const std::string& pathData, size_t maxComplexity = 1000);
    std::string simplifyPath(const std::string& pathData, float tolerance = 1.0f);
    
    // Font utilities for text elements
    std::vector<std::string> getSystemFonts();
    bool isFontAvailable(const std::string& fontFamily);
    std::string findSimilarFont(const std::string& requestedFont);
}

} // namespace QuantumCanvas::IO
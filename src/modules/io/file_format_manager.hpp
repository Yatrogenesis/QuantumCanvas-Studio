#pragma once

#include "../../core/rendering/rendering_engine.hpp"
#include "../../core/memory/memory_manager.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <filesystem>
#include <functional>
#include <atomic>
#include <mutex>
#include <future>
#include <chrono>
#include <variant>

namespace QuantumCanvas::IO {

// Forward declarations
class Document;
class Image;
class VectorPath;
class Layer;

// Supported file formats
enum class FileFormat {
    // Raster formats
    PNG,
    JPEG,
    TIFF,
    BMP,
    GIF,
    WEBP,
    PSD,        // Adobe Photoshop
    PSB,        // Large Photoshop documents
    XCF,        // GIMP
    
    // Vector formats
    SVG,
    AI,         // Adobe Illustrator
    EPS,        // Encapsulated PostScript
    PDF,        // Portable Document Format
    CDR,        // CorelDRAW
    WMF,        // Windows Metafile
    EMF,        // Enhanced Metafile
    
    // CAD formats
    DWG,        // AutoCAD Drawing
    DXF,        // Drawing Exchange Format
    STEP,       // Standard for Exchange of Product Data
    IGES,       // Initial Graphics Exchange Specification
    STL,        // Stereolithography
    OBJ,        // Wavefront OBJ
    PLY,        // Polygon File Format
    
    // Raw formats
    CR2,        // Canon Raw
    NEF,        // Nikon Raw
    ARW,        // Sony Raw
    DNG,        // Adobe Digital Negative
    RAF,        // Fujifilm Raw
    ORF,        // Olympus Raw
    
    // QuantumCanvas native
    QCSX,       // QuantumCanvas Studio eXtended
    QCS,        // QuantumCanvas Studio
    
    // Other formats
    ICO,        // Windows Icon
    CUR,        // Windows Cursor
    ANI,        // Animated Cursor
    
    Unknown
};

// File format capabilities
struct FormatCapabilities {
    bool supportsLayers = false;
    bool supportsVectorData = false;
    bool supportsRasterData = false;
    bool supports3DData = false;
    bool supportsAnimation = false;
    bool supportsMetadata = false;
    bool supportsCompression = false;
    bool supportsMultiPage = false;
    bool supportsTransparency = false;
    bool supportsColorProfiles = false;
    bool supportsTextLayers = false;
    bool supportsEffects = false;
    
    // Supported bit depths
    std::vector<uint8_t> supportedBitDepths{8};
    
    // Supported color modes
    enum ColorMode {
        RGB,
        RGBA,
        CMYK,
        Grayscale,
        Indexed,
        LAB
    };
    std::vector<ColorMode> supportedColorModes{RGB, RGBA};
    
    // Maximum dimensions
    uint32_t maxWidth = 65536;
    uint32_t maxHeight = 65536;
    uint64_t maxFileSize = UINT64_MAX;
    
    // Quality settings (for lossy formats)
    struct QualityRange {
        float min = 0.0f;
        float max = 1.0f;
        float defaultValue = 0.9f;
    } qualityRange;
    
    // Compression settings
    struct CompressionSettings {
        enum Type {
            None,
            LZW,
            ZIP,
            JPEG,
            RLE,
            PackBits
        };
        std::vector<Type> supportedTypes{None};
        Type defaultType = None;
    } compression;
};

// Load/Save options for file operations
struct LoadOptions {
    // Color management
    bool applyColorProfile = true;
    bool convertToWorkingColorSpace = false;
    std::string targetColorProfile;
    
    // Import settings
    float dpi = 300.0f;
    std::array<uint32_t, 2> targetSize{0, 0};  // 0,0 = keep original
    bool preserveAspectRatio = true;
    
    // Layer handling
    bool flattenLayers = false;
    bool loadHiddenLayers = true;
    bool loadLayerEffects = true;
    
    // Vector import settings
    float vectorScale = 1.0f;
    bool convertTextToPaths = false;
    bool preserveEditability = true;
    
    // RAW processing settings (for camera RAW formats)
    struct RAWSettings {
        float exposure = 0.0f;
        float highlights = 0.0f;
        float shadows = 0.0f;
        float whites = 0.0f;
        float blacks = 0.0f;
        float temperature = 5500.0f;
        float tint = 0.0f;
        float vibrance = 0.0f;
        float saturation = 0.0f;
        float clarity = 0.0f;
        float dehaze = 0.0f;
        bool autoToneAdjustments = false;
    } rawSettings;
    
    // Performance settings
    bool useGPUAcceleration = true;
    uint32_t maxMemoryUsage = 1024 * 1024 * 1024;  // 1GB
    
    // Progress callback
    std::function<bool(float)> progressCallback;  // Return false to cancel
};

struct SaveOptions {
    // Quality settings (for lossy formats)
    float quality = 0.9f;
    
    // Compression settings
    FormatCapabilities::CompressionSettings::Type compression = 
        FormatCapabilities::CompressionSettings::Type::None;
    
    // Color management
    bool embedColorProfile = true;
    std::string outputColorProfile;
    bool convertColorProfile = false;
    
    // Layer handling
    bool preserveLayers = true;
    bool saveLayerEffects = true;
    bool saveHiddenLayers = false;
    
    // Metadata
    bool preserveMetadata = true;
    bool stripPersonalMetadata = false;
    
    // Format-specific options
    struct PNGOptions {
        uint8_t compressionLevel = 6;  // 0-9
        bool interlaced = false;
    } pngOptions;
    
    struct JPEGOptions {
        bool progressive = false;
        bool optimizeHuffman = true;
        uint8_t smoothing = 0;
    } jpegOptions;
    
    struct TIFFOptions {
        bool bigTiff = false;  // Use BigTIFF for large files
        bool tiled = false;
        uint32_t tileSize = 256;
        uint8_t predictor = 1;  // LZW/ZIP predictor
    } tiffOptions;
    
    struct PDFOptions {
        float version = 1.7f;
        bool vectorizeText = true;
        bool embedFonts = true;
        bool allowCopy = true;
        bool allowPrint = true;
        std::string password;
    } pdfOptions;
    
    struct QCSXOptions {
        uint8_t compressionLevel = 6;
        bool compressImages = true;
        bool saveHistory = false;
        bool savePreview = true;
        std::array<uint32_t, 2> previewSize{256, 256};
    } qcsxOptions;
    
    // Progress callback
    std::function<bool(float)> progressCallback;  // Return false to cancel
};

// File format detection result
struct FormatDetectionResult {
    FileFormat format = FileFormat::Unknown;
    float confidence = 0.0f;  // 0.0 to 1.0
    std::string mimeType;
    std::vector<std::string> possibleExtensions;
    std::string detectionMethod;  // "extension", "magic", "content", etc.
    
    bool isSupported() const { return format != FileFormat::Unknown; }
    bool isConfident() const { return confidence >= 0.8f; }
};

// File information structure
struct FileInfo {
    std::filesystem::path filePath;
    FileFormat format = FileFormat::Unknown;
    uint64_t fileSize = 0;
    std::chrono::system_clock::time_point lastModified;
    
    // Image properties
    std::array<uint32_t, 2> dimensions{0, 0};
    uint8_t bitDepth = 8;
    uint8_t channels = 3;
    float dpi = 72.0f;
    std::string colorProfile;
    
    // Document properties
    uint32_t pageCount = 1;
    uint32_t layerCount = 1;
    bool hasTransparency = false;
    bool isAnimated = false;
    
    // Metadata
    std::unordered_map<std::string, std::string> metadata;
    
    // Validation
    bool isValid = false;
    std::string errorMessage;
};

// Base interface for file format handlers
class IFormatHandler {
public:
    virtual ~IFormatHandler() = default;
    
    // Format identification
    virtual FileFormat getFormat() const = 0;
    virtual std::vector<std::string> getExtensions() const = 0;
    virtual std::vector<std::string> getMimeTypes() const = 0;
    virtual FormatCapabilities getCapabilities() const = 0;
    
    // Format detection
    virtual FormatDetectionResult detectFormat(const std::filesystem::path& filePath) const = 0;
    virtual FormatDetectionResult detectFormat(const std::vector<uint8_t>& data) const = 0;
    
    // File information
    virtual FileInfo getFileInfo(const std::filesystem::path& filePath) const = 0;
    
    // Load operations
    virtual std::future<std::shared_ptr<Document>> loadDocument(
        const std::filesystem::path& filePath, 
        const LoadOptions& options) = 0;
    
    virtual std::future<std::shared_ptr<Image>> loadImage(
        const std::filesystem::path& filePath,
        const LoadOptions& options) = 0;
    
    // Save operations
    virtual std::future<bool> saveDocument(
        const std::shared_ptr<Document>& document,
        const std::filesystem::path& filePath,
        const SaveOptions& options) = 0;
    
    virtual std::future<bool> saveImage(
        const std::shared_ptr<Image>& image,
        const std::filesystem::path& filePath,
        const SaveOptions& options) = 0;
    
    // Validation
    virtual bool validateFile(const std::filesystem::path& filePath) const = 0;
    virtual bool canLoad() const = 0;
    virtual bool canSave() const = 0;
};

// Main file format manager
class FileFormatManager final {
public:
    explicit FileFormatManager(Rendering::RenderingEngine& engine);
    ~FileFormatManager();
    
    // Disable copy, enable move
    FileFormatManager(const FileFormatManager&) = delete;
    FileFormatManager& operator=(const FileFormatManager&) = delete;
    FileFormatManager(FileFormatManager&& other) noexcept;
    FileFormatManager& operator=(FileFormatManager&& other) noexcept;
    
    // Initialization
    bool initialize();
    void shutdown();
    bool isInitialized() const { return initialized_; }
    
    // Format handler management
    void registerHandler(std::unique_ptr<IFormatHandler> handler);
    void unregisterHandler(FileFormat format);
    IFormatHandler* getHandler(FileFormat format);
    const IFormatHandler* getHandler(FileFormat format) const;
    
    // Format detection
    FormatDetectionResult detectFormat(const std::filesystem::path& filePath) const;
    FormatDetectionResult detectFormat(const std::vector<uint8_t>& data) const;
    FileFormat detectFormatByExtension(const std::string& extension) const;
    std::vector<FileFormat> getSupportedFormats() const;
    
    // Format information
    FormatCapabilities getFormatCapabilities(FileFormat format) const;
    std::vector<std::string> getFormatExtensions(FileFormat format) const;
    std::string getFormatName(FileFormat format) const;
    FileInfo getFileInfo(const std::filesystem::path& filePath) const;
    
    // Load operations
    std::future<std::shared_ptr<Document>> loadDocument(
        const std::filesystem::path& filePath,
        const LoadOptions& options = {});
    
    std::future<std::shared_ptr<Image>> loadImage(
        const std::filesystem::path& filePath,
        const LoadOptions& options = {});
    
    // Save operations
    std::future<bool> saveDocument(
        const std::shared_ptr<Document>& document,
        const std::filesystem::path& filePath,
        const SaveOptions& options = {});
    
    std::future<bool> saveImage(
        const std::shared_ptr<Image>& image,
        const std::filesystem::path& filePath,
        const SaveOptions& options = {});
    
    // Batch operations
    struct BatchLoadJob {
        std::filesystem::path inputPath;
        LoadOptions options;
        std::promise<std::shared_ptr<Document>> promise;
    };
    
    struct BatchSaveJob {
        std::shared_ptr<Document> document;
        std::filesystem::path outputPath;
        SaveOptions options;
        std::promise<bool> promise;
    };
    
    void processBatchLoad(const std::vector<BatchLoadJob>& jobs);
    void processBatchSave(const std::vector<BatchSaveJob>& jobs);
    
    // Format conversion
    std::future<bool> convertFile(
        const std::filesystem::path& inputPath,
        const std::filesystem::path& outputPath,
        const LoadOptions& loadOptions = {},
        const SaveOptions& saveOptions = {});
    
    // Thumbnail generation
    std::future<std::shared_ptr<Image>> generateThumbnail(
        const std::filesystem::path& filePath,
        const std::array<uint32_t, 2>& size = {256, 256});
    
    // File validation
    bool validateFile(const std::filesystem::path& filePath) const;
    std::vector<std::string> validateFiles(const std::vector<std::filesystem::path>& filePaths) const;
    
    // Import/Export presets
    struct ImportPreset {
        std::string name;
        std::string description;
        FileFormat targetFormat;
        LoadOptions options;
    };
    
    struct ExportPreset {
        std::string name;
        std::string description;
        FileFormat targetFormat;
        SaveOptions options;
    };
    
    void addImportPreset(const ImportPreset& preset);
    void addExportPreset(const ExportPreset& preset);
    void removeImportPreset(const std::string& name);
    void removeExportPreset(const std::string& name);
    
    std::vector<ImportPreset> getImportPresets() const;
    std::vector<ExportPreset> getExportPresets() const;
    ImportPreset getImportPreset(const std::string& name) const;
    ExportPreset getExportPreset(const std::string& name) const;
    
    // Plugin support for custom formats
    void loadFormatPlugin(const std::filesystem::path& pluginPath);
    void unloadFormatPlugin(const std::string& pluginName);
    std::vector<std::string> getLoadedPlugins() const;
    
    // Performance settings
    void setMaxConcurrency(uint32_t maxThreads) { maxConcurrency_ = maxThreads; }
    uint32_t getMaxConcurrency() const { return maxConcurrency_; }
    
    void setMemoryBudget(size_t budgetBytes) { memoryBudget_ = budgetBytes; }
    size_t getMemoryBudget() const { return memoryBudget_; }
    
    void setCacheEnabled(bool enabled) { cacheEnabled_ = enabled; }
    bool isCacheEnabled() const { return cacheEnabled_; }
    
    // Statistics
    struct FileFormatStats {
        uint32_t documentsLoaded = 0;
        uint32_t documentsLoaded = 0;
        uint32_t imagesLoaded = 0;
        uint32_t imagesSaved = 0;
        uint32_t conversionsPerformed = 0;
        uint32_t thumbnailsGenerated = 0;
        uint64_t bytesRead = 0;
        uint64_t bytesWritten = 0;
        std::chrono::microseconds totalLoadTime{0};
        std::chrono::microseconds totalSaveTime{0};
        size_t cacheHits = 0;
        size_t cacheMisses = 0;
        
        // Format-specific statistics
        std::unordered_map<FileFormat, uint32_t> formatUsageCount;
        std::unordered_map<FileFormat, std::chrono::microseconds> formatProcessingTime;
    };
    
    FileFormatStats getStats() const;
    void resetStats();
    
    // Error handling and logging
    struct ErrorInfo {
        enum Severity {
            Warning,
            Error,
            Critical
        };
        
        Severity severity = Warning;
        std::string message;
        std::string filePath;
        FileFormat format = FileFormat::Unknown;
        std::chrono::system_clock::time_point timestamp;
    };
    
    std::vector<ErrorInfo> getRecentErrors() const;
    void clearErrorLog();

private:
    Rendering::RenderingEngine& engine_;
    std::atomic<bool> initialized_{false};
    
    // Format handlers
    std::unordered_map<FileFormat, std::unique_ptr<IFormatHandler>> handlers_;
    mutable std::mutex handlersMutex_;
    
    // Extension to format mapping
    std::unordered_map<std::string, FileFormat> extensionMap_;
    
    // Import/Export presets
    std::vector<ImportPreset> importPresets_;
    std::vector<ExportPreset> exportPresets_;
    mutable std::mutex presetsMutex_;
    
    // Plugin management
    std::unordered_map<std::string, void*> loadedPlugins_;  // Plugin name -> handle
    mutable std::mutex pluginsMutex_;
    
    // Performance settings
    uint32_t maxConcurrency_ = 0;  // 0 = auto-detect
    size_t memoryBudget_ = 2LL * 1024 * 1024 * 1024;  // 2GB default
    bool cacheEnabled_ = true;
    
    // Thread pool for async operations
    class ThreadPool;
    std::unique_ptr<ThreadPool> threadPool_;
    
    // Cache for thumbnails and metadata
    struct CacheEntry {
        FileInfo fileInfo;
        std::shared_ptr<Image> thumbnail;
        std::chrono::system_clock::time_point lastAccess;
        std::chrono::system_clock::time_point fileModified;
        size_t memorySize;
    };
    
    mutable std::mutex cacheMutex_;
    std::unordered_map<std::string, CacheEntry> fileCache_;
    size_t maxCacheSize_ = 1000;
    size_t currentCacheMemory_ = 0;
    
    // Statistics
    mutable std::mutex statsMutex_;
    FileFormatStats stats_;
    
    // Error logging
    mutable std::mutex errorMutex_;
    std::vector<ErrorInfo> errorLog_;
    size_t maxErrorLogSize_ = 1000;
    
    // Internal methods
    void registerBuiltInHandlers();
    void buildExtensionMap();
    void createDefaultPresets();
    
    FormatDetectionResult detectByMagicBytes(const std::vector<uint8_t>& data) const;
    FormatDetectionResult detectByContent(const std::filesystem::path& filePath) const;
    
    void updateCache(const std::string& filePath, const FileInfo& info, 
                    std::shared_ptr<Image> thumbnail = nullptr);
    void cleanupCache();
    
    void logError(const ErrorInfo& error);
    
    // Magic byte patterns for format detection
    struct MagicPattern {
        std::vector<uint8_t> pattern;
        std::vector<uint8_t> mask;  // Optional mask for pattern matching
        size_t offset = 0;
        FileFormat format;
        float confidence = 1.0f;
    };
    
    std::vector<MagicPattern> magicPatterns_;
    void initializeMagicPatterns();
    
    // Format name mapping
    static std::unordered_map<FileFormat, std::string> formatNames_;
    static void initializeFormatNames();
};

// Utility functions for file format operations
namespace FormatUtils {
    // File extension utilities
    std::string getFileExtension(const std::filesystem::path& filePath);
    std::string normalizeExtension(const std::string& extension);
    std::vector<std::string> splitExtensions(const std::string& extensions);
    
    // MIME type utilities
    std::string getMimeTypeForExtension(const std::string& extension);
    FileFormat getFormatForMimeType(const std::string& mimeType);
    
    // File size utilities
    std::string formatFileSize(uint64_t sizeBytes);
    bool isFileSizeReasonable(uint64_t sizeBytes, FileFormat format);
    
    // Path utilities
    std::filesystem::path generateUniqueFilePath(const std::filesystem::path& basePath);
    bool isValidFilename(const std::string& filename);
    std::string sanitizeFilename(const std::string& filename);
    
    // Format conversion utilities
    bool isLossyFormat(FileFormat format);
    bool supportsTransparency(FileFormat format);
    bool supportsLayers(FileFormat format);
    bool supportsMetadata(FileFormat format);
    
    // Quality estimation
    float estimateCompressionRatio(FileFormat format, float quality);
    uint64_t estimateOutputFileSize(const std::array<uint32_t, 2>& dimensions, 
                                   uint8_t channels, FileFormat format, float quality = 0.9f);
    
    // Color profile utilities
    bool formatSupportsColorProfiles(FileFormat format);
    std::vector<uint8_t> extractColorProfile(const std::filesystem::path& filePath);
    bool embedColorProfile(const std::filesystem::path& filePath, 
                          const std::vector<uint8_t>& profileData);
    
    // Metadata utilities
    std::unordered_map<std::string, std::string> extractBasicMetadata(
        const std::filesystem::path& filePath);
    bool preserveTimestamps(const std::filesystem::path& sourcePath,
                           const std::filesystem::path& targetPath);
    
    // Validation utilities
    bool isValidImageDimensions(const std::array<uint32_t, 2>& dimensions);
    bool isValidBitDepth(uint8_t bitDepth);
    bool isValidChannelCount(uint8_t channels);
    
    // Performance utilities
    size_t calculateMemoryRequirement(const std::array<uint32_t, 2>& dimensions,
                                     uint8_t channels, uint8_t bitDepth);
    bool shouldUseStreamingIO(uint64_t fileSize);
    uint32_t calculateOptimalThreadCount(uint64_t totalDataSize);
}

} // namespace QuantumCanvas::IO
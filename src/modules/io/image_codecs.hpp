#pragma once

#include "file_format_manager.hpp"
#include "../../core/memory/memory_manager.hpp"
#include <memory>
#include <vector>
#include <array>
#include <functional>
#include <atomic>
#include <mutex>

namespace QuantumCanvas::IO {

// Forward declarations
class Image;

// Image codec interface for different formats
class IImageCodec {
public:
    virtual ~IImageCodec() = default;
    
    // Codec identification
    virtual FileFormat getFormat() const = 0;
    virtual std::string getCodecName() const = 0;
    virtual std::string getCodecVersion() const = 0;
    
    // Capability queries
    virtual bool canEncode() const = 0;
    virtual bool canDecode() const = 0;
    virtual bool supportsAnimation() const { return false; }
    virtual bool supportsMultiPage() const { return false; }
    virtual bool supportsLosslessCompression() const = 0;
    virtual bool supportsLossyCompression() const = 0;
    virtual bool supportsTransparency() const = 0;
    virtual bool supportsColorProfiles() const { return false; }
    virtual bool supportsMetadata() const { return false; }
    
    // Supported bit depths and color modes
    virtual std::vector<uint8_t> getSupportedBitDepths() const = 0;
    virtual std::vector<uint8_t> getSupportedChannelCounts() const = 0;
    
    // Format detection
    virtual bool canDecodeFile(const std::filesystem::path& filePath) const = 0;
    virtual bool canDecodeData(const std::vector<uint8_t>& data) const = 0;
    
    // Decoding
    virtual std::shared_ptr<Image> decode(
        const std::filesystem::path& filePath,
        const LoadOptions& options = {}) = 0;
    
    virtual std::shared_ptr<Image> decode(
        const std::vector<uint8_t>& data,
        const LoadOptions& options = {}) = 0;
    
    // Encoding
    virtual std::vector<uint8_t> encode(
        const std::shared_ptr<Image>& image,
        const SaveOptions& options = {}) = 0;
    
    virtual bool encode(
        const std::shared_ptr<Image>& image,
        const std::filesystem::path& filePath,
        const SaveOptions& options = {}) = 0;
    
    // Streaming support for large images
    virtual bool supportsStreamingDecode() const { return false; }
    virtual bool supportsStreamingEncode() const { return false; }
    
    // Quality estimation for lossy formats
    virtual float estimateQuality(const std::vector<uint8_t>& data) const { return 1.0f; }
    virtual size_t estimateFileSize(const std::shared_ptr<Image>& image, float quality) const = 0;
};

// PNG codec implementation
class PNGCodec final : public IImageCodec {
public:
    explicit PNGCodec(Core::MemoryManager& memoryManager);
    ~PNGCodec() override;
    
    // IImageCodec interface
    FileFormat getFormat() const override { return FileFormat::PNG; }
    std::string getCodecName() const override { return "libpng"; }
    std::string getCodecVersion() const override;
    
    bool canEncode() const override { return true; }
    bool canDecode() const override { return true; }
    bool supportsLosslessCompression() const override { return true; }
    bool supportsLossyCompression() const override { return false; }
    bool supportsTransparency() const override { return true; }
    bool supportsColorProfiles() const override { return true; }
    bool supportsMetadata() const override { return true; }
    
    std::vector<uint8_t> getSupportedBitDepths() const override { return {1, 2, 4, 8, 16}; }
    std::vector<uint8_t> getSupportedChannelCounts() const override { return {1, 2, 3, 4}; }
    
    bool canDecodeFile(const std::filesystem::path& filePath) const override;
    bool canDecodeData(const std::vector<uint8_t>& data) const override;
    
    std::shared_ptr<Image> decode(const std::filesystem::path& filePath, 
                                 const LoadOptions& options = {}) override;
    std::shared_ptr<Image> decode(const std::vector<uint8_t>& data,
                                 const LoadOptions& options = {}) override;
    
    std::vector<uint8_t> encode(const std::shared_ptr<Image>& image,
                               const SaveOptions& options = {}) override;
    bool encode(const std::shared_ptr<Image>& image,
               const std::filesystem::path& filePath,
               const SaveOptions& options = {}) override;
    
    size_t estimateFileSize(const std::shared_ptr<Image>& image, float quality) const override;

private:
    Core::MemoryManager& memoryManager_;
    
    // libpng callbacks
    static void pngReadCallback(void* png_ptr, uint8_t* data, size_t length);
    static void pngWriteCallback(void* png_ptr, uint8_t* data, size_t length);
    static void pngFlushCallback(void* png_ptr);
    static void pngErrorCallback(void* png_ptr, const char* message);
    static void pngWarningCallback(void* png_ptr, const char* message);
    
    // Internal decoding/encoding helpers
    struct PNGReadState {
        const uint8_t* data;
        size_t size;
        size_t offset;
    };
    
    struct PNGWriteState {
        std::vector<uint8_t> buffer;
    };
    
    std::shared_ptr<Image> decodePNGData(PNGReadState& readState, const LoadOptions& options);
    std::vector<uint8_t> encodePNGData(const std::shared_ptr<Image>& image, const SaveOptions& options);
    
    // Metadata handling
    void extractMetadata(void* png_ptr, void* info_ptr, std::unordered_map<std::string, std::string>& metadata);
    void embedMetadata(void* png_ptr, void* info_ptr, const std::unordered_map<std::string, std::string>& metadata);
    
    // Color profile handling
    void extractColorProfile(void* png_ptr, void* info_ptr, std::vector<uint8_t>& profileData);
    void embedColorProfile(void* png_ptr, void* info_ptr, const std::vector<uint8_t>& profileData);
};

// JPEG codec implementation
class JPEGCodec final : public IImageCodec {
public:
    explicit JPEGCodec(Core::MemoryManager& memoryManager);
    ~JPEGCodec() override;
    
    // IImageCodec interface
    FileFormat getFormat() const override { return FileFormat::JPEG; }
    std::string getCodecName() const override { return "libjpeg-turbo"; }
    std::string getCodecVersion() const override;
    
    bool canEncode() const override { return true; }
    bool canDecode() const override { return true; }
    bool supportsLosslessCompression() const override { return false; }
    bool supportsLossyCompression() const override { return true; }
    bool supportsTransparency() const override { return false; }
    bool supportsColorProfiles() const override { return true; }
    bool supportsMetadata() const override { return true; }
    
    std::vector<uint8_t> getSupportedBitDepths() const override { return {8}; }
    std::vector<uint8_t> getSupportedChannelCounts() const override { return {1, 3}; }
    
    bool canDecodeFile(const std::filesystem::path& filePath) const override;
    bool canDecodeData(const std::vector<uint8_t>& data) const override;
    
    std::shared_ptr<Image> decode(const std::filesystem::path& filePath,
                                 const LoadOptions& options = {}) override;
    std::shared_ptr<Image> decode(const std::vector<uint8_t>& data,
                                 const LoadOptions& options = {}) override;
    
    std::vector<uint8_t> encode(const std::shared_ptr<Image>& image,
                               const SaveOptions& options = {}) override;
    bool encode(const std::shared_ptr<Image>& image,
               const std::filesystem::path& filePath,
               const SaveOptions& options = {}) override;
    
    float estimateQuality(const std::vector<uint8_t>& data) const override;
    size_t estimateFileSize(const std::shared_ptr<Image>& image, float quality) const override;

private:
    Core::MemoryManager& memoryManager_;
    
    // libjpeg callbacks
    static void jpegErrorExit(void* cinfo);
    static void jpegOutputMessage(void* cinfo);
    
    // Memory source/destination
    struct JPEGMemorySource {
        void* pub;  // public fields
        const uint8_t* data;
        size_t size;
    };
    
    struct JPEGMemoryDestination {
        void* pub;  // public fields
        std::vector<uint8_t>* buffer;
        uint8_t* temp_buffer;
        size_t temp_size;
    };
    
    void setupMemorySource(void* cinfo, const uint8_t* data, size_t size);
    void setupMemoryDestination(void* cinfo, std::vector<uint8_t>& buffer);
    
    // Quality analysis
    struct JPEGQuantTable {
        uint16_t table[64];
        bool is_set;
    };
    
    float analyzeQuantizationTables(const std::vector<JPEGQuantTable>& tables) const;
    
    // EXIF/metadata handling
    void extractEXIFData(void* cinfo, std::unordered_map<std::string, std::string>& metadata);
    void embedEXIFData(void* cinfo, const std::unordered_map<std::string, std::string>& metadata);
    
    // Color space conversion
    void convertColorSpace(std::shared_ptr<Image>& image, int jpegColorSpace);
};

// TIFF codec implementation
class TIFFCodec final : public IImageCodec {
public:
    explicit TIFFCodec(Core::MemoryManager& memoryManager);
    ~TIFFCodec() override;
    
    // IImageCodec interface
    FileFormat getFormat() const override { return FileFormat::TIFF; }
    std::string getCodecName() const override { return "libtiff"; }
    std::string getCodecVersion() const override;
    
    bool canEncode() const override { return true; }
    bool canDecode() const override { return true; }
    bool supportsAnimation() const override { return false; }
    bool supportsMultiPage() const override { return true; }
    bool supportsLosslessCompression() const override { return true; }
    bool supportsLossyCompression() const override { return true; }  // JPEG compression
    bool supportsTransparency() const override { return true; }
    bool supportsColorProfiles() const override { return true; }
    bool supportsMetadata() const override { return true; }
    
    std::vector<uint8_t> getSupportedBitDepths() const override { return {1, 2, 4, 8, 16, 32}; }
    std::vector<uint8_t> getSupportedChannelCounts() const override { return {1, 3, 4}; }
    
    bool canDecodeFile(const std::filesystem::path& filePath) const override;
    bool canDecodeData(const std::vector<uint8_t>& data) const override;
    
    std::shared_ptr<Image> decode(const std::filesystem::path& filePath,
                                 const LoadOptions& options = {}) override;
    std::shared_ptr<Image> decode(const std::vector<uint8_t>& data,
                                 const LoadOptions& options = {}) override;
    
    std::vector<uint8_t> encode(const std::shared_ptr<Image>& image,
                               const SaveOptions& options = {}) override;
    bool encode(const std::shared_ptr<Image>& image,
               const std::filesystem::path& filePath,
               const SaveOptions& options = {}) override;
    
    size_t estimateFileSize(const std::shared_ptr<Image>& image, float quality) const override;
    
    // TIFF-specific methods
    uint32_t getPageCount(const std::filesystem::path& filePath) const;
    std::shared_ptr<Image> decodePage(const std::filesystem::path& filePath, uint32_t pageIndex,
                                     const LoadOptions& options = {});

private:
    Core::MemoryManager& memoryManager_;
    
    // libtiff memory I/O
    struct TIFFMemoryIO {
        const uint8_t* data;
        size_t size;
        size_t offset;
        std::vector<uint8_t>* output_buffer;  // for writing
    };
    
    static int tiffReadProc(void* handle, void* buffer, int size);
    static int tiffWriteProc(void* handle, void* buffer, int size);
    static uint64_t tiffSeekProc(void* handle, uint64_t offset, int whence);
    static int tiffCloseProc(void* handle);
    static uint64_t tiffSizeProc(void* handle);
    
    // TIFF tag handling
    void extractTIFFTags(void* tiff, std::unordered_map<std::string, std::string>& metadata);
    void setTIFFTags(void* tiff, const std::unordered_map<std::string, std::string>& metadata,
                    const SaveOptions& options);
    
    // Compression selection
    uint16_t selectCompression(const SaveOptions& options, uint8_t channels, uint8_t bitDepth);
    
    // Predictor selection for LZW/ZIP
    uint16_t selectPredictor(uint8_t channels, uint8_t bitDepth);
    
    // Multi-page handling
    std::vector<std::shared_ptr<Image>> decodeAllPages(void* tiff, const LoadOptions& options);
    bool encodeMultiPage(const std::vector<std::shared_ptr<Image>>& images,
                        void* tiff, const SaveOptions& options);
};

// WebP codec implementation
class WebPCodec final : public IImageCodec {
public:
    explicit WebPCodec(Core::MemoryManager& memoryManager);
    ~WebPCodec() override;
    
    // IImageCodec interface
    FileFormat getFormat() const override { return FileFormat::WEBP; }
    std::string getCodecName() const override { return "libwebp"; }
    std::string getCodecVersion() const override;
    
    bool canEncode() const override { return true; }
    bool canDecode() const override { return true; }
    bool supportsAnimation() const override { return true; }
    bool supportsLosslessCompression() const override { return true; }
    bool supportsLossyCompression() const override { return true; }
    bool supportsTransparency() const override { return true; }
    bool supportsMetadata() const override { return true; }
    
    std::vector<uint8_t> getSupportedBitDepths() const override { return {8}; }
    std::vector<uint8_t> getSupportedChannelCounts() const override { return {3, 4}; }
    
    bool canDecodeFile(const std::filesystem::path& filePath) const override;
    bool canDecodeData(const std::vector<uint8_t>& data) const override;
    
    std::shared_ptr<Image> decode(const std::filesystem::path& filePath,
                                 const LoadOptions& options = {}) override;
    std::shared_ptr<Image> decode(const std::vector<uint8_t>& data,
                                 const LoadOptions& options = {}) override;
    
    std::vector<uint8_t> encode(const std::shared_ptr<Image>& image,
                               const SaveOptions& options = {}) override;
    bool encode(const std::shared_ptr<Image>& image,
               const std::filesystem::path& filePath,
               const SaveOptions& options = {}) override;
    
    size_t estimateFileSize(const std::shared_ptr<Image>& image, float quality) const override;
    
    // WebP-specific methods
    bool isAnimated(const std::filesystem::path& filePath) const;
    bool isAnimated(const std::vector<uint8_t>& data) const;
    std::vector<std::shared_ptr<Image>> decodeAnimation(const std::filesystem::path& filePath);
    std::vector<std::shared_ptr<Image>> decodeAnimation(const std::vector<uint8_t>& data);

private:
    Core::MemoryManager& memoryManager_;
    
    // WebP feature detection
    struct WebPFeatures {
        bool hasAlpha;
        bool hasAnimation;
        bool isLossless;
        uint32_t width;
        uint32_t height;
        uint32_t frameCount;
    };
    
    WebPFeatures analyzeWebPFeatures(const std::vector<uint8_t>& data) const;
    
    // Encoding configuration
    void configureEncoder(void* config, const SaveOptions& options, bool hasAlpha);
    void configureLosslessEncoder(void* config, const SaveOptions& options);
};

// Raw image codec for camera RAW formats
class RAWCodec final : public IImageCodec {
public:
    explicit RAWCodec(Core::MemoryManager& memoryManager);
    ~RAWCodec() override;
    
    // IImageCodec interface
    FileFormat getFormat() const override { return rawFormat_; }
    std::string getCodecName() const override { return "LibRaw"; }
    std::string getCodecVersion() const override;
    
    bool canEncode() const override { return false; }  // RAW is read-only
    bool canDecode() const override { return true; }
    bool supportsLosslessCompression() const override { return true; }
    bool supportsLossyCompression() const override { return false; }
    bool supportsTransparency() const override { return false; }
    bool supportsColorProfiles() const override { return true; }
    bool supportsMetadata() const override { return true; }
    
    std::vector<uint8_t> getSupportedBitDepths() const override { return {16}; }
    std::vector<uint8_t> getSupportedChannelCounts() const override { return {3}; }
    
    bool canDecodeFile(const std::filesystem::path& filePath) const override;
    bool canDecodeData(const std::vector<uint8_t>& data) const override;
    
    std::shared_ptr<Image> decode(const std::filesystem::path& filePath,
                                 const LoadOptions& options = {}) override;
    std::shared_ptr<Image> decode(const std::vector<uint8_t>& data,
                                 const LoadOptions& options = {}) override;
    
    std::vector<uint8_t> encode(const std::shared_ptr<Image>& image,
                               const SaveOptions& options = {}) override;
    bool encode(const std::shared_ptr<Image>& image,
               const std::filesystem::path& filePath,
               const SaveOptions& options = {}) override;
    
    size_t estimateFileSize(const std::shared_ptr<Image>& image, float quality) const override;
    
    // RAW-specific methods
    void setRAWFormat(FileFormat format) { rawFormat_ = format; }
    
    struct RAWInfo {
        std::string cameraMake;
        std::string cameraModel;
        uint32_t isoSpeed;
        float shutterSpeed;
        float aperture;
        float focalLength;
        std::chrono::system_clock::time_point timestamp;
        
        // Color information
        std::array<float, 4> colorMatrix[3];  // 3x4 color matrices
        std::array<float, 3> whiteBalance;    // As shot white balance
        float colorTemperature;
        
        // Image properties
        std::array<uint32_t, 2> rawSize;
        std::array<uint32_t, 2> outputSize;
        uint16_t blackLevel;
        uint16_t whiteLevel;
        float gamma;
    };
    
    RAWInfo getRAWInfo(const std::filesystem::path& filePath) const;
    
    // Generate thumbnail from RAW (fast preview)
    std::shared_ptr<Image> generateThumbnail(const std::filesystem::path& filePath,
                                            const std::array<uint32_t, 2>& size = {256, 256});

private:
    Core::MemoryManager& memoryManager_;
    FileFormat rawFormat_ = FileFormat::Unknown;
    
    // LibRaw processor wrapper
    class LibRawProcessor;
    
    // RAW processing parameters
    void configureProcessor(LibRawProcessor& processor, const LoadOptions& options);
    
    // Demosaic algorithms
    enum class DemosaicAlgorithm {
        Linear = 0,
        VNG = 1,
        PPG = 2,
        AHD = 3,
        DCB = 5,
        DHT = 11,
        AAHD = 12
    };
    
    DemosaicAlgorithm selectDemosaicAlgorithm(const LoadOptions& options);
    
    // Color space conversion for RAW
    void applyColorCorrection(std::shared_ptr<Image>& image, const RAWInfo& info,
                             const LoadOptions& options);
    
    // White balance adjustment
    void applyWhiteBalance(std::shared_ptr<Image>& image, float temperature, float tint);
    
    // Exposure adjustment
    void applyExposure(std::shared_ptr<Image>& image, float exposure, 
                      float highlights, float shadows, float whites, float blacks);
    
    // Camera-specific corrections
    void applyLensCorrections(std::shared_ptr<Image>& image, const RAWInfo& info);
    void applyNoiseReduction(std::shared_ptr<Image>& image, const LoadOptions& options);
};

// Image codec registry
class ImageCodecRegistry final {
public:
    static ImageCodecRegistry& instance();
    
    // Codec management
    void registerCodec(std::unique_ptr<IImageCodec> codec);
    void unregisterCodec(FileFormat format);
    IImageCodec* getCodec(FileFormat format);
    const IImageCodec* getCodec(FileFormat format) const;
    
    // Format detection and selection
    FileFormat detectFormat(const std::filesystem::path& filePath) const;
    FileFormat detectFormat(const std::vector<uint8_t>& data) const;
    std::vector<FileFormat> getSupportedFormats() const;
    
    // Codec queries
    std::vector<FileFormat> getDecodableFormats() const;
    std::vector<FileFormat> getEncodableFormats() const;
    bool canDecode(FileFormat format) const;
    bool canEncode(FileFormat format) const;
    
    // Best codec selection
    IImageCodec* selectBestCodec(const std::shared_ptr<Image>& image, 
                                const SaveOptions& options) const;

private:
    ImageCodecRegistry() = default;
    ~ImageCodecRegistry() = default;
    
    std::unordered_map<FileFormat, std::unique_ptr<IImageCodec>> codecs_;
    mutable std::mutex codecsMutex_;
    
    void registerBuiltInCodecs();
};

// Utility functions for image codec operations
namespace CodecUtils {
    // Format detection by magic bytes
    struct MagicBytes {
        std::vector<uint8_t> signature;
        size_t offset;
        FileFormat format;
    };
    
    std::vector<MagicBytes> getAllMagicBytes();
    FileFormat detectByMagicBytes(const std::vector<uint8_t>& data);
    
    // Quality estimation
    float estimateImageQuality(const std::shared_ptr<Image>& original,
                              const std::shared_ptr<Image>& compressed);
    
    // Compression ratio calculation
    float calculateCompressionRatio(size_t originalSize, size_t compressedSize);
    
    // Color space utilities for codecs
    void convertRGBtoYUV(const std::shared_ptr<Image>& image);
    void convertYUVtoRGB(const std::shared_ptr<Image>& image);
    
    // Bit depth conversion
    void convertBitDepth(std::shared_ptr<Image>& image, uint8_t targetBitDepth);
    
    // Channel conversion
    void addAlphaChannel(std::shared_ptr<Image>& image, float alpha = 1.0f);
    void removeAlphaChannel(std::shared_ptr<Image>& image);
    void convertToGrayscale(std::shared_ptr<Image>& image);
    
    // Progressive encoding support
    bool shouldUseProgressiveEncoding(const std::shared_ptr<Image>& image);
    
    // Memory optimization for large images
    size_t calculateOptimalTileSize(const std::array<uint32_t, 2>& imageSize, 
                                   uint8_t channels, uint8_t bitDepth);
    
    bool shouldUseTiledProcessing(const std::array<uint32_t, 2>& imageSize,
                                 size_t availableMemory);
    
    // Error diffusion dithering (for bit depth reduction)
    void applyFloydSteinbergDithering(std::shared_ptr<Image>& image, uint8_t targetBitDepth);
    
    // Metadata standardization
    std::unordered_map<std::string, std::string> standardizeMetadata(
        const std::unordered_map<std::string, std::string>& metadata, FileFormat format);
}

} // namespace QuantumCanvas::IO
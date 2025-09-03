#pragma once

#include "../../core/rendering/rendering_engine.hpp"
#include "../../core/math/vector3.hpp"
#include "../../core/math/vector4.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <array>
#include <functional>
#include <filesystem>
#include <atomic>
#include <mutex>
#include <chrono>

namespace QuantumCanvas::Raster {

// Forward declarations
class Image;
class ColorProfile;

// Standard color spaces
enum class ColorSpace {
    sRGB,
    AdobeRGB,
    ProPhotoRGB,
    Rec2020,
    DCI_P3,
    CMYK,
    LAB,
    XYZ,
    HSV,
    HSL,
    LUV,
    Custom
};

// Color gamut descriptors
enum class ColorGamut {
    sRGB,           // ~35% of visible spectrum
    AdobeRGB,       // ~50% of visible spectrum
    ProPhotoRGB,    // ~90% of visible spectrum
    Rec2020,        // ~76% of visible spectrum
    P3_D65,         // ~45% of visible spectrum
    NTSC,           // Legacy broadcast standard
    Custom
};

// Rendering intents for color conversion
enum class RenderingIntent {
    Perceptual,         // Preserves overall color relationships
    RelativeColorimetric, // Preserves in-gamut colors exactly
    Saturation,         // Preserves saturation, may shift hue
    AbsoluteColorimetric  // Preserves exact color values
};

// White point standards
enum class WhitePoint {
    D50,    // 5003K - ICC standard illuminant
    D55,    // 5503K
    D65,    // 6504K - sRGB standard
    D75,    // 7504K
    A,      // 2856K - Incandescent
    C,      // 6774K - Daylight
    E,      // Equal energy
    F2,     // 4230K - Cool white fluorescent
    F7,     // 6500K - Daylight fluorescent
    F11,    // 4000K - Narrow band fluorescent
    Custom
};

// Color temperature and tint adjustment
struct ColorTemperature {
    float temperature = 6500.0f;  // Kelvin (1000K - 25000K)
    float tint = 0.0f;           // Green-Magenta shift (-100 to +100)
    
    // Convert to white point
    std::array<float, 3> toWhitePoint() const;
    
    // Create from white point
    static ColorTemperature fromWhitePoint(const std::array<float, 3>& whitePoint);
};

// ICC profile information
struct ICCProfileInfo {
    std::string name;
    std::string description;
    std::string manufacturer;
    std::string model;
    std::string copyright;
    ColorSpace colorSpace;
    ColorGamut gamut;
    WhitePoint whitePoint;
    std::array<float, 3> whitePointXYZ;
    std::array<float, 9> primaryMatrix;  // RGB to XYZ matrix
    std::vector<uint8_t> profileData;    // Raw ICC profile data
    std::filesystem::path filePath;
    
    // Profile validation
    bool isValid() const;
    
    // Gamut coverage calculation
    float calculateGamutCoverage(ColorGamut referenceGamut) const;
};

// Color conversion matrix (3x3)
struct ColorMatrix {
    std::array<float, 9> matrix{
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f
    };
    
    ColorMatrix() = default;
    ColorMatrix(const std::array<float, 9>& m) : matrix(m) {}
    
    // Matrix operations
    ColorMatrix operator*(const ColorMatrix& other) const;
    ColorMatrix inverse() const;
    std::array<float, 3> transform(const std::array<float, 3>& color) const;
    
    // Standard matrices
    static ColorMatrix sRGBtoXYZ();
    static ColorMatrix XYZtosRGB();
    static ColorMatrix AdobeRGBtoXYZ();
    static ColorMatrix XYZtoAdobeRGB();
    static ColorMatrix ProPhotoRGBtoXYZ();
    static ColorMatrix XYZtoProPhotoRGB();
};

// Gamut mapping algorithms
enum class GamutMappingMethod {
    Clip,               // Hard clipping to gamut boundaries
    Scale,              // Scale entire color volume
    Compress,           // Compress out-of-gamut colors
    PerceptualSmooth,   // Perceptual smooth mapping
    Lightness,          // Preserve lightness, compress chroma
    Saturation,         // Preserve saturation, compress lightness
    HPMINDE,           // High Performance Minimum Delta E
    Custom
};

// Color profile for color space management
class ColorProfile {
public:
    explicit ColorProfile(ColorSpace colorSpace = ColorSpace::sRGB);
    explicit ColorProfile(const std::filesystem::path& iccProfilePath);
    explicit ColorProfile(const std::vector<uint8_t>& iccData);
    ~ColorProfile();
    
    // Profile information
    const ICCProfileInfo& getInfo() const { return info_; }
    ColorSpace getColorSpace() const { return info_.colorSpace; }
    ColorGamut getGamut() const { return info_.gamut; }
    WhitePoint getWhitePoint() const { return info_.whitePoint; }
    
    // Profile validation
    bool isValid() const { return valid_; }
    std::string getLastError() const { return lastError_; }
    
    // Color conversion matrices
    const ColorMatrix& getToXYZMatrix() const { return toXYZMatrix_; }
    const ColorMatrix& getFromXYZMatrix() const { return fromXYZMatrix_; }
    
    // Lookup tables (LUTs) for complex transforms
    bool hasLUT() const { return !lut_.empty(); }
    const std::vector<std::array<float, 3>>& getLUT() const { return lut_; }
    
    // Transform functions
    std::array<float, 3> transformToXYZ(const std::array<float, 3>& color) const;
    std::array<float, 3> transformFromXYZ(const std::array<float, 3>& color) const;
    
    // Gamma correction
    float getGamma() const { return gamma_; }
    std::array<float, 3> applyGamma(const std::array<float, 3>& color) const;
    std::array<float, 3> removeGamma(const std::array<float, 3>& color) const;
    
    // Serialization
    std::vector<uint8_t> serialize() const;
    bool deserialize(const std::vector<uint8_t>& data);
    
    // Standard profiles
    static std::shared_ptr<ColorProfile> sRGB();
    static std::shared_ptr<ColorProfile> AdobeRGB();
    static std::shared_ptr<ColorProfile> ProPhotoRGB();
    static std::shared_ptr<ColorProfile> Rec2020();
    static std::shared_ptr<ColorProfile> DCIP3();

private:
    ICCProfileInfo info_;
    ColorMatrix toXYZMatrix_;
    ColorMatrix fromXYZMatrix_;
    std::vector<std::array<float, 3>> lut_;  // 3D LUT for complex transforms
    float gamma_ = 2.2f;
    bool valid_ = false;
    std::string lastError_;
    
    // ICC profile parsing
    bool parseICCProfile(const std::vector<uint8_t>& data);
    void initializeStandardProfile(ColorSpace colorSpace);
    void calculateMatrices();
};

// Color temperature and white balance adjustment
class WhiteBalanceAdjuster {
public:
    WhiteBalanceAdjuster() = default;
    ~WhiteBalanceAdjuster() = default;
    
    // White balance adjustment
    void setSourceTemperature(float kelvin, float tint = 0.0f);
    void setTargetTemperature(float kelvin, float tint = 0.0f);
    void setTemperatureShift(float deltaKelvin, float deltaTint = 0.0f);
    
    // Auto white balance detection
    ColorTemperature detectWhiteBalance(const Image& image, 
                                       const std::array<uint32_t, 4>& sampleRegion = {0, 0, 0, 0});
    
    // Apply white balance correction
    void adjustImage(Image& image);
    void adjustColor(std::array<float, 3>& rgb);
    
    // Bradford chromatic adaptation
    static ColorMatrix calculateBradfordMatrix(const std::array<float, 3>& sourceWhite,
                                              const std::array<float, 3>& targetWhite);

private:
    ColorTemperature sourceTemp_{6500.0f, 0.0f};
    ColorTemperature targetTemp_{6500.0f, 0.0f};
    ColorMatrix adaptationMatrix_;
    bool matrixDirty_ = true;
    
    void updateAdaptationMatrix();
    std::array<float, 3> kelvinToXYZ(float kelvin, float tint) const;
};

// Gamut mapping for color space conversion
class GamutMapper {
public:
    explicit GamutMapper(GamutMappingMethod method = GamutMappingMethod::PerceptualSmooth);
    ~GamutMapper() = default;
    
    // Mapping configuration
    void setMethod(GamutMappingMethod method) { method_ = method; }
    GamutMappingMethod getMethod() const { return method_; }
    
    void setCompressionAmount(float amount) { compressionAmount_ = std::clamp(amount, 0.0f, 1.0f); }
    float getCompressionAmount() const { return compressionAmount_; }
    
    // Gamut mapping
    std::array<float, 3> mapColor(const std::array<float, 3>& color, 
                                 const ColorProfile& sourceProfile,
                                 const ColorProfile& targetProfile);
    
    void mapImage(Image& image,
                  const ColorProfile& sourceProfile,
                  const ColorProfile& targetProfile);
    
    // Gamut testing
    bool isInGamut(const std::array<float, 3>& color, const ColorProfile& profile);
    float calculateGamutDistance(const std::array<float, 3>& color, const ColorProfile& profile);
    
    // Custom mapping function
    void setCustomMappingFunction(std::function<std::array<float, 3>(const std::array<float, 3>&)> func) {
        customMappingFunction_ = std::move(func);
    }

private:
    GamutMappingMethod method_;
    float compressionAmount_ = 0.8f;
    std::function<std::array<float, 3>(const std::array<float, 3>&)> customMappingFunction_;
    
    // Mapping algorithm implementations
    std::array<float, 3> clipMapping(const std::array<float, 3>& color, const ColorProfile& profile);
    std::array<float, 3> scaleMapping(const std::array<float, 3>& color, const ColorProfile& profile);
    std::array<float, 3> compressMapping(const std::array<float, 3>& color, const ColorProfile& profile);
    std::array<float, 3> perceptualMapping(const std::array<float, 3>& color, 
                                          const ColorProfile& sourceProfile,
                                          const ColorProfile& targetProfile);
};

// Soft proofing for print preview
class SoftProofingEngine {
public:
    SoftProofingEngine() = default;
    ~SoftProofingEngine() = default;
    
    // Proofing setup
    void setSourceProfile(std::shared_ptr<ColorProfile> profile) { sourceProfile_ = profile; }
    void setTargetProfile(std::shared_ptr<ColorProfile> profile) { targetProfile_ = profile; }
    void setDisplayProfile(std::shared_ptr<ColorProfile> profile) { displayProfile_ = profile; }
    
    void setRenderingIntent(RenderingIntent intent) { renderingIntent_ = intent; }
    RenderingIntent getRenderingIntent() const { return renderingIntent_; }
    
    void setGamutWarning(bool enable) { gamutWarning_ = enable; }
    bool getGamutWarning() const { return gamutWarning_; }
    
    void setGamutWarningColor(const std::array<float, 3>& color) { gamutWarningColor_ = color; }
    
    // Proofing operations
    void generateProof(const Image& sourceImage, Image& proofImage);
    void generateGamutMask(const Image& sourceImage, Image& gamutMask);
    
    // Gamut analysis
    struct GamutAnalysis {
        float inGamutPercentage;
        float outOfGamutPercentage;
        std::array<float, 3> maxDeltaE;
        float averageDeltaE;
        std::vector<std::array<uint32_t, 2>> outOfGamutPixels;
    };
    
    GamutAnalysis analyzeGamut(const Image& image);

private:
    std::shared_ptr<ColorProfile> sourceProfile_;
    std::shared_ptr<ColorProfile> targetProfile_;
    std::shared_ptr<ColorProfile> displayProfile_;
    RenderingIntent renderingIntent_ = RenderingIntent::Perceptual;
    bool gamutWarning_ = false;
    std::array<float, 3> gamutWarningColor_{1.0f, 0.0f, 1.0f}; // Magenta
    
    std::array<float, 3> simulatePrintColor(const std::array<float, 3>& sourceColor);
};

// Main color management system
class ColorManager final {
public:
    explicit ColorManager(Rendering::RenderingEngine& engine);
    ~ColorManager();
    
    // Disable copy, enable move
    ColorManager(const ColorManager&) = delete;
    ColorManager& operator=(const ColorManager&) = delete;
    ColorManager(ColorManager&& other) noexcept;
    ColorManager& operator=(ColorManager&& other) noexcept;
    
    // Initialization
    bool initialize();
    void shutdown();
    bool isInitialized() const { return initialized_; }
    
    // Profile management
    bool loadProfile(const std::filesystem::path& profilePath);
    bool loadProfile(const std::vector<uint8_t>& profileData, const std::string& name);
    void unloadProfile(const std::string& name);
    std::shared_ptr<ColorProfile> getProfile(const std::string& name);
    std::vector<std::string> getAvailableProfiles() const;
    
    // Working color space
    void setWorkingColorSpace(std::shared_ptr<ColorProfile> profile);
    std::shared_ptr<ColorProfile> getWorkingColorSpace() const { return workingColorSpace_; }
    
    // Display profile management
    void setDisplayProfile(std::shared_ptr<ColorProfile> profile);
    std::shared_ptr<ColorProfile> getDisplayProfile() const { return displayProfile_; }
    void autoDetectDisplayProfile();
    
    // Color conversion
    std::array<float, 3> convertColor(const std::array<float, 3>& color,
                                     const ColorProfile& sourceProfile,
                                     const ColorProfile& targetProfile,
                                     RenderingIntent intent = RenderingIntent::Perceptual);
    
    void convertImage(Image& image,
                     const ColorProfile& sourceProfile,
                     const ColorProfile& targetProfile,
                     RenderingIntent intent = RenderingIntent::Perceptual);
    
    // GPU-accelerated color conversion
    bool convertImageGPU(Rendering::ResourceId sourceTexture,
                        Rendering::ResourceId targetTexture,
                        const std::array<uint32_t, 2>& size,
                        const ColorProfile& sourceProfile,
                        const ColorProfile& targetProfile,
                        RenderingIntent intent = RenderingIntent::Perceptual);
    
    // White balance adjustment
    WhiteBalanceAdjuster& getWhiteBalanceAdjuster() { return whiteBalanceAdjuster_; }
    const WhiteBalanceAdjuster& getWhiteBalanceAdjuster() const { return whiteBalanceAdjuster_; }
    
    // Gamut mapping
    GamutMapper& getGamutMapper() { return gamutMapper_; }
    const GamutMapper& getGamutMapper() const { return gamutMapper_; }
    
    // Soft proofing
    SoftProofingEngine& getSoftProofingEngine() { return softProofingEngine_; }
    const SoftProofingEngine& getSoftProofingEngine() const { return softProofingEngine_; }
    
    // Color space utilities
    static std::array<float, 3> RGBtoXYZ(const std::array<float, 3>& rgb, const ColorMatrix& matrix);
    static std::array<float, 3> XYZtoRGB(const std::array<float, 3>& xyz, const ColorMatrix& matrix);
    static std::array<float, 3> XYZtoLAB(const std::array<float, 3>& xyz, const std::array<float, 3>& whitePoint);
    static std::array<float, 3> LABtoXYZ(const std::array<float, 3>& lab, const std::array<float, 3>& whitePoint);
    static std::array<float, 3> RGBtoHSV(const std::array<float, 3>& rgb);
    static std::array<float, 3> HSVtoRGB(const std::array<float, 3>& hsv);
    
    // Delta E color difference calculation
    static float calculateDeltaE76(const std::array<float, 3>& lab1, const std::array<float, 3>& lab2);
    static float calculateDeltaE94(const std::array<float, 3>& lab1, const std::array<float, 3>& lab2);
    static float calculateDeltaE2000(const std::array<float, 3>& lab1, const std::array<float, 3>& lab2);
    
    // Color temperature utilities
    static std::array<float, 3> blackbodyToXYZ(float temperatureK);
    static float XYZtoTemperature(const std::array<float, 3>& xyz);
    
    // Profile installation and system integration
    void installSystemProfile(const std::filesystem::path& profilePath);
    void uninstallSystemProfile(const std::string& profileName);
    std::vector<std::filesystem::path> getSystemProfilePaths();
    
    // Settings and preferences
    void setDefaultRenderingIntent(RenderingIntent intent) { defaultRenderingIntent_ = intent; }
    RenderingIntent getDefaultRenderingIntent() const { return defaultRenderingIntent_; }
    
    void setEnableColorManagement(bool enable) { colorManagementEnabled_ = enable; }
    bool isColorManagementEnabled() const { return colorManagementEnabled_; }
    
    // Performance settings
    void setUseGPUAcceleration(bool useGPU) { useGPUAcceleration_ = useGPU; }
    bool getUseGPUAcceleration() const { return useGPUAcceleration_; }
    
    // Statistics
    struct ColorManagerStats {
        uint32_t conversionsPerformed = 0;
        uint32_t profilesLoaded = 0;
        uint64_t pixelsConverted = 0;
        std::chrono::microseconds conversionTime{0};
        std::chrono::microseconds gpuTime{0};
        size_t gpuMemoryUsed = 0;
        float averageDeltaE = 0.0f;
    };
    
    ColorManagerStats getStats() const;
    void resetStats();

private:
    Rendering::RenderingEngine& engine_;
    std::atomic<bool> initialized_{false};
    
    // Profile storage
    std::unordered_map<std::string, std::shared_ptr<ColorProfile>> profiles_;
    mutable std::mutex profilesMutex_;
    
    // Color space management
    std::shared_ptr<ColorProfile> workingColorSpace_;
    std::shared_ptr<ColorProfile> displayProfile_;
    
    // Color processing components
    WhiteBalanceAdjuster whiteBalanceAdjuster_;
    GamutMapper gamutMapper_;
    SoftProofingEngine softProofingEngine_;
    
    // GPU resources
    Rendering::PipelineId colorConversionPipelineId_ = 0;
    Rendering::PipelineId gamutMappingPipelineId_ = 0;
    Rendering::ResourceId conversionUniformBuffer_ = 0;
    Rendering::ResourceId lutTexture_ = 0;
    
    // Settings
    RenderingIntent defaultRenderingIntent_ = RenderingIntent::Perceptual;
    bool colorManagementEnabled_ = true;
    bool useGPUAcceleration_ = true;
    
    // Performance statistics
    mutable std::mutex statsMutex_;
    ColorManagerStats stats_;
    
    // LUT cache for complex conversions
    struct LUTCacheEntry {
        std::string sourceProfileName;
        std::string targetProfileName;
        RenderingIntent intent;
        Rendering::ResourceId lutTextureId;
        std::array<uint32_t, 3> lutSize;
        std::chrono::system_clock::time_point creationTime;
        uint32_t accessCount;
    };
    
    mutable std::mutex lutCacheMutex_;
    std::unordered_map<uint64_t, LUTCacheEntry> lutCache_;
    
    // Internal methods
    bool createGPUResources();
    void destroyGPUResources();
    
    void loadStandardProfiles();
    void loadSystemProfiles();
    
    uint64_t calculateLUTCacheKey(const std::string& sourceProfile,
                                 const std::string& targetProfile,
                                 RenderingIntent intent) const;
    
    Rendering::ResourceId createConversionLUT(const ColorProfile& sourceProfile,
                                             const ColorProfile& targetProfile,
                                             RenderingIntent intent);
    
    void cleanupLUTCache();
    
    // Platform-specific profile detection
#ifdef _WIN32
    void loadWindowsProfiles();
#elif defined(__APPLE__)
    void loadMacOSProfiles();
#else
    void loadLinuxProfiles();
#endif
};

// Utility functions for color management
namespace ColorUtils {
    // Standard illuminants (CIE XYZ coordinates)
    std::array<float, 3> getIlluminant(WhitePoint whitePoint);
    
    // Chromatic adaptation transforms
    ColorMatrix getChromaticAdaptationMatrix(const std::array<float, 3>& sourceWhite,
                                            const std::array<float, 3>& targetWhite,
                                            const std::string& method = "Bradford");
    
    // Color difference calculations
    float deltaE76(const std::array<float, 3>& lab1, const std::array<float, 3>& lab2);
    float deltaE94(const std::array<float, 3>& lab1, const std::array<float, 3>& lab2);
    float deltaE2000(const std::array<float, 3>& lab1, const std::array<float, 3>& lab2);
    
    // Gamut volume calculations
    float calculateGamutVolume(const ColorProfile& profile);
    float calculateGamutOverlap(const ColorProfile& profile1, const ColorProfile& profile2);
    
    // Color mixing
    std::array<float, 3> mixColors(const std::array<float, 3>& color1,
                                  const std::array<float, 3>& color2,
                                  float ratio, ColorSpace colorSpace = ColorSpace::LAB);
    
    // Perceptual color ordering
    std::vector<std::array<float, 3>> sortColorsByPerception(const std::vector<std::array<float, 3>>& colors);
    
    // Color harmony generation
    std::vector<std::array<float, 3>> generateComplementaryColors(const std::array<float, 3>& baseColor);
    std::vector<std::array<float, 3>> generateTriadicColors(const std::array<float, 3>& baseColor);
    std::vector<std::array<float, 3>> generateAnalogousColors(const std::array<float, 3>& baseColor);
    
    // ICC profile utilities
    bool validateICCProfile(const std::vector<uint8_t>& profileData);
    std::string extractProfileDescription(const std::vector<uint8_t>& profileData);
    ColorSpace detectColorSpace(const std::vector<uint8_t>& profileData);
}

} // namespace QuantumCanvas::Raster
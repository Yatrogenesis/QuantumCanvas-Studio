#include "color_manager.hpp"
#include "../../core/rendering/rendering_engine.hpp"
#include "../../core/memory/memory_manager.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <iomanip>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace QuantumCanvas::Raster {

// Forward declaration for Image class
class Image;

// ColorTemperature implementation
std::array<float, 3> ColorTemperature::toWhitePoint() const {
    // Convert Kelvin temperature to XYZ white point
    // Simplified implementation based on Planckian locus
    float T = std::clamp(temperature, 1000.0f, 25000.0f);
    
    float x, y;
    
    if (T >= 1667 && T <= 4000) {
        x = (-4.6070e9 / (T * T * T)) + (2.9678e6 / (T * T)) + (0.09911e3 / T) + 0.244063;
    } else if (T > 4000 && T <= 25000) {
        x = (-2.0064e9 / (T * T * T)) + (1.9018e6 / (T * T)) + (0.24748e3 / T) + 0.237040;
    } else {
        x = 0.3127; // D65 default
    }
    
    if (T >= 1667 && T <= 2222) {
        y = -1.1063814 * (x * x * x) - 1.34811020 * (x * x) + 2.18555832 * x - 0.20219683;
    } else if (T > 2222 && T <= 4000) {
        y = -0.9549476 * (x * x * x) - 1.37418593 * (x * x) + 2.09137015 * x - 0.16748867;
    } else if (T > 4000 && T <= 25000) {
        y = 3.0817580 * (x * x * x) - 5.87338670 * (x * x) + 3.75112997 * x - 0.37001483;
    } else {
        y = 0.3290; // D65 default
    }
    
    // Apply tint adjustment
    y += tint * 0.001f; // Simple tint adjustment
    
    // Convert to XYZ
    float Y = 1.0f; // Normalized luminance
    float X = (Y / y) * x;
    float Z = (Y / y) * (1.0f - x - y);
    
    return {X, Y, Z};
}

ColorTemperature ColorTemperature::fromWhitePoint(const std::array<float, 3>& whitePoint) {
    // Convert XYZ white point to temperature
    // Simplified McCamy's approximation
    float X = whitePoint[0];
    float Y = whitePoint[1];
    float Z = whitePoint[2];
    
    float x = X / (X + Y + Z);
    float y = Y / (X + Y + Z);
    
    float n = (x - 0.3320f) / (0.1858f - y);
    float CCT = 449.0f * n * n * n + 3525.0f * n * n + 6823.3f * n + 5520.33f;
    
    ColorTemperature temp;
    temp.temperature = std::clamp(CCT, 1000.0f, 25000.0f);
    temp.tint = 0.0f; // Would calculate tint from deviation
    
    return temp;
}

// ColorMatrix implementation
ColorMatrix ColorMatrix::operator*(const ColorMatrix& other) const {
    ColorMatrix result;
    
    // 3x3 matrix multiplication
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            result.matrix[row * 3 + col] = 
                matrix[row * 3 + 0] * other.matrix[0 * 3 + col] +
                matrix[row * 3 + 1] * other.matrix[1 * 3 + col] +
                matrix[row * 3 + 2] * other.matrix[2 * 3 + col];
        }
    }
    
    return result;
}

ColorMatrix ColorMatrix::inverse() const {
    // Calculate inverse of 3x3 matrix
    const float* m = matrix.data();
    
    float det = m[0] * (m[4] * m[8] - m[5] * m[7]) -
                m[1] * (m[3] * m[8] - m[5] * m[6]) +
                m[2] * (m[3] * m[7] - m[4] * m[6]);
    
    if (std::abs(det) < 1e-10f) {
        return ColorMatrix{}; // Return identity if not invertible
    }
    
    float invDet = 1.0f / det;
    
    ColorMatrix inv;
    inv.matrix[0] = (m[4] * m[8] - m[5] * m[7]) * invDet;
    inv.matrix[1] = (m[2] * m[7] - m[1] * m[8]) * invDet;
    inv.matrix[2] = (m[1] * m[5] - m[2] * m[4]) * invDet;
    inv.matrix[3] = (m[5] * m[6] - m[3] * m[8]) * invDet;
    inv.matrix[4] = (m[0] * m[8] - m[2] * m[6]) * invDet;
    inv.matrix[5] = (m[2] * m[3] - m[0] * m[5]) * invDet;
    inv.matrix[6] = (m[3] * m[7] - m[4] * m[6]) * invDet;
    inv.matrix[7] = (m[1] * m[6] - m[0] * m[7]) * invDet;
    inv.matrix[8] = (m[0] * m[4] - m[1] * m[3]) * invDet;
    
    return inv;
}

std::array<float, 3> ColorMatrix::transform(const std::array<float, 3>& color) const {
    return {
        matrix[0] * color[0] + matrix[1] * color[1] + matrix[2] * color[2],
        matrix[3] * color[0] + matrix[4] * color[1] + matrix[5] * color[2],
        matrix[6] * color[0] + matrix[7] * color[1] + matrix[8] * color[2]
    };
}

ColorMatrix ColorMatrix::sRGBtoXYZ() {
    // sRGB to XYZ transformation matrix (D65 white point)
    return ColorMatrix({
        0.4124564f, 0.3575761f, 0.1804375f,
        0.2126729f, 0.7151522f, 0.0721750f,
        0.0193339f, 0.1191920f, 0.9503041f
    });
}

ColorMatrix ColorMatrix::XYZtosRGB() {
    return sRGBtoXYZ().inverse();
}

ColorMatrix ColorMatrix::AdobeRGBtoXYZ() {
    // Adobe RGB to XYZ transformation matrix
    return ColorMatrix({
        0.5767309f, 0.1855540f, 0.1881852f,
        0.2973769f, 0.6273491f, 0.0752741f,
        0.0270343f, 0.0706872f, 0.9911085f
    });
}

ColorMatrix ColorMatrix::XYZtoAdobeRGB() {
    return AdobeRGBtoXYZ().inverse();
}

ColorMatrix ColorMatrix::ProPhotoRGBtoXYZ() {
    // ProPhoto RGB to XYZ transformation matrix
    return ColorMatrix({
        0.7976749f, 0.1351917f, 0.0313534f,
        0.2880402f, 0.7118741f, 0.0000857f,
        0.0000000f, 0.0000000f, 0.8252100f
    });
}

ColorMatrix ColorMatrix::XYZtoProPhotoRGB() {
    return ProPhotoRGBtoXYZ().inverse();
}

// ColorProfile implementation
ColorProfile::ColorProfile(ColorSpace colorSpace) {
    initializeStandardProfile(colorSpace);
}

ColorProfile::ColorProfile(const std::filesystem::path& iccProfilePath) {
    if (std::filesystem::exists(iccProfilePath)) {
        std::ifstream file(iccProfilePath, std::ios::binary);
        if (file.is_open()) {
            std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)),
                                     std::istreambuf_iterator<char>());
            valid_ = parseICCProfile(data);
            info_.filePath = iccProfilePath;
        }
    }
    
    if (!valid_) {
        initializeStandardProfile(ColorSpace::sRGB);
    }
}

ColorProfile::ColorProfile(const std::vector<uint8_t>& iccData) {
    valid_ = parseICCProfile(iccData);
    
    if (!valid_) {
        initializeStandardProfile(ColorSpace::sRGB);
    }
}

ColorProfile::~ColorProfile() = default;

std::array<float, 3> ColorProfile::transformToXYZ(const std::array<float, 3>& color) const {
    // Apply gamma removal first
    std::array<float, 3> linear = removeGamma(color);
    
    // Transform to XYZ
    return toXYZMatrix_.transform(linear);
}

std::array<float, 3> ColorProfile::transformFromXYZ(const std::array<float, 3>& color) const {
    // Transform from XYZ
    std::array<float, 3> linear = fromXYZMatrix_.transform(color);
    
    // Apply gamma correction
    return applyGamma(linear);
}

std::array<float, 3> ColorProfile::applyGamma(const std::array<float, 3>& color) const {
    std::array<float, 3> result;
    
    for (int i = 0; i < 3; ++i) {
        if (info_.colorSpace == ColorSpace::sRGB) {
            // sRGB gamma curve
            if (color[i] <= 0.0031308f) {
                result[i] = 12.92f * color[i];
            } else {
                result[i] = 1.055f * std::pow(color[i], 1.0f / 2.4f) - 0.055f;
            }
        } else {
            // Simple power law gamma
            result[i] = std::pow(std::max(0.0f, color[i]), 1.0f / gamma_);
        }
    }
    
    return result;
}

std::array<float, 3> ColorProfile::removeGamma(const std::array<float, 3>& color) const {
    std::array<float, 3> result;
    
    for (int i = 0; i < 3; ++i) {
        if (info_.colorSpace == ColorSpace::sRGB) {
            // sRGB gamma curve (inverse)
            if (color[i] <= 0.04045f) {
                result[i] = color[i] / 12.92f;
            } else {
                result[i] = std::pow((color[i] + 0.055f) / 1.055f, 2.4f);
            }
        } else {
            // Simple power law gamma (inverse)
            result[i] = std::pow(std::max(0.0f, color[i]), gamma_);
        }
    }
    
    return result;
}

std::vector<uint8_t> ColorProfile::serialize() const {
    // Return the raw ICC profile data
    return info_.profileData;
}

bool ColorProfile::deserialize(const std::vector<uint8_t>& data) {
    return parseICCProfile(data);
}

std::shared_ptr<ColorProfile> ColorProfile::sRGB() {
    static std::weak_ptr<ColorProfile> cached;
    auto profile = cached.lock();
    if (!profile) {
        profile = std::make_shared<ColorProfile>(ColorSpace::sRGB);
        cached = profile;
    }
    return profile;
}

std::shared_ptr<ColorProfile> ColorProfile::AdobeRGB() {
    static std::weak_ptr<ColorProfile> cached;
    auto profile = cached.lock();
    if (!profile) {
        profile = std::make_shared<ColorProfile>(ColorSpace::AdobeRGB);
        cached = profile;
    }
    return profile;
}

std::shared_ptr<ColorProfile> ColorProfile::ProPhotoRGB() {
    static std::weak_ptr<ColorProfile> cached;
    auto profile = cached.lock();
    if (!profile) {
        profile = std::make_shared<ColorProfile>(ColorSpace::ProPhotoRGB);
        cached = profile;
    }
    return profile;
}

std::shared_ptr<ColorProfile> ColorProfile::Rec2020() {
    static std::weak_ptr<ColorProfile> cached;
    auto profile = cached.lock();
    if (!profile) {
        profile = std::make_shared<ColorProfile>(ColorSpace::Rec2020);
        cached = profile;
    }
    return profile;
}

std::shared_ptr<ColorProfile> ColorProfile::DCIP3() {
    static std::weak_ptr<ColorProfile> cached;
    auto profile = cached.lock();
    if (!profile) {
        profile = std::make_shared<ColorProfile>(ColorSpace::DCI_P3);
        cached = profile;
    }
    return profile;
}

bool ColorProfile::parseICCProfile(const std::vector<uint8_t>& data) {
    if (data.size() < 128) { // Minimum ICC profile size
        lastError_ = "ICC profile too small";
        return false;
    }
    
    // Basic ICC profile parsing (simplified)
    info_.profileData = data;
    
    // Read profile header
    if (data.size() >= 4) {
        uint32_t profileSize = 
            (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
        
        if (profileSize != data.size()) {
            lastError_ = "Invalid ICC profile size";
            return false;
        }
    }
    
    // Set default values (would parse from ICC profile)
    info_.name = "Custom ICC Profile";
    info_.colorSpace = ColorSpace::sRGB; // Would detect from profile
    info_.gamut = ColorGamut::sRGB;
    info_.whitePoint = WhitePoint::D65;
    info_.whitePointXYZ = {0.95047f, 1.00000f, 1.08883f}; // D65
    
    calculateMatrices();
    valid_ = true;
    
    return true;
}

void ColorProfile::initializeStandardProfile(ColorSpace colorSpace) {
    info_.colorSpace = colorSpace;
    
    switch (colorSpace) {
        case ColorSpace::sRGB:
            info_.name = "sRGB";
            info_.description = "sRGB IEC61966-2.1";
            info_.gamut = ColorGamut::sRGB;
            info_.whitePoint = WhitePoint::D65;
            info_.whitePointXYZ = {0.95047f, 1.00000f, 1.08883f};
            toXYZMatrix_ = ColorMatrix::sRGBtoXYZ();
            gamma_ = 2.2f; // Approximate gamma for sRGB
            break;
            
        case ColorSpace::AdobeRGB:
            info_.name = "Adobe RGB (1998)";
            info_.description = "Adobe RGB (1998) color space";
            info_.gamut = ColorGamut::AdobeRGB;
            info_.whitePoint = WhitePoint::D65;
            info_.whitePointXYZ = {0.95047f, 1.00000f, 1.08883f};
            toXYZMatrix_ = ColorMatrix::AdobeRGBtoXYZ();
            gamma_ = 2.2f;
            break;
            
        case ColorSpace::ProPhotoRGB:
            info_.name = "ProPhoto RGB";
            info_.description = "ProPhoto RGB color space";
            info_.gamut = ColorGamut::ProPhotoRGB;
            info_.whitePoint = WhitePoint::D50;
            info_.whitePointXYZ = {0.96422f, 1.00000f, 0.82521f};
            toXYZMatrix_ = ColorMatrix::ProPhotoRGBtoXYZ();
            gamma_ = 1.8f;
            break;
            
        default:
            // Default to sRGB
            initializeStandardProfile(ColorSpace::sRGB);
            return;
    }
    
    calculateMatrices();
    valid_ = true;
}

void ColorProfile::calculateMatrices() {
    fromXYZMatrix_ = toXYZMatrix_.inverse();
}

// WhiteBalanceAdjuster implementation
void WhiteBalanceAdjuster::setSourceTemperature(float kelvin, float tint) {
    sourceTemp_.temperature = kelvin;
    sourceTemp_.tint = tint;
    matrixDirty_ = true;
}

void WhiteBalanceAdjuster::setTargetTemperature(float kelvin, float tint) {
    targetTemp_.temperature = kelvin;
    targetTemp_.tint = tint;
    matrixDirty_ = true;
}

void WhiteBalanceAdjuster::setTemperatureShift(float deltaKelvin, float deltaTint) {
    targetTemp_.temperature = sourceTemp_.temperature + deltaKelvin;
    targetTemp_.tint = sourceTemp_.tint + deltaTint;
    matrixDirty_ = true;
}

ColorTemperature WhiteBalanceAdjuster::detectWhiteBalance(const Image& image, 
                                                         const std::array<uint32_t, 4>& sampleRegion) {
    // Simplified auto white balance using gray world assumption
    // In practice, would use more sophisticated algorithms
    
    std::array<float, 3> avgColor = {0.0f, 0.0f, 0.0f};
    uint32_t sampleCount = 0;
    
    // Would sample pixels and calculate average
    // For now, return a default temperature
    ColorTemperature detected;
    detected.temperature = 6500.0f; // D65
    detected.tint = 0.0f;
    
    return detected;
}

void WhiteBalanceAdjuster::adjustImage(Image& image) {
    if (matrixDirty_) {
        updateAdaptationMatrix();
        matrixDirty_ = false;
    }
    
    // Apply white balance adjustment to entire image
    // Would implement actual image processing
}

void WhiteBalanceAdjuster::adjustColor(std::array<float, 3>& rgb) {
    if (matrixDirty_) {
        updateAdaptationMatrix();
        matrixDirty_ = false;
    }
    
    rgb = adaptationMatrix_.transform(rgb);
}

ColorMatrix WhiteBalanceAdjuster::calculateBradfordMatrix(const std::array<float, 3>& sourceWhite,
                                                         const std::array<float, 3>& targetWhite) {
    // Bradford chromatic adaptation transform
    ColorMatrix bradford({
        0.8951000f, 0.2664000f, -0.1614000f,
        -0.7502000f, 1.7135000f, 0.0367000f,
        0.0389000f, -0.0685000f, 1.0296000f
    });
    
    ColorMatrix bradfordInv = bradford.inverse();
    
    auto sourceRGB = bradford.transform(sourceWhite);
    auto targetRGB = bradford.transform(targetWhite);
    
    ColorMatrix adaptation({
        targetRGB[0] / sourceRGB[0], 0, 0,
        0, targetRGB[1] / sourceRGB[1], 0,
        0, 0, targetRGB[2] / sourceRGB[2]
    });
    
    return bradfordInv * adaptation * bradford;
}

void WhiteBalanceAdjuster::updateAdaptationMatrix() {
    auto sourceWhite = sourceTemp_.toWhitePoint();
    auto targetWhite = targetTemp_.toWhitePoint();
    
    adaptationMatrix_ = calculateBradfordMatrix(sourceWhite, targetWhite);
}

std::array<float, 3> WhiteBalanceAdjuster::kelvinToXYZ(float kelvin, float tint) const {
    ColorTemperature temp{kelvin, tint};
    return temp.toWhitePoint();
}

// GamutMapper implementation
GamutMapper::GamutMapper(GamutMappingMethod method) : method_(method) {
}

std::array<float, 3> GamutMapper::mapColor(const std::array<float, 3>& color, 
                                          const ColorProfile& sourceProfile,
                                          const ColorProfile& targetProfile) {
    // Convert to common color space (LAB) for gamut mapping
    auto xyz = sourceProfile.transformToXYZ(color);
    auto lab = ColorManager::XYZtoLAB(xyz, sourceProfile.getInfo().whitePointXYZ);
    
    // Apply gamut mapping in LAB space
    std::array<float, 3> mappedLab;
    
    switch (method_) {
        case GamutMappingMethod::Clip:
            mappedLab = clipMapping(lab, targetProfile);
            break;
        case GamutMappingMethod::Scale:
            mappedLab = scaleMapping(lab, targetProfile);
            break;
        case GamutMappingMethod::Compress:
            mappedLab = compressMapping(lab, targetProfile);
            break;
        case GamutMappingMethod::PerceptualSmooth:
            mappedLab = perceptualMapping(lab, sourceProfile, targetProfile);
            break;
        default:
            mappedLab = lab; // No mapping
            break;
    }
    
    // Convert back to target color space
    auto mappedXYZ = ColorManager::LABtoXYZ(mappedLab, targetProfile.getInfo().whitePointXYZ);
    return targetProfile.transformFromXYZ(mappedXYZ);
}

void GamutMapper::mapImage(Image& image, const ColorProfile& sourceProfile,
                          const ColorProfile& targetProfile) {
    // Would process entire image
    // For now, placeholder implementation
}

bool GamutMapper::isInGamut(const std::array<float, 3>& color, const ColorProfile& profile) {
    // Simple gamut check - would implement proper gamut boundary testing
    for (int i = 0; i < 3; ++i) {
        if (color[i] < 0.0f || color[i] > 1.0f) {
            return false;
        }
    }
    return true;
}

float GamutMapper::calculateGamutDistance(const std::array<float, 3>& color, const ColorProfile& profile) {
    // Calculate distance from gamut boundary
    float maxDistance = 0.0f;
    
    for (int i = 0; i < 3; ++i) {
        if (color[i] < 0.0f) {
            maxDistance = std::max(maxDistance, -color[i]);
        } else if (color[i] > 1.0f) {
            maxDistance = std::max(maxDistance, color[i] - 1.0f);
        }
    }
    
    return maxDistance;
}

std::array<float, 3> GamutMapper::clipMapping(const std::array<float, 3>& color, const ColorProfile& profile) {
    std::array<float, 3> result;
    for (int i = 0; i < 3; ++i) {
        result[i] = std::clamp(color[i], 0.0f, 1.0f);
    }
    return result;
}

std::array<float, 3> GamutMapper::scaleMapping(const std::array<float, 3>& color, const ColorProfile& profile) {
    float maxComponent = std::max({std::abs(color[0]), std::abs(color[1]), std::abs(color[2])});
    
    if (maxComponent > 1.0f) {
        return {color[0] / maxComponent, color[1] / maxComponent, color[2] / maxComponent};
    }
    
    return color;
}

std::array<float, 3> GamutMapper::compressMapping(const std::array<float, 3>& color, const ColorProfile& profile) {
    std::array<float, 3> result;
    
    for (int i = 0; i < 3; ++i) {
        if (color[i] > 1.0f) {
            float excess = color[i] - 1.0f;
            result[i] = 1.0f - std::exp(-excess * compressionAmount_);
        } else if (color[i] < 0.0f) {
            float excess = -color[i];
            result[i] = -1.0f + std::exp(-excess * compressionAmount_);
        } else {
            result[i] = color[i];
        }
    }
    
    return result;
}

std::array<float, 3> GamutMapper::perceptualMapping(const std::array<float, 3>& color, 
                                                   const ColorProfile& sourceProfile,
                                                   const ColorProfile& targetProfile) {
    // Implement perceptual gamut mapping
    // For now, use compression mapping as fallback
    return compressMapping(color, targetProfile);
}

// ColorManager implementation
ColorManager::ColorManager(Rendering::RenderingEngine& engine) : engine_(engine) {
    // Set default profiles
    workingColorSpace_ = ColorProfile::sRGB();
    displayProfile_ = ColorProfile::sRGB();
}

ColorManager::~ColorManager() {
    shutdown();
}

ColorManager::ColorManager(ColorManager&& other) noexcept
    : engine_(other.engine_)
    , initialized_(other.initialized_.load())
    , profiles_(std::move(other.profiles_))
    , workingColorSpace_(std::move(other.workingColorSpace_))
    , displayProfile_(std::move(other.displayProfile_))
    , whiteBalanceAdjuster_(std::move(other.whiteBalanceAdjuster_))
    , gamutMapper_(std::move(other.gamutMapper_))
    , softProofingEngine_(std::move(other.softProofingEngine_))
    , defaultRenderingIntent_(other.defaultRenderingIntent_)
    , colorManagementEnabled_(other.colorManagementEnabled_)
    , useGPUAcceleration_(other.useGPUAcceleration_)
    , stats_(other.stats_) {
}

ColorManager& ColorManager::operator=(ColorManager&& other) noexcept {
    if (this != &other) {
        engine_ = other.engine_;
        initialized_ = other.initialized_.load();
        profiles_ = std::move(other.profiles_);
        workingColorSpace_ = std::move(other.workingColorSpace_);
        displayProfile_ = std::move(other.displayProfile_);
        whiteBalanceAdjuster_ = std::move(other.whiteBalanceAdjuster_);
        gamutMapper_ = std::move(other.gamutMapper_);
        softProofingEngine_ = std::move(other.softProofingEngine_);
        defaultRenderingIntent_ = other.defaultRenderingIntent_;
        colorManagementEnabled_ = other.colorManagementEnabled_;
        useGPUAcceleration_ = other.useGPUAcceleration_;
        stats_ = other.stats_;
    }
    return *this;
}

bool ColorManager::initialize() {
    if (initialized_) {
        return true;
    }
    
    if (!createGPUResources()) {
        std::cerr << "[ColorManager] Failed to create GPU resources" << std::endl;
        return false;
    }
    
    loadStandardProfiles();
    loadSystemProfiles();
    
    initialized_ = true;
    std::cout << "[ColorManager] Initialization complete" << std::endl;
    return true;
}

void ColorManager::shutdown() {
    if (!initialized_) {
        return;
    }
    
    cleanupLUTCache();
    destroyGPUResources();
    
    std::lock_guard<std::mutex> lock(profilesMutex_);
    profiles_.clear();
    
    initialized_ = false;
    std::cout << "[ColorManager] Shutdown complete" << std::endl;
}

bool ColorManager::loadProfile(const std::filesystem::path& profilePath) {
    auto profile = std::make_shared<ColorProfile>(profilePath);
    if (profile->isValid()) {
        std::lock_guard<std::mutex> lock(profilesMutex_);
        profiles_[profile->getInfo().name] = profile;
        
        std::lock_guard<std::mutex> statsLock(statsMutex_);
        stats_.profilesLoaded++;
        
        std::cout << "[ColorManager] Loaded profile: " << profile->getInfo().name << std::endl;
        return true;
    }
    
    std::cerr << "[ColorManager] Failed to load profile: " << profilePath << std::endl;
    return false;
}

bool ColorManager::loadProfile(const std::vector<uint8_t>& profileData, const std::string& name) {
    auto profile = std::make_shared<ColorProfile>(profileData);
    if (profile->isValid()) {
        std::lock_guard<std::mutex> lock(profilesMutex_);
        profiles_[name] = profile;
        
        std::lock_guard<std::mutex> statsLock(statsMutex_);
        stats_.profilesLoaded++;
        
        std::cout << "[ColorManager] Loaded profile: " << name << std::endl;
        return true;
    }
    
    return false;
}

void ColorManager::unloadProfile(const std::string& name) {
    std::lock_guard<std::mutex> lock(profilesMutex_);
    profiles_.erase(name);
}

std::shared_ptr<ColorProfile> ColorManager::getProfile(const std::string& name) {
    std::lock_guard<std::mutex> lock(profilesMutex_);
    auto it = profiles_.find(name);
    if (it != profiles_.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::string> ColorManager::getAvailableProfiles() const {
    std::lock_guard<std::mutex> lock(profilesMutex_);
    std::vector<std::string> names;
    names.reserve(profiles_.size());
    
    for (const auto& [name, profile] : profiles_) {
        names.push_back(name);
    }
    
    return names;
}

void ColorManager::setWorkingColorSpace(std::shared_ptr<ColorProfile> profile) {
    if (profile && profile->isValid()) {
        workingColorSpace_ = profile;
        std::cout << "[ColorManager] Working color space set to: " << profile->getInfo().name << std::endl;
    }
}

void ColorManager::setDisplayProfile(std::shared_ptr<ColorProfile> profile) {
    if (profile && profile->isValid()) {
        displayProfile_ = profile;
        std::cout << "[ColorManager] Display profile set to: " << profile->getInfo().name << std::endl;
    }
}

void ColorManager::autoDetectDisplayProfile() {
    // Would implement system-specific display profile detection
    std::cout << "[ColorManager] Auto-detecting display profile..." << std::endl;
    
    // For now, use sRGB as default
    setDisplayProfile(ColorProfile::sRGB());
}

std::array<float, 3> ColorManager::convertColor(const std::array<float, 3>& color,
                                               const ColorProfile& sourceProfile,
                                               const ColorProfile& targetProfile,
                                               RenderingIntent intent) {
    if (!colorManagementEnabled_) {
        return color;
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Convert source -> XYZ -> target
    auto xyz = sourceProfile.transformToXYZ(color);
    auto result = targetProfile.transformFromXYZ(xyz);
    
    // Apply gamut mapping if needed
    if (!gamutMapper_.isInGamut(result, targetProfile)) {
        result = gamutMapper_.mapColor(color, sourceProfile, targetProfile);
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    std::lock_guard<std::mutex> lock(statsMutex_);
    stats_.conversionsPerformed++;
    stats_.conversionTime += duration;
    
    return result;
}

void ColorManager::convertImage(Image& image, const ColorProfile& sourceProfile,
                               const ColorProfile& targetProfile, RenderingIntent intent) {
    // Would implement full image conversion
    std::lock_guard<std::mutex> lock(statsMutex_);
    stats_.conversionsPerformed++;
    // stats_.pixelsConverted += image.width * image.height;
}

bool ColorManager::convertImageGPU(Rendering::ResourceId sourceTexture,
                                  Rendering::ResourceId targetTexture,
                                  const std::array<uint32_t, 2>& size,
                                  const ColorProfile& sourceProfile,
                                  const ColorProfile& targetProfile,
                                  RenderingIntent intent) {
    if (!useGPUAcceleration_) {
        return false;
    }
    
    // Would implement GPU color conversion using compute shaders
    return false; // Not implemented yet
}

// Static utility methods
std::array<float, 3> ColorManager::RGBtoXYZ(const std::array<float, 3>& rgb, const ColorMatrix& matrix) {
    return matrix.transform(rgb);
}

std::array<float, 3> ColorManager::XYZtoRGB(const std::array<float, 3>& xyz, const ColorMatrix& matrix) {
    return matrix.transform(xyz);
}

std::array<float, 3> ColorManager::XYZtoLAB(const std::array<float, 3>& xyz, const std::array<float, 3>& whitePoint) {
    float xn = xyz[0] / whitePoint[0];
    float yn = xyz[1] / whitePoint[1];
    float zn = xyz[2] / whitePoint[2];
    
    auto f = [](float t) {
        const float delta = 6.0f / 29.0f;
        if (t > delta * delta * delta) {
            return std::pow(t, 1.0f / 3.0f);
        } else {
            return (t / (3 * delta * delta)) + (4.0f / 29.0f);
        }
    };
    
    float fx = f(xn);
    float fy = f(yn);
    float fz = f(zn);
    
    float L = 116.0f * fy - 16.0f;
    float a = 500.0f * (fx - fy);
    float b = 200.0f * (fy - fz);
    
    return {L, a, b};
}

std::array<float, 3> ColorManager::LABtoXYZ(const std::array<float, 3>& lab, const std::array<float, 3>& whitePoint) {
    float L = lab[0];
    float a = lab[1];
    float b = lab[2];
    
    float fy = (L + 16.0f) / 116.0f;
    float fx = a / 500.0f + fy;
    float fz = fy - b / 200.0f;
    
    auto f_inv = [](float t) {
        const float delta = 6.0f / 29.0f;
        if (t > delta) {
            return t * t * t;
        } else {
            return 3 * delta * delta * (t - 4.0f / 29.0f);
        }
    };
    
    float xn = f_inv(fx);
    float yn = f_inv(fy);
    float zn = f_inv(fz);
    
    return {
        xn * whitePoint[0],
        yn * whitePoint[1],
        zn * whitePoint[2]
    };
}

std::array<float, 3> ColorManager::RGBtoHSV(const std::array<float, 3>& rgb) {
    float max_val = std::max({rgb[0], rgb[1], rgb[2]});
    float min_val = std::min({rgb[0], rgb[1], rgb[2]});
    float delta = max_val - min_val;
    
    float h = 0, s = 0, v = max_val;
    
    if (delta > 0) {
        s = delta / max_val;
        
        if (max_val == rgb[0]) {
            h = 60 * (std::fmod((rgb[1] - rgb[2]) / delta, 6.0f));
        } else if (max_val == rgb[1]) {
            h = 60 * ((rgb[2] - rgb[0]) / delta + 2);
        } else {
            h = 60 * ((rgb[0] - rgb[1]) / delta + 4);
        }
        
        if (h < 0) h += 360;
    }
    
    return {h, s, v};
}

std::array<float, 3> ColorManager::HSVtoRGB(const std::array<float, 3>& hsv) {
    float h = hsv[0], s = hsv[1], v = hsv[2];
    
    float c = v * s;
    float x = c * (1.0f - std::abs(std::fmod(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;
    
    std::array<float, 3> rgb;
    if (h >= 0 && h < 60) {
        rgb = {c, x, 0};
    } else if (h >= 60 && h < 120) {
        rgb = {x, c, 0};
    } else if (h >= 120 && h < 180) {
        rgb = {0, c, x};
    } else if (h >= 180 && h < 240) {
        rgb = {0, x, c};
    } else if (h >= 240 && h < 300) {
        rgb = {x, 0, c};
    } else {
        rgb = {c, 0, x};
    }
    
    return {rgb[0] + m, rgb[1] + m, rgb[2] + m};
}

float ColorManager::calculateDeltaE76(const std::array<float, 3>& lab1, const std::array<float, 3>& lab2) {
    float dL = lab1[0] - lab2[0];
    float da = lab1[1] - lab2[1];
    float db = lab1[2] - lab2[2];
    
    return std::sqrt(dL * dL + da * da + db * db);
}

float ColorManager::calculateDeltaE94(const std::array<float, 3>& lab1, const std::array<float, 3>& lab2) {
    // CIE94 Delta E calculation (simplified)
    return calculateDeltaE76(lab1, lab2); // For now, use CIE76
}

float ColorManager::calculateDeltaE2000(const std::array<float, 3>& lab1, const std::array<float, 3>& lab2) {
    // CIEDE2000 Delta E calculation (simplified)
    return calculateDeltaE76(lab1, lab2); // For now, use CIE76
}

ColorManager::ColorManagerStats ColorManager::getStats() const {
    std::lock_guard<std::mutex> lock(statsMutex_);
    return stats_;
}

void ColorManager::resetStats() {
    std::lock_guard<std::mutex> lock(statsMutex_);
    stats_ = ColorManagerStats{};
}

bool ColorManager::createGPUResources() {
    // Create uniform buffer for color conversion parameters
    conversionUniformBuffer_ = engine_.create_buffer(
        1024, // 1KB for conversion parameters
        Rendering::BufferUsage::Uniform | Rendering::BufferUsage::Dynamic
    );
    
    return conversionUniformBuffer_ != 0;
}

void ColorManager::destroyGPUResources() {
    if (conversionUniformBuffer_ != 0) {
        engine_.destroy_buffer(conversionUniformBuffer_);
        conversionUniformBuffer_ = 0;
    }
    
    if (lutTexture_ != 0) {
        engine_.destroy_texture(lutTexture_);
        lutTexture_ = 0;
    }
}

void ColorManager::loadStandardProfiles() {
    std::lock_guard<std::mutex> lock(profilesMutex_);
    
    profiles_["sRGB"] = ColorProfile::sRGB();
    profiles_["Adobe RGB"] = ColorProfile::AdobeRGB();
    profiles_["ProPhoto RGB"] = ColorProfile::ProPhotoRGB();
    profiles_["Rec.2020"] = ColorProfile::Rec2020();
    profiles_["DCI-P3"] = ColorProfile::DCIP3();
    
    std::cout << "[ColorManager] Standard profiles loaded" << std::endl;
}

void ColorManager::loadSystemProfiles() {
    // Would load system-specific color profiles
    #ifdef _WIN32
        // loadWindowsProfiles();
    #elif defined(__APPLE__)
        // loadMacOSProfiles();
    #else
        // loadLinuxProfiles();
    #endif
    
    std::cout << "[ColorManager] System profiles loaded" << std::endl;
}

void ColorManager::cleanupLUTCache() {
    std::lock_guard<std::mutex> lock(lutCacheMutex_);
    
    for (const auto& [hash, entry] : lutCache_) {
        if (entry.lutTextureId != 0) {
            engine_.destroy_texture(entry.lutTextureId);
        }
    }
    
    lutCache_.clear();
}

} // namespace QuantumCanvas::Raster
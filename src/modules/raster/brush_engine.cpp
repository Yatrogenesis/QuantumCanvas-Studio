#include "brush_engine.hpp"
#include "../../core/rendering/rendering_engine.hpp"
#include "../../core/memory/memory_manager.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <random>
#include <cassert>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace QuantumCanvas::Raster {

// Forward declaration for Image class (would be implemented separately)
class Image {
public:
    uint32_t width = 0;
    uint32_t height = 0;
    std::vector<uint8_t> data;
    
    Image(uint32_t w, uint32_t h) : width(w), height(h) {
        data.resize(w * h * 4); // RGBA
    }
    
    void setPixel(uint32_t x, uint32_t y, const std::array<float, 4>& color) {
        if (x >= width || y >= height) return;
        size_t idx = (y * width + x) * 4;
        data[idx + 0] = static_cast<uint8_t>(color[0] * 255);
        data[idx + 1] = static_cast<uint8_t>(color[1] * 255);
        data[idx + 2] = static_cast<uint8_t>(color[2] * 255);
        data[idx + 3] = static_cast<uint8_t>(color[3] * 255);
    }
    
    std::array<float, 4> getPixel(uint32_t x, uint32_t y) const {
        if (x >= width || y >= height) return {0.0f, 0.0f, 0.0f, 0.0f};
        size_t idx = (y * width + x) * 4;
        return {
            data[idx + 0] / 255.0f,
            data[idx + 1] / 255.0f,
            data[idx + 2] / 255.0f,
            data[idx + 3] / 255.0f
        };
    }
};

// FluidSimulator implementation for realistic paint behavior
FluidSimulator::FluidSimulator(Rendering::RenderingEngine& engine)
    : engine_(engine) {
}

FluidSimulator::~FluidSimulator() {
    shutdown();
}

bool FluidSimulator::initialize(const std::array<uint32_t, 2>& gridSize) {
    if (initialized_) {
        return true;
    }
    
    gridSize_ = gridSize;
    
    // Create GPU textures for fluid simulation
    velocityTexture_ = engine_.create_texture_2d(
        gridSize[0], gridSize[1], Rendering::TextureFormat::RG32Float,
        Rendering::TextureUsage::Storage | Rendering::TextureUsage::Sampled
    );
    
    paintTexture_ = engine_.create_texture_2d(
        gridSize[0], gridSize[1], Rendering::TextureFormat::RGBA32Float,
        Rendering::TextureUsage::Storage | Rendering::TextureUsage::Sampled
    );
    
    pressureTexture_ = engine_.create_texture_2d(
        gridSize[0], gridSize[1], Rendering::TextureFormat::R32Float,
        Rendering::TextureUsage::Storage | Rendering::TextureUsage::Sampled
    );
    
    if (!createSimulationShaders()) {
        std::cerr << "[FluidSimulator] Failed to create simulation shaders" << std::endl;
        return false;
    }
    
    initialized_ = true;
    std::cout << "[FluidSimulator] Initialized with grid size " << gridSize[0] << "x" << gridSize[1] << std::endl;
    return true;
}

void FluidSimulator::shutdown() {
    if (!initialized_) {
        return;
    }
    
    destroyResources();
    initialized_ = false;
    std::cout << "[FluidSimulator] Shutdown complete" << std::endl;
}

void FluidSimulator::step(float deltaTime) {
    if (!initialized_) {
        return;
    }
    
    // Simplified fluid simulation steps:
    // 1. Advection - move paint with velocity field
    // 2. Diffusion - spread velocity and paint
    // 3. Pressure projection - ensure incompressibility
    
    // For now, just apply simple diffusion
    applyDiffusion(deltaTime);
}

void FluidSimulator::addPaint(const std::array<float, 2>& position, 
                             const std::array<float, 4>& color,
                             float amount, float viscosity) {
    if (!initialized_) {
        return;
    }
    
    // Convert world position to grid coordinates
    uint32_t gridX = static_cast<uint32_t>(position[0] * gridSize_[0]);
    uint32_t gridY = static_cast<uint32_t>(position[1] * gridSize_[1]);
    
    if (gridX >= gridSize_[0] || gridY >= gridSize_[1]) {
        return;
    }
    
    // Add paint to the simulation (would update GPU texture)
    // This is a simplified CPU implementation
}

bool FluidSimulator::createSimulationShaders() {
    // Create compute shaders for fluid simulation
    // This would require actual compute shader code
    
    // For now, return true as placeholder
    return true;
}

void FluidSimulator::destroyResources() {
    if (velocityTexture_ != 0) {
        engine_.destroy_texture(velocityTexture_);
        velocityTexture_ = 0;
    }
    
    if (paintTexture_ != 0) {
        engine_.destroy_texture(paintTexture_);
        paintTexture_ = 0;
    }
    
    if (pressureTexture_ != 0) {
        engine_.destroy_texture(pressureTexture_);
        pressureTexture_ = 0;
    }
}

void FluidSimulator::applyDiffusion(float deltaTime) {
    // Apply diffusion using compute shader
    // This would dispatch the diffusion compute shader
}

// BrushStroke implementation
BrushStroke::BrushStroke(const BrushSettings& settings) : settings_(settings) {
}

void BrushStroke::AddDab(const BrushDab& dab) {
    if (finalized_) {
        return;
    }
    
    dabs_.push_back(dab);
    
    // Apply spacing check
    if (dabs_.size() > 1) {
        const auto& prev = dabs_[dabs_.size() - 2];
        float spacing = CalculateSpacing(prev, dab);
        
        if (spacing < settings_.spacing * dab.size) {
            // Too close to previous dab, might want to skip or interpolate
        }
    }
}

void BrushStroke::Finalize() {
    if (finalized_) {
        return;
    }
    
    SmoothStroke();
    finalized_ = true;
}

void BrushStroke::SmoothStroke() {
    if (dabs_.size() < 3) {
        return;
    }
    
    // Apply simple smoothing filter
    std::vector<BrushDab> smoothed = dabs_;
    
    for (size_t i = 1; i < smoothed.size() - 1; ++i) {
        const auto& prev = dabs_[i - 1];
        const auto& current = dabs_[i];
        const auto& next = dabs_[i + 1];
        
        // Average position
        smoothed[i].position.x = (prev.position.x + current.position.x + next.position.x) / 3.0f;
        smoothed[i].position.y = (prev.position.y + current.position.y + next.position.y) / 3.0f;
        
        // Average pressure
        smoothed[i].pressure = (prev.pressure + current.pressure + next.pressure) / 3.0f;
    }
    
    dabs_ = smoothed;
}

float BrushStroke::CalculateSpacing(const BrushDab& prev, const BrushDab& current) {
    float dx = current.position.x - prev.position.x;
    float dy = current.position.y - prev.position.y;
    return std::sqrt(dx * dx + dy * dy);
}

// BrushEngine implementation
BrushEngine::BrushEngine() {
    // Set default brush settings
    current_settings_.size = 20.0f;
    current_settings_.opacity = 1.0f;
    current_settings_.flow = 1.0f;
    current_settings_.hardness = 0.8f;
    current_settings_.spacing = 0.25f;
    current_settings_.blend_mode = BlendMode::NORMAL;
    
    std::cout << "[BrushEngine] Initialized" << std::endl;
}

BrushEngine::~BrushEngine() {
    Shutdown();
}

bool BrushEngine::Initialize() {
    if (!InitializeShaders()) {
        std::cerr << "[BrushEngine] Failed to initialize shaders" << std::endl;
        return false;
    }
    
    if (!InitializeTextures()) {
        std::cerr << "[BrushEngine] Failed to initialize textures" << std::endl;
        return false;
    }
    
    if (!LoadDefaultPresets()) {
        std::cerr << "[BrushEngine] Failed to load default presets" << std::endl;
        return false;
    }
    
    // Initialize bristle data
    bristle_data_ = std::make_unique<BristleData>();
    
    std::cout << "[BrushEngine] Initialization complete" << std::endl;
    return true;
}

void BrushEngine::Shutdown() {
    brush_presets_.clear();
    bristle_data_.reset();
    
    std::cout << "[BrushEngine] Shutdown complete" << std::endl;
}

void BrushEngine::SetBrushSettings(const BrushSettings& settings) {
    current_settings_ = settings;
    
    // Update bristle simulation if needed
    if (settings.bristle_count > 0) {
        InitializeBristles(settings.bristle_count, settings.bristle_stiffness, settings.bristle_length);
    }
}

std::unique_ptr<BrushStroke> BrushEngine::BeginStroke(const BrushDab& initial_dab) {
    auto stroke = std::make_unique<BrushStroke>(current_settings_);
    stroke->AddDab(initial_dab);
    return stroke;
}

void BrushEngine::ContinueStroke(BrushStroke* stroke, const BrushDab& dab) {
    if (stroke && !stroke->IsFinalized()) {
        stroke->AddDab(dab);
    }
}

void BrushEngine::EndStroke(BrushStroke* stroke) {
    if (stroke && !stroke->IsFinalized()) {
        stroke->Finalize();
    }
}

void BrushEngine::RenderDab(const BrushDab& dab, const BrushSettings& settings) {
    // Apply brush dynamics
    BrushDab processed_dab = dab;
    processed_dab.size = CalculatePressureResponse(dab.pressure, settings.pressure_size, settings.size);
    processed_dab.opacity = CalculatePressureResponse(dab.pressure, settings.pressure_opacity, settings.opacity);
    processed_dab.flow = CalculatePressureResponse(dab.pressure, settings.pressure_flow, settings.flow);
    
    // Apply tilt offset
    if (settings.tilt_angle) {
        Core::Math::Vector2 offset = ApplyTiltOffset(
            processed_dab.position, 
            processed_dab.tilt_x, 
            processed_dab.tilt_y, 
            processed_dab.size
        );
        processed_dab.position = offset;
    }
    
    // Render based on brush type
    switch (settings.type) {
        case BrushType::ROUND:
            RenderRoundBrush(processed_dab, settings);
            break;
        case BrushType::TEXTURED:
            RenderTexturedBrush(processed_dab, settings);
            break;
        case BrushType::BRISTLE:
            RenderBristleBrush(processed_dab, settings);
            break;
        default:
            RenderRoundBrush(processed_dab, settings);
            break;
    }
}

void BrushEngine::SaveBrushPreset(const std::string& name, const BrushSettings& settings) {
    brush_presets_[name] = settings;
    std::cout << "[BrushEngine] Saved brush preset: " << name << std::endl;
}

bool BrushEngine::LoadBrushPreset(const std::string& name, BrushSettings& settings) {
    auto it = brush_presets_.find(name);
    if (it != brush_presets_.end()) {
        settings = it->second;
        return true;
    }
    return false;
}

std::vector<std::string> BrushEngine::GetBrushPresets() const {
    std::vector<std::string> names;
    names.reserve(brush_presets_.size());
    
    for (const auto& [name, settings] : brush_presets_) {
        names.push_back(name);
    }
    
    return names;
}

// Private implementation methods

bool BrushEngine::InitializeShaders() {
    // Create GPU shaders for brush rendering
    // This would load actual shader code
    
    std::cout << "[BrushEngine] Shaders initialized" << std::endl;
    return true;
}

bool BrushEngine::InitializeTextures() {
    // Create default brush texture (circular gradient)
    const uint32_t texture_size = 256;
    std::vector<uint8_t> texture_data(texture_size * texture_size);
    
    float center = texture_size * 0.5f;
    float max_distance = center;
    
    for (uint32_t y = 0; y < texture_size; ++y) {
        for (uint32_t x = 0; x < texture_size; ++x) {
            float dx = x - center;
            float dy = y - center;
            float distance = std::sqrt(dx * dx + dy * dy);
            float alpha = std::max(0.0f, 1.0f - distance / max_distance);
            
            // Apply smooth falloff
            alpha = BrushUtils::SmoothStep(0.0f, 1.0f, alpha);
            
            texture_data[y * texture_size + x] = static_cast<uint8_t>(alpha * 255);
        }
    }
    
    // default_brush_texture_ would be created here
    
    std::cout << "[BrushEngine] Default textures created" << std::endl;
    return true;
}

bool BrushEngine::LoadDefaultPresets() {
    // Create some default brush presets
    
    // Round brush
    BrushSettings round_brush;
    round_brush.type = BrushType::ROUND;
    round_brush.size = 20.0f;
    round_brush.hardness = 0.8f;
    round_brush.opacity = 1.0f;
    round_brush.flow = 1.0f;
    round_brush.pressure_size = true;
    round_brush.pressure_opacity = true;
    SaveBrushPreset("Round", round_brush);
    
    // Soft brush
    BrushSettings soft_brush;
    soft_brush.type = BrushType::ROUND;
    soft_brush.size = 30.0f;
    soft_brush.hardness = 0.3f;
    soft_brush.opacity = 0.7f;
    soft_brush.flow = 0.8f;
    soft_brush.pressure_size = true;
    soft_brush.pressure_opacity = true;
    SaveBrushPreset("Soft", soft_brush);
    
    // Textured brush
    BrushSettings textured_brush;
    textured_brush.type = BrushType::TEXTURED;
    textured_brush.size = 25.0f;
    textured_brush.hardness = 0.6f;
    textured_brush.opacity = 0.9f;
    textured_brush.flow = 0.9f;
    textured_brush.texture_scale = 1.0f;
    SaveBrushPreset("Textured", textured_brush);
    
    // Bristle brush
    BrushSettings bristle_brush;
    bristle_brush.type = BrushType::BRISTLE;
    bristle_brush.size = 15.0f;
    bristle_brush.hardness = 0.9f;
    bristle_brush.opacity = 0.8f;
    bristle_brush.flow = 0.7f;
    bristle_brush.bristle_count = 32;
    bristle_brush.bristle_stiffness = 0.8f;
    bristle_brush.bristle_length = 1.2f;
    SaveBrushPreset("Bristle", bristle_brush);
    
    std::cout << "[BrushEngine] Default presets loaded" << std::endl;
    return true;
}

void BrushEngine::RenderRoundBrush(const BrushDab& dab, const BrushSettings& settings) {
    // Render circular brush dab
    // This would use GPU rendering for performance
    
    float radius = dab.size * 0.5f;
    std::array<float, 4> brush_color = {
        settings.color_jitter > 0 ? settings.color_jitter : 0.0f,
        settings.hue_jitter > 0 ? settings.hue_jitter : 0.0f,
        settings.saturation_jitter > 0 ? settings.saturation_jitter : 0.0f,
        dab.opacity
    };
    
    // Apply color dynamics based on pressure
    if (settings.pressure_opacity) {
        brush_color[3] *= CalculatePressureResponse(dab.pressure, true, 1.0f);
    }
}

void BrushEngine::RenderTexturedBrush(const BrushDab& dab, const BrushSettings& settings) {
    // Render brush with texture
    RenderRoundBrush(dab, settings);
    
    // Apply texture modulation
    if (settings.texture && settings.texture_scale > 0) {
        // Sample texture at brush position with scaling
    }
}

void BrushEngine::RenderBristleBrush(const BrushDab& dab, const BrushSettings& settings) {
    if (!bristle_data_ || settings.bristle_count == 0) {
        RenderRoundBrush(dab, settings);
        return;
    }
    
    // Update bristle simulation
    UpdateBristleSimulation(dab, settings);
    
    // Render individual bristles
    for (size_t i = 0; i < bristle_data_->positions.size(); ++i) {
        BrushDab bristle_dab = dab;
        bristle_dab.position.x = bristle_data_->positions[i].x;
        bristle_dab.position.y = bristle_data_->positions[i].y;
        bristle_dab.size *= 0.3f; // Smaller size for individual bristles
        bristle_dab.opacity *= bristle_data_->pressures[i];
        
        RenderRoundBrush(bristle_dab, settings);
    }
}

void BrushEngine::UpdateBristleSimulation(const BrushDab& dab, const BrushSettings& settings) {
    if (!bristle_data_ || bristle_data_->positions.empty()) {
        return;
    }
    
    // Simple bristle physics simulation
    for (size_t i = 0; i < bristle_data_->positions.size(); ++i) {
        auto& pos = bristle_data_->positions[i];
        auto& vel = bristle_data_->velocities[i];
        auto& pressure = bristle_data_->pressures[i];
        
        // Apply forces toward brush center with stiffness
        float dx = dab.position.x - pos.x;
        float dy = dab.position.y - pos.y;
        
        float spring_force = settings.bristle_stiffness * 0.1f;
        vel.x += dx * spring_force;
        vel.y += dy * spring_force;
        
        // Apply damping
        vel.x *= 0.9f;
        vel.y *= 0.9f;
        
        // Update position
        pos.x += vel.x;
        pos.y += vel.y;
        
        // Update pressure based on distance from center
        float distance = std::sqrt(dx * dx + dy * dy);
        float max_distance = dab.size * 0.5f;
        pressure = std::max(0.1f, 1.0f - distance / max_distance) * dab.pressure;
    }
}

void BrushEngine::InitializeBristles(uint32_t count, float stiffness, float length) {
    if (!bristle_data_) {
        return;
    }
    
    bristle_data_->positions.clear();
    bristle_data_->velocities.clear();
    bristle_data_->pressures.clear();
    
    bristle_data_->positions.reserve(count);
    bristle_data_->velocities.reserve(count);
    bristle_data_->pressures.reserve(count);
    
    // Arrange bristles in a circle
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> angle_dist(0.0f, 2.0f * M_PI);
    std::uniform_real_distribution<float> radius_dist(0.0f, 1.0f);
    
    for (uint32_t i = 0; i < count; ++i) {
        float angle = angle_dist(gen);
        float radius = radius_dist(gen) * length;
        
        bristle_data_->positions.push_back({
            radius * std::cos(angle),
            radius * std::sin(angle),
            0.0f
        });
        
        bristle_data_->velocities.push_back({0.0f, 0.0f, 0.0f});
        bristle_data_->pressures.push_back(1.0f);
    }
    
    std::cout << "[BrushEngine] Initialized " << count << " bristles" << std::endl;
}

float BrushEngine::CalculatePressureResponse(float pressure, bool enabled, float base_value) {
    if (!enabled) {
        return base_value;
    }
    
    // Apply pressure curve
    float response = std::pow(pressure, 0.7f); // Slight curve for more natural feel
    return base_value * response;
}

Core::Math::Vector2 BrushEngine::ApplyTiltOffset(const Core::Math::Vector2& pos, 
                                                float tilt_x, float tilt_y, float size) {
    // Apply tilt offset to brush position
    float offset_scale = size * 0.1f; // 10% of brush size
    
    return Core::Math::Vector2{
        pos.x + tilt_x * offset_scale,
        pos.y + tilt_y * offset_scale
    };
}

void BrushEngine::ApplyBlendMode(BlendMode mode, const Core::Math::Vector3& base_color, 
                                const Core::Math::Vector3& brush_color, float opacity,
                                Core::Math::Vector3& result) {
    switch (mode) {
        case BlendMode::NORMAL:
            result = base_color * (1.0f - opacity) + brush_color * opacity;
            break;
            
        case BlendMode::MULTIPLY:
            result = base_color * (1.0f - opacity) + (base_color * brush_color) * opacity;
            break;
            
        case BlendMode::SCREEN: {
            Core::Math::Vector3 screen_result = {
                1.0f - (1.0f - base_color.x) * (1.0f - brush_color.x),
                1.0f - (1.0f - base_color.y) * (1.0f - brush_color.y),
                1.0f - (1.0f - base_color.z) * (1.0f - brush_color.z)
            };
            result = base_color * (1.0f - opacity) + screen_result * opacity;
            break;
        }
        
        case BlendMode::OVERLAY: {
            Core::Math::Vector3 overlay_result;
            for (int i = 0; i < 3; ++i) {
                if (base_color[i] < 0.5f) {
                    overlay_result[i] = 2.0f * base_color[i] * brush_color[i];
                } else {
                    overlay_result[i] = 1.0f - 2.0f * (1.0f - base_color[i]) * (1.0f - brush_color[i]);
                }
            }
            result = base_color * (1.0f - opacity) + overlay_result * opacity;
            break;
        }
        
        default:
            result = base_color * (1.0f - opacity) + brush_color * opacity;
            break;
    }
}

// BrushUtils implementation
namespace BrushUtils {
    float SmoothStep(float edge0, float edge1, float x) {
        float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }
    
    float CalculateBrushFalloff(float distance, float hardness) {
        if (distance >= 1.0f) {
            return 0.0f;
        }
        
        float falloff = 1.0f - distance;
        return std::pow(falloff, 1.0f / hardness);
    }
    
    Core::Math::Vector3 HSVtoRGB(float h, float s, float v) {
        float c = v * s;
        float x = c * (1.0f - std::abs(std::fmod(h / 60.0f, 2.0f) - 1.0f));
        float m = v - c;
        
        Core::Math::Vector3 rgb;
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
        
        return {rgb.x + m, rgb.y + m, rgb.z + m};
    }
    
    Core::Math::Vector3 RGBtoHSV(const Core::Math::Vector3& rgb) {
        float max_val = std::max({rgb.x, rgb.y, rgb.z});
        float min_val = std::min({rgb.x, rgb.y, rgb.z});
        float delta = max_val - min_val;
        
        float h = 0, s = 0, v = max_val;
        
        if (delta > 0) {
            s = delta / max_val;
            
            if (max_val == rgb.x) {
                h = 60 * (std::fmod((rgb.y - rgb.z) / delta, 6.0f));
            } else if (max_val == rgb.y) {
                h = 60 * ((rgb.z - rgb.x) / delta + 2);
            } else {
                h = 60 * ((rgb.x - rgb.y) / delta + 4);
            }
            
            if (h < 0) h += 360;
        }
        
        return {h, s, v};
    }
    
    float PerlinNoise(float x, float y) {
        // Simplified Perlin noise implementation
        int X = static_cast<int>(std::floor(x)) & 255;
        int Y = static_cast<int>(std::floor(y)) & 255;
        
        x -= std::floor(x);
        y -= std::floor(y);
        
        float u = x * x * x * (x * (x * 6 - 15) + 10);
        float v = y * y * y * (y * (y * 6 - 15) + 10);
        
        // Simplified gradient calculation (would use proper permutation table)
        float a = (X + Y) / 510.0f;
        float b = (X + Y + 1) / 510.0f;
        float c = (X + Y + 256) / 510.0f;
        float d = (X + Y + 257) / 510.0f;
        
        return (a * (1 - u) + b * u) * (1 - v) + (c * (1 - u) + d * u) * v;
    }
    
    float SimplexNoise(float x, float y) {
        // Simplified simplex noise (would use proper implementation)
        return PerlinNoise(x, y) * 0.5f + 0.5f;
    }
}

} // namespace QuantumCanvas::Raster
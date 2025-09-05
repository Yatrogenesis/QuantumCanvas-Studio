#include "vector_renderer.hpp"
#include "../../core/rendering/rendering_engine.hpp"
#include "../../core/rendering/shader_compiler.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <cassert>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace QuantumCanvas::Vector {

// Utility functions for Bézier curve mathematics
namespace BezierMath {
    // Evaluate cubic Bézier curve at parameter t
    std::array<float, 2> evaluateCubic(const std::array<float, 2>& p0,
                                      const std::array<float, 2>& p1,
                                      const std::array<float, 2>& p2,
                                      const std::array<float, 2>& p3,
                                      float t) {
        float t2 = t * t;
        float t3 = t2 * t;
        float mt = 1.0f - t;
        float mt2 = mt * mt;
        float mt3 = mt2 * mt;
        
        return {
            mt3 * p0[0] + 3 * mt2 * t * p1[0] + 3 * mt * t2 * p2[0] + t3 * p3[0],
            mt3 * p0[1] + 3 * mt2 * t * p1[1] + 3 * mt * t2 * p2[1] + t3 * p3[1]
        };
    }
    
    // Calculate arc length of cubic Bézier curve using adaptive sampling
    float calculateArcLength(const std::array<float, 2>& p0,
                            const std::array<float, 2>& p1,
                            const std::array<float, 2>& p2,
                            const std::array<float, 2>& p3,
                            float tolerance = 0.1f) {
        const int maxSamples = 64;
        float length = 0.0f;
        
        auto prev_point = p0;
        for (int i = 1; i <= maxSamples; ++i) {
            float t = static_cast<float>(i) / maxSamples;
            auto current_point = evaluateCubic(p0, p1, p2, p3, t);
            
            float dx = current_point[0] - prev_point[0];
            float dy = current_point[1] - prev_point[1];
            length += std::sqrt(dx * dx + dy * dy);
            
            prev_point = current_point;
        }
        
        return length;
    }
    
    // Tessellate cubic Bézier curve with adaptive subdivision
    std::vector<VectorVertex> tessellateCubic(const std::array<float, 2>& p0,
                                             const std::array<float, 2>& p1,
                                             const std::array<float, 2>& p2,
                                             const std::array<float, 2>& p3,
                                             float tolerance,
                                             int maxDepth) {
        std::vector<VectorVertex> vertices;
        tessellateCubicRecursive(p0, p1, p2, p3, 0.0f, 1.0f, tolerance, maxDepth, 0, vertices);
        return vertices;
    }
    
    void tessellateCubicRecursive(const std::array<float, 2>& p0,
                                 const std::array<float, 2>& p1,
                                 const std::array<float, 2>& p2,
                                 const std::array<float, 2>& p3,
                                 float t_start, float t_end,
                                 float tolerance, int maxDepth, int currentDepth,
                                 std::vector<VectorVertex>& vertices) {
        
        // Base case - maximum depth reached or curve is flat enough
        if (currentDepth >= maxDepth) {
            VectorVertex vertex;
            vertex.position = p0;
            vertex.tex_coords = {t_start, 0.0f};
            vertex.color = {1.0f, 1.0f, 1.0f, 1.0f};
            vertex.normal = {0.0f, 0.0f};
            vertices.push_back(vertex);
            return;
        }
        
        // Check if curve is flat enough using distance to chord
        float chord_length_sq = (p3[0] - p0[0]) * (p3[0] - p0[0]) + (p3[1] - p0[1]) * (p3[1] - p0[1]);
        
        if (chord_length_sq < tolerance * tolerance) {
            VectorVertex vertex;
            vertex.position = p0;
            vertex.tex_coords = {t_start, 0.0f};
            vertex.color = {1.0f, 1.0f, 1.0f, 1.0f};
            vertex.normal = {0.0f, 0.0f};
            vertices.push_back(vertex);
            return;
        }
        
        // Subdivide curve at midpoint using de Casteljau's algorithm
        float t_mid = (t_start + t_end) * 0.5f;
        
        // First level
        auto p01 = std::array<float, 2>{
            (p0[0] + p1[0]) * 0.5f,
            (p0[1] + p1[1]) * 0.5f
        };
        auto p12 = std::array<float, 2>{
            (p1[0] + p2[0]) * 0.5f,
            (p1[1] + p2[1]) * 0.5f
        };
        auto p23 = std::array<float, 2>{
            (p2[0] + p3[0]) * 0.5f,
            (p2[1] + p3[1]) * 0.5f
        };
        
        // Second level
        auto p012 = std::array<float, 2>{
            (p01[0] + p12[0]) * 0.5f,
            (p01[1] + p12[1]) * 0.5f
        };
        auto p123 = std::array<float, 2>{
            (p12[0] + p23[0]) * 0.5f,
            (p12[1] + p23[1]) * 0.5f
        };
        
        // Third level - split point
        auto p_split = std::array<float, 2>{
            (p012[0] + p123[0]) * 0.5f,
            (p012[1] + p123[1]) * 0.5f
        };
        
        // Recursively tessellate left and right halves
        tessellateCubicRecursive(p0, p01, p012, p_split, t_start, t_mid, tolerance, maxDepth, currentDepth + 1, vertices);
        tessellateCubicRecursive(p_split, p123, p23, p3, t_mid, t_end, tolerance, maxDepth, currentDepth + 1, vertices);
    }
}

// PathTessellator implementation
PathTessellator::PathTessellator(const VectorRenderConfig& config)
    : config_(config) {
    
    // Initialize tessellation parameters
    tolerance_ = config.tessellationTolerance;
    maxDepth_ = static_cast<int>(config.maxTessellationDepth);
    adaptiveTessellation_ = config.adaptiveTessellation;
    
    std::cout << "[PathTessellator] Initialized with tolerance=" << tolerance_ 
              << ", maxDepth=" << maxDepth_ << std::endl;
}

PathTessellator::~PathTessellator() = default;

std::vector<VectorVertex> PathTessellator::tessellate(const VectorPath& path) {
    std::vector<VectorVertex> vertices;
    
    if (path.commands.empty()) {
        return vertices;
    }
    
    std::array<float, 2> currentPos = {0.0f, 0.0f};
    std::array<float, 2> pathStart = {0.0f, 0.0f};
    
    for (const auto& cmd : path.commands) {
        switch (cmd.type) {
            case PathCommandType::MoveTo:
                currentPos = {cmd.points[0], cmd.points[1]};
                pathStart = currentPos;
                break;
                
            case PathCommandType::LineTo: {
                auto endPos = std::array<float, 2>{cmd.points[0], cmd.points[1]};
                tessellateLineSegment(currentPos, endPos, vertices);
                currentPos = endPos;
                break;
            }
            
            case PathCommandType::CurveTo: {
                auto cp1 = std::array<float, 2>{cmd.points[0], cmd.points[1]};
                auto cp2 = std::array<float, 2>{cmd.points[2], cmd.points[3]};
                auto endPos = std::array<float, 2>{cmd.points[4], cmd.points[5]};
                tessellateCubicBezier(currentPos, cp1, cp2, endPos, vertices);
                currentPos = endPos;
                break;
            }
            
            case PathCommandType::QuadTo: {
                auto cp = std::array<float, 2>{cmd.points[0], cmd.points[1]};
                auto endPos = std::array<float, 2>{cmd.points[2], cmd.points[3]};
                tessellateQuadraticBezier(currentPos, cp, endPos, vertices);
                currentPos = endPos;
                break;
            }
            
            case PathCommandType::ArcTo: {
                auto endPos = std::array<float, 2>{cmd.points[0], cmd.points[1]};
                float radiusX = cmd.points[2];
                float radiusY = cmd.points[3];
                float rotation = cmd.points[4];
                bool largeArc = cmd.points[5] > 0.5f;
                bool sweep = cmd.points[6] > 0.5f;
                tessellateEllipticalArc(currentPos, endPos, radiusX, radiusY, rotation, largeArc, sweep, vertices);
                currentPos = endPos;
                break;
            }
            
            case PathCommandType::ClosePath:
                if (currentPos[0] != pathStart[0] || currentPos[1] != pathStart[1]) {
                    tessellateLineSegment(currentPos, pathStart, vertices);
                }
                currentPos = pathStart;
                break;
        }
    }
    
    return vertices;
}

void PathTessellator::tessellateLineSegment(const std::array<float, 2>& start,
                                          const std::array<float, 2>& end,
                                          std::vector<VectorVertex>& vertices) {
    VectorVertex startVertex;
    startVertex.position = start;
    startVertex.tex_coords = {0.0f, 0.0f};
    startVertex.color = {1.0f, 1.0f, 1.0f, 1.0f};
    startVertex.normal = {0.0f, 0.0f};
    vertices.push_back(startVertex);
    
    VectorVertex endVertex;
    endVertex.position = end;
    endVertex.tex_coords = {1.0f, 0.0f};
    endVertex.color = {1.0f, 1.0f, 1.0f, 1.0f};
    endVertex.normal = {0.0f, 0.0f};
    vertices.push_back(endVertex);
}

void PathTessellator::tessellateCubicBezier(const std::array<float, 2>& p0,
                                          const std::array<float, 2>& p1,
                                          const std::array<float, 2>& p2,
                                          const std::array<float, 2>& p3,
                                          std::vector<VectorVertex>& vertices) {
    if (adaptiveTessellation_) {
        auto bezierVertices = BezierMath::tessellateCubic(p0, p1, p2, p3, tolerance_, maxDepth_);
        vertices.insert(vertices.end(), bezierVertices.begin(), bezierVertices.end());
    } else {
        // Fixed subdivision
        const int segments = 16;
        for (int i = 0; i <= segments; ++i) {
            float t = static_cast<float>(i) / segments;
            auto point = BezierMath::evaluateCubic(p0, p1, p2, p3, t);
            
            VectorVertex vertex;
            vertex.position = point;
            vertex.tex_coords = {t, 0.0f};
            vertex.color = {1.0f, 1.0f, 1.0f, 1.0f};
            vertex.normal = {0.0f, 0.0f};
            vertices.push_back(vertex);
        }
    }
}

void PathTessellator::tessellateQuadraticBezier(const std::array<float, 2>& p0,
                                              const std::array<float, 2>& p1,
                                              const std::array<float, 2>& p2,
                                              std::vector<VectorVertex>& vertices) {
    // Convert quadratic to cubic Bézier
    auto cp1 = std::array<float, 2>{
        p0[0] + (2.0f / 3.0f) * (p1[0] - p0[0]),
        p0[1] + (2.0f / 3.0f) * (p1[1] - p0[1])
    };
    auto cp2 = std::array<float, 2>{
        p2[0] + (2.0f / 3.0f) * (p1[0] - p2[0]),
        p2[1] + (2.0f / 3.0f) * (p1[1] - p2[1])
    };
    
    tessellateCubicBezier(p0, cp1, cp2, p2, vertices);
}

void PathTessellator::tessellateEllipticalArc(const std::array<float, 2>& start,
                                            const std::array<float, 2>& end,
                                            float radiusX, float radiusY,
                                            float rotation, bool largeArc, bool sweep,
                                            std::vector<VectorVertex>& vertices) {
    // Simplified arc tessellation - convert to line segments
    const int segments = 32;
    
    // Calculate arc parameters (simplified implementation)
    float dx = end[0] - start[0];
    float dy = end[1] - start[1];
    float distance = std::sqrt(dx * dx + dy * dy);
    
    if (distance < 0.001f) {
        return; // Start and end are too close
    }
    
    // For simplicity, create a circular arc approximation
    float angle_step = M_PI / segments;
    
    for (int i = 0; i <= segments; ++i) {
        float t = static_cast<float>(i) / segments;
        float angle = angle_step * i;
        
        // Simple interpolation with slight curve
        float curve_factor = std::sin(t * M_PI) * 0.1f * radiusX;
        
        VectorVertex vertex;
        vertex.position = {
            start[0] + t * dx + curve_factor * std::cos(angle + rotation),
            start[1] + t * dy + curve_factor * std::sin(angle + rotation)
        };
        vertex.tex_coords = {t, 0.0f};
        vertex.color = {1.0f, 1.0f, 1.0f, 1.0f};
        vertex.normal = {0.0f, 0.0f};
        vertices.push_back(vertex);
    }
}

// GPUTessellator implementation (GPU-accelerated tessellation)
GPUTessellator::GPUTessellator(Rendering::RenderingEngine& engine)
    : engine_(engine) {
    
    // Initialize GPU resources for tessellation
    initialized_ = initializeGPUResources();
    
    if (initialized_) {
        std::cout << "[GPUTessellator] GPU tessellation initialized" << std::endl;
    } else {
        std::cout << "[GPUTessellator] Falling back to CPU tessellation" << std::endl;
    }
}

GPUTessellator::~GPUTessellator() {
    // Cleanup GPU resources
}

bool GPUTessellator::tessellate(const VectorPath& path, std::vector<VectorVertex>& vertices) {
    if (!initialized_) {
        return false; // Fall back to CPU tessellation
    }
    
    // Upload path data to GPU
    // Run tessellation compute shader
    // Download results
    
    // For now, return false to use CPU tessellation
    return false;
}

bool GPUTessellator::initializeGPUResources() {
    // Create tessellation compute shader
    // Allocate GPU buffers for path data and results
    // This would require compute shader implementation
    return false; // Not yet implemented
}

// VectorRenderer implementation
VectorRenderer::VectorRenderer(Rendering::RenderingEngine& engine, const VectorRenderConfig& config)
    : engine_(engine), config_(config) {
    
    // Create tessellator
    tessellator_ = std::make_unique<PathTessellator>(config);
    
    // Create GPU tessellator if enabled
    if (config.enableGPUTessellation) {
        gpu_tessellator_ = std::make_unique<GPUTessellator>(engine);
    }
    
    std::cout << "[VectorRenderer] Initialized" << std::endl;
}

VectorRenderer::~VectorRenderer() = default;

VectorRenderer::VectorRenderer(VectorRenderer&& other) noexcept
    : engine_(other.engine_)
    , config_(other.config_)
    , tessellator_(std::move(other.tessellator_))
    , gpu_tessellator_(std::move(other.gpu_tessellator_))
    , vertex_buffer_id_(std::exchange(other.vertex_buffer_id_, 0))
    , index_buffer_id_(std::exchange(other.index_buffer_id_, 0))
    , pipeline_id_(std::exchange(other.pipeline_id_, 0))
    , initialized_(other.initialized_.load())
    , stats_(other.stats_) {
}

VectorRenderer& VectorRenderer::operator=(VectorRenderer&& other) noexcept {
    if (this != &other) {
        engine_ = other.engine_;
        config_ = other.config_;
        tessellator_ = std::move(other.tessellator_);
        gpu_tessellator_ = std::move(other.gpu_tessellator_);
        vertex_buffer_id_ = std::exchange(other.vertex_buffer_id_, 0);
        index_buffer_id_ = std::exchange(other.index_buffer_id_, 0);
        pipeline_id_ = std::exchange(other.pipeline_id_, 0);
        initialized_ = other.initialized_.load();
        stats_ = other.stats_;
    }
    return *this;
}

bool VectorRenderer::initialize() {
    if (initialized_) {
        return true;
    }
    
    // Create vertex buffer
    vertex_buffer_id_ = engine_.create_buffer(
        sizeof(VectorVertex) * MAX_VERTICES_PER_BATCH,
        Rendering::BufferUsage::Vertex | Rendering::BufferUsage::Dynamic
    );
    
    // Create index buffer  
    index_buffer_id_ = engine_.create_buffer(
        sizeof(uint32_t) * MAX_INDICES_PER_BATCH,
        Rendering::BufferUsage::Index | Rendering::BufferUsage::Dynamic
    );
    
    // Create rendering pipeline
    if (!createRenderingPipeline()) {
        std::cerr << "[VectorRenderer] Failed to create rendering pipeline" << std::endl;
        return false;
    }
    
    initialized_ = true;
    std::cout << "[VectorRenderer] Initialization complete" << std::endl;
    return true;
}

void VectorRenderer::shutdown() {
    if (!initialized_) {
        return;
    }
    
    // Release GPU resources
    if (vertex_buffer_id_ != 0) {
        engine_.destroy_buffer(vertex_buffer_id_);
        vertex_buffer_id_ = 0;
    }
    
    if (index_buffer_id_ != 0) {
        engine_.destroy_buffer(index_buffer_id_);
        index_buffer_id_ = 0;
    }
    
    if (pipeline_id_ != 0) {
        engine_.destroy_pipeline(pipeline_id_);
        pipeline_id_ = 0;
    }
    
    initialized_ = false;
    std::cout << "[VectorRenderer] Shutdown complete" << std::endl;
}

void VectorRenderer::render(const std::vector<VectorPath>& paths, const VectorTransform& transform) {
    if (!initialized_) {
        std::cerr << "[VectorRenderer] Renderer not initialized" << std::endl;
        return;
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Begin batch
    beginBatch();
    
    // Process each path
    for (const auto& path : paths) {
        renderPath(path, transform);
    }
    
    // End batch and render
    endBatch();
    
    // Update statistics
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    stats_.paths_rendered += paths.size();
    stats_.total_render_time += duration;
    if (stats_.paths_rendered > 0) {
        stats_.average_render_time = 
            std::chrono::microseconds(stats_.total_render_time.count() / stats_.paths_rendered);
    }
}

void VectorRenderer::renderPath(const VectorPath& path, const VectorTransform& transform) {
    // Tessellate path
    std::vector<VectorVertex> vertices;
    
    // Try GPU tessellation first
    bool gpu_tessellated = false;
    if (gpu_tessellator_ && config_.enableGPUTessellation) {
        gpu_tessellated = gpu_tessellator_->tessellate(path, vertices);
        if (gpu_tessellated) {
            stats_.gpu_tessellations++;
        }
    }
    
    // Fall back to CPU tessellation
    if (!gpu_tessellated) {
        vertices = tessellator_->tessellate(path);
        stats_.cpu_tessellations++;
    }
    
    if (vertices.empty()) {
        return;
    }
    
    // Apply transform to vertices
    for (auto& vertex : vertices) {
        applyTransform(vertex, transform);
    }
    
    // Add to current batch
    addToBatch(vertices, path.style);
    
    stats_.vertices_generated += vertices.size();
}

void VectorRenderer::beginBatch() {
    current_vertices_.clear();
    current_indices_.clear();
    current_vertex_offset_ = 0;
}

void VectorRenderer::endBatch() {
    if (current_vertices_.empty()) {
        return;
    }
    
    // Upload vertex data
    engine_.update_buffer(vertex_buffer_id_, 0, 
                         current_vertices_.data(), 
                         current_vertices_.size() * sizeof(VectorVertex));
    
    // Upload index data
    engine_.update_buffer(index_buffer_id_, 0,
                         current_indices_.data(),
                         current_indices_.size() * sizeof(uint32_t));
    
    // Set pipeline and buffers
    engine_.set_pipeline(pipeline_id_);
    engine_.set_vertex_buffer(0, vertex_buffer_id_, 0, sizeof(VectorVertex));
    engine_.set_index_buffer(index_buffer_id_, Rendering::IndexFormat::Uint32, 0);
    
    // Draw
    engine_.draw_indexed(current_indices_.size(), 1, 0, 0, 0);
    
    stats_.draw_calls++;
}

void VectorRenderer::addToBatch(const std::vector<VectorVertex>& vertices, const VectorStyle& style) {
    if (current_vertices_.size() + vertices.size() > MAX_VERTICES_PER_BATCH) {
        // Batch is full, render current batch and start new one
        endBatch();
        beginBatch();
    }
    
    // Add vertices to batch
    size_t vertex_start = current_vertices_.size();
    current_vertices_.insert(current_vertices_.end(), vertices.begin(), vertices.end());
    
    // Generate indices for line strips
    for (size_t i = 1; i < vertices.size(); ++i) {
        current_indices_.push_back(static_cast<uint32_t>(vertex_start + i - 1));
        current_indices_.push_back(static_cast<uint32_t>(vertex_start + i));
    }
}

void VectorRenderer::applyTransform(VectorVertex& vertex, const VectorTransform& transform) {
    // Apply 2D transformation matrix
    float x = vertex.position[0];
    float y = vertex.position[1];
    
    vertex.position[0] = transform.matrix[0] * x + transform.matrix[2] * y + transform.matrix[4];
    vertex.position[1] = transform.matrix[1] * x + transform.matrix[3] * y + transform.matrix[5];
}

bool VectorRenderer::createRenderingPipeline() {
    // This would create a WebGPU rendering pipeline
    // For now, return a dummy pipeline ID
    pipeline_id_ = 1; // Dummy ID
    return true;
}

VectorRenderer::RenderStats VectorRenderer::getStats() const {
    return stats_;
}

void VectorRenderer::resetStats() {
    stats_ = RenderStats{};
}

void VectorRenderer::setConfig(const VectorRenderConfig& config) {
    config_ = config;
    
    // Update tessellator configuration
    if (tessellator_) {
        tessellator_->setTolerance(config.tessellationTolerance);
        tessellator_->setMaxDepth(config.maxTessellationDepth);
        tessellator_->setAdaptiveTessellation(config.adaptiveTessellation);
    }
}

// Utility functions
std::array<float, 4> calculateBounds(const VectorPath& path) {
    if (path.commands.empty()) {
        return {0.0f, 0.0f, 0.0f, 0.0f};
    }
    
    float min_x = std::numeric_limits<float>::max();
    float min_y = std::numeric_limits<float>::max();
    float max_x = std::numeric_limits<float>::lowest();
    float max_y = std::numeric_limits<float>::lowest();
    
    for (const auto& cmd : path.commands) {
        for (size_t i = 0; i < cmd.points.size(); i += 2) {
            float x = cmd.points[i];
            float y = cmd.points[i + 1];
            
            min_x = std::min(min_x, x);
            min_y = std::min(min_y, y);
            max_x = std::max(max_x, x);
            max_y = std::max(max_y, y);
        }
    }
    
    return {min_x, min_y, max_x, max_y};
}

VectorTransform createIdentityTransform() {
    VectorTransform transform;
    transform.matrix = {1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f};
    return transform;
}

VectorTransform createTranslationTransform(float x, float y) {
    VectorTransform transform;
    transform.matrix = {1.0f, 0.0f, 0.0f, 1.0f, x, y};
    return transform;
}

VectorTransform createScaleTransform(float sx, float sy) {
    VectorTransform transform;
    transform.matrix = {sx, 0.0f, 0.0f, sy, 0.0f, 0.0f};
    return transform;
}

VectorTransform createRotationTransform(float angle) {
    float cos_a = std::cos(angle);
    float sin_a = std::sin(angle);
    
    VectorTransform transform;
    transform.matrix = {cos_a, sin_a, -sin_a, cos_a, 0.0f, 0.0f};
    return transform;
}

VectorTransform multiplyTransforms(const VectorTransform& a, const VectorTransform& b) {
    VectorTransform result;
    
    // Matrix multiplication for 2D affine transforms
    // [a c e]   [g i k]   [a*g+c*h  a*i+c*j  a*k+c*l+e]
    // [b d f] * [h j l] = [b*g+d*h  b*i+d*j  b*k+d*l+f]
    // [0 0 1]   [0 0 1]   [0        0        1        ]
    
    result.matrix[0] = a.matrix[0] * b.matrix[0] + a.matrix[2] * b.matrix[1];  // a*g + c*h
    result.matrix[1] = a.matrix[1] * b.matrix[0] + a.matrix[3] * b.matrix[1];  // b*g + d*h
    result.matrix[2] = a.matrix[0] * b.matrix[2] + a.matrix[2] * b.matrix[3];  // a*i + c*j
    result.matrix[3] = a.matrix[1] * b.matrix[2] + a.matrix[3] * b.matrix[3];  // b*i + d*j
    result.matrix[4] = a.matrix[0] * b.matrix[4] + a.matrix[2] * b.matrix[5] + a.matrix[4];  // a*k + c*l + e
    result.matrix[5] = a.matrix[1] * b.matrix[4] + a.matrix[3] * b.matrix[5] + a.matrix[5];  // b*k + d*l + f
    
    return result;
}

} // namespace QuantumCanvas::Vector
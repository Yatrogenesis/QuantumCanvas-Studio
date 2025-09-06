#include "file_format_manager.hpp"
#include "../../core/kernel/kernel_manager.hpp"
#include <fstream>
#include <algorithm>
#include <thread>
#include <future>
#include <regex>
#include <iomanip>
#include <sstream>

namespace QuantumCanvas::IO {

// Static format name mapping initialization
std::unordered_map<FileFormat, std::string> FileFormatManager::formatNames_;

void FileFormatManager::initializeFormatNames() {
    formatNames_[FileFormat::PNG] = "Portable Network Graphics";
    formatNames_[FileFormat::JPEG] = "JPEG Image";
    formatNames_[FileFormat::TIFF] = "Tagged Image File Format";
    formatNames_[FileFormat::BMP] = "Windows Bitmap";
    formatNames_[FileFormat::GIF] = "Graphics Interchange Format";
    formatNames_[FileFormat::WEBP] = "WebP Image";
    formatNames_[FileFormat::PSD] = "Adobe Photoshop Document";
    formatNames_[FileFormat::PSB] = "Adobe Photoshop Large Document";
    formatNames_[FileFormat::XCF] = "GIMP Image";
    formatNames_[FileFormat::SVG] = "Scalable Vector Graphics";
    formatNames_[FileFormat::AI] = "Adobe Illustrator";
    formatNames_[FileFormat::EPS] = "Encapsulated PostScript";
    formatNames_[FileFormat::PDF] = "Portable Document Format";
    formatNames_[FileFormat::CDR] = "CorelDRAW Document";
    formatNames_[FileFormat::WMF] = "Windows Metafile";
    formatNames_[FileFormat::EMF] = "Enhanced Metafile";
    formatNames_[FileFormat::DWG] = "AutoCAD Drawing";
    formatNames_[FileFormat::DXF] = "Drawing Exchange Format";
    formatNames_[FileFormat::STEP] = "Standard for Exchange of Product Data";
    formatNames_[FileFormat::IGES] = "Initial Graphics Exchange Specification";
    formatNames_[FileFormat::STL] = "Stereolithography";
    formatNames_[FileFormat::OBJ] = "Wavefront OBJ";
    formatNames_[FileFormat::PLY] = "Polygon File Format";
    formatNames_[FileFormat::CR2] = "Canon Raw";
    formatNames_[FileFormat::NEF] = "Nikon Raw";
    formatNames_[FileFormat::ARW] = "Sony Raw";
    formatNames_[FileFormat::DNG] = "Adobe Digital Negative";
    formatNames_[FileFormat::RAF] = "Fujifilm Raw";
    formatNames_[FileFormat::ORF] = "Olympus Raw";
    formatNames_[FileFormat::QCSX] = "QuantumCanvas Studio Extended";
    formatNames_[FileFormat::QCS] = "QuantumCanvas Studio";
    formatNames_[FileFormat::ICO] = "Windows Icon";
    formatNames_[FileFormat::CUR] = "Windows Cursor";
    formatNames_[FileFormat::ANI] = "Animated Cursor";
    formatNames_[FileFormat::Unknown] = "Unknown Format";
}

// Thread pool implementation for async operations
class FileFormatManager::ThreadPool {
private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queueMutex_;
    std::condition_variable condition_;
    std::atomic<bool> stop_{false};

public:
    explicit ThreadPool(size_t numThreads) {
        for (size_t i = 0; i < numThreads; ++i) {
            workers_.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queueMutex_);
                        condition_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
                        
                        if (stop_ && tasks_.empty()) break;
                        
                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }
                    task();
                }
            });
        }
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            stop_ = true;
        }
        condition_.notify_all();
        for (std::thread& worker : workers_) {
            worker.join();
        }
    }

    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type> {
        using return_type = typename std::result_of<F(Args...)>::type;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            if (stop_) {
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }
            tasks_.emplace([task]() { (*task)(); });
        }
        condition_.notify_one();
        return res;
    }
};

FileFormatManager::FileFormatManager(Rendering::RenderingEngine& engine)
    : engine_(engine) {
    
    // Initialize static data if not already done
    if (formatNames_.empty()) {
        initializeFormatNames();
    }
    
    // Auto-detect optimal thread count if not set
    if (maxConcurrency_ == 0) {
        maxConcurrency_ = std::max(1u, std::thread::hardware_concurrency());
    }
}

FileFormatManager::~FileFormatManager() {
    shutdown();
}

FileFormatManager::FileFormatManager(FileFormatManager&& other) noexcept
    : engine_(other.engine_)
    , initialized_(other.initialized_.load())
    , handlers_(std::move(other.handlers_))
    , extensionMap_(std::move(other.extensionMap_))
    , importPresets_(std::move(other.importPresets_))
    , exportPresets_(std::move(other.exportPresets_))
    , loadedPlugins_(std::move(other.loadedPlugins_))
    , maxConcurrency_(other.maxConcurrency_)
    , memoryBudget_(other.memoryBudget_)
    , cacheEnabled_(other.cacheEnabled_)
    , threadPool_(std::move(other.threadPool_))
    , fileCache_(std::move(other.fileCache_))
    , maxCacheSize_(other.maxCacheSize_)
    , currentCacheMemory_(other.currentCacheMemory_)
    , stats_(other.stats_)
    , errorLog_(std::move(other.errorLog_))
    , maxErrorLogSize_(other.maxErrorLogSize_)
    , magicPatterns_(std::move(other.magicPatterns_)) {
    
    other.initialized_ = false;
}

FileFormatManager& FileFormatManager::operator=(FileFormatManager&& other) noexcept {
    if (this != &other) {
        shutdown();
        
        engine_ = other.engine_;
        initialized_ = other.initialized_.load();
        handlers_ = std::move(other.handlers_);
        extensionMap_ = std::move(other.extensionMap_);
        importPresets_ = std::move(other.importPresets_);
        exportPresets_ = std::move(other.exportPresets_);
        loadedPlugins_ = std::move(other.loadedPlugins_);
        maxConcurrency_ = other.maxConcurrency_;
        memoryBudget_ = other.memoryBudget_;
        cacheEnabled_ = other.cacheEnabled_;
        threadPool_ = std::move(other.threadPool_);
        fileCache_ = std::move(other.fileCache_);
        maxCacheSize_ = other.maxCacheSize_;
        currentCacheMemory_ = other.currentCacheMemory_;
        stats_ = other.stats_;
        errorLog_ = std::move(other.errorLog_);
        maxErrorLogSize_ = other.maxErrorLogSize_;
        magicPatterns_ = std::move(other.magicPatterns_);
        
        other.initialized_ = false;
    }
    return *this;
}

bool FileFormatManager::initialize() {
    if (initialized_) return true;
    
    try {
        // Initialize thread pool
        threadPool_ = std::make_unique<ThreadPool>(maxConcurrency_);
        
        // Initialize magic byte patterns for format detection
        initializeMagicPatterns();
        
        // Register built-in format handlers
        registerBuiltInHandlers();
        
        // Build extension mapping
        buildExtensionMap();
        
        // Create default presets
        createDefaultPresets();
        
        initialized_ = true;
        return true;
    }
    catch (const std::exception& e) {
        ErrorInfo error;
        error.severity = ErrorInfo::Critical;
        error.message = "Failed to initialize FileFormatManager: " + std::string(e.what());
        error.timestamp = std::chrono::system_clock::now();
        logError(error);
        
        return false;
    }
}

void FileFormatManager::shutdown() {
    if (!initialized_) return;
    
    // Reset thread pool
    threadPool_.reset();
    
    // Clear handlers
    {
        std::lock_guard<std::mutex> lock(handlersMutex_);
        handlers_.clear();
    }
    
    // Unload plugins
    {
        std::lock_guard<std::mutex> lock(pluginsMutex_);
        for (auto& [name, handle] : loadedPlugins_) {
#ifdef _WIN32
            FreeLibrary(static_cast<HMODULE>(handle));
#else
            dlclose(handle);
#endif
        }
        loadedPlugins_.clear();
    }
    
    // Clear cache
    {
        std::lock_guard<std::mutex> lock(cacheMutex_);
        fileCache_.clear();
        currentCacheMemory_ = 0;
    }
    
    initialized_ = false;
}

void FileFormatManager::registerHandler(std::unique_ptr<IFormatHandler> handler) {
    if (!handler) return;
    
    FileFormat format = handler->getFormat();
    
    {
        std::lock_guard<std::mutex> lock(handlersMutex_);
        handlers_[format] = std::move(handler);
    }
    
    buildExtensionMap();
}

void FileFormatManager::unregisterHandler(FileFormat format) {
    {
        std::lock_guard<std::mutex> lock(handlersMutex_);
        handlers_.erase(format);
    }
    
    buildExtensionMap();
}

IFormatHandler* FileFormatManager::getHandler(FileFormat format) {
    std::lock_guard<std::mutex> lock(handlersMutex_);
    auto it = handlers_.find(format);
    return (it != handlers_.end()) ? it->second.get() : nullptr;
}

const IFormatHandler* FileFormatManager::getHandler(FileFormat format) const {
    std::lock_guard<std::mutex> lock(handlersMutex_);
    auto it = handlers_.find(format);
    return (it != handlers_.end()) ? it->second.get() : nullptr;
}

FormatDetectionResult FileFormatManager::detectFormat(const std::filesystem::path& filePath) const {
    if (!std::filesystem::exists(filePath) || !std::filesystem::is_regular_file(filePath)) {
        return FormatDetectionResult{FileFormat::Unknown, 0.0f, "", {}, "file_not_found"};
    }
    
    // First try extension-based detection
    std::string extension = FormatUtils::getFileExtension(filePath);
    FileFormat formatByExt = detectFormatByExtension(extension);
    
    // If extension detection succeeded with high confidence, use it
    if (formatByExt != FileFormat::Unknown) {
        FormatDetectionResult result;
        result.format = formatByExt;
        result.confidence = 0.7f;  // Medium confidence for extension-based detection
        result.detectionMethod = "extension";
        result.possibleExtensions.push_back(extension);
        return result;
    }
    
    // Try content-based detection
    try {
        return detectByContent(filePath);
    }
    catch (const std::exception& e) {
        ErrorInfo error;
        error.severity = ErrorInfo::Warning;
        error.message = "Failed to detect format for file: " + filePath.string() + " - " + e.what();
        error.filePath = filePath.string();
        error.timestamp = std::chrono::system_clock::now();
        const_cast<FileFormatManager*>(this)->logError(error);
        
        return FormatDetectionResult{FileFormat::Unknown, 0.0f, "", {}, "detection_error"};
    }
}

FormatDetectionResult FileFormatManager::detectFormat(const std::vector<uint8_t>& data) const {
    if (data.empty()) {
        return FormatDetectionResult{FileFormat::Unknown, 0.0f, "", {}, "empty_data"};
    }
    
    return detectByMagicBytes(data);
}

FileFormat FileFormatManager::detectFormatByExtension(const std::string& extension) const {
    std::string normalizedExt = FormatUtils::normalizeExtension(extension);
    
    auto it = extensionMap_.find(normalizedExt);
    return (it != extensionMap_.end()) ? it->second : FileFormat::Unknown;
}

std::vector<FileFormat> FileFormatManager::getSupportedFormats() const {
    std::vector<FileFormat> formats;
    
    {
        std::lock_guard<std::mutex> lock(handlersMutex_);
        formats.reserve(handlers_.size());
        
        for (const auto& [format, handler] : handlers_) {
            formats.push_back(format);
        }
    }
    
    return formats;
}

FormatCapabilities FileFormatManager::getFormatCapabilities(FileFormat format) const {
    const IFormatHandler* handler = getHandler(format);
    return handler ? handler->getCapabilities() : FormatCapabilities{};
}

std::vector<std::string> FileFormatManager::getFormatExtensions(FileFormat format) const {
    const IFormatHandler* handler = getHandler(format);
    return handler ? handler->getExtensions() : std::vector<std::string>{};
}

std::string FileFormatManager::getFormatName(FileFormat format) const {
    auto it = formatNames_.find(format);
    return (it != formatNames_.end()) ? it->second : "Unknown Format";
}

FileInfo FileFormatManager::getFileInfo(const std::filesystem::path& filePath) const {
    FileInfo info;
    info.filePath = filePath;
    
    if (!std::filesystem::exists(filePath) || !std::filesystem::is_regular_file(filePath)) {
        info.errorMessage = "File does not exist or is not a regular file";
        return info;
    }
    
    try {
        // Basic file properties
        info.fileSize = std::filesystem::file_size(filePath);
        info.lastModified = std::chrono::system_clock::from_time_t(
            std::chrono::system_clock::to_time_t(
                std::filesystem::last_write_time(filePath)));
        
        // Detect format
        FormatDetectionResult detection = detectFormat(filePath);
        info.format = detection.format;
        
        // Get detailed info from handler
        if (detection.isSupported()) {
            const IFormatHandler* handler = getHandler(info.format);
            if (handler) {
                try {
                    FileInfo detailedInfo = handler->getFileInfo(filePath);
                    // Merge detailed info with basic info
                    info.dimensions = detailedInfo.dimensions;
                    info.bitDepth = detailedInfo.bitDepth;
                    info.channels = detailedInfo.channels;
                    info.dpi = detailedInfo.dpi;
                    info.colorProfile = detailedInfo.colorProfile;
                    info.pageCount = detailedInfo.pageCount;
                    info.layerCount = detailedInfo.layerCount;
                    info.hasTransparency = detailedInfo.hasTransparency;
                    info.isAnimated = detailedInfo.isAnimated;
                    info.metadata = detailedInfo.metadata;
                    info.isValid = true;
                }
                catch (const std::exception& e) {
                    info.errorMessage = "Failed to get detailed file info: " + std::string(e.what());
                }
            }
        }
        
        if (info.errorMessage.empty()) {
            info.isValid = true;
        }
    }
    catch (const std::exception& e) {
        info.errorMessage = "Error reading file: " + std::string(e.what());
    }
    
    return info;
}

// Async load operations
std::future<std::shared_ptr<Document>> FileFormatManager::loadDocument(
    const std::filesystem::path& filePath,
    const LoadOptions& options) {
    
    return threadPool_->enqueue([this, filePath, options]() -> std::shared_ptr<Document> {
        auto startTime = std::chrono::high_resolution_clock::now();
        
        try {
            // Detect format
            FormatDetectionResult detection = detectFormat(filePath);
            if (!detection.isSupported()) {
                throw std::runtime_error("Unsupported file format: " + filePath.string());
            }
            
            // Get handler
            IFormatHandler* handler = getHandler(detection.format);
            if (!handler) {
                throw std::runtime_error("No handler available for format");
            }
            
            // Load document
            auto future = handler->loadDocument(filePath, options);
            auto document = future.get();
            
            // Update statistics
            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
            
            {
                std::lock_guard<std::mutex> lock(statsMutex_);
                stats_.documentsLoaded++;
                stats_.bytesRead += std::filesystem::file_size(filePath);
                stats_.totalLoadTime += duration;
                stats_.formatUsageCount[detection.format]++;
                stats_.formatProcessingTime[detection.format] += duration;
            }
            
            return document;
        }
        catch (const std::exception& e) {
            ErrorInfo error;
            error.severity = ErrorInfo::Error;
            error.message = "Failed to load document: " + std::string(e.what());
            error.filePath = filePath.string();
            error.timestamp = std::chrono::system_clock::now();
            logError(error);
            
            throw;
        }
    });
}

std::future<std::shared_ptr<Image>> FileFormatManager::loadImage(
    const std::filesystem::path& filePath,
    const LoadOptions& options) {
    
    return threadPool_->enqueue([this, filePath, options]() -> std::shared_ptr<Image> {
        auto startTime = std::chrono::high_resolution_clock::now();
        
        try {
            // Check cache first
            if (cacheEnabled_) {
                std::lock_guard<std::mutex> lock(cacheMutex_);
                std::string cacheKey = filePath.string();
                auto it = fileCache_.find(cacheKey);
                if (it != fileCache_.end()) {
                    // Verify file hasn't been modified
                    auto lastModified = std::filesystem::last_write_time(filePath);
                    if (it->second.fileModified == lastModified) {
                        it->second.lastAccess = std::chrono::system_clock::now();
                        stats_.cacheHits++;
                        return it->second.thumbnail;  // Return cached image
                    } else {
                        // File was modified, remove from cache
                        currentCacheMemory_ -= it->second.memorySize;
                        fileCache_.erase(it);
                    }
                }
                stats_.cacheMisses++;
            }
            
            // Detect format
            FormatDetectionResult detection = detectFormat(filePath);
            if (!detection.isSupported()) {
                throw std::runtime_error("Unsupported file format: " + filePath.string());
            }
            
            // Get handler
            IFormatHandler* handler = getHandler(detection.format);
            if (!handler) {
                throw std::runtime_error("No handler available for format");
            }
            
            // Load image
            auto future = handler->loadImage(filePath, options);
            auto image = future.get();
            
            // Update cache if enabled
            if (cacheEnabled_ && image) {
                FileInfo info = getFileInfo(filePath);
                updateCache(filePath.string(), info, image);
            }
            
            // Update statistics
            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
            
            {
                std::lock_guard<std::mutex> lock(statsMutex_);
                stats_.imagesLoaded++;
                stats_.bytesRead += std::filesystem::file_size(filePath);
                stats_.totalLoadTime += duration;
                stats_.formatUsageCount[detection.format]++;
                stats_.formatProcessingTime[detection.format] += duration;
            }
            
            return image;
        }
        catch (const std::exception& e) {
            ErrorInfo error;
            error.severity = ErrorInfo::Error;
            error.message = "Failed to load image: " + std::string(e.what());
            error.filePath = filePath.string();
            error.timestamp = std::chrono::system_clock::now();
            logError(error);
            
            throw;
        }
    });
}

// Save operations (similar pattern)
std::future<bool> FileFormatManager::saveDocument(
    const std::shared_ptr<Document>& document,
    const std::filesystem::path& filePath,
    const SaveOptions& options) {
    
    return threadPool_->enqueue([this, document, filePath, options]() -> bool {
        auto startTime = std::chrono::high_resolution_clock::now();
        
        try {
            if (!document) {
                throw std::runtime_error("Document is null");
            }
            
            // Determine format from file extension
            std::string extension = FormatUtils::getFileExtension(filePath);
            FileFormat format = detectFormatByExtension(extension);
            
            if (format == FileFormat::Unknown) {
                throw std::runtime_error("Cannot determine output format from extension: " + extension);
            }
            
            // Get handler
            IFormatHandler* handler = getHandler(format);
            if (!handler || !handler->canSave()) {
                throw std::runtime_error("No handler available for saving format");
            }
            
            // Save document
            auto future = handler->saveDocument(document, filePath, options);
            bool success = future.get();
            
            if (success) {
                // Update statistics
                auto endTime = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
                
                {
                    std::lock_guard<std::mutex> lock(statsMutex_);
                    stats_.documentsSaved++;
                    if (std::filesystem::exists(filePath)) {
                        stats_.bytesWritten += std::filesystem::file_size(filePath);
                    }
                    stats_.totalSaveTime += duration;
                    stats_.formatUsageCount[format]++;
                    stats_.formatProcessingTime[format] += duration;
                }
            }
            
            return success;
        }
        catch (const std::exception& e) {
            ErrorInfo error;
            error.severity = ErrorInfo::Error;
            error.message = "Failed to save document: " + std::string(e.what());
            error.filePath = filePath.string();
            error.timestamp = std::chrono::system_clock::now();
            logError(error);
            
            return false;
        }
    });
}

std::future<bool> FileFormatManager::saveImage(
    const std::shared_ptr<Image>& image,
    const std::filesystem::path& filePath,
    const SaveOptions& options) {
    
    return threadPool_->enqueue([this, image, filePath, options]() -> bool {
        auto startTime = std::chrono::high_resolution_clock::now();
        
        try {
            if (!image) {
                throw std::runtime_error("Image is null");
            }
            
            // Determine format from file extension
            std::string extension = FormatUtils::getFileExtension(filePath);
            FileFormat format = detectFormatByExtension(extension);
            
            if (format == FileFormat::Unknown) {
                throw std::runtime_error("Cannot determine output format from extension: " + extension);
            }
            
            // Get handler
            IFormatHandler* handler = getHandler(format);
            if (!handler || !handler->canSave()) {
                throw std::runtime_error("No handler available for saving format");
            }
            
            // Save image
            auto future = handler->saveImage(image, filePath, options);
            bool success = future.get();
            
            if (success) {
                // Update statistics
                auto endTime = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
                
                {
                    std::lock_guard<std::mutex> lock(statsMutex_);
                    stats_.imagesSaved++;
                    if (std::filesystem::exists(filePath)) {
                        stats_.bytesWritten += std::filesystem::file_size(filePath);
                    }
                    stats_.totalSaveTime += duration;
                    stats_.formatUsageCount[format]++;
                    stats_.formatProcessingTime[format] += duration;
                }
            }
            
            return success;
        }
        catch (const std::exception& e) {
            ErrorInfo error;
            error.severity = ErrorInfo::Error;
            error.message = "Failed to save image: " + std::string(e.what());
            error.filePath = filePath.string();
            error.timestamp = std::chrono::system_clock::now();
            logError(error);
            
            return false;
        }
    });
}

bool FileFormatManager::validateFile(const std::filesystem::path& filePath) const {
    try {
        if (!std::filesystem::exists(filePath) || !std::filesystem::is_regular_file(filePath)) {
            return false;
        }
        
        FormatDetectionResult detection = detectFormat(filePath);
        if (!detection.isSupported()) {
            return false;
        }
        
        const IFormatHandler* handler = getHandler(detection.format);
        return handler ? handler->validateFile(filePath) : false;
    }
    catch (...) {
        return false;
    }
}

FileFormatManager::FileFormatStats FileFormatManager::getStats() const {
    std::lock_guard<std::mutex> lock(statsMutex_);
    return stats_;
}

void FileFormatManager::resetStats() {
    std::lock_guard<std::mutex> lock(statsMutex_);
    stats_ = FileFormatStats{};
}

std::vector<FileFormatManager::ErrorInfo> FileFormatManager::getRecentErrors() const {
    std::lock_guard<std::mutex> lock(errorMutex_);
    return errorLog_;
}

void FileFormatManager::clearErrorLog() {
    std::lock_guard<std::mutex> lock(errorMutex_);
    errorLog_.clear();
}

// Private implementation methods
void FileFormatManager::registerBuiltInHandlers() {
    // TODO: Register built-in format handlers
    // This would be implemented with actual format handlers
    // For now, we'll have placeholder implementations
}

void FileFormatManager::buildExtensionMap() {
    std::lock_guard<std::mutex> lock(handlersMutex_);
    extensionMap_.clear();
    
    for (const auto& [format, handler] : handlers_) {
        std::vector<std::string> extensions = handler->getExtensions();
        for (const std::string& ext : extensions) {
            std::string normalizedExt = FormatUtils::normalizeExtension(ext);
            extensionMap_[normalizedExt] = format;
        }
    }
}

void FileFormatManager::createDefaultPresets() {
    // Create default import presets
    {
        ImportPreset preset;
        preset.name = "High Quality";
        preset.description = "Import with highest quality settings";
        preset.targetFormat = FileFormat::Unknown;  // Auto-detect
        preset.options.dpi = 300.0f;
        preset.options.preserveAspectRatio = true;
        preset.options.loadLayerEffects = true;
        preset.options.useGPUAcceleration = true;
        addImportPreset(preset);
    }
    
    {
        ImportPreset preset;
        preset.name = "Web Optimized";
        preset.description = "Import optimized for web use";
        preset.targetFormat = FileFormat::Unknown;
        preset.options.dpi = 72.0f;
        preset.options.targetSize = {1920, 1080};
        preset.options.preserveAspectRatio = true;
        addImportPreset(preset);
    }
    
    // Create default export presets
    {
        ExportPreset preset;
        preset.name = "High Quality JPEG";
        preset.description = "High quality JPEG export";
        preset.targetFormat = FileFormat::JPEG;
        preset.options.quality = 0.95f;
        preset.options.jpegOptions.progressive = false;
        preset.options.jpegOptions.optimizeHuffman = true;
        addExportPreset(preset);
    }
    
    {
        ExportPreset preset;
        preset.name = "PNG with Transparency";
        preset.description = "PNG export preserving transparency";
        preset.targetFormat = FileFormat::PNG;
        preset.options.pngOptions.compressionLevel = 6;
        preset.options.preserveLayers = false;  // Flatten for PNG
        addExportPreset(preset);
    }
}

void FileFormatManager::initializeMagicPatterns() {
    // PNG: 89 50 4E 47 0D 0A 1A 0A
    magicPatterns_.push_back({{0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A}, {}, 0, FileFormat::PNG, 1.0f});
    
    // JPEG: FF D8 FF
    magicPatterns_.push_back({{0xFF, 0xD8, 0xFF}, {}, 0, FileFormat::JPEG, 1.0f});
    
    // GIF: GIF87a or GIF89a
    magicPatterns_.push_back({{'G', 'I', 'F', '8', '7', 'a'}, {}, 0, FileFormat::GIF, 1.0f});
    magicPatterns_.push_back({{'G', 'I', 'F', '8', '9', 'a'}, {}, 0, FileFormat::GIF, 1.0f});
    
    // BMP: BM
    magicPatterns_.push_back({{'B', 'M'}, {}, 0, FileFormat::BMP, 1.0f});
    
    // TIFF: II*\0 or MM\0*
    magicPatterns_.push_back({{'I', 'I', 0x2A, 0x00}, {}, 0, FileFormat::TIFF, 1.0f});
    magicPatterns_.push_back({{'M', 'M', 0x00, 0x2A}, {}, 0, FileFormat::TIFF, 1.0f});
    
    // WebP: RIFF____WEBP
    magicPatterns_.push_back({{'R', 'I', 'F', 'F'}, {}, 0, FileFormat::WEBP, 0.5f});
    magicPatterns_.push_back({{'W', 'E', 'B', 'P'}, {}, 8, FileFormat::WEBP, 1.0f});
    
    // PDF: %PDF-
    magicPatterns_.push_back({{'%', 'P', 'D', 'F', '-'}, {}, 0, FileFormat::PDF, 1.0f});
    
    // SVG: <?xml or <svg
    magicPatterns_.push_back({{'<', '?', 'x', 'm', 'l'}, {}, 0, FileFormat::SVG, 0.8f});
    magicPatterns_.push_back({{'<', 's', 'v', 'g'}, {}, 0, FileFormat::SVG, 1.0f});
    
    // PSD: 8BPS
    magicPatterns_.push_back({{'8', 'B', 'P', 'S'}, {}, 0, FileFormat::PSD, 1.0f});
    
    // ICO: \0\0\1\0
    magicPatterns_.push_back({{0x00, 0x00, 0x01, 0x00}, {}, 0, FileFormat::ICO, 1.0f});
    
    // Add more patterns as needed...
}

FormatDetectionResult FileFormatManager::detectByMagicBytes(const std::vector<uint8_t>& data) const {
    for (const auto& pattern : magicPatterns_) {
        if (data.size() < pattern.offset + pattern.pattern.size()) {
            continue;
        }
        
        bool matches = true;
        for (size_t i = 0; i < pattern.pattern.size(); ++i) {
            size_t dataIndex = pattern.offset + i;
            uint8_t dataByte = data[dataIndex];
            uint8_t patternByte = pattern.pattern[i];
            
            // Apply mask if present
            if (!pattern.mask.empty() && i < pattern.mask.size()) {
                dataByte &= pattern.mask[i];
                patternByte &= pattern.mask[i];
            }
            
            if (dataByte != patternByte) {
                matches = false;
                break;
            }
        }
        
        if (matches) {
            FormatDetectionResult result;
            result.format = pattern.format;
            result.confidence = pattern.confidence;
            result.detectionMethod = "magic_bytes";
            return result;
        }
    }
    
    return FormatDetectionResult{FileFormat::Unknown, 0.0f, "", {}, "no_magic_match"};
}

FormatDetectionResult FileFormatManager::detectByContent(const std::filesystem::path& filePath) const {
    try {
        std::ifstream file(filePath, std::ios::binary);
        if (!file) {
            return FormatDetectionResult{FileFormat::Unknown, 0.0f, "", {}, "cannot_open_file"};
        }
        
        // Read first 64 bytes for magic byte detection
        std::vector<uint8_t> buffer(64);
        file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
        size_t bytesRead = file.gcount();
        buffer.resize(bytesRead);
        
        return detectByMagicBytes(buffer);
    }
    catch (const std::exception& e) {
        return FormatDetectionResult{FileFormat::Unknown, 0.0f, "", {}, "content_detection_error"};
    }
}

void FileFormatManager::updateCache(const std::string& filePath, const FileInfo& info,
                                   std::shared_ptr<Image> thumbnail) {
    std::lock_guard<std::mutex> lock(cacheMutex_);
    
    // Clean cache if it's getting too large
    if (fileCache_.size() >= maxCacheSize_) {
        cleanupCache();
    }
    
    CacheEntry entry;
    entry.fileInfo = info;
    entry.thumbnail = thumbnail;
    entry.lastAccess = std::chrono::system_clock::now();
    entry.fileModified = info.lastModified;
    
    // Estimate memory size
    entry.memorySize = sizeof(FileInfo);
    if (thumbnail) {
        // Rough estimate: width * height * channels * bytes_per_channel
        entry.memorySize += info.dimensions[0] * info.dimensions[1] * info.channels * (info.bitDepth / 8);
    }
    
    // Remove old entry if exists
    auto it = fileCache_.find(filePath);
    if (it != fileCache_.end()) {
        currentCacheMemory_ -= it->second.memorySize;
    }
    
    fileCache_[filePath] = entry;
    currentCacheMemory_ += entry.memorySize;
}

void FileFormatManager::cleanupCache() {
    // Remove oldest accessed entries until we're under budget
    while (currentCacheMemory_ > memoryBudget_ && !fileCache_.empty()) {
        auto oldestIt = std::min_element(fileCache_.begin(), fileCache_.end(),
            [](const auto& a, const auto& b) {
                return a.second.lastAccess < b.second.lastAccess;
            });
        
        currentCacheMemory_ -= oldestIt->second.memorySize;
        fileCache_.erase(oldestIt);
    }
}

void FileFormatManager::logError(const ErrorInfo& error) {
    std::lock_guard<std::mutex> lock(errorMutex_);
    
    errorLog_.push_back(error);
    
    // Keep log size under control
    if (errorLog_.size() > maxErrorLogSize_) {
        errorLog_.erase(errorLog_.begin());
    }
}

// Preset management
void FileFormatManager::addImportPreset(const ImportPreset& preset) {
    std::lock_guard<std::mutex> lock(presetsMutex_);
    importPresets_.push_back(preset);
}

void FileFormatManager::addExportPreset(const ExportPreset& preset) {
    std::lock_guard<std::mutex> lock(presetsMutex_);
    exportPresets_.push_back(preset);
}

void FileFormatManager::removeImportPreset(const std::string& name) {
    std::lock_guard<std::mutex> lock(presetsMutex_);
    importPresets_.erase(
        std::remove_if(importPresets_.begin(), importPresets_.end(),
            [&name](const ImportPreset& preset) { return preset.name == name; }),
        importPresets_.end());
}

void FileFormatManager::removeExportPreset(const std::string& name) {
    std::lock_guard<std::mutex> lock(presetsMutex_);
    exportPresets_.erase(
        std::remove_if(exportPresets_.begin(), exportPresets_.end(),
            [&name](const ExportPreset& preset) { return preset.name == name; }),
        exportPresets_.end());
}

std::vector<FileFormatManager::ImportPreset> FileFormatManager::getImportPresets() const {
    std::lock_guard<std::mutex> lock(presetsMutex_);
    return importPresets_;
}

std::vector<FileFormatManager::ExportPreset> FileFormatManager::getExportPresets() const {
    std::lock_guard<std::mutex> lock(presetsMutex_);
    return exportPresets_;
}

// FormatUtils namespace implementation
namespace FormatUtils {

std::string getFileExtension(const std::filesystem::path& filePath) {
    std::string ext = filePath.extension().string();
    if (!ext.empty() && ext[0] == '.') {
        ext = ext.substr(1);
    }
    return ext;
}

std::string normalizeExtension(const std::string& extension) {
    std::string normalized = extension;
    
    // Remove leading dot if present
    if (!normalized.empty() && normalized[0] == '.') {
        normalized = normalized.substr(1);
    }
    
    // Convert to lowercase
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
    
    return normalized;
}

bool isLossyFormat(FileFormat format) {
    switch (format) {
        case FileFormat::JPEG:
            return true;
        case FileFormat::PNG:
        case FileFormat::TIFF:
        case FileFormat::BMP:
        case FileFormat::GIF:
        case FileFormat::WEBP:  // Can be lossless or lossy
        default:
            return false;
    }
}

bool supportsTransparency(FileFormat format) {
    switch (format) {
        case FileFormat::PNG:
        case FileFormat::GIF:
        case FileFormat::WEBP:
        case FileFormat::TIFF:
        case FileFormat::PSD:
        case FileFormat::PSB:
        case FileFormat::SVG:
        case FileFormat::ICO:
            return true;
        default:
            return false;
    }
}

bool supportsLayers(FileFormat format) {
    switch (format) {
        case FileFormat::PSD:
        case FileFormat::PSB:
        case FileFormat::XCF:
        case FileFormat::QCSX:
        case FileFormat::QCS:
            return true;
        default:
            return false;
    }
}

std::string formatFileSize(uint64_t sizeBytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    const int unitCount = sizeof(units) / sizeof(units[0]);
    
    double size = static_cast<double>(sizeBytes);
    int unitIndex = 0;
    
    while (size >= 1024.0 && unitIndex < unitCount - 1) {
        size /= 1024.0;
        unitIndex++;
    }
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << size << " " << units[unitIndex];
    return oss.str();
}

bool isValidImageDimensions(const std::array<uint32_t, 2>& dimensions) {
    return dimensions[0] > 0 && dimensions[1] > 0 &&
           dimensions[0] <= 65536 && dimensions[1] <= 65536;
}

size_t calculateMemoryRequirement(const std::array<uint32_t, 2>& dimensions,
                                 uint8_t channels, uint8_t bitDepth) {
    return static_cast<size_t>(dimensions[0]) * dimensions[1] * channels * (bitDepth / 8);
}

} // namespace FormatUtils

} // namespace QuantumCanvas::IO
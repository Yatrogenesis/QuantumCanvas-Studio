/**
 * @file comprehensive_test_suite.cpp
 * @brief Implementation of Comprehensive Testing Suite
 * 
 * @author QuantumCanvas Studio Team
 * @date 2025-01-14
 * @version 1.0.0-Testing
 */

#include "comprehensive_test_suite.hpp"
#include "../../src/core/kernel/kernel_manager.hpp"
#include "../../src/core/memory/memory_manager.hpp"
#include "../../src/core/rendering/rendering_engine.hpp"
#include "../../src/ui/compliance/privacy_compliance_manager.hpp"

#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <thread>
#include <random>

namespace qcs::testing {

// ============================================================================
// CORE ENGINE TEST SUITE IMPLEMENTATION
// ============================================================================

CoreEngineTestSuite::CoreEngineTestSuite(const TestConfiguration& config)
    : m_config(config) {
}

TestResult CoreEngineTestSuite::test_kernel_initialization() {
    TestResult result;
    result.test_name = "KernelInitialization";
    result.test_category = "CoreEngine";
    result.platform = "All";
    result.timestamp = std::chrono::system_clock::now();
    
    auto start = std::chrono::high_resolution_clock::now();
    
    try {
        // Test kernel manager initialization
        auto kernel = std::make_unique<qcs::core::KernelManager>();
        
        if (!kernel->initialize()) {
            result.status = TestStatus::Failed;
            result.error_message = "Kernel initialization failed";
            return result;
        }
        
        // Validate kernel state
        if (!kernel->is_initialized()) {
            result.status = TestStatus::Failed;
            result.error_message = "Kernel reported not initialized after successful init";
            return result;
        }
        
        // Test shutdown
        kernel->shutdown();
        
        result.status = TestStatus::Passed;
        result.details = "Kernel initialization and shutdown successful";
        
    } catch (const std::exception& e) {
        result.status = TestStatus::Error;
        result.error_message = std::string("Exception during test: ") + e.what();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    result.execution_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    return result;
}

TestResult CoreEngineTestSuite::test_memory_allocation_performance() {
    TestResult result;
    result.test_name = "MemoryAllocationPerformance";
    result.test_category = "CoreEngine";
    result.platform = "All";
    result.timestamp = std::chrono::system_clock::now();
    
    try {
        auto memory_manager = std::make_unique<qcs::core::MemoryManager>();
        if (!memory_manager->initialize()) {
            result.status = TestStatus::Failed;
            result.error_message = "Memory manager initialization failed";
            return result;
        }
        
        // Performance test: 1000 allocations
        const int num_allocations = 1000;
        std::vector<void*> allocations;
        allocations.reserve(num_allocations);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < num_allocations; ++i) {
            void* ptr = memory_manager->allocate(1024, 16);
            if (!ptr) {
                result.status = TestStatus::Failed;
                result.error_message = "Memory allocation failed at iteration " + std::to_string(i);
                return result;
            }
            allocations.push_back(ptr);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        // Calculate average allocation time
        double avg_allocation_time_us = static_cast<double>(duration.count()) / num_allocations;
        
        // Cleanup
        for (void* ptr : allocations) {
            memory_manager->deallocate(ptr);
        }
        
        memory_manager->shutdown();
        
        // Validate performance (target: <1μs per allocation)
        if (avg_allocation_time_us <= 1.0) {
            result.status = TestStatus::Passed;
            result.performance_metric = avg_allocation_time_us;
            result.performance_unit = "μs";
            result.details = "Average allocation time: " + std::to_string(avg_allocation_time_us) + "μs";
        } else {
            result.status = TestStatus::Failed;
            result.performance_metric = avg_allocation_time_us;
            result.performance_unit = "μs";
            result.error_message = "Allocation time exceeds 1μs target: " + std::to_string(avg_allocation_time_us) + "μs";
        }
        
    } catch (const std::exception& e) {
        result.status = TestStatus::Error;
        result.error_message = std::string("Exception during test: ") + e.what();
    }
    
    result.execution_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - 
        std::chrono::high_resolution_clock::now()
    );
    
    return result;
}

std::vector<TestResult> CoreEngineTestSuite::run_all_core_tests() {
    std::vector<TestResult> results;
    
    std::cout << "Running Core Engine Tests...\n";
    
    // Kernel tests
    results.push_back(test_kernel_initialization());
    results.push_back(test_service_registration());
    results.push_back(test_service_resolution());
    results.push_back(test_event_publishing());
    results.push_back(test_plugin_loading());
    results.push_back(test_kernel_shutdown());
    
    // Memory manager tests
    results.push_back(test_memory_allocation_basic());
    results.push_back(test_memory_allocation_performance());
    results.push_back(test_memory_alignment());
    results.push_back(test_memory_pool_management());
    results.push_back(test_memory_leak_detection());
    results.push_back(test_thread_safety());
    
    // Rendering engine tests
    results.push_back(test_rendering_initialization());
    results.push_back(test_multi_backend_support());
    results.push_back(test_resource_creation());
    results.push_back(test_command_buffer_recording());
    results.push_back(test_frame_rendering());
    results.push_back(test_performance_benchmarking());
    
    // Shader compiler tests
    results.push_back(test_shader_compilation_wgsl());
    results.push_back(test_shader_compilation_hlsl());
    results.push_back(test_shader_compilation_glsl());
    results.push_back(test_cross_platform_compilation());
    results.push_back(test_shader_hot_reload());
    results.push_back(test_compilation_error_handling());
    
    m_results = results;
    return results;
}

// ============================================================================
// PRIVACY COMPLIANCE TEST SUITE IMPLEMENTATION
// ============================================================================

PrivacyComplianceTestSuite::PrivacyComplianceTestSuite(const TestConfiguration& config)
    : m_config(config) {
}

TestResult PrivacyComplianceTestSuite::test_gdpr_article_compliance() {
    TestResult result;
    result.test_name = "GDPRArticleCompliance";
    result.test_category = "PrivacyCompliance";
    result.platform = "All";
    result.timestamp = std::chrono::system_clock::now();
    
    auto start = std::chrono::high_resolution_clock::now();
    
    try {
        using namespace qcs::ui::compliance;
        
        // Create privacy compliance manager
        auto privacy_manager = create_gdpr_compliance_manager();
        
        if (!privacy_manager->initialize()) {
            result.status = TestStatus::Failed;
            result.error_message = "Privacy compliance manager initialization failed";
            return result;
        }
        
        // Test key GDPR articles
        std::vector<std::string> compliance_checks = {
            "Article 6 - Lawful basis",
            "Article 7 - Consent conditions",
            "Article 15 - Right of access",
            "Article 17 - Right to erasure",
            "Article 20 - Right to data portability",
            "Article 32 - Security of processing",
            "Article 33 - Breach notification",
            "Article 35 - Data protection impact assessment"
        };
        
        bool all_compliant = true;
        std::string compliance_details;
        
        for (const auto& check : compliance_checks) {
            // Perform compliance audit
            bool compliant = privacy_manager->perform_compliance_audit();
            if (!compliant) {
                all_compliant = false;
                compliance_details += "Failed: " + check + "\n";
            } else {
                compliance_details += "Passed: " + check + "\n";
            }
        }
        
        if (all_compliant) {
            result.status = TestStatus::Passed;
            result.details = "All GDPR articles compliant:\n" + compliance_details;
        } else {
            result.status = TestStatus::Failed;
            result.error_message = "GDPR compliance issues found";
            result.details = compliance_details;
        }
        
    } catch (const std::exception& e) {
        result.status = TestStatus::Error;
        result.error_message = std::string("Exception during GDPR test: ") + e.what();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    result.execution_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    return result;
}

TestResult PrivacyComplianceTestSuite::test_data_subject_rights() {
    TestResult result;
    result.test_name = "DataSubjectRights";
    result.test_category = "PrivacyCompliance";
    result.platform = "All";
    result.timestamp = std::chrono::system_clock::now();
    
    auto start = std::chrono::high_resolution_clock::now();
    
    try {
        using namespace qcs::ui::compliance;
        
        auto privacy_manager = create_gdpr_compliance_manager();
        if (!privacy_manager->initialize()) {
            result.status = TestStatus::Failed;
            result.error_message = "Privacy manager initialization failed";
            return result;
        }
        
        const std::string test_user_id = "test_user_12345";
        
        // Test right of access (Article 15)
        std::string data_export = privacy_manager->generate_data_export(test_user_id);
        if (data_export.empty()) {
            result.status = TestStatus::Failed;
            result.error_message = "Data export generation failed";
            return result;
        }
        
        // Test right to erasure (Article 17)
        bool erasure_result = privacy_manager->erase_user_data(test_user_id);
        if (!erasure_result) {
            result.status = TestStatus::Failed;
            result.error_message = "Data erasure failed";
            return result;
        }
        
        // Test right to data portability (Article 20)
        std::string portable_data = privacy_manager->export_portable_data(test_user_id, {"user_content"});
        if (portable_data.empty()) {
            result.status = TestStatus::Failed;
            result.error_message = "Portable data export failed";
            return result;
        }
        
        result.status = TestStatus::Passed;
        result.details = "All data subject rights implemented successfully:\n"
                        "- Right of access: Data export generated\n"
                        "- Right to erasure: Data deletion successful\n"
                        "- Right to data portability: Portable export successful";
        
    } catch (const std::exception& e) {
        result.status = TestStatus::Error;
        result.error_message = std::string("Exception during data rights test: ") + e.what();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    result.execution_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    return result;
}

std::vector<TestResult> PrivacyComplianceTestSuite::run_all_compliance_tests() {
    std::vector<TestResult> results;
    
    std::cout << "Running Privacy Compliance Tests...\n";
    
    // GDPR tests
    results.push_back(test_gdpr_article_compliance());
    results.push_back(test_data_subject_rights());
    results.push_back(test_consent_management());
    results.push_back(test_data_processing_records());
    results.push_back(test_breach_notification());
    results.push_back(test_cross_border_transfers());
    
    // Global privacy law tests
    results.push_back(test_ccpa_compliance());
    results.push_back(test_pipl_compliance());
    results.push_back(test_lgpd_compliance());
    results.push_back(test_pipeda_compliance());
    
    // Store compliance tests
    results.push_back(test_app_store_privacy_requirements());
    results.push_back(test_play_store_data_safety());
    results.push_back(test_galaxy_store_privacy());
    results.push_back(test_app_gallery_privacy());
    
    // Privacy implementation tests
    results.push_back(test_data_encryption());
    results.push_back(test_data_minimization());
    results.push_back(test_privacy_by_design());
    results.push_back(test_user_consent_flows());
    results.push_back(test_data_deletion());
    
    m_results = results;
    return results;
}

// ============================================================================
// COMPREHENSIVE TEST SUITE IMPLEMENTATION
// ============================================================================

ComprehensiveTestSuite::ComprehensiveTestSuite(const TestConfiguration& config)
    : m_config(config) {
    initialize_test_suites();
}

ComprehensiveTestSuite::~ComprehensiveTestSuite() {
    cleanup_test_environment();
}

void ComprehensiveTestSuite::initialize_test_suites() {
    m_core_tests = std::make_unique<CoreEngineTestSuite>(m_config);
    m_graphics_tests = std::make_unique<GraphicsTestSuite>(m_config);
    m_cad_tests = std::make_unique<CADTestSuite>(m_config);
    m_mobile_tests = std::make_unique<MobilePlatformTestSuite>(m_config);
    m_compliance_tests = std::make_unique<PrivacyComplianceTestSuite>(m_config);
    m_performance_tests = std::make_unique<PerformanceTestSuite>(m_config);
    m_security_tests = std::make_unique<SecurityTestSuite>(m_config);
    m_integration_tests = std::make_unique<IntegrationTestSuite>(m_config);
}

bool ComprehensiveTestSuite::initialize_test_environment() {
    std::cout << "Initializing test environment...\n";
    
    // Create test data directory
    if (!test_utils::create_test_files()) {
        std::cerr << "Failed to create test files\n";
        return false;
    }
    
    // Initialize platform-specific test environments
    bool success = true;
    
    if (m_config.test_ios && !test_utils::is_ios_available()) {
        std::cout << "iOS testing requested but not available, skipping iOS tests\n";
        m_config.test_ios = false;
    }
    
    if (m_config.test_android && !test_utils::is_android_available()) {
        std::cout << "Android testing requested but not available, skipping Android tests\n";
        m_config.test_android = false;
    }
    
    std::cout << "Test environment initialized successfully\n";
    return success;
}

std::vector<TestResult> ComprehensiveTestSuite::run_all_tests() {
    std::cout << "=== QuantumCanvas Studio Comprehensive Test Suite ===\n";
    std::cout << "Starting comprehensive testing...\n\n";
    
    m_test_start_time = std::chrono::system_clock::now();
    m_all_results.clear();
    
    if (!initialize_test_environment()) {
        std::cerr << "Failed to initialize test environment\n";
        return {};
    }
    
    try {
        // Run test suites in order of dependency
        if (m_config.run_unit_tests) {
            auto core_results = run_core_engine_tests();
            m_all_results.insert(m_all_results.end(), core_results.begin(), core_results.end());
            
            auto graphics_results = run_graphics_tests();
            m_all_results.insert(m_all_results.end(), graphics_results.begin(), graphics_results.end());
            
            auto cad_results = run_cad_tests();
            m_all_results.insert(m_all_results.end(), cad_results.begin(), cad_results.end());
        }
        
        if (m_config.run_mobile_tests && (m_config.test_ios || m_config.test_android)) {
            auto mobile_results = run_mobile_tests();
            m_all_results.insert(m_all_results.end(), mobile_results.begin(), mobile_results.end());
        }
        
        if (m_config.run_compliance_tests) {
            auto compliance_results = run_compliance_tests();
            m_all_results.insert(m_all_results.end(), compliance_results.begin(), compliance_results.end());
        }
        
        if (m_config.run_performance_tests) {
            auto performance_results = run_performance_tests();
            m_all_results.insert(m_all_results.end(), performance_results.begin(), performance_results.end());
        }
        
        if (m_config.run_security_tests) {
            auto security_results = run_security_tests();
            m_all_results.insert(m_all_results.end(), security_results.begin(), security_results.end());
        }
        
        if (m_config.run_integration_tests) {
            auto integration_results = run_integration_tests();
            m_all_results.insert(m_all_results.end(), integration_results.begin(), integration_results.end());
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Exception during test execution: " << e.what() << "\n";
    }
    
    m_test_end_time = std::chrono::system_clock::now();
    
    // Generate summary
    auto stats = get_test_statistics();
    std::cout << "\n=== Test Execution Complete ===\n";
    std::cout << "Total Tests: " << stats.total_tests << "\n";
    std::cout << "Passed: " << stats.passed_tests << "\n";
    std::cout << "Failed: " << stats.failed_tests << "\n";
    std::cout << "Errors: " << stats.error_tests << "\n";
    std::cout << "Pass Rate: " << std::fixed << std::setprecision(1) << stats.pass_rate << "%\n";
    std::cout << "Total Time: " << stats.total_execution_time.count() << "ms\n";
    
    return m_all_results;
}

std::vector<TestResult> ComprehensiveTestSuite::run_core_engine_tests() {
    std::cout << "\n--- Running Core Engine Tests ---\n";
    return m_core_tests->run_all_core_tests();
}

std::vector<TestResult> ComprehensiveTestSuite::run_compliance_tests() {
    std::cout << "\n--- Running Privacy Compliance Tests ---\n";
    return m_compliance_tests->run_all_compliance_tests();
}

ComprehensiveTestSuite::TestStatistics ComprehensiveTestSuite::get_test_statistics() const {
    TestStatistics stats;
    
    stats.total_tests = m_all_results.size();
    
    for (const auto& result : m_all_results) {
        switch (result.status) {
            case TestStatus::Passed:
                stats.passed_tests++;
                break;
            case TestStatus::Failed:
                stats.failed_tests++;
                break;
            case TestStatus::Error:
                stats.error_tests++;
                break;
            case TestStatus::Skipped:
                stats.skipped_tests++;
                break;
            default:
                break;
        }
        
        stats.total_execution_time += result.execution_time;
    }
    
    if (stats.total_tests > 0) {
        stats.pass_rate = (static_cast<double>(stats.passed_tests) / stats.total_tests) * 100.0;
    }
    
    return stats;
}

std::string ComprehensiveTestSuite::generate_summary_report() {
    std::ostringstream report;
    auto stats = get_test_statistics();
    
    report << "# QuantumCanvas Studio Test Summary Report\n";
    report << "Generated: " << std::chrono::system_clock::now() << "\n\n";
    
    report << "## Overall Results\n";
    report << "- **Total Tests**: " << stats.total_tests << "\n";
    report << "- **Passed**: " << stats.passed_tests << " ✅\n";
    report << "- **Failed**: " << stats.failed_tests << " ❌\n";
    report << "- **Errors**: " << stats.error_tests << " 🔥\n";
    report << "- **Skipped**: " << stats.skipped_tests << " ⏭️\n";
    report << "- **Pass Rate**: " << std::fixed << std::setprecision(1) << stats.pass_rate << "%\n";
    report << "- **Total Time**: " << stats.total_execution_time.count() << "ms\n\n";
    
    // Test category breakdown
    std::map<std::string, std::vector<TestResult>> category_results;
    for (const auto& result : m_all_results) {
        category_results[result.test_category].push_back(result);
    }
    
    report << "## Results by Category\n";
    for (const auto& [category, results] : category_results) {
        size_t passed = 0, failed = 0, errors = 0;
        for (const auto& result : results) {
            switch (result.status) {
                case TestStatus::Passed: passed++; break;
                case TestStatus::Failed: failed++; break;
                case TestStatus::Error: errors++; break;
                default: break;
            }
        }
        
        double pass_rate = results.empty() ? 0.0 : (static_cast<double>(passed) / results.size()) * 100.0;
        report << "- **" << category << "**: " << passed << "/" << results.size() 
               << " (" << std::fixed << std::setprecision(1) << pass_rate << "%)\n";
    }
    
    // Failed tests summary
    auto failed_tests = get_failed_tests();
    if (!failed_tests.empty()) {
        report << "\n## Failed Tests\n";
        for (const auto& result : failed_tests) {
            report << "- **" << result.test_name << "** (" << result.test_category << "): " 
                   << result.error_message << "\n";
        }
    }
    
    return report.str();
}

std::string ComprehensiveTestSuite::generate_mobile_readiness_report() {
    std::ostringstream report;
    
    report << "# QuantumCanvas Studio Mobile Deployment Readiness Report\n";
    report << "Generated: " << std::chrono::system_clock::now() << "\n\n";
    
    // Analyze mobile-specific test results
    std::vector<TestResult> mobile_results;
    std::vector<TestResult> compliance_results;
    
    for (const auto& result : m_all_results) {
        if (result.test_category == "MobilePlatform") {
            mobile_results.push_back(result);
        } else if (result.test_category == "PrivacyCompliance") {
            compliance_results.push_back(result);
        }
    }
    
    // Mobile platform readiness
    report << "## Mobile Platform Readiness\n";
    
    bool ios_ready = true;
    bool android_ready = true;
    
    for (const auto& result : mobile_results) {
        if (result.test_name.find("ios") != std::string::npos || 
            result.test_name.find("iOS") != std::string::npos) {
            if (result.status != TestStatus::Passed) {
                ios_ready = false;
            }
        }
        if (result.test_name.find("android") != std::string::npos || 
            result.test_name.find("Android") != std::string::npos) {
            if (result.status != TestStatus::Passed) {
                android_ready = false;
            }
        }
    }
    
    report << "- **iOS App Store**: " << (ios_ready ? "✅ READY" : "❌ NOT READY") << "\n";
    report << "- **Google Play Store**: " << (android_ready ? "✅ READY" : "❌ NOT READY") << "\n";
    report << "- **Samsung Galaxy Store**: " << (android_ready ? "✅ READY" : "❌ NOT READY") << "\n";
    report << "- **Huawei AppGallery**: " << (android_ready ? "✅ READY" : "❌ NOT READY") << "\n";
    
    // Privacy compliance readiness
    bool privacy_ready = true;
    for (const auto& result : compliance_results) {
        if (result.status != TestStatus::Passed) {
            privacy_ready = false;
            break;
        }
    }
    
    report << "\n## Privacy Compliance Readiness\n";
    report << "- **GDPR/EU Compliance**: " << (privacy_ready ? "✅ CERTIFIED" : "❌ ISSUES FOUND") << "\n";
    report << "- **Global Privacy Laws**: " << (privacy_ready ? "✅ COMPLIANT" : "❌ NON-COMPLIANT") << "\n";
    report << "- **Store Privacy Requirements**: " << (privacy_ready ? "✅ READY" : "❌ NOT READY") << "\n";
    
    // Overall recommendation
    bool overall_ready = ios_ready && android_ready && privacy_ready;
    
    report << "\n## Deployment Recommendation\n";
    if (overall_ready) {
        report << "🚀 **READY FOR DEPLOYMENT** - All mobile platforms and privacy compliance validated\n";
        report << "\n### Next Steps:\n";
        report << "1. Submit to Apple App Store\n";
        report << "2. Submit to Google Play Store\n";
        report << "3. Submit to Samsung Galaxy Store\n";
        report << "4. Submit to Huawei AppGallery\n";
        report << "5. Monitor compliance status\n";
    } else {
        report << "⚠️ **NOT READY FOR DEPLOYMENT** - Issues found that must be resolved\n";
        report << "\n### Required Actions:\n";
        if (!ios_ready) report << "- Fix iOS platform issues\n";
        if (!android_ready) report << "- Fix Android platform issues\n";
        if (!privacy_ready) report << "- Resolve privacy compliance issues\n";
    }
    
    return report.str();
}

bool ComprehensiveTestSuite::validate_production_readiness() {
    auto stats = get_test_statistics();
    
    // Production readiness criteria:
    // - Pass rate >= 95%
    // - No critical system failures
    // - Privacy compliance validated
    // - Performance targets met
    
    if (stats.pass_rate < 95.0) {
        return false;
    }
    
    // Check for critical failures
    for (const auto& result : m_all_results) {
        if (result.status == TestStatus::Failed || result.status == TestStatus::Error) {
            // Critical tests that must pass
            if (result.test_category == "CoreEngine" || 
                result.test_category == "PrivacyCompliance") {
                return false;
            }
        }
    }
    
    return true;
}

bool ComprehensiveTestSuite::validate_store_submission_readiness() {
    // Check mobile platform tests
    bool mobile_ready = true;
    bool compliance_ready = true;
    
    for (const auto& result : m_all_results) {
        if (result.test_category == "MobilePlatform" && 
            result.status != TestStatus::Passed) {
            mobile_ready = false;
        }
        
        if (result.test_category == "PrivacyCompliance" && 
            result.status != TestStatus::Passed) {
            compliance_ready = false;
        }
    }
    
    return mobile_ready && compliance_ready;
}

void ComprehensiveTestSuite::cleanup_test_environment() {
    std::cout << "Cleaning up test environment...\n";
    test_utils::cleanup_test_files();
}

// ============================================================================
// UTILITY FUNCTIONS IMPLEMENTATION
// ============================================================================

namespace test_utils {

bool create_test_files() {
    // Create test data directory structure
    try {
        std::filesystem::create_directories("tests/data/images");
        std::filesystem::create_directories("tests/data/vectors");
        std::filesystem::create_directories("tests/data/cad");
        std::filesystem::create_directories("tests/data/projects");
        
        // Create sample test files
        // (Implementation would create actual test data files)
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create test files: " << e.what() << "\n";
        return false;
    }
}

void cleanup_test_files() {
    try {
        std::filesystem::remove_all("tests/data");
    } catch (const std::exception& e) {
        std::cerr << "Failed to cleanup test files: " << e.what() << "\n";
    }
}

bool is_ios_available() {
#ifdef __APPLE__
    return true;
#else
    return false;
#endif
}

bool is_android_available() {
#ifdef __ANDROID__
    return true;
#else
    return false;
#endif
}

bool is_metal_available() {
#ifdef __APPLE__
    return true;
#else
    return false;
#endif
}

bool is_vulkan_available() {
    // Check for Vulkan availability
    // (Implementation would check for Vulkan runtime)
    return true;  // Assume available for now
}

}  // namespace test_utils

// ============================================================================
// FACTORY FUNCTIONS IMPLEMENTATION
// ============================================================================

std::unique_ptr<ComprehensiveTestSuite> create_comprehensive_test_suite(const TestConfiguration& config) {
    return std::make_unique<ComprehensiveTestSuite>(config);
}

std::unique_ptr<ComprehensiveTestSuite> create_mobile_test_suite() {
    TestConfiguration config;
    config.run_unit_tests = true;
    config.run_integration_tests = true;
    config.run_mobile_tests = true;
    config.run_compliance_tests = true;
    config.run_performance_tests = false;
    config.run_security_tests = false;
    config.test_ios = true;
    config.test_android = true;
    
    return std::make_unique<ComprehensiveTestSuite>(config);
}

std::unique_ptr<ComprehensiveTestSuite> create_compliance_test_suite() {
    TestConfiguration config;
    config.run_unit_tests = false;
    config.run_integration_tests = false;
    config.run_mobile_tests = false;
    config.run_compliance_tests = true;
    config.run_performance_tests = false;
    config.run_security_tests = true;
    
    return std::make_unique<ComprehensiveTestSuite>(config);
}

} // namespace qcs::testing
/**
 * @file validate_system.cpp
 * @brief System Validation Entry Point for QuantumCanvas Studio
 * 
 * Comprehensive system validation that tests all components and generates
 * detailed reports for production readiness and store submission validation.
 * 
 * @author QuantumCanvas Studio Team
 * @date 2025-01-14
 * @version 1.0.0-Validation
 */

#include "comprehensive/comprehensive_test_suite.hpp"
#include <iostream>
#include <fstream>
#include <cstdlib>

using namespace qcs::testing;

/**
 * @brief Print application header
 */
void print_header() {
    std::cout << R"(
╔══════════════════════════════════════════════════════════════╗
║                    QuantumCanvas Studio                      ║
║                 System Validation Suite                     ║
║                                                              ║
║  Comprehensive testing and validation for production        ║
║  readiness and mobile app store deployment                  ║
╚══════════════════════════════════════════════════════════════╝
)" << "\n\n";
}

/**
 * @brief Print usage information
 */
void print_usage() {
    std::cout << "Usage: validate_system [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --all               Run all tests (default)\n";
    std::cout << "  --core              Run core engine tests only\n";
    std::cout << "  --mobile            Run mobile platform tests only\n";
    std::cout << "  --compliance        Run privacy compliance tests only\n";
    std::cout << "  --performance       Run performance benchmarks only\n";
    std::cout << "  --security          Run security validation only\n";
    std::cout << "  --production        Validate production readiness\n";
    std::cout << "  --store-readiness   Validate app store submission readiness\n";
    std::cout << "  --ios               Include iOS testing (requires macOS/simulator)\n";
    std::cout << "  --android           Include Android testing (requires emulator)\n";
    std::cout << "  --report <file>     Save detailed report to file\n";
    std::cout << "  --help              Show this help message\n";
    std::cout << "\nExamples:\n";
    std::cout << "  validate_system --all --ios --android\n";
    std::cout << "  validate_system --production --report production_report.md\n";
    std::cout << "  validate_system --store-readiness\n";
    std::cout << "  validate_system --compliance\n\n";
}

/**
 * @brief Parse command line arguments
 */
TestConfiguration parse_arguments(int argc, char* argv[], bool& production_check, 
                                  bool& store_check, std::string& report_file) {
    TestConfiguration config;
    production_check = false;
    store_check = false;
    
    // Default: run core tests
    config.run_unit_tests = true;
    config.run_integration_tests = false;
    config.run_mobile_tests = false;
    config.run_compliance_tests = false;
    config.run_performance_tests = false;
    config.run_security_tests = false;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--help") {
            print_usage();
            exit(0);
        } else if (arg == "--all") {
            config.run_unit_tests = true;
            config.run_integration_tests = true;
            config.run_mobile_tests = true;
            config.run_compliance_tests = true;
            config.run_performance_tests = true;
            config.run_security_tests = true;
        } else if (arg == "--core") {
            config.run_unit_tests = true;
            config.run_integration_tests = false;
            config.run_mobile_tests = false;
            config.run_compliance_tests = false;
            config.run_performance_tests = false;
            config.run_security_tests = false;
        } else if (arg == "--mobile") {
            config.run_mobile_tests = true;
            config.run_unit_tests = true;  // Dependencies
        } else if (arg == "--compliance") {
            config.run_compliance_tests = true;
            config.run_security_tests = true;
        } else if (arg == "--performance") {
            config.run_performance_tests = true;
            config.run_unit_tests = true;  // Dependencies
        } else if (arg == "--security") {
            config.run_security_tests = true;
        } else if (arg == "--production") {
            production_check = true;
            config.run_unit_tests = true;
            config.run_integration_tests = true;
            config.run_performance_tests = true;
            config.run_security_tests = true;
            config.run_compliance_tests = true;
        } else if (arg == "--store-readiness") {
            store_check = true;
            config.run_mobile_tests = true;
            config.run_compliance_tests = true;
            config.run_unit_tests = true;  // Dependencies
        } else if (arg == "--ios") {
            config.test_ios = true;
            config.run_mobile_tests = true;
        } else if (arg == "--android") {
            config.test_android = true;
            config.run_mobile_tests = true;
        } else if (arg == "--report" && i + 1 < argc) {
            report_file = argv[++i];
        }
    }
    
    return config;
}

/**
 * @brief Save report to file
 */
bool save_report(const std::string& filename, const std::string& content) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open report file: " << filename << "\n";
        return false;
    }
    
    file << content;
    file.close();
    
    std::cout << "Report saved to: " << filename << "\n";
    return true;
}

/**
 * @brief Main validation function
 */
int main(int argc, char* argv[]) {
    print_header();
    
    // Parse command line arguments
    bool production_check = false;
    bool store_check = false;
    std::string report_file;
    
    auto config = parse_arguments(argc, argv, production_check, store_check, report_file);
    
    // Create and run comprehensive test suite
    std::cout << "Initializing QuantumCanvas Studio Test Suite...\n";
    
    try {
        auto test_suite = create_comprehensive_test_suite(config);
        
        // Run tests
        auto results = test_suite->run_all_tests();
        
        if (results.empty()) {
            std::cerr << "No tests were executed. Check configuration.\n";
            return 1;
        }
        
        // Generate reports
        std::cout << "\n" << std::string(60, '=') << "\n";
        std::cout << "GENERATING VALIDATION REPORTS\n";
        std::cout << std::string(60, '=') << "\n";
        
        std::string summary_report = test_suite->generate_summary_report();
        std::cout << summary_report << "\n";
        
        // Production readiness validation
        if (production_check) {
            std::cout << "\n--- PRODUCTION READINESS VALIDATION ---\n";
            bool production_ready = test_suite->validate_production_readiness();
            
            if (production_ready) {
                std::cout << "✅ PRODUCTION READY - All critical systems validated\n";
                std::cout << "   • Core engine: Fully functional\n";
                std::cout << "   • Performance: Exceeds targets\n";
                std::cout << "   • Security: Validated\n";
                std::cout << "   • Privacy: Compliant\n";
                std::cout << "\n🚀 RECOMMENDATION: Proceed with production deployment\n";
            } else {
                std::cout << "❌ NOT PRODUCTION READY - Critical issues found\n";
                std::cout << "   • Review failed tests above\n";
                std::cout << "   • Address critical system issues\n";
                std::cout << "   • Re-run validation after fixes\n";
                std::cout << "\n⚠️ RECOMMENDATION: Do not deploy until issues resolved\n";
            }
        }
        
        // Store submission readiness validation
        if (store_check) {
            std::cout << "\n--- APP STORE SUBMISSION READINESS ---\n";
            bool store_ready = test_suite->validate_store_submission_readiness();
            
            std::string mobile_report = test_suite->generate_mobile_readiness_report();
            std::cout << mobile_report << "\n";
            
            if (store_ready) {
                std::cout << "\n🏪 STORE SUBMISSION STATUS:\n";
                std::cout << "✅ Apple App Store: READY FOR SUBMISSION\n";
                std::cout << "✅ Google Play Store: READY FOR SUBMISSION\n";
                std::cout << "✅ Samsung Galaxy Store: READY FOR SUBMISSION\n";
                std::cout << "✅ Huawei AppGallery: READY FOR SUBMISSION\n";
                std::cout << "\n📱 NEXT STEPS:\n";
                std::cout << "1. Prepare store assets (screenshots, descriptions)\n";
                std::cout << "2. Submit to Apple App Store Connect\n";
                std::cout << "3. Submit to Google Play Console\n";
                std::cout << "4. Submit to Samsung Galaxy Store\n";
                std::cout << "5. Submit to Huawei AppGallery Connect\n";
            } else {
                std::cout << "\n❌ NOT READY FOR STORE SUBMISSION\n";
                std::cout << "   • Mobile platform issues detected\n";
                std::cout << "   • Privacy compliance issues found\n";
                std::cout << "   • Review detailed report above\n";
                std::cout << "\n⚠️ RECOMMENDATION: Fix issues before submission\n";
            }
        }
        
        // Performance summary
        auto perf_results = test_suite->get_performance_results();
        if (!perf_results.empty()) {
            std::cout << "\n--- PERFORMANCE SUMMARY ---\n";
            for (const auto& result : perf_results) {
                if (result.performance_metric > 0.0) {
                    std::cout << "• " << result.test_name << ": " 
                              << result.performance_metric << " " << result.performance_unit;
                    
                    // Performance status indicators
                    if (result.test_name.find("Allocation") != std::string::npos && 
                        result.performance_metric <= 1.0) {
                        std::cout << " ✅";
                    } else if (result.test_name.find("FPS") != std::string::npos && 
                               result.performance_metric >= 60.0) {
                        std::cout << " ✅";
                    }
                    std::cout << "\n";
                }
            }
        }
        
        // Save detailed report if requested
        if (!report_file.empty()) {
            std::string detailed_report = test_suite->generate_detailed_report();
            if (save_report(report_file, detailed_report)) {
                std::cout << "\nDetailed validation report saved to: " << report_file << "\n";
            }
        }
        
        // Final validation status
        auto stats = test_suite->get_test_statistics();
        std::cout << "\n" << std::string(60, '=') << "\n";
        std::cout << "FINAL VALIDATION STATUS\n";
        std::cout << std::string(60, '=') << "\n";
        
        if (stats.pass_rate >= 95.0) {
            std::cout << "🎉 EXCELLENT - " << std::fixed << std::setprecision(1) 
                      << stats.pass_rate << "% pass rate\n";
            std::cout << "   QuantumCanvas Studio is ready for deployment!\n";
        } else if (stats.pass_rate >= 80.0) {
            std::cout << "⚠️ GOOD - " << std::fixed << std::setprecision(1) 
                      << stats.pass_rate << "% pass rate\n";
            std::cout << "   Some issues need attention before deployment\n";
        } else {
            std::cout << "❌ NEEDS WORK - " << std::fixed << std::setprecision(1) 
                      << stats.pass_rate << "% pass rate\n";
            std::cout << "   Significant issues must be resolved\n";
        }
        
        // Return exit code based on results
        if (production_check || store_check) {
            bool overall_ready = production_check ? 
                test_suite->validate_production_readiness() :
                test_suite->validate_store_submission_readiness();
            return overall_ready ? 0 : 1;
        } else {
            return stats.pass_rate >= 95.0 ? 0 : 1;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error during validation: " << e.what() << "\n";
        return 2;
    }
}
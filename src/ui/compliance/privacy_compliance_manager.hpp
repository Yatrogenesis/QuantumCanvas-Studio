/**
 * @file privacy_compliance_manager.hpp
 * @brief Universal Privacy Compliance Manager for QuantumCanvas Studio
 * 
 * Implements comprehensive privacy compliance for all platforms:
 * - GDPR/EU General Data Protection Regulation (EU 2016/679)
 * - CCPA/CPRA California Consumer Privacy Act
 * - iOS App Tracking Transparency (ATT)
 * - Android Privacy Dashboard & Granular Permissions
 * - PlayStore Data Safety Requirements
 * - AppStore Privacy Manifest Requirements
 * - Samsung Galaxy Store Privacy Policy
 * - Huawei AppGallery Privacy Compliance
 * - UK GDPR (post-Brexit modifications)
 * - Brazilian LGPD Lei Geral de Proteção de Dados
 * 
 * @author QuantumCanvas Team
 * @date 2025-01-14
 * @version 1.0.0-Privacy
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <chrono>
#include <optional>

namespace qcs::ui::compliance {

// ============================================================================
// GDPR/EU PRIVACY COMPLIANCE FRAMEWORK
// ============================================================================

/**
 * @brief GDPR Article 6 - Lawful bases for processing
 */
enum class LawfulBasis {
    Consent = 1,                    // Article 6(1)(a) - Explicit consent
    Contract = 2,                   // Article 6(1)(b) - Contract performance
    LegalObligation = 3,            // Article 6(1)(c) - Legal obligation
    VitalInterests = 4,             // Article 6(1)(d) - Vital interests
    PublicTask = 5,                 // Article 6(1)(e) - Public task
    LegitimateInterests = 6         // Article 6(1)(f) - Legitimate interests
};

/**
 * @brief GDPR Article 9 - Special categories of personal data
 */
enum class SpecialCategoryData {
    None = 0,
    RacialEthnicOrigin = 1,
    PoliticalOpinions = 2,
    ReligiousBeliefs = 3,
    TradeUnionMembership = 4,
    Genetic = 5,
    Biometric = 6,
    Health = 7,
    SexLife = 8,
    SexualOrientation = 9
};

/**
 * @brief Data subject rights under GDPR
 */
enum class DataSubjectRights {
    Information = 1,                // Articles 13-14 - Right to information
    Access = 2,                     // Article 15 - Right of access
    Rectification = 3,              // Article 16 - Right to rectification
    Erasure = 4,                    // Article 17 - Right to erasure ("right to be forgotten")
    RestrictProcessing = 5,         // Article 18 - Right to restriction of processing
    DataPortability = 6,            // Article 20 - Right to data portability
    Object = 7,                     // Article 21 - Right to object
    AutomatedDecisionMaking = 8     // Article 22 - Automated individual decision-making
};

/**
 * @brief Personal data categories processed by QuantumCanvas Studio
 */
struct PersonalDataCategory {
    std::string name;
    std::string description;
    LawfulBasis lawful_basis;
    std::vector<SpecialCategoryData> special_categories;
    std::string purpose;
    std::chrono::days retention_period;
    bool requires_consent = false;
    bool is_essential_for_service = false;
    std::vector<std::string> third_party_recipients;
    std::string transfer_mechanism;  // For international transfers
    
    // GDPR Article 30 - Records of processing activities
    std::string controller_name = "QuantumCanvas Studio";
    std::string controller_contact;
    std::string dpo_contact;        // Data Protection Officer
    std::vector<std::string> joint_controllers;
    std::string processor_name;
    std::vector<std::string> categories_of_recipients;
    std::optional<std::string> third_country_transfer;
    std::string safeguards_description;
    std::chrono::system_clock::time_point time_limit_erasure;
    std::string technical_organizational_measures;
};

/**
 * @brief Consent record for GDPR compliance
 */
struct ConsentRecord {
    std::string user_id;
    std::string data_category;
    bool consent_given = false;
    std::chrono::system_clock::time_point timestamp;
    std::string consent_method;     // "explicit_click", "opt_in_checkbox", etc.
    std::string consent_text;       // Exact text shown to user
    std::string version;            // Privacy policy version
    std::string ip_address;         // For audit trail
    std::string user_agent;         // Browser/app information
    std::optional<std::chrono::system_clock::time_point> withdrawal_timestamp;
    std::string withdrawal_method;
    bool is_valid = true;
    
    // Additional GDPR requirements
    bool freely_given = true;       // GDPR Article 4(11)
    bool specific = true;
    bool informed = true;
    bool unambiguous = true;
    std::string evidence_of_consent;
};

/**
 * @brief Data breach notification (GDPR Article 33-34)
 */
struct DataBreachRecord {
    std::string breach_id;
    std::chrono::system_clock::time_point occurrence_time;
    std::chrono::system_clock::time_point discovery_time;
    std::string breach_type;        // "confidentiality", "integrity", "availability"
    std::string description;
    std::vector<std::string> affected_data_categories;
    size_t approximate_number_of_individuals;
    size_t approximate_number_of_records;
    
    // Risk assessment
    enum class RiskLevel {
        Low,
        High,
        VeryHigh
    } risk_level = RiskLevel::Low;
    
    std::string likely_consequences;
    std::string measures_taken;
    std::string measures_proposed;
    
    // Notification status
    bool supervisory_authority_notified = false;
    std::chrono::system_clock::time_point authority_notification_time;
    bool data_subjects_notified = false;
    std::chrono::system_clock::time_point subjects_notification_time;
    std::string notification_method;
    
    std::string dpo_assessment;
    bool notification_delay_justified = false;
    std::string delay_justification;
};

// ============================================================================
// PLATFORM-SPECIFIC PRIVACY COMPLIANCE
// ============================================================================

/**
 * @brief iOS App Tracking Transparency (ATT) compliance
 */
struct IOSATTCompliance {
    bool att_framework_integrated = false;
    std::string att_request_reason;
    bool show_pre_permission_explanation = true;
    std::string pre_permission_text;
    bool tracking_authorized = false;
    std::chrono::system_clock::time_point authorization_timestamp;
    
    // NSUserTrackingUsageDescription requirement
    std::string tracking_usage_description;
    
    enum class ATTStatus {
        NotDetermined = 0,
        Restricted = 1,
        Denied = 2,
        Authorized = 3
    } current_status = ATTStatus::NotDetermined;
    
    // SKAdNetwork compliance for attribution
    bool skadnetwork_configured = false;
    std::vector<std::string> skadnetwork_ids;
};

/**
 * @brief Android Privacy Dashboard & Granular Permissions compliance
 */
struct AndroidPrivacyCompliance {
    // Android 13+ granular media permissions
    struct MediaPermissions {
        bool read_media_images = false;
        bool read_media_video = false;
        bool read_media_audio = false;
        bool read_external_storage_legacy = false;  // Legacy permission
    } media_permissions;
    
    // Privacy Dashboard requirements
    struct PrivacyDashboard {
        bool data_access_auditing_enabled = true;
        std::string data_safety_form_submitted;
        bool privacy_policy_linked = true;
        std::string privacy_policy_url;
        bool data_deletion_instructions_provided = true;
        std::string data_deletion_url;
    } privacy_dashboard;
    
    // Scoped Storage compliance (Android 10+)
    bool scoped_storage_compliant = true;
    bool requests_all_files_access = false;        // Requires Play Console justification
    std::string all_files_access_justification;
    
    // Runtime permissions model
    std::unordered_map<std::string, bool> runtime_permissions;
    std::unordered_map<std::string, std::string> permission_rationales;
};

/**
 * @brief Multi-store compliance configuration
 */
struct StorePrivacyCompliance {
    // Google Play Store
    struct PlayStoreCompliance {
        bool data_safety_form_completed = false;
        std::string data_safety_form_version;
        bool privacy_policy_approved = false;
        bool sensitive_permissions_justified = false;
        std::vector<std::string> sensitive_permissions_list;
        bool families_policy_compliant = true;      // If targeting children
        bool restricted_use_compliant = true;       // User Data policy
    } play_store;
    
    // Samsung Galaxy Store
    struct GalaxyStoreCompliance {
        bool samsung_privacy_policy_compliant = false;
        bool knox_security_reviewed = false;
        bool one_ui_integration_approved = false;
        std::string galaxy_store_content_rating;
    } galaxy_store;
    
    // Huawei AppGallery
    struct AppGalleryCompliance {
        bool huawei_privacy_policy_compliant = false;
        bool hms_integration_approved = false;
        bool china_cybersecurity_law_compliant = false;
        bool pipl_compliance_verified = false;      // Personal Information Protection Law
        std::string appgallery_content_rating;
    } app_gallery;
    
    // Apple App Store (additional to ATT)
    struct AppStoreCompliance {
        bool privacy_manifest_submitted = false;
        std::string privacy_manifest_version;
        bool privacy_nutrition_labels_accurate = false;
        bool third_party_sdk_disclosures_complete = false;
        std::vector<std::string> required_reason_apis_justified;
    } app_store;
};

// ============================================================================
// PRIVACY COMPLIANCE MANAGER
// ============================================================================

/**
 * @brief Comprehensive Privacy Compliance Manager
 */
class PrivacyComplianceManager {
public:
    /**
     * @brief Privacy compliance configuration
     */
    struct ComplianceConfig {
        // Regional compliance requirements
        bool gdpr_compliance_required = true;       // EU/EEA
        bool uk_gdpr_compliance_required = false;   // UK post-Brexit
        bool ccpa_compliance_required = false;      // California, USA
        bool lgpd_compliance_required = false;      // Brazil
        bool pipl_compliance_required = false;      // China
        bool pipeda_compliance_required = false;    // Canada
        
        // Platform-specific compliance
        IOSATTCompliance ios_att;
        AndroidPrivacyCompliance android_privacy;
        StorePrivacyCompliance store_compliance;
        
        // Data Protection Officer information
        struct DPOInfo {
            std::string name;
            std::string email;
            std::string phone;
            std::string address;
            bool required = false;              // Required for certain types of processing
        } dpo_info;
        
        // Company information
        struct ControllerInfo {
            std::string company_name = "QuantumCanvas Studio";
            std::string legal_address;
            std::string contact_email;
            std::string privacy_officer_email;
            std::string representative_eu_name;    // For non-EU controllers
            std::string representative_eu_address;
        } controller_info;
        
        // Technical and organizational measures (GDPR Article 32)
        struct SecurityMeasures {
            bool encryption_at_rest = true;
            bool encryption_in_transit = true;
            bool pseudonymization_implemented = false;
            bool access_controls_implemented = true;
            bool audit_logging_enabled = true;
            bool backup_security_verified = true;
            bool incident_response_plan_active = true;
            std::string security_certification;    // ISO 27001, SOC 2, etc.
        } security_measures;
    };
    
private:
    ComplianceConfig m_config;
    std::vector<PersonalDataCategory> m_data_categories;
    std::unordered_map<std::string, ConsentRecord> m_consent_records;
    std::vector<DataBreachRecord> m_breach_records;
    
    // Compliance status tracking
    struct ComplianceStatus {
        bool gdpr_compliant = false;
        bool ccpa_compliant = false;
        bool ios_att_compliant = false;
        bool android_privacy_compliant = false;
        std::chrono::system_clock::time_point last_audit;
        std::vector<std::string> outstanding_issues;
        std::vector<std::string> recommendations;
    } m_compliance_status;
    
    // Data processing activities log (GDPR Article 30)
    struct ProcessingActivity {
        std::string activity_id;
        std::string name;
        std::string purpose;
        std::vector<std::string> categories_of_data_subjects;
        std::vector<PersonalDataCategory> personal_data_categories;
        std::vector<std::string> recipients;
        std::optional<std::string> third_country_transfers;
        std::string time_limits;
        std::string security_measures;
        std::chrono::system_clock::time_point created;
        std::chrono::system_clock::time_point last_updated;
    };
    std::vector<ProcessingActivity> m_processing_activities;
    
    // Callbacks for platform-specific implementations
    std::function<void(const std::string&)> m_consent_ui_callback;
    std::function<void(DataSubjectRights, const std::string&)> m_rights_request_callback;
    std::function<void(const DataBreachRecord&)> m_breach_notification_callback;

public:
    explicit PrivacyComplianceManager(const ComplianceConfig& config = {});
    virtual ~PrivacyComplianceManager();
    
    // ========================================================================
    // INITIALIZATION & CONFIGURATION
    // ========================================================================
    
    bool initialize();
    void shutdown();
    bool configure_compliance(const ComplianceConfig& config);
    ComplianceConfig get_configuration() const { return m_config; }
    
    // ========================================================================
    // DATA CATEGORY MANAGEMENT (GDPR Article 30)
    // ========================================================================
    
    void register_data_category(const PersonalDataCategory& category);
    void update_data_category(const std::string& name, const PersonalDataCategory& category);
    void remove_data_category(const std::string& name);
    std::vector<PersonalDataCategory> get_all_data_categories() const;
    std::optional<PersonalDataCategory> get_data_category(const std::string& name) const;
    
    // ========================================================================
    // CONSENT MANAGEMENT (GDPR Articles 7, 8)
    // ========================================================================
    
    bool request_consent(const std::string& user_id, const std::string& data_category, const std::string& consent_text);
    bool withdraw_consent(const std::string& user_id, const std::string& data_category);
    bool has_valid_consent(const std::string& user_id, const std::string& data_category) const;
    ConsentRecord get_consent_record(const std::string& user_id, const std::string& data_category) const;
    std::vector<ConsentRecord> get_all_consents(const std::string& user_id) const;
    
    // Consent validation (GDPR Article 4(11) requirements)
    bool validate_consent_freely_given(const ConsentRecord& record) const;
    bool validate_consent_specific(const ConsentRecord& record) const;
    bool validate_consent_informed(const ConsentRecord& record) const;
    bool validate_consent_unambiguous(const ConsentRecord& record) const;
    
    // ========================================================================
    // DATA SUBJECT RIGHTS (GDPR Articles 15-22)
    // ========================================================================
    
    // Right of access (Article 15)
    std::string generate_data_export(const std::string& user_id);
    std::vector<PersonalDataCategory> get_user_data_categories(const std::string& user_id) const;
    
    // Right to rectification (Article 16)
    bool rectify_user_data(const std::string& user_id, const std::string& data_category, const std::string& corrected_data);
    
    // Right to erasure - "Right to be forgotten" (Article 17)
    bool erase_user_data(const std::string& user_id, const std::vector<std::string>& categories = {});
    bool can_erase_data(const std::string& user_id, const std::string& data_category) const;
    
    // Right to restriction of processing (Article 18)
    bool restrict_processing(const std::string& user_id, const std::string& data_category, const std::string& reason);
    bool lift_processing_restriction(const std::string& user_id, const std::string& data_category);
    
    // Right to data portability (Article 20)
    std::string export_portable_data(const std::string& user_id, const std::vector<std::string>& categories);
    bool import_portable_data(const std::string& user_id, const std::string& portable_data);
    
    // Right to object (Article 21)
    bool object_to_processing(const std::string& user_id, const std::string& data_category, const std::string& reason);
    
    // ========================================================================
    // DATA BREACH MANAGEMENT (GDPR Articles 33-34)
    // ========================================================================
    
    void report_data_breach(const DataBreachRecord& breach);
    bool requires_supervisory_authority_notification(const DataBreachRecord& breach) const;
    bool requires_data_subject_notification(const DataBreachRecord& breach) const;
    void notify_supervisory_authority(const std::string& breach_id);
    void notify_data_subjects(const std::string& breach_id);
    std::vector<DataBreachRecord> get_breach_records() const;
    
    // ========================================================================
    // PLATFORM-SPECIFIC COMPLIANCE
    // ========================================================================
    
    // iOS App Tracking Transparency
    void configure_ios_att(const IOSATTCompliance& att_config);
    bool request_ios_tracking_authorization();
    IOSATTCompliance::ATTStatus get_ios_tracking_status() const;
    
    // Android Privacy Compliance
    void configure_android_privacy(const AndroidPrivacyCompliance& privacy_config);
    bool request_android_permissions(const std::vector<std::string>& permissions);
    bool check_android_permission(const std::string& permission) const;
    
    // Store compliance verification
    bool verify_play_store_compliance();
    bool verify_app_store_compliance();
    bool verify_galaxy_store_compliance();
    bool verify_app_gallery_compliance();
    
    // ========================================================================
    // COMPLIANCE MONITORING & REPORTING
    // ========================================================================
    
    bool perform_compliance_audit();
    ComplianceStatus get_compliance_status() const { return m_compliance_status; }
    std::vector<std::string> get_compliance_issues() const;
    std::string generate_compliance_report() const;
    std::string generate_privacy_impact_assessment() const;  // GDPR Article 35
    
    // Records of processing activities (GDPR Article 30)
    void register_processing_activity(const ProcessingActivity& activity);
    std::vector<ProcessingActivity> get_processing_activities() const;
    std::string generate_processing_record() const;
    
    // ========================================================================
    // PRIVACY POLICY & TRANSPARENCY
    // ========================================================================
    
    std::string generate_privacy_notice() const;            // GDPR Articles 13-14
    std::string generate_cookie_policy() const;
    std::string generate_data_retention_policy() const;
    std::string generate_children_privacy_policy() const;   // If applicable
    
    // ========================================================================
    // CALLBACKS & INTEGRATION
    // ========================================================================
    
    void set_consent_ui_callback(std::function<void(const std::string&)> callback);
    void set_rights_request_callback(std::function<void(DataSubjectRights, const std::string&)> callback);
    void set_breach_notification_callback(std::function<void(const DataBreachRecord&)> callback);
    
    // ========================================================================
    // UTILITY & VALIDATION
    // ========================================================================
    
    bool is_user_in_eu(const std::string& user_id) const;
    bool is_user_in_california(const std::string& user_id) const;
    bool is_user_minor(const std::string& user_id) const;
    std::string get_user_jurisdiction(const std::string& user_id) const;
    
    static bool validate_email_format(const std::string& email);
    static bool validate_phone_format(const std::string& phone);
    static std::string anonymize_data(const std::string& data, const std::string& method = "hash");
    static std::string pseudonymize_data(const std::string& data, const std::string& key);

private:
    // Internal compliance validation methods
    bool validate_gdpr_compliance() const;
    bool validate_ccpa_compliance() const;
    bool validate_data_minimization() const;
    bool validate_purpose_limitation() const;
    bool validate_storage_limitation() const;
    bool validate_accuracy_requirement() const;
    bool validate_security_measures() const;
    bool validate_accountability_principle() const;
    
    // Internal data management
    void cleanup_expired_data();
    void audit_data_access(const std::string& user_id, const std::string& data_category, const std::string& purpose);
    void log_processing_activity(const std::string& activity, const std::string& user_id, const std::string& legal_basis);
    
    // Platform-specific implementations
    void initialize_ios_att();
    void initialize_android_privacy();
    void initialize_store_compliance();
    
    // Notification and communication
    void send_breach_notification_to_authority(const DataBreachRecord& breach);
    void send_breach_notification_to_subjects(const DataBreachRecord& breach);
    void send_privacy_notice_to_user(const std::string& user_id);
};

// ============================================================================
// FACTORY FUNCTIONS & UTILITIES
// ============================================================================

/**
 * @brief Create privacy compliance manager with regional configuration
 */
std::unique_ptr<PrivacyComplianceManager> create_gdpr_compliance_manager();
std::unique_ptr<PrivacyComplianceManager> create_ccpa_compliance_manager();
std::unique_ptr<PrivacyComplianceManager> create_global_compliance_manager();

/**
 * @brief Privacy compliance validation utilities
 */
namespace privacy_validation {
    bool validate_privacy_policy_completeness(const std::string& policy_text);
    bool validate_consent_form_compliance(const std::string& consent_text);
    bool validate_data_processing_lawfulness(const PersonalDataCategory& category);
    std::vector<std::string> get_missing_privacy_disclosures(const PrivacyComplianceManager& manager);
    std::vector<std::string> get_compliance_recommendations(const PrivacyComplianceManager& manager);
}

/**
 * @brief GDPR-specific utilities
 */
namespace gdpr_utils {
    std::string get_lawful_basis_description(LawfulBasis basis);
    std::string get_data_subject_right_description(DataSubjectRights right);
    bool requires_dpo_appointment(const PrivacyComplianceManager::ComplianceConfig& config);
    bool requires_data_protection_impact_assessment(const PersonalDataCategory& category);
    std::chrono::hours get_breach_notification_deadline(const DataBreachRecord& breach);
}

/**
 * @brief Platform-specific compliance templates
 */
namespace compliance_templates {
    std::string get_ios_att_dialog_template();
    std::string get_android_permission_rationale_template();
    std::string get_gdpr_consent_form_template();
    std::string get_ccpa_opt_out_form_template();
    std::string get_privacy_policy_template();
    std::string get_cookie_consent_banner_template();
}

} // namespace qcs::ui::compliance
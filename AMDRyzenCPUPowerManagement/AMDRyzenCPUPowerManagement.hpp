#ifndef AMDRyzenCPUPowerManagement_h
#define AMDRyzenCPUPowerManagement_h

//Support for macOS 10.13
#include <Library/LegacyIOService.h>

//#include <IOKit/IOService.h>
//#include <IOKit/IOLib.h>

#include <math.h>
#include <IOKit/pci/IOPCIDevice.h>
#include <IOKit/IOTimerEventSource.h>

#include <i386/proc_reg.h>
#include <libkern/libkern.h>

#include <Headers/kern_efi.hpp>

#include <Headers/kern_util.hpp>
#include <Headers/kern_cpu.hpp>
#include <Headers/kern_time.hpp>

#include <VirtualSMCSDK/kern_vsmcapi.hpp>
#include <VirtualSMCSDK/AppleSmc.h>

#include "KeyImplementations.hpp"

#include "symresolver/kernel_resolver.h"


#include <i386/cpuid.h>

#define OC_OEM_VENDOR_VARIABLE_NAME        u"oem-vendor"
#define OC_OEM_BOARD_VARIABLE_NAME         u"oem-board"

#define BASEBOARD_STRING_MAX 64

extern "C" {

#include "Headers/osfmk/i386/pmCPU.h"
#include "Headers/osfmk/i386/cpu_topology.h"

    int cpu_number(void);
    void mp_rendezvous_no_intrs(void (*action_func)(void *), void *arg);

    void
    mp_rendezvous(void (*setup_func)(void *),
                  void (*action_func)(void *),
                  void (*teardown_func)(void *),
                  void *arg);

    void
    i386_deactivate_cpu(void);
//    int wrmsr_carefully(uint32_t msr, uint64_t val);



typedef enum {
    ID                      = 0x02,
    VERSION                 = 0x03,
    TPR                     = 0x08,
    APR                     = 0x09,
    PPR                     = 0x0A,
    EOI                     = 0x0B,
    REMOTE_READ             = 0x0C,
    LDR                     = 0x0D,
    DFR                     = 0x0E,
    SVR                     = 0x0F,
    ISR_BASE                = 0x10,
    TMR_BASE                = 0x18,
    IRR_BASE                = 0x20,
    ERROR_STATUS            = 0x28,
    LVT_CMCI                = 0x2F,
    ICR                     = 0x30,
    ICRD                    = 0x31,
    LVT_TIMER               = 0x32,
    LVT_THERMAL             = 0x33,
    LVT_PERFCNT             = 0x34,
    LVT_LINT0               = 0x35,
    LVT_LINT1               = 0x36,
    LVT_ERROR               = 0x37,
    TIMER_INITIAL_COUNT     = 0x38,
    TIMER_CURRENT_COUNT     = 0x39,
    TIMER_DIVIDE_CONFIG     = 0x3E,
} lapic_register_t;


kern_return_t
processor_exit_from_user(processor_t     processor);

};


/**
 * Offset table: https://github.com/torvalds/linux/blob/master/drivers/hwmon/k10temp.c#L78
 */
typedef struct tctl_offset {
    uint8_t model;
    char const *id;
    int offset;
} TempOffset;




class AMDRyzenCPUPowerManagement : public IOService {
    OSDeclareDefaultStructors(AMDRyzenCPUPowerManagement)
    
    /**
     *  VirtualSMC service registration notifier
     */
    IONotifier *vsmcNotifier {nullptr};
    
    static bool vsmcNotificationHandler(void *sensors, void *refCon, IOService *vsmc, IONotifier *notifier);
    
    /**
     *  Registered plugin instance
     */
    VirtualSMCAPI::Plugin vsmcPlugin {
        xStringify(PRODUCT_NAME),
        parseModuleVersion(xStringify(MODULE_VERSION)),
        VirtualSMCAPI::Version,
    };
    
    
public:
    
    char kMODULE_VERSION[12]{};
    
    


    
    
    
    /**
     *  MSRs supported by AMD 17h CPU from:
     *  https://github.com/LibreHardwareMonitor/LibreHardwareMonitor/blob/master/LibreHardwareMonitorLib/Hardware/Cpu/Amd17Cpu.cs
     * and
     * Processor Programming Reference for AMD 17h CPU,
     *
     */
    
    static constexpr uint32_t kCOFVID_STATUS = 0xC0010071;
    static constexpr uint32_t k17H_M01H_SVI = 0x0005A000;
    static constexpr uint32_t kF17H_M01H_THM_TCON_CUR_TMP = 0x00059800;
    static constexpr uint32_t kF17H_M70H_CCD1_TEMP = 0x00059954;
    static constexpr uint32_t kF17H_TEMP_OFFSET_FLAG = 0x80000;
    static constexpr uint32_t kF18H_TEMP_OFFSET_FLAG = 0x60000;
    static constexpr uint8_t kFAMILY_17H_PCI_CONTROL_REGISTER = 0x60;
    static constexpr uint32_t kMSR_HWCR = 0xC0010015;
    static constexpr uint32_t kMSR_CORE_ENERGY_STAT = 0xC001029A;
    static constexpr uint32_t kMSR_HARDWARE_PSTATE_STATUS = 0xC0010293;
    static constexpr uint32_t kMSR_PKG_ENERGY_STAT = 0xC001029B;
    static constexpr uint32_t kMSR_PSTATE_0 = 0xC0010064;
    static constexpr uint32_t kMSR_PSTATE_LEN = 8;
    static constexpr uint32_t kMSR_PSTATE_STAT = 0xC0010063;
    static constexpr uint32_t kMSR_PSTATE_CTL = 0xC0010062;
    static constexpr uint32_t kMSR_PWR_UNIT = 0xC0010299;
    static constexpr uint32_t kMSR_MPERF = 0x000000E7;
    static constexpr uint32_t kMSR_APERF = 0x000000E8;
    static constexpr uint32_t kMSR_PERF_CTL_0 = 0xC0010000;
    static constexpr uint32_t kMSR_PERF_CTR_0 = 0xC0010004;
    static constexpr uint32_t kMSR_PERF_IRPC = 0xC00000E9;
    
    
//    static constexpr uint32_t EF = 0x88;
    
    static constexpr uint32_t kEFI_VARIABLE_NON_VOLATILE = 0x00000001;
    static constexpr uint32_t kEFI_VARIABLE_BOOTSERVICE_ACCESS = 0x00000002;
    static constexpr uint32_t kEFI_VARIABLE_RUNTIME_ACCESS = 0x00000004;
    
    
    /**
     *  Key name index mapping
     */
    static constexpr size_t MaxIndexCount = sizeof("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ") - 1;
    static constexpr const char *KeyIndexes = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    
    
    /**
     *  Supported SMC keys
     */
    static constexpr SMC_KEY KeyPC0C = SMC_MAKE_IDENTIFIER('P','C','0','C');
    static constexpr SMC_KEY KeyPC0G = SMC_MAKE_IDENTIFIER('P','C','0','G');
    static constexpr SMC_KEY KeyPC0R = SMC_MAKE_IDENTIFIER('P','C','0','R');
    static constexpr SMC_KEY KeyPC3C = SMC_MAKE_IDENTIFIER('P','C','3','C');
    static constexpr SMC_KEY KeyPCAC = SMC_MAKE_IDENTIFIER('P','C','A','C');
    static constexpr SMC_KEY KeyPCAM = SMC_MAKE_IDENTIFIER('P','C','A','M');
    static constexpr SMC_KEY KeyPCEC = SMC_MAKE_IDENTIFIER('P','C','E','C');
    static constexpr SMC_KEY KeyPCGC = SMC_MAKE_IDENTIFIER('P','C','G','C');
    static constexpr SMC_KEY KeyPCGM = SMC_MAKE_IDENTIFIER('P','C','G','M');
    static constexpr SMC_KEY KeyPCPC = SMC_MAKE_IDENTIFIER('P','C','P','C');
    static constexpr SMC_KEY KeyPCPG = SMC_MAKE_IDENTIFIER('P','C','P','G');
    static constexpr SMC_KEY KeyPCPR = SMC_MAKE_IDENTIFIER('P','C','P','R');
    static constexpr SMC_KEY KeyPSTR = SMC_MAKE_IDENTIFIER('P','S','T','R');
    static constexpr SMC_KEY KeyPCPT = SMC_MAKE_IDENTIFIER('P','C','P','T');
    static constexpr SMC_KEY KeyPCTR = SMC_MAKE_IDENTIFIER('P','C','T','R');
    static constexpr SMC_KEY KeyTCxD(size_t i) { return SMC_MAKE_IDENTIFIER('T','C',KeyIndexes[i],'D'); }
    static constexpr SMC_KEY KeyTCxE(size_t i) { return SMC_MAKE_IDENTIFIER('T','C',KeyIndexes[i],'E'); }
    static constexpr SMC_KEY KeyTCxF(size_t i) { return SMC_MAKE_IDENTIFIER('T','C',KeyIndexes[i],'F'); }
    static constexpr SMC_KEY KeyTCxG(size_t i) { return SMC_MAKE_IDENTIFIER('T','C',KeyIndexes[i],'G'); }
    static constexpr SMC_KEY KeyTCxJ(size_t i) { return SMC_MAKE_IDENTIFIER('T','C',KeyIndexes[i],'J'); }
    static constexpr SMC_KEY KeyTCxH(size_t i) { return SMC_MAKE_IDENTIFIER('T','C',KeyIndexes[i],'H'); }
    static constexpr SMC_KEY KeyTCxP(size_t i) { return SMC_MAKE_IDENTIFIER('T','C',KeyIndexes[i],'P'); }
    static constexpr SMC_KEY KeyTCxT(size_t i) { return SMC_MAKE_IDENTIFIER('T','C',KeyIndexes[i],'T'); }
    static constexpr SMC_KEY KeyTCxp(size_t i) { return SMC_MAKE_IDENTIFIER('T','C',KeyIndexes[i],'p'); }
    
    static constexpr SMC_KEY KeyTCxC(size_t i) { return SMC_MAKE_IDENTIFIER('T','C',KeyIndexes[i],'C'); }
    static constexpr SMC_KEY KeyTCxc(size_t i) { return SMC_MAKE_IDENTIFIER('T','C',KeyIndexes[i],'c'); }
    

    virtual bool init(OSDictionary *dictionary = 0) override;
    virtual void free(void) override;
    
    virtual bool start(IOService *provider) override;
    virtual void stop(IOService *provider) override;
    
    void fetchOEMBaseBoardInfo();

    bool read_msr(uint32_t addr, uint64_t *value);
    bool write_msr(uint32_t addr, uint64_t value);
    
    
    void updateClockSpeed(uint8_t physical);
    void calculateEffectiveFrequency(uint8_t physical);
    void updateInstructionDelta(uint8_t physical);
    void applyPowerControl();
    void updatePowerControl();
    
    void setCPBState(bool enabled);
    bool getCPBState();
    
    void updatePackageTemp();
    void updatePackageEnergy();
    
    void registerRequest();
    
    void dumpPstate(uint8_t cpu_num);
    void writePstate(const uint64_t *buf);
    
    uint32_t totalNumberOfPhysicalCores;
    uint32_t totalNumberOfLogicalCores;
    
    uint8_t cpuFamily;
    uint8_t cpuModel;
    uint8_t cpuSupportedByCurrentVersion;
    
    //Cache size in KB
    uint32_t cpuCacheL1_perCore;
    uint32_t cpuCacheL2_perCore;
    uint32_t cpuCacheL3;
    
    char boardVender[BASEBOARD_STRING_MAX]{};
    char boardName[BASEBOARD_STRING_MAX]{};
    bool boardInfoValid = false;
    
    
    /**
     *  Hard allocate space for cached readings.
     */
    float effFreq_perCore[CPUInfo::MaxCpus] {};
    float PACKAGE_TEMPERATURE_perPackage[CPUInfo::MaxCpus];
    
    uint64_t lastMPERF_PerCore[CPUInfo::MaxCpus];
    uint64_t lastAPERF_PerCore[CPUInfo::MaxCpus];
    uint64_t deltaAPERF_PerCore[CPUInfo::MaxCpus];
    uint64_t deltaMPERF_PerCore[CPUInfo::MaxCpus];
    
//    uint64_t lastAPERF_PerCore[CPUInfo::MaxCpus];
    
    uint64_t instructionDelta_PerCore[CPUInfo::MaxCpus];
    uint64_t lastInstructionDelta_perCore[CPUInfo::MaxCpus];
    
    float loadIndex_PerCore[CPUInfo::MaxCpus];
    
    bool PPMEnabled = false;
    float PStateStepUpRatio = 0.36;
    float PStateStepDownRatio = 0.05;
    
    uint8_t PStateCur_perCore[CPUInfo::MaxCpus];
    uint8_t PStateCtl = 0;
    uint64_t PStateDef_perCore[CPUInfo::MaxCpus][8];
    uint8_t PStateEnabledLen = 0;
    float PStateDefClock_perCore[CPUInfo::MaxCpus][8];
    bool cpbSupported;
    
    
    uint64_t lastUpdateTime;
    uint64_t lastUpdateEnergyValue;
    
    double uniPackageEnegry;
    
    bool disablePrivilegeCheck = false;

    kern_return_t (*kunc_alert)(int,unsigned,const char*,const char*,const char*,
                                const char*,const char*,const char*,const char*,const char*,unsigned*) {nullptr};
    
private:
    
    IOWorkLoop *workLoop;
    IOTimerEventSource *timerEventSource;
    IOSimpleLock *mpLock {nullptr};
    
    bool serviceInitialized = false;
    
    uint32_t updateTimeInterval = 1000;
    uint32_t actualUpdateTimeInterval = 1;
    uint32_t timeOfLastUpdate = 0;
    uint32_t estimatedRequestTimeInterval = 0;
    uint32_t timeOfLastMissedRequest = 0;
    
    float tempOffset = 0;
    
    int (*wrmsr_carefully)(uint32_t, uint32_t, uint32_t) {nullptr};

    
    CPUInfo::CpuTopology cpuTopology {};
    
    IOPCIDevice *fIOPCIDevice;
    
    bool setupKeysVsmc();
    bool getPCIService();
    
};
#endif
#ifndef ATA_DRIVER_H
#define ATA_DRIVER_H

#include "mm/paging.h"
#include "types.h"

// Primary bus
#define ATA_PRIMARY_IO       0x1F0
#define ATA_PRIMARY_CTRL     0x3F6

// Secondary bus
#define ATA_SECONDARY_IO     0x170
#define ATA_SECONDARY_CTRL   0x376

// I/O port offsets from <bus>_IO
#define ATA_REG_DATA         0x00    // Data register (16 bits)
#define ATA_REG_ERROR        0x01    // Error register (read)
#define ATA_REG_FEATURES     0x01    // Features register (write)
#define ATA_REG_SECCOUNT     0x02    // Sector Count register
#define ATA_REG_LBA_LOW      0x03    // Sector number register
#define ATA_REG_LBA_MID      0x04    // Cylinder low register
#define ATA_REG_LBA_HIGH     0x05    // Cylinder high register
#define ATA_REG_HDDEVSEL     0x06    // Head / Device select register
#define ATA_REG_COMMAND      0x07    // Command register (write)
#define ATA_REG_STATUS       0x07    // Status register (read)

// Control port offsets from <bus>_CTRL
#define ATA_REG_CONTROL      0x00
#define ATA_REG_ALTSTATUS    0x00

#define ATA_SR_ERR  0x01  // Error
#define ATA_SR_DRQ  0x08  // Data request ready
#define ATA_SR_DF   0x20  // Device fault
#define ATA_SR_BSY  0x80  // Busy

// ATA commands
#define ATA_CMD_READ_PIO          0x20
#define ATA_CMD_READ_PIO_EXT      0x24
#define ATA_CMD_WRITE_PIO         0x30
#define ATA_CMD_WRITE_PIO_EXT     0x34
#define ATA_CMD_IDENTIFY          0xEC
#define ATA_CMD_FLUSH             0xE7
#define ATA_CMD_FLUSH_EXT         0xEA

#define ATA_SECTOR_SIZE 512

typedef struct device_id_struct {
    uint16_t io_base;       // io base port, 0x170 or 0x1F0
    uint16_t ctrl_base;     // ctrl base prt, 0x376 or 0x3F6 
    uint8_t master;         // 0 = master, 1 = slave
} device_id_t;

typedef struct ata_drive_struct {
    device_id_t device_id;
    uint32_t size_in_sectors;  // size of the drive in sectors (as defined above)
    uint8_t exists;         // 0 if driver api request handled without error, else 1 
} ata_drive_t;

typedef struct __attribute__((packed)) identify_device_data_struct {
  struct {
    uint16_t Reserved1 : 1;
    uint16_t Retired3 : 1;
    uint16_t ResponseIncomplete : 1;
    uint16_t Retired2 : 3;
    uint16_t FixedDevice : 1;
    uint16_t RemovableMedia : 1;
    uint16_t Retired1 : 7;
    uint16_t DeviceType : 1;
  } GeneralConfiguration;
  uint16_t NumCylinders;
  uint16_t SpecificConfiguration;
  uint16_t NumHeads;
  uint16_t Retired1[2];
  uint16_t NumSectorsPerTrack;
  uint16_t VendorUnique1[3];
  uint8_t  SerialNumber[20];
  uint16_t Retired2[2];
  uint16_t Obsolete1;
  uint8_t  FirmwareRevision[8];
  uint8_t  ModelNumber[40];
  uint8_t  MaximumBlockTransfer;
  uint8_t  VendorUnique2;
  struct {
    uint16_t FeatureSupported : 1;
    uint16_t Reserved : 15;
  } TrustedComputing;
  struct {
    uint8_t  CurrentLongPhysicalSectorAlignment : 2;
    uint8_t  ReservedByte49 : 6;
    uint8_t  DmaSupported : 1;
    uint8_t  LbaSupported : 1;
    uint8_t  IordyDisable : 1;
    uint8_t  IordySupported : 1;
    uint8_t  Reserved1 : 1;
    uint8_t  StandybyTimerSupport : 1;
    uint8_t  Reserved2 : 2;
    uint16_t ReservedWord50;
  } Capabilities;
  uint16_t ObsoleteWords51[2];
  uint16_t TranslationFieldsValid : 3;
  uint16_t Reserved3 : 5;
  uint16_t FreeFallControlSensitivity : 8;
  uint16_t NumberOfCurrentCylinders;
  uint16_t NumberOfCurrentHeads;
  uint16_t CurrentSectorsPerTrack;
  uint32_t  CurrentSectorCapacity;
  uint8_t  CurrentMultiSectorSetting;
  uint8_t  MultiSectorSettingValid : 1;
  uint8_t  ReservedByte59 : 3;
  uint8_t  SanitizeFeatureSupported : 1;
  uint8_t  CryptoScrambleExtCommandSupported : 1;
  uint8_t  OverwriteExtCommandSupported : 1;
  uint8_t  BlockEraseExtCommandSupported : 1;
  uint32_t  UserAddressableSectors;
  uint16_t ObsoleteWord62;
  uint16_t MultiWordDMASupport : 8;
  uint16_t MultiWordDMAActive : 8;
  uint16_t AdvancedPIOModes : 8;
  uint16_t ReservedByte64 : 8;
  uint16_t MinimumMWXferCycleTime;
  uint16_t RecommendedMWXferCycleTime;
  uint16_t MinimumPIOCycleTime;
  uint16_t MinimumPIOCycleTimeIORDY;
  struct {
    uint16_t ZonedCapabilities : 2;
    uint16_t NonVolatileWriteCache : 1;
    uint16_t ExtendedUserAddressableSectorsSupported : 1;
    uint16_t DeviceEncryptsAllUserData : 1;
    uint16_t ReadZeroAfterTrimSupported : 1;
    uint16_t Optional28BitCommandsSupported : 1;
    uint16_t IEEE1667 : 1;
    uint16_t DownloadMicrocodeDmaSupported : 1;
    uint16_t SetMaxSetPasswordUnlockDmaSupported : 1;
    uint16_t WriteBufferDmaSupported : 1;
    uint16_t ReadBufferDmaSupported : 1;
    uint16_t DeviceConfigIdentifySetDmaSupported : 1;
    uint16_t LPSAERCSupported : 1;
    uint16_t DeterministicReadAfterTrimSupported : 1;
    uint16_t CFastSpecSupported : 1;
  } AdditionalSupported;
  uint16_t ReservedWords70[5];
  uint16_t QueueDepth : 5;
  uint16_t ReservedWord75 : 11;
  struct {
    uint16_t Reserved0 : 1;
    uint16_t SataGen1 : 1;
    uint16_t SataGen2 : 1;
    uint16_t SataGen3 : 1;
    uint16_t Reserved1 : 4;
    uint16_t NCQ : 1;
    uint16_t HIPM : 1;
    uint16_t PhyEvents : 1;
    uint16_t NcqUnload : 1;
    uint16_t NcqPriority : 1;
    uint16_t HostAutoPS : 1;
    uint16_t DeviceAutoPS : 1;
    uint16_t ReadLogDMA : 1;
    uint16_t Reserved2 : 1;
    uint16_t CurrentSpeed : 3;
    uint16_t NcqStreaming : 1;
    uint16_t NcqQueueMgmt : 1;
    uint16_t NcqReceiveSend : 1;
    uint16_t DEVSLPtoReducedPwrState : 1;
    uint16_t Reserved3 : 8;
  } SerialAtaCapabilities;
  struct {
    uint16_t Reserved0 : 1;
    uint16_t NonZeroOffsets : 1;
    uint16_t DmaSetupAutoActivate : 1;
    uint16_t DIPM : 1;
    uint16_t InOrderData : 1;
    uint16_t HardwareFeatureControl : 1;
    uint16_t SoftwareSettingsPreservation : 1;
    uint16_t NCQAutosense : 1;
    uint16_t DEVSLP : 1;
    uint16_t HybridInformation : 1;
    uint16_t Reserved1 : 6;
  } SerialAtaFeaturesSupported;
  struct {
    uint16_t Reserved0 : 1;
    uint16_t NonZeroOffsets : 1;
    uint16_t DmaSetupAutoActivate : 1;
    uint16_t DIPM : 1;
    uint16_t InOrderData : 1;
    uint16_t HardwareFeatureControl : 1;
    uint16_t SoftwareSettingsPreservation : 1;
    uint16_t DeviceAutoPS : 1;
    uint16_t DEVSLP : 1;
    uint16_t HybridInformation : 1;
    uint16_t Reserved1 : 6;
  } SerialAtaFeaturesEnabled;
  uint16_t MajorRevision;
  uint16_t MinorRevision;
  struct {
    uint16_t SmartCommands : 1;
    uint16_t SecurityMode : 1;
    uint16_t RemovableMediaFeature : 1;
    uint16_t PowerManagement : 1;
    uint16_t Reserved1 : 1;
    uint16_t WriteCache : 1;
    uint16_t LookAhead : 1;
    uint16_t ReleaseInterrupt : 1;
    uint16_t ServiceInterrupt : 1;
    uint16_t DeviceReset : 1;
    uint16_t HostProtectedArea : 1;
    uint16_t Obsolete1 : 1;
    uint16_t WriteBuffer : 1;
    uint16_t ReadBuffer : 1;
    uint16_t Nop : 1;
    uint16_t Obsolete2 : 1;
    uint16_t DownloadMicrocode : 1;
    uint16_t DmaQueued : 1;
    uint16_t Cfa : 1;
    uint16_t AdvancedPm : 1;
    uint16_t Msn : 1;
    uint16_t PowerUpInStandby : 1;
    uint16_t ManualPowerUp : 1;
    uint16_t Reserved2 : 1;
    uint16_t SetMax : 1;
    uint16_t Acoustics : 1;
    uint16_t BigLba : 1;
    uint16_t DeviceConfigOverlay : 1;
    uint16_t FlushCache : 1;
    uint16_t FlushCacheExt : 1;
    uint16_t WordValid83 : 2;
    uint16_t SmartErrorLog : 1;
    uint16_t SmartSelfTest : 1;
    uint16_t MediaSerialNumber : 1;
    uint16_t MediaCardPassThrough : 1;
    uint16_t StreamingFeature : 1;
    uint16_t GpLogging : 1;
    uint16_t WriteFua : 1;
    uint16_t WriteQueuedFua : 1;
    uint16_t WWN64Bit : 1;
    uint16_t URGReadStream : 1;
    uint16_t URGWriteStream : 1;
    uint16_t ReservedForTechReport : 2;
    uint16_t IdleWithUnloadFeature : 1;
    uint16_t WordValid : 2;
  } CommandSetSupport;
  struct {
    uint16_t SmartCommands : 1;
    uint16_t SecurityMode : 1;
    uint16_t RemovableMediaFeature : 1;
    uint16_t PowerManagement : 1;
    uint16_t Reserved1 : 1;
    uint16_t WriteCache : 1;
    uint16_t LookAhead : 1;
    uint16_t ReleaseInterrupt : 1;
    uint16_t ServiceInterrupt : 1;
    uint16_t DeviceReset : 1;
    uint16_t HostProtectedArea : 1;
    uint16_t Obsolete1 : 1;
    uint16_t WriteBuffer : 1;
    uint16_t ReadBuffer : 1;
    uint16_t Nop : 1;
    uint16_t Obsolete2 : 1;
    uint16_t DownloadMicrocode : 1;
    uint16_t DmaQueued : 1;
    uint16_t Cfa : 1;
    uint16_t AdvancedPm : 1;
    uint16_t Msn : 1;
    uint16_t PowerUpInStandby : 1;
    uint16_t ManualPowerUp : 1;
    uint16_t Reserved2 : 1;
    uint16_t SetMax : 1;
    uint16_t Acoustics : 1;
    uint16_t BigLba : 1;
    uint16_t DeviceConfigOverlay : 1;
    uint16_t FlushCache : 1;
    uint16_t FlushCacheExt : 1;
    uint16_t Resrved3 : 1;
    uint16_t Words119_120Valid : 1;
    uint16_t SmartErrorLog : 1;
    uint16_t SmartSelfTest : 1;
    uint16_t MediaSerialNumber : 1;
    uint16_t MediaCardPassThrough : 1;
    uint16_t StreamingFeature : 1;
    uint16_t GpLogging : 1;
    uint16_t WriteFua : 1;
    uint16_t WriteQueuedFua : 1;
    uint16_t WWN64Bit : 1;
    uint16_t URGReadStream : 1;
    uint16_t URGWriteStream : 1;
    uint16_t ReservedForTechReport : 2;
    uint16_t IdleWithUnloadFeature : 1;
    uint16_t Reserved4 : 2;
  } CommandSetActive;
  uint16_t UltraDMASupport : 8;
  uint16_t UltraDMAActive : 8;
  struct {
    uint16_t TimeRequired : 15;
    uint16_t ExtendedTimeReported : 1;
  } NormalSecurityEraseUnit;
  struct {
    uint16_t TimeRequired : 15;
    uint16_t ExtendedTimeReported : 1;
  } EnhancedSecurityEraseUnit;
  uint16_t CurrentAPMLevel : 8;
  uint16_t ReservedWord91 : 8;
  uint16_t MasterPasswordID;
  uint16_t HardwareResetResult;
  uint16_t CurrentAcousticValue : 8;
  uint16_t RecommendedAcousticValue : 8;
  uint16_t StreamMinRequestSize;
  uint16_t StreamingTransferTimeDMA;
  uint16_t StreamingAccessLatencyDMAPIO;
  uint32_t  StreamingPerfGranularity;
  uint32_t  Max48BitLBA[2];
  uint16_t StreamingTransferTime;
  uint16_t DsmCap;
  struct {
    uint16_t LogicalSectorsPerPhysicalSector : 4;
    uint16_t Reserved0 : 8;
    uint16_t LogicalSectorLongerThan256Words : 1;
    uint16_t MultipleLogicalSectorsPerPhysicalSector : 1;
    uint16_t Reserved1 : 2;
  } PhysicalLogicalSectorSize;
  uint16_t InterSeekDelay;
  uint16_t WorldWideName[4];
  uint16_t ReservedForWorldWideName128[4];
  uint16_t ReservedForTlcTechnicalReport;
  uint16_t WordsPerLogicalSector[2];
  struct {
    uint16_t ReservedForDrqTechnicalReport : 1;
    uint16_t WriteReadVerify : 1;
    uint16_t WriteUncorrectableExt : 1;
    uint16_t ReadWriteLogDmaExt : 1;
    uint16_t DownloadMicrocodeMode3 : 1;
    uint16_t FreefallControl : 1;
    uint16_t SenseDataReporting : 1;
    uint16_t ExtendedPowerConditions : 1;
    uint16_t Reserved0 : 6;
    uint16_t WordValid : 2;
  } CommandSetSupportExt;
  struct {
    uint16_t ReservedForDrqTechnicalReport : 1;
    uint16_t WriteReadVerify : 1;
    uint16_t WriteUncorrectableExt : 1;
    uint16_t ReadWriteLogDmaExt : 1;
    uint16_t DownloadMicrocodeMode3 : 1;
    uint16_t FreefallControl : 1;
    uint16_t SenseDataReporting : 1;
    uint16_t ExtendedPowerConditions : 1;
    uint16_t Reserved0 : 6;
    uint16_t Reserved1 : 2;
  } CommandSetActiveExt;
  uint16_t ReservedForExpandedSupportandActive[6];
  uint16_t MsnSupport : 2;
  uint16_t ReservedWord127 : 14;
  struct {
    uint16_t SecuritySupported : 1;
    uint16_t SecurityEnabled : 1;
    uint16_t SecurityLocked : 1;
    uint16_t SecurityFrozen : 1;
    uint16_t SecurityCountExpired : 1;
    uint16_t EnhancedSecurityEraseSupported : 1;
    uint16_t Reserved0 : 2;
    uint16_t SecurityLevel : 1;
    uint16_t Reserved1 : 7;
  } SecurityStatus;
  uint16_t ReservedWord129[31];
  struct {
    uint16_t MaximumCurrentInMA : 12;
    uint16_t CfaPowerMode1Disabled : 1;
    uint16_t CfaPowerMode1Required : 1;
    uint16_t Reserved0 : 1;
    uint16_t Word160Supported : 1;
  } CfaPowerMode1;
  uint16_t ReservedForCfaWord161[7];
  uint16_t NominalFormFactor : 4;
  uint16_t ReservedWord168 : 12;
  struct {
    uint16_t SupportsTrim : 1;
    uint16_t Reserved0 : 15;
  } DataSetManagementFeature;
  uint16_t AdditionalProductID[4];
  uint16_t ReservedForCfaWord174[2];
  uint16_t CurrentMediaSerialNumber[30];
  struct {
    uint16_t Supported : 1;
    uint16_t Reserved0 : 1;
    uint16_t WriteSameSuported : 1;
    uint16_t ErrorRecoveryControlSupported : 1;
    uint16_t FeatureControlSuported : 1;
    uint16_t DataTablesSuported : 1;
    uint16_t Reserved1 : 6;
    uint16_t VendorSpecific : 4;
  } SCTCommandTransport;
  uint16_t ReservedWord207[2];
  struct {
    uint16_t AlignmentOfLogicalWithinPhysical : 14;
    uint16_t Word209Supported : 1;
    uint16_t Reserved0 : 1;
  } BlockAlignment;
  uint16_t WriteReadVerifySectorCountMode3Only[2];
  uint16_t WriteReadVerifySectorCountMode2Only[2];
  struct {
    uint16_t NVCachePowerModeEnabled : 1;
    uint16_t Reserved0 : 3;
    uint16_t NVCacheFeatureSetEnabled : 1;
    uint16_t Reserved1 : 3;
    uint16_t NVCachePowerModeVersion : 4;
    uint16_t NVCacheFeatureSetVersion : 4;
  } NVCacheCapabilities;
  uint16_t NVCacheSizeLSW;
  uint16_t NVCacheSizeMSW;
  uint16_t NominalMediaRotationRate;
  uint16_t ReservedWord218;
  struct {
    uint8_t NVCacheEstimatedTimeToSpinUpInSeconds;
    uint8_t Reserved;
  } NVCacheOptions;
  uint16_t WriteReadVerifySectorCountMode : 8;
  uint16_t ReservedWord220 : 8;
  uint16_t ReservedWord221;
  struct {
    uint16_t MajorVersion : 12;
    uint16_t TransportType : 4;
  } TransportMajorVersion;
  uint16_t TransportMinorVersion;
  uint16_t ReservedWord224[6];
  uint32_t  ExtendedNumberOfUserAddressableSectors[2];
  uint16_t MinBlocksPerDownloadMicrocodeMode03;
  uint16_t MaxBlocksPerDownloadMicrocodeMode03;
  uint16_t ReservedWord236[19];
  uint16_t Signature : 8;
  uint16_t CheckSum : 8;
} identify_device_data_t;


/**
 * Waits until BSY (busy) flag is cleared for the given drive.
 */
void ata_wait_bsy(ata_drive_t *drive);

/**
 * Waits until DRQ (data ready) flag is set for the given drive.
 */
void ata_wait_drq(ata_drive_t *drive);

/**
 * Registers ATA IRQ handlers and clears driver state.
 */
void initiate_ata_driver();

/**
 * Reads sectors from an ATA drive using 28-bit LBA PIO.
 * @buffer must be at least sector_count * 512 bytes.
 *
 * Returns: 1 if error occurred, 0 on success.
 */
uint8_t ata_read28_request(ata_drive_t *drive,
                          uint32_t sector_address,
                          uint8_t sector_count,
                          uint8_t *buffer);

/**
 * Writes sectors to an ATA drive using 28-bit LBA PIO.
 * The command will generate IRQ after each sector is written.
 * @buffer must contain sector_count * 512 bytes.
 *
 * Returns: 1 on error, 0 on success.
 */
uint8_t ata_write28_request(ata_drive_t *drive,
                           uint32_t sector_address,
                           uint8_t sector_count,
                           uint8_t *buffer);

/**
 * Sends FLUSH CACHE to the drive, forcing it to commit
 * any pending writes to disk.
 *
 * Returns: 1 on error, 0 on success.
 */
uint8_t ata_flush_cache(ata_drive_t *drive);

/**
 * Sends IDENTIFY command to a drive and stores parsed result.
 * Identifies if a drive exists and reads its identify block.
 *
 * Returns: 1 if error or drive missing, 0 if identify succeeded.
 */
uint8_t ata_send_identify_command(ata_drive_t *drive, 
                                  identify_device_data_t *buffer);

/**
 * Prints decoded identify information for debugging/display.
 */
void print_identify_device_data(const identify_device_data_t *id);

#endif // ATA_DRIVER_H
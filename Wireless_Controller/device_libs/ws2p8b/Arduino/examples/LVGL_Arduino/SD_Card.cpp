#include "SD_Card.h"

bool SDCard_Flag;
bool SDCard_Finish;

uint16_t SDCard_Size;
uint16_t Flash_Size;

void SD_D3_Dis(){
  Set_EXIO(EXIO_PIN4,Low);
  vTaskDelay(pdMS_TO_TICKS(10));
}
void SD_D3_EN(){
  Set_EXIO(EXIO_PIN4,High);
  vTaskDelay(pdMS_TO_TICKS(10));
}
void SD_Init() {
  // SD MMC
  if(!SD_MMC.setPins(SD_CLK_PIN, SD_CMD_PIN, SD_D0_PIN,-1,-1,-1)){
    printf("SD MMC: Pin change failed!\r\n");
    return;
  }
  SD_D3_EN();
  if (SD_MMC.begin("/sdcard", true)) {                              
    printf("SD card initialization successful!\r\n");
  } else {
    printf("SD card initialization failed!\r\n");
  }
  uint8_t cardType = SD_MMC.cardType();
  if(cardType == CARD_NONE){
    printf("No SD card attached\r\n");
    return;
  }
  else{
    printf("SD Card Type: ");
    if(cardType == CARD_MMC){
      printf("MMC\r\n");
    } else if(cardType == CARD_SD){
      printf("SDSC\r\n");
    } else if(cardType == CARD_SDHC){
      printf("SDHC\r\n");
    } else {
      printf("UNKNOWN\r\n");
    }
    uint64_t totalBytes = SD_MMC.totalBytes();
    uint64_t usedBytes = SD_MMC.usedBytes();
    SDCard_Size = totalBytes/(1024*1024);
    printf("Total space: %llu\n", totalBytes);
    printf("Used space: %llu\n", usedBytes);
    printf("Free space: %llu\n", totalBytes - usedBytes);
  }
}
bool File_Search(const char* directory, const char* fileName)    
{
  File Path = SD_MMC.open(directory);
  if (!Path) {
    printf("Path: <%s> does not exist\r\n",directory);
    return false;
  }
  File file = Path.openNextFile();
  while (file) {
    if (strcmp(file.name(), fileName) == 0) {                           
      if (strcmp(directory, "/") == 0)
        printf("File '%s%s' found in root directory.\r\n",directory,fileName);  
      else
        printf("File '%s/%s' found in root directory.\r\n",directory,fileName); 
      Path.close();                                                     
      return true;                                                     
    }
    file = Path.openNextFile();                                        
  }
  if (strcmp(directory, "/") == 0)
    printf("File '%s%s' not found in root directory.\r\n",directory,fileName);           
  else
    printf("File '%s/%s' not found in root directory.\r\n",directory,fileName);          
  Path.close();                                                         
  return false;                                                         
}
uint16_t Folder_retrieval(const char* directory, const char* fileExtension, char File_Name[][100],uint16_t maxFiles)    
{
  File Path = SD_MMC.open(directory);
  if (!Path) {
    printf("Path: <%s> does not exist\r\n",directory);
    return false;
  }
  
  uint16_t fileCount = 0;
  char filePath[100];
  File file = Path.openNextFile();
  while (file && fileCount < maxFiles) {
    if (!file.isDirectory() && strstr(file.name(), fileExtension)) {
      strncpy(File_Name[fileCount], file.name(), sizeof(File_Name[fileCount])); 
      if (strcmp(directory, "/") == 0) {                                      
        snprintf(filePath, 100, "%s%s", directory, file.name());   
      } else {                                                            
        snprintf(filePath, 100, "%s/%s", directory, file.name());
      }
      printf("File found: %s\r\n", filePath);
      fileCount++;
    }
    file = Path.openNextFile();                                      
  }
  Path.close();                                                         
  if (fileCount > 0) {
    printf("Retrieved %d mp3 files\r\n",fileCount);
    return fileCount;                                                 
  } else {
    printf("No files with extension '%s' found in directory: %s\r\n", fileExtension, directory);
    return 0;                                                         
  }
}

void Flash_test()
{
  printf("/********** RAM Test**********/\r\n");
  // Get Flash size
  uint32_t flashSize = ESP.getFlashChipSize();
  Flash_Size = flashSize/1024/1024;
  printf("Flash size: %d MB \r\n", flashSize/1024/1024);

  printf("/******* RAM Test Over********/\r\n\r\n");
}